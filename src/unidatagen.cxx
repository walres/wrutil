/**
 * \file unidatagen.cxx
 *
 * \brief Program for generating data tables from the Unicode Character
 *        Database text files
 *
 * \copyright
 * \parblock
 *
 *   Copyright 2013-2016 James S. Waller
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 * \endparblock
 */
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <locale>
#include <map>
#include <utility>
#include <vector>
#include <wrutil/ctype.h>
#include <wrutil/filesystem.h>
#include <wrutil/string_view.h>
#include <wrutil/UnicodeData.h>

/*
 * UnicodeData.txt fields
 */
enum
{
        CHAR_CODE = 0,
        CHAR_NAME,
        GENERAL_CATEGORY,
        CANON_COMBI_CLASS,
        BIDI_CLASS,
        DECOMPOSITION,
        DECIMAL_DIGIT_VALUE,
        SPECIAL_DIGIT_VALUE,
        NUMERIC_VALUE,
        BIDI_MIRRORED,
        UNICODE_1_NAME,
        ISO_COMMENT,
        UPPERCASE,
        LOWERCASE,
        TITLECASE,
        NUM_UNIDATA_FIELDS  // must appear last!
};

//--------------------------------------

struct PageIndex :
        std::array<int16_t, wr::ucd::PAGE_INDEX_MAX + 1>
{
        PageIndex() { fill(wr::ucd::PAGE_NOT_USED); }
};

//--------------------------------------

template <typename T> using PageArray = std::vector<wr::ucd::Page<T>>;

//--------------------------------------

struct
{
        PageIndex                           category_index,
                                            uppercase_index,
                                            lowercase_index,
                                            titlecase_index,
                                            digit_index,
                                            xdigit_index,
                                            property_index,
                                            char_class_index;
        std::vector<wr::ucd::CategoryPage>  category_page;
        std::vector<wr::ucd::CasePage>      uppercase_page,
                                            lowercase_page,
                                            titlecase_page;
        std::vector<wr::ucd::DigitPage>     digit_page,
                                            xdigit_page;
        std::vector<wr::ucd::PropertyPage>  property_page;
        std::vector<wr::ucd::CharClassPage> char_class_page;
}
data;

//--------------------------------------

static char32_t charCode(const wr::string_view &s);

static int addChar(char32_t c, wr::string_view (&field)[NUM_UNIDATA_FIELDS],
                   unsigned int line_no);

static int readProperties(std::istream &in, const wr::string_view &file_name);

template <typename T> static T &pageEntry(PageIndex &index, PageArray<T> &pages,
                                          char32_t c);

template <typename T> static void createDefaultPage(PageIndex &index,
                                                    PageArray<T> &pages);

template <typename T> static size_t removeDuplicatePages(PageIndex &index,
                                                         PageArray<T> &pages);

static void generateCharClasses();

template <typename PageType>
static void outputCXXFile(const char *name, const char *description,
                          const char *symbol_prefix, PageIndex &page_index,
                          std::vector<PageType> &pages, size_t &total_size);

static void outputPageIndex(const PageIndex &index,
                            const wr::string_view &name, std::ostream &output);

template <typename T> static void outputPages(const PageArray<T> &pages,
                                              const wr::string_view &name,
                                              std::ostream &output);

// our own version of some functions in ctype32.h
inline uint8_t category(char32_t c)
      { return wr::ucd::lookup(
                data.category_index.data(), data.category_page.data(), c); }

inline uint8_t major_category(char32_t c)
      { return category(c) & wr::ucd::MAJOR_CATEGORY_MASK; }

inline uint64_t properties(char32_t c)
      { return wr::ucd::lookup(
                data.property_index.data(), data.property_page.data(), c); }

//--------------------------------------

