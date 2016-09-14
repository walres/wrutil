/**
 * \file UnicodeData.h
 *
 * \brief Unicode character data access API
 *
 * \copyright
 * \parblock
 *
 *   Copyright 2016 James S. Waller
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
#ifndef WRUTIL_UNICODE_DATA_H
#define WRUTIL_UNICODE_DATA_H

#include <wrutil/Config.h>
#include <stdint.h>
#include <array>
#include <locale>


namespace wr {
namespace ucd {


/**
 * \brief Unicode general categories
 */
enum : uint8_t {
        MAJOR_CATEGORY_MASK = 0xf0,

        LETTER = 0x00,
        UPPERCASE_LETTER = LETTER,              // Lu
        LOWERCASE_LETTER,                       // Ll
        TITLECASE_LETTER,                       // Lt
        MODIFIER_LETTER,                        // Lm
        OTHER_LETTER,                           // Lo

        MARK = 0x10,
        NONSPACING_MARK = MARK,                 // Mn
        SPACING_MARK,                           // Mc
        ENCLOSING_MARK,                         // Me

        NUMBER = 0x20,
        DECIMAL_NUMBER = NUMBER,                // Nd
        LETTER_NUMBER,                          // Nl
        OTHER_NUMBER,                           // No

        PUNCTUATION = 0x30,
        CONNECTOR_PUNCTUATION = PUNCTUATION,    // Pc
        DASH_PUNCTUATION,                       // Pd
        OPEN_PUNCTUATION,                       // Ps
        CLOSE_PUNCTUATION,                      // Pe
        INITIAL_PUNCTUATION,                    // Pi
        FINAL_PUNCTUATION,                      // Pf
        OTHER_PUNCTUATION,                      // Po

        SYMBOL = 0x40,
        MATH_SYMBOL = SYMBOL,                   // Sm
        CURRENCY_SYMBOL,                        // Sc
        MODIFIER_SYMBOL,                        // Sk
        OTHER_SYMBOL,                           // So

        SEPARATOR = 0x50,
        SPACE_SEPARATOR = SEPARATOR,            // Zs
        LINE_SEPARATOR,                         // Zl
        PARAGRAPH_SEPARATOR,                    // Zp

        OTHER = 0x60,
        CONTROL = OTHER,                        // Cc
        FORMAT,                                 // Cf
        SURROGATE,                              // Cs
        PRIVATE_USE,                            // Co
        UNASSIGNED                              // Cn
};

//--------------------------------------
/**
 * \brief Unicode properties
 */
enum : uint64_t
{
        // core / contributory properties
        WHITE_SPACE = 1ULL << 0,
        BIDI_CONTROL = 1ULL << 1,
        JOIN_CONTROL = 1ULL << 2,
        DASH = 1ULL << 3,
        HYPHEN = 1ULL << 4,
        QUOTATION_MARK = 1ULL << 5,
        TERMINAL_PUNCTUATION = 1ULL << 6,
        OTHER_MATH = 1ULL << 7,
        HEX_DIGIT = 1ULL << 8,
        ASCII_HEX_DIGIT = 1ULL << 9,
        OTHER_ALPHABETIC = 1ULL << 10,
        IDEOGRAPHIC = 1ULL << 11,
        DIACRITIC = 1ULL << 12,
        EXTENDER = 1ULL << 13,
        OTHER_LOWERCASE = 1ULL << 14,
        OTHER_UPPERCASE = 1ULL << 15,
        NONCHARACTER_CODE_POINT = 1ULL << 16,
        OTHER_GRAPHEME_EXTEND = 1ULL << 17,
        IDS_BINARY_OPERATOR = 1ULL << 18,
        IDS_TRINARY_OPERATOR = 1ULL << 19,
        RADICAL = 1ULL << 20,
        UNIFIED_IDEOGRAPH = 1ULL << 21,
        OTHER_DEFAULT_IGNORABLE_CODE_POINT = 1ULL << 22,
        DEPRECATED = 1ULL << 23,
        SOFT_DOTTED = 1ULL << 24,
        LOGICAL_ORDER_EXCEPTION = 1ULL << 25,
        OTHER_ID_START = 1ULL << 26,
        OTHER_ID_CONTINUE = 1ULL << 27,
        SENTENCE_TERMINAL = 1ULL << 28,
        VARIATION_SELECTOR = 1ULL << 29,
        PATTERN_WHITE_SPACE = 1ULL << 30,
        PATTERN_SYNTAX = 1ULL << 31,
        PREPENDED_CONCATENATION_MARK = 1ULL << 32,