int
main(
        int    argc,
        char **argv
)
try {
        wr::path        output_dir;
        std::string     line,
                        range_name;
        wr::string_view field[NUM_UNIDATA_FIELDS];
        char32_t        range_start = static_cast<char32_t>(-1);
        int             exit_status = EXIT_SUCCESS;

        if (argc > 1) {
                output_dir = wr::current_path();
                wr::current_path(argv[1]);
        }

        std::ifstream file("UnicodeData.txt");

        if (!file.is_open()) {
                std::cerr << "cannot open UnicodeData.txt\n";
                exit_status = EXIT_FAILURE;
        }

        for (unsigned int line_no = 1; file.good(); ++line_no) {
                line.clear();
                std::getline(file, line);
                line += '\n';  // want sentinel for strto*() functions

                field[0] = wr::string_view(line).split('#').first.trim();

                if (field[0].empty()) {
                        continue;
                }

                for (int i = 0; i < (NUM_UNIDATA_FIELDS - 1); ++i) {
                        auto split = field[i].split(';');
                        field[i] = split.first.trim();
                        field[i + 1] = split.second;
                }

                char32_t c;

                try {
                        c = charCode(field[CHAR_CODE]);
                } catch (std::exception &) {
                        std::cerr << "UnicodeData.txt line " << line_no
                                  << ": malformed character code \""
                                  << field[CHAR_CODE] << "\"\n";
                        exit_status = EXIT_FAILURE;
                        break;
                }

                if (c >= wr::ucd::CODE_SPACE_SIZE) {
                        std::cerr << "UnicodeData.txt line " << line_no
                                  << ": character code \"" << field[CHAR_CODE]
                                  << "\" exceeds maximum (10ffff)\n";
                        exit_status = EXIT_FAILURE;
                        break;
                } else if (!range_name.empty()) {
                        if (!field[CHAR_NAME].has_suffix(", Last>")) {
                                std::cerr << "UnicodeData.txt line " << line_no
                                          << ": character " << field[CHAR_CODE]
                                          << ": expected <" << range_name
                                          << ", Last>\n";
                                exit_status = EXIT_FAILURE;
                                break;
                        }

                        for (char32_t i = range_start; i <= c; ++i) {
                                exit_status = addChar(i, field, line_no);
                                if (exit_status != EXIT_SUCCESS) {
                                        break;
                                }
                        }

                        range_name.clear();
                        range_start = static_cast<char32_t>(-1);
                } else if (field[CHAR_NAME].has_suffix(", First>")) {
                        range_name = field[CHAR_NAME].split('<').second
                                                     .split(", First>").first
                                                     .to_string();
                        range_start = c;
                } else {
                        exit_status = addChar(c, field, line_no);
                }

                if (exit_status != EXIT_SUCCESS) {
                        break;
                }
        }

        if (file.bad()) {
                exit_status = EXIT_FAILURE;
        } else if (range_start != static_cast<char32_t>(-1)) {
                std::cerr << "UnicodeData.txt: missing entry <" << range_name
                          << ", Last> at end of input\n";
                exit_status = EXIT_FAILURE;
        }

        for (const auto file_name: { "PropList.txt",
                                     "DerivedCoreProperties.txt" }) {
                if (exit_status != EXIT_SUCCESS) {
                        break;
                }
                file.close();
                file.open(file_name);
                if (file.is_open()) {
                        exit_status = readProperties(file, file_name);
                } else {
                        std::cerr << "cannot open " << file_name << '\n';
                        exit_status = EXIT_FAILURE;
                }
        }

        if (!output_dir.empty()) {
                wr::current_path(output_dir);
        }

        if (exit_status == EXIT_SUCCESS) {
                size_t total_size = 0;

                outputCXXFile("GeneralCategories.cxx", "General Categories",
                              "category", data.category_index,
                              data.category_page, total_size);

                outputCXXFile("UpperCasing.cxx", "Uppercasing", "uppercase",
                              data.uppercase_index, data.uppercase_page,
                              total_size);

                outputCXXFile("LowerCasing.cxx", "Lowercasing", "lowercase",
                              data.lowercase_index, data.lowercase_page,
                              total_size);

                outputCXXFile("TitleCasing.cxx", "Titlecasing", "titlecase",
                              data.titlecase_index, data.titlecase_page,
                              total_size);

                outputCXXFile("DecimalDigits.cxx", "Decimal Digits", "digit",
                              data.digit_index, data.digit_page, total_size);

                outputCXXFile("HexDigits.cxx", "Hex Digits", "xdigit",
                              data.xdigit_index, data.xdigit_page, total_size);

                outputCXXFile("CoreProperties.cxx", "Core Properties",
                              "property",
                              data.property_index, data.property_page,
                              total_size);

                generateCharClasses();

                outputCXXFile("CharClasses.cxx", "Character Classes",
                              "char_class", data.char_class_index,
                              data.char_class_page, total_size);

                std::cerr << total_size / 1024 << "KB total\n";
        }

        return exit_status;
} catch (std::exception &e) {
        std::cerr << argv[0] << ": " << e.what() << '\n';
        return EXIT_FAILURE;
}

//--------------------------------------

static char32_t
charCode(
        const wr::string_view &s
)
{
        auto c = stoi(s.to_string(), nullptr, 16);
        if ((c < 0) || (c > 0x10ffff)) {
                throw std::out_of_range(
                        "character code outside valid range 0-10ffff");
        }
        return static_cast<char32_t>(c);
}

//--------------------------------------

static int
addChar(
        char32_t          c,
        wr::string_view (&field)[NUM_UNIDATA_FIELDS],
        unsigned int      line_no
)
{
        assert(c < wr::ucd::CODE_SPACE_SIZE);

        auto    category_name = field[GENERAL_CATEGORY];
        uint8_t category      = 0xff;

        if (category_name.size() == 2) {
                switch (category_name[0]) {
                case 'L':
                        switch (category_name[1]) {
                        case 'u':
                                category = wr::ucd::UPPERCASE_LETTER;
                                break;
                        case 'l':
                                category = wr::ucd::LOWERCASE_LETTER;
                                break;
                        case 't':
                                category = wr::ucd::TITLECASE_LETTER;
                                break;
                        case 'm':
                                category = wr::ucd::MODIFIER_LETTER;
                                break;
                        case 'o':
                                category = wr::ucd::OTHER_LETTER;
                                break;
                        default:
                                break;
                        }
                        break;
                case 'M':
                        switch (category_name[1]) {
                        case 'n':
                                category = wr::ucd::NONSPACING_MARK;
                                break;
                        case 'c':
                                category = wr::ucd::SPACING_MARK;
                                break;
                        case 'e':
                                category = wr::ucd::ENCLOSING_MARK;
                                break;
                        default:
                                break;
                        }
                        break;
                case 'N':
                        switch (category_name[1]) {
                        case 'd':
                                category = wr::ucd::DECIMAL_NUMBER;
                                break;
                        case 'l':
                                category = wr::ucd::LETTER_NUMBER;
                                break;
                        case 'o':
                                category = wr::ucd::OTHER_NUMBER;
                                break;
                        default:
                                break;
                        }
                        break;
                case 'P':
                        switch (category_name[1]) {
                        case 'c':
                                category = wr::ucd::CONNECTOR_PUNCTUATION;
                                break;
                        case 'd':
                                category = wr::ucd::DASH_PUNCTUATION;
                                break;
                        case 's':
                                category = wr::ucd::OPEN_PUNCTUATION;
                                break;
                        case 'e':
                                category = wr::ucd::CLOSE_PUNCTUATION;
                                break;
                        case 'i':
                                category = wr::ucd::INITIAL_PUNCTUATION;
                                break;
                        case 'f':
                                category = wr::ucd::FINAL_PUNCTUATION;
                                break;
                        case 'o':
                                category = wr::ucd::OTHER_PUNCTUATION;
                                break;
                        default:
                                break;
                        }
                        break;
                case 'S':
                        switch (category_name[1]) {
                        case 'm':
                                category = wr::ucd::MATH_SYMBOL;
                                break;
                        case 'c':
                                category = wr::ucd::CURRENCY_SYMBOL;
                                break;
                        case 'k':
                                category = wr::ucd::MODIFIER_SYMBOL;
                                break;
                        case 'o':
                                category = wr::ucd::OTHER_SYMBOL;
                                break;
                        default:
                                break;
                        }
                        break;
                case 'Z':
                        switch (category_name[1]) {
                        case 's':
                                category = wr::ucd::SPACE_SEPARATOR;
                                break;
                        case 'l':
                                category = wr::ucd::LINE_SEPARATOR;
                                break;
                        case 'p':
                                category = wr::ucd::PARAGRAPH_SEPARATOR;
                                break;
                        default:
                                break;
                        }
                        break;
                case 'C':
                        switch (category_name[1]) {
                        case 'c':
                                category = wr::ucd::CONTROL;
                                break;
                        case 'f':
                                category = wr::ucd::FORMAT;
                                break;
                        case 's':
                                category = wr::ucd::SURROGATE;
                                break;
                        case 'o':
                                category = wr::ucd::PRIVATE_USE;
                                break;
                        case 'n':
                                category = wr::ucd::UNASSIGNED;
                                break;
                        default:
                                break;
                        }
                        break;
                default:
                        break;
                }
        }

        if (category != 0xff) {
                pageEntry(data.category_index,
                          data.category_page, c) = category;
        } else {
                std::cerr << "input line " << line_no << ": character "
                          << field[CHAR_CODE] << ": invalid category \""
                          << category_name << "\"\n";
        }

        switch (category) {
        case wr::ucd::UPPERCASE_LETTER: case wr::ucd::LOWERCASE_LETTER:
        case wr::ucd::TITLECASE_LETTER:
                if (!field[UPPERCASE].empty()) try {
                        pageEntry(data.uppercase_index,
                                  data.uppercase_page,
                                  c) = charCode(field[UPPERCASE]);
                } catch (std::exception &) {
                        std::cerr << "UnicodeData.txt line " << line_no
                                  << ": character " << field[CHAR_CODE]
                                  << ": invalid uppercase character code \""
                                  << field[UPPERCASE] << "\"\n";
                }

                if (!field[LOWERCASE].empty()) try {
                        auto lower = charCode(field[LOWERCASE]);
                        pageEntry(data.lowercase_index,
                                  data.lowercase_page,
                                  c) = charCode(field[LOWERCASE]);
                } catch (std::exception &) {
                        std::cerr << "UnicodeData.txt line " << line_no
                                  << ": character " << field[CHAR_CODE]
                                  << ": invalid lowercase character code \""
                                  << field[LOWERCASE] << "\"\n";
                }

                if (!field[TITLECASE].empty()) try {
                        pageEntry(data.titlecase_index,
                                  data.titlecase_page,
                                  c) = charCode(field[TITLECASE]);
                } catch (std::exception &) {
                        std::cerr << "UnicodeData.txt line " << line_no
                                  << ": character " << field[CHAR_CODE]
                                  << ": invalid titlecase character code \""
                                  << field[TITLECASE] << "\"\n";
                }
                break;
        case wr::ucd::DECIMAL_NUMBER:
                if (!field[DECIMAL_DIGIT_VALUE].empty()) try {
                        int val = stoi(field[DECIMAL_DIGIT_VALUE].to_string());
                        if ((val < 0) || (val > 9)) {
                                throw std::out_of_range(
                                        "digit value outside valid range 0-9");
                        }
                        pageEntry(data.digit_index, data.digit_page, c)
                                = static_cast<int8_t>(val);
                } catch (std::exception &) {
                        std::cerr << "UnicodeData.txt line " << line_no
                                  << ": character " << field[CHAR_CODE]
                                  << ": invalid decimal digit value \""
                                  << field[DECIMAL_DIGIT_VALUE] << "\"\n";
                }
                break;
        default:
                break;
        }

        return EXIT_SUCCESS;
}