        // derived core properties
        MATH = 1ULL << 40,
        ALPHABETIC = 1ULL << 41,
        LOWERCASE = 1ULL << 42,
        UPPERCASE = 1ULL << 43,
        CASED = 1ULL << 44,
        CASE_IGNORABLE = 1ULL << 45,
        CHANGES_WHEN_LOWERCASED = 1ULL << 46,
        CHANGES_WHEN_UPPERCASED = 1ULL << 47,
        CHANGES_WHEN_TITLECASED = 1ULL << 48,
        CHANGES_WHEN_CASEFOLDED = 1ULL << 49,
        CHANGES_WHEN_CASEMAPPED = 1ULL << 50,
        ID_START = 1ULL << 51,
        ID_CONTINUE = 1ULL << 52,
        XID_START = 1ULL << 53,
        XID_CONTINUE = 1ULL << 54,
        DEFAULT_IGNORABLE_CODE_POINT = 1ULL << 55,
        GRAPHEME_EXTEND = 1ULL << 56,
        GRAPHEME_BASE = 1ULL << 57,
        GRAPHEME_LINK = 1ULL << 58,
};

//--------------------------------------

enum
{
        CODE_SPACE_SIZE = 0x110000,

        PAGE_SIZE       = 256,
        PAGE_INDEX_MAX  = (CODE_SPACE_SIZE / PAGE_SIZE) - 1,
        PAGE_NOT_USED   = -1
};

//--------------------------------------

template <typename T> using Page = std::array<T, PAGE_SIZE>;

using CategoryPage  = Page<uint8_t>;
using CasePage      = Page<char32_t>;
using DigitPage     = Page<int8_t>;
using PropertyPage  = Page<uint64_t>;
using CharClassPage = Page<std::ctype_base::mask>;

//--------------------------------------

// page arrays
extern WRUTIL_API const CategoryPage  category_page[];
extern WRUTIL_API const CasePage      uppercase_page[],
                                      lowercase_page[],
                                      titlecase_page[];
extern WRUTIL_API const DigitPage     digit_page[],
                                      xdigit_page[];
extern WRUTIL_API const PropertyPage  property_page[];
extern WRUTIL_API const CharClassPage char_class_page[];

/* page indexes: arrays of indices into page arrays
   where -1 = not used (uppercase/lowercase/titlecase_index only) */
extern WRUTIL_API const int16_t category_index[],
                                uppercase_index[],
                                lowercase_index[],
                                titlecase_index[],
                                digit_index[],
                                xdigit_index[],
                                property_index[],
                                char_class_index[];

//--------------------------------------

constexpr std::ctype_base::mask nprint_bits
        = std::ctype_base::space | std::ctype_base::cntrl
                | std::ctype_base::upper | std::ctype_base::lower
                | std::ctype_base::alpha | std::ctype_base::digit
                | std::ctype_base::punct | std::ctype_base::xdigit
#if WR_HAVE_STD_CTYPE_BLANK
                | std::ctype_base::blank
#endif
                ;

constexpr std::ctype_base::mask print_bits
        = std::ctype_base::print & (~nprint_bits);

constexpr std::ctype_base::mask nalpha_bits
        = std::ctype_base::space | print_bits | std::ctype_base::cntrl
                | std::ctype_base::upper | std::ctype_base::lower
                | std::ctype_base::digit | std::ctype_base::punct
                | std::ctype_base::xdigit
#if WR_HAVE_STD_CTYPE_BLANK
                | std::ctype_base::blank
#endif
                ;

constexpr std::ctype_base::mask alpha_bits
        = std::ctype_base::alpha & (~nalpha_bits);

constexpr std::ctype_base::mask nxdigit_bits
        = std::ctype_base::space | print_bits | std::ctype_base::cntrl
                | std::ctype_base::upper | std::ctype_base::lower
                | std::ctype_base::alpha | std::ctype_base::digit
                | std::ctype_base::punct
#if WR_HAVE_STD_CTYPE_BLANK
                | std::ctype_base::blank
#endif
                ;

constexpr std::ctype_base::mask xdigit_bits
        = std::ctype_base::xdigit & (~nxdigit_bits);

constexpr std::ctype_base::mask nblank_bits
        = std::ctype_base::space | print_bits | std::ctype_base::cntrl
                | std::ctype_base::upper | std::ctype_base::lower
                | std::ctype_base::alpha | std::ctype_base::digit
                | std::ctype_base::punct | std::ctype_base::xdigit;

constexpr std::ctype_base::mask blank_bits
#if WR_HAVE_STD_CTYPE_BLANK
        = std::ctype_base::blank & (~nblank_bits);
#else
        = 0;
#endif

//--------------------------------------

template <typename T> inline T lookup(const int16_t *page_index,
                                      const Page<T> *pages, char32_t c)
        { return pages[page_index[c >> 8]][c & 0xff]; }

inline uint8_t category(char32_t c)
        { return lookup(category_index, category_page, c); }

inline uint8_t major_category(char32_t c)
        { return category(c) & MAJOR_CATEGORY_MASK; }

inline uint64_t properties(char32_t c)
        { return lookup(property_index, property_page, c); }

inline std::ctype_base::mask class_(char32_t c)
        { return ucd::lookup(ucd::char_class_index, ucd::char_class_page, c); }


} // namespace ucd
} // namespace wr


#endif // !WRUTIL_UNICODE_DATA_H