//--------------------------------------

static int
readProperties(
        std::istream          &in,
        const wr::string_view &file_name
)
try {
        static const std::map<wr::string_view, uint64_t> PROPERTIES = {
                { "White_Space", wr::ucd::WHITE_SPACE },
                { "Bidi_Control", wr::ucd::BIDI_CONTROL },
                { "Join_Control", wr::ucd::JOIN_CONTROL },
                { "Dash", wr::ucd::DASH },
                { "Hyphen", wr::ucd::HYPHEN },
                { "Quotation_Mark", wr::ucd::QUOTATION_MARK },
                { "Terminal_Punctuation", wr::ucd::TERMINAL_PUNCTUATION },
                { "Other_Math", wr::ucd::OTHER_MATH },
                { "Hex_Digit", wr::ucd::HEX_DIGIT },
                { "ASCII_Hex_Digit", wr::ucd::ASCII_HEX_DIGIT },
                { "Other_Alphabetic", wr::ucd::OTHER_ALPHABETIC },
                { "Ideographic", wr::ucd::IDEOGRAPHIC },
                { "Diacritic", wr::ucd::DIACRITIC },
                { "Extender", wr::ucd::EXTENDER },
                { "Other_Lowercase", wr::ucd::OTHER_LOWERCASE },
                { "Other_Uppercase", wr::ucd::OTHER_UPPERCASE },
                { "Noncharacter_Code_Point", wr::ucd::NONCHARACTER_CODE_POINT },
                { "Other_Grapheme_Extend", wr::ucd::OTHER_GRAPHEME_EXTEND },
                { "IDS_Binary_Operator", wr::ucd::IDS_BINARY_OPERATOR },
                { "IDS_Trinary_Operator", wr::ucd::IDS_TRINARY_OPERATOR },
                { "Radical", wr::ucd::RADICAL },
                { "Unified_Ideograph", wr::ucd::UNIFIED_IDEOGRAPH },
                { "Other_Default_Ignorable_Code_Point",
                                wr::ucd::OTHER_DEFAULT_IGNORABLE_CODE_POINT },
                { "Deprecated", wr::ucd::DEPRECATED },
                { "Soft_Dotted", wr::ucd::SOFT_DOTTED },
                { "Logical_Order_Exception", wr::ucd::LOGICAL_ORDER_EXCEPTION },
                { "Other_ID_Start", wr::ucd::OTHER_ID_START },
                { "Other_ID_Continue", wr::ucd::OTHER_ID_CONTINUE },
                { "Sentence_Terminal", wr::ucd::SENTENCE_TERMINAL },
                { "Variation_Selector", wr::ucd::VARIATION_SELECTOR },
                { "Pattern_White_Space", wr::ucd::PATTERN_WHITE_SPACE },
                { "Pattern_Syntax", wr::ucd::PATTERN_SYNTAX },
                { "Prepended_Concatenation_Mark",
                                        wr::ucd::PREPENDED_CONCATENATION_MARK },
                { "Math", wr::ucd::MATH },
                { "Alphabetic", wr::ucd::ALPHABETIC },
                { "Lowercase", wr::ucd::LOWERCASE },
                { "Uppercase", wr::ucd::UPPERCASE },
                { "Cased", wr::ucd::CASED },
                { "Case_Ignorable", wr::ucd::CASE_IGNORABLE },
                { "Changes_When_Lowercased", wr::ucd::CHANGES_WHEN_LOWERCASED },
                { "Changes_When_Uppercased", wr::ucd::CHANGES_WHEN_UPPERCASED },
                { "Changes_When_Titlecased", wr::ucd::CHANGES_WHEN_TITLECASED },
                { "Changes_When_Casefolded", wr::ucd::CHANGES_WHEN_CASEFOLDED },
                { "Changes_When_Casemapped", wr::ucd::CHANGES_WHEN_CASEMAPPED },
                { "ID_Start", wr::ucd::ID_START },
                { "ID_Continue", wr::ucd::ID_CONTINUE },
                { "XID_Start", wr::ucd::XID_START },
                { "XID_Continue", wr::ucd::XID_CONTINUE },
                { "Default_Ignorable_Code_Point",
                                        wr::ucd::DEFAULT_IGNORABLE_CODE_POINT },
                { "Grapheme_Extend", wr::ucd::GRAPHEME_EXTEND },
                { "Grapheme_Base", wr::ucd::GRAPHEME_BASE },
                { "Grapheme_Link", wr::ucd::GRAPHEME_LINK },
        };

        std::string line, property_name;
        uint64_t    property_bit = 0;
        int         exit_status = EXIT_SUCCESS;

        for (unsigned int line_no = 1; in.good(); ++line_no) {
                line.clear();
                std::getline(in, line);
                line += '\n';

                auto contents = wr::string_view(line).split('#').first.trim();
                if (contents.empty()) {
                        continue;
                }

                auto     fields           = contents.split(';');
                auto     code_point_range = fields.first.trim().split("..");
                char32_t c1, c2;

                try {
                        c1 = charCode(code_point_range.first.trim());
                        if (!code_point_range.second.trim().empty()) {
                                c2 = charCode(code_point_range.second);
                        } else {
                                c2 = c1;
                        }
                } catch (const std::exception &) {
                        std::cerr << file_name << " line " << line_no
                                  << ": malformed code point ";
                        if (!code_point_range.second.empty()) {
                                std::cerr << "range \"";
                        }
                        std::cerr << fields.first << "\"\n";
                        exit_status = EXIT_FAILURE;
                        break;
                }

                if (c1 >= wr::ucd::CODE_SPACE_SIZE) {
                        continue;
                }
                if (c2 >= wr::ucd::CODE_SPACE_SIZE) {
                        c2 = wr::ucd::CODE_SPACE_SIZE - 1;
                }
                                
                if (fields.second.trim() != property_name) {
                        property_name = fields.second.to_string();
                        auto i = PROPERTIES.find(property_name);

                        if (i != PROPERTIES.end()) {
                                property_bit = i->second;
                        } else {
                                std::cerr << file_name << " line " << line_no
                                          << ": warning: ignoring unrecognised "
                                             "property \"" << property_name
                                          << "\"\n";
                                property_bit = 0;
                        }
                }

                if (property_bit) {
                        for (char32_t c = c1; c <= c2; ++c) {
                                pageEntry(data.property_index,
                                          data.property_page,
                                          c) |= property_bit;
                        }
                }

                if (property_bit & wr::ucd::HEX_DIGIT) {
                        for (char32_t c = c1; c <= c2; ++c) {
                                auto value = static_cast<uint8_t>(c - c1);
                                if (category(c) != wr::ucd::DECIMAL_NUMBER) {
                                        value += 10;
                                };
                                pageEntry(data.xdigit_index,
                                          data.xdigit_page, c) = value;
                        }
                }
        }

        if (in.bad()) {
                std::cerr << file_name << ": I/O error\n";
                exit_status = EXIT_FAILURE;
        }

        return exit_status;
} catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << '\n';
        return EXIT_FAILURE;
}

//--------------------------------------

template <typename PageType> struct InitPage;

//--------------------------------------

template <>
struct InitPage<wr::ucd::CategoryPage>
{
        static void init(wr::ucd::CategoryPage &page, char32_t /* start */)
                { page.fill(wr::ucd::UNASSIGNED); }

        static const char *typeName() { return "CategoryPage"; }
};

//--------------------------------------

template <>
struct InitPage<wr::ucd::CasePage>
{
        static void
        init(
                wr::ucd::CasePage &page,
                char32_t           start
        )
        {
                for (auto &i: page) {
                        i = start++;
                }
        }

        static const char *typeName() { return "CasePage"; }
};

//--------------------------------------

template <>
struct InitPage<wr::ucd::DigitPage>
{
        static void init(wr::ucd::DigitPage &page, char32_t /* start */)
                { page.fill(-1); }

        static const char *typeName() { return "DigitPage"; }
};

//--------------------------------------

template <>
struct InitPage<wr::ucd::PropertyPage>
{
        static void init(wr::ucd::PropertyPage &page, char32_t /* start */)
                { page.fill(0); }

        static const char *typeName() { return "PropertyPage"; }
};

//--------------------------------------

template <>
struct InitPage<wr::ucd::CharClassPage>
{
        static void init(wr::ucd::CharClassPage &page, char32_t /* start */)
                { page.fill({}); }

        static const char *typeName() { return "CharClassPage"; }
};

//--------------------------------------

template <typename T> static T &
pageEntry(
        PageIndex    &index,
        PageArray<T> &pages,
        char32_t      c
)
{
        auto i = c / wr::ucd::PAGE_SIZE;
        assert(i <= wr::ucd::PAGE_INDEX_MAX);

        if (index[i] == wr::ucd::PAGE_NOT_USED) {
                pages.emplace_back();
                InitPage<wr::ucd::Page<T>>::init(pages.back(),
                        static_cast<char32_t>(i) * wr::ucd::PAGE_SIZE);
                assert(pages.size() <= (wr::ucd::PAGE_INDEX_MAX + 1));
                index[i] = static_cast<int16_t>(pages.size() - 1);
        }

        return pages[index[i]][c % wr::ucd::PAGE_SIZE];
}

//--------------------------------------

template <typename T> static void
createDefaultPage(
        PageIndex    &index,
        PageArray<T> &pages
)
{
        int16_t default_page_ix = -1;

        for (int i = 0; i <= wr::ucd::PAGE_INDEX_MAX; ++i) {
                if (index[i] >= 0) {
                        continue;
                }
                if (default_page_ix < 0) {
                        pages.emplace_back();
                        InitPage<wr::ucd::Page<T>>::init(pages.back(),
                                static_cast<char32_t>(i) * wr::ucd::PAGE_SIZE);
                        assert(pages.size() <= (wr::ucd::PAGE_INDEX_MAX + 1));
                        default_page_ix
                                 = static_cast<int16_t>(pages.size() - 1);
                }
                index[i] = default_page_ix;
        }
}

//--------------------------------------

template <typename T> static size_t
removeDuplicatePages(
        PageIndex    &index,
        PageArray<T> &pages
)
{
        if (pages.empty()) {
                return 0;
        }

        size_t n_dup = 0;

        for (int i = 0, k = static_cast<int>(pages.size()) - 1; i < k; ++i) {
                for (int j = i + 1; j <= k; ) {
                        if (pages[i] == pages[j]) {
                                ++n_dup;
                                pages.erase(pages.begin() + j);
                                k = static_cast<int>(pages.size()) - 1;

                                for (auto &ix: index) {
                                        if (ix == j) {
                                                ix = i;
                                        } else if (ix > j) {
                                                --ix;
                                        }
                                }
                        } else {
                                ++j;
                        }
                }
        }

        return n_dup;
}

//--------------------------------------

static void
generateCharClasses()
{
        for (char32_t c = 0; c < wr::ucd::CODE_SPACE_SIZE; ++c) {
                std::ctype_base::mask class_mask = {};
                auto props     = properties(c);
                auto cat       = category(c);
                auto cat_major = cat & wr::ucd::MAJOR_CATEGORY_MASK;

                if (props & wr::ucd::ALPHABETIC) {
                        class_mask |= wr::ucd::alpha_bits;
                }

                if (props & wr::ucd::LOWERCASE) {
                        class_mask |= std::ctype_base::lower;
                } else if (props & wr::ucd::UPPERCASE) {
                        class_mask |= std::ctype_base::upper;
                }

                if (((cat_major == wr::ucd::PUNCTUATION)
                        || ((c <= 0x7f) && (cat_major == wr::ucd::SYMBOL)))
                    && !(props & wr::ucd::ALPHABETIC)) {
                        class_mask |= std::ctype_base::punct;
                }

                if (cat == wr::ucd::DECIMAL_NUMBER) {
                        class_mask |= std::ctype_base::digit;
                }

                if (props & wr::ucd::HEX_DIGIT) {
                        class_mask |= wr::ucd::xdigit_bits;
                }

                if (props & wr::ucd::WHITE_SPACE) {
                        class_mask |= std::ctype_base::space;
                }

                if (c == 9) {
                        class_mask |= wr::ucd::blank_bits;
                        class_mask |= std::ctype_base::cntrl;
                } else if (cat == wr::ucd::SPACE_SEPARATOR) {
                        class_mask |= wr::ucd::blank_bits;
                        class_mask |= wr::ucd::print_bits;
                } else if (cat == wr::ucd::CONTROL) {
                        class_mask |= std::ctype_base::cntrl;
                } else if (!(class_mask & std::ctype_base::space)
                           && (cat != wr::ucd::SURROGATE)
                           && (cat != wr::ucd::UNASSIGNED)) {
                        class_mask |= wr::ucd::print_bits;
                }

                if (class_mask) {
                        pageEntry(data.char_class_index,
                                  data.char_class_page, c) = class_mask;
                }
        }
}

//--------------------------------------

template <typename PageType> static void
outputCXXFile(
        const char            *name,
        const char            *description,
        const char            *symbol_prefix,
        PageIndex             &page_index,
        std::vector<PageType> &pages,
        size_t                &total_size
)
try {
        if (typeid(PageType) != typeid(wr::ucd::CasePage)) {
                // default pages not created for casing data
                createDefaultPage(page_index, pages);
        }

        removeDuplicatePages(page_index, pages);

        std::ofstream output;
        output.exceptions(output.failbit);
        output.open(name);

        output << "/*\n"
                  " * " << name << "\n"
                  " * This file is automatically generated by unidatagen.\n"
                  " */\n"
                  "\n"
                  "#include <wrutil/UnicodeData.h>\n"
                  "\n"
                  "\n"
                  "namespace wr {\n"
                  "namespace ucd {\n"
                  "\n"
                  "\n";

        outputPageIndex(page_index, symbol_prefix, output);
        output << "\n//--------------------------------------\n\n";
        outputPages(pages, symbol_prefix, output);

        output << "\n"
                  "\n"
                  "} // namespace ucd\n"
                  "} // namespace wr\n"
                  "\n";

        size_t size = sizeof(PageIndex) + (sizeof(PageType) * pages.size());
        total_size += size;
        std::cerr << description << ": " << size / 1024
                  << "KB in " << pages.size() << " pages\n";
} catch (const std::ios_base::failure &) {
        throw std::runtime_error(
                "I/O error occurred writing " + std::string(name) + '\n');
}

//--------------------------------------

static void
outputPageIndex(
        const PageIndex       &index,
        const wr::string_view &name,
        std::ostream          &output
)
{
        output << "WRUTIL_API const int16_t " << name << "_index[] = {\n"
                  "        ";

        const char *sep = "";

        for (int i = 0, size = wr::ucd::PAGE_INDEX_MAX + 1; i < size; ) {
                for (int j = 0; (i < size) && (j < 8); ++i, ++j) {
                        output << sep << index[i];
                        sep = ", ";
                }

                sep = ",\n        ";
        }

        output << "\n"
                  "};\n"
                  "\n";
}

//-----------------------------------------------

template <typename T> static void
outputPages(
        const PageArray<T>    &pages,
        const wr::string_view &name,
        std::ostream          &output
)
{
        output << "WRUTIL_API const " << InitPage<wr::ucd::Page<T>>::typeName()
               << ' ' << name << "_page[] = {\n";

        const char *sep = "";

        for (const auto &page: pages) {
                output << sep << "        { /* "
                       << (&page - &pages[0]) << " / " << pages.size() - 1
                       << " */\n                ";

                sep = "";

                for (size_t i = 0; i < page.size(); ) {
                        for (size_t j = 0; (i < page.size()) && (j < 8);
                                                                ++i, ++j) {
                                output << sep << static_cast<int64_t>(page[i]);
                                sep = ", ";
                        }

                        sep = ",\n                ";
                }

                output << "\n        }";
                sep = ",\n";
        }

        output << "\n};\n";
}
