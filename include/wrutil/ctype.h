/**
 * \file ctype.h
 *
 * \brief Unicode character classification API
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
#ifndef WRUTIL_CTYPE_H
#define WRUTIL_CTYPE_H

#include <locale>
#include <wrutil/Config.h>
#include <wrutil/UnicodeData.h>


namespace wr {


enum : char32_t
{
        INVALID_CHAR = 0x0000fffdU
};

//--------------------------------------

inline bool isuspace(char32_t c)
        { return (ucd::class_(c) & std::ctype_base::space) != 0; }

inline bool isudigit(char32_t c)
        { return (ucd::class_(c) & std::ctype_base::digit) != 0; }

inline bool isupunct(char32_t c)
        { return (ucd::class_(c) & std::ctype_base::punct) != 0; }

inline bool isusymbol(char32_t c)
        { return ucd::major_category(c) == ucd::SYMBOL; }

inline bool isucntrl(char32_t c)
        { return (ucd::class_(c) & std::ctype_base::cntrl) != 0; }

inline bool isuupper(char32_t c)
        { return (ucd::class_(c) & std::ctype_base::upper) != 0; }

inline bool isulower(char32_t c)
        { return (ucd::class_(c) & std::ctype_base::lower) != 0; }

inline bool isutitle(char32_t c)
        { return (ucd::properties(c)
                        & (ucd::LOWERCASE | ucd::UPPERCASE | ucd::CASED))
                                == ucd::CASED; }

//--------------------------------------

bool isuprint(char32_t c);
bool isublank(char32_t c);


inline bool
isugraph(
        char32_t c
)
{
#if WR_HAVE_STD_CTYPE_BLANK
        if (ucd::print_bits & ucd::blank_bits) {
                return (ucd::class_(c) & (std::ctype_base::print
                                          | std::ctype_base::blank))
                        == std::ctype_base::print;
        } else {
                return isuprint(c) && !isublank(c);
        }
#else
        return isuprint(c) && !isublank(c);
#endif
}

//--------------------------------------

inline bool
isualpha(
        char32_t c
)
{
        if (ucd::alpha_bits) {
                return (ucd::class_(c) & std::ctype_base::alpha) != 0;
        } else {
                return (ucd::properties(c) & ucd::ALPHABETIC) != 0;
        }
}

//--------------------------------------

inline bool
isualnum(
        char32_t c
)
{
        if (ucd::alpha_bits) {
                return (ucd::class_(c) & (std::ctype_base::alpha
                                          | std::ctype_base::digit)) != 0;
        } else {
                return isualpha(c) || isudigit(c);
        }
}

//--------------------------------------

inline bool
isublank(
        char32_t c
)
{
#if WR_HAVE_STD_CTYPE_BLANK
        if (ucd::blank_bits) {
                return (ucd::class_(c) & std::ctype_base::blank) != 0;
        } else {
                return (c == 9) || (ucd::category(c) == ucd::SPACE_SEPARATOR);
        }
#else
        return (c == 9) || (ucd::category(c) == ucd::SPACE_SEPARATOR);
#endif
}

//--------------------------------------

inline bool
isuprint(
        char32_t c
)
{
        auto cclass = ucd::class_(c);
        if (ucd::print_bits) {
                return (cclass & std::ctype_base::print) != 0;
        } else {
                auto cat = ucd::category(c);
                return (cat == wr::ucd::SPACE_SEPARATOR)
                        || (!(cclass & std::ctype_base::space)
                             && (cat != wr::ucd::CONTROL)
                             && (cat != wr::ucd::SURROGATE)
                             && (cat != wr::ucd::UNASSIGNED));
        }
}

//--------------------------------------

inline bool
isuxdigit(
        char32_t c
)
{
        if (ucd::xdigit_bits) {
                return (ucd::class_(c) & std::ctype_base::xdigit) != 0;
        } else {
                return (ucd::properties(c) & ucd::HEX_DIGIT) != 0;
        }
}

//--------------------------------------

WRUTIL_API char32_t lookup_alt_case(const int16_t *page_index,
                                    const ucd::CasePage *pages, char32_t c);

inline char32_t touupper(char32_t c)
        { return lookup_alt_case(ucd::uppercase_index,
                                 ucd::uppercase_page, c); }

inline char32_t toulower(char32_t c)
        { return lookup_alt_case(ucd::lowercase_index,
                                 ucd::lowercase_page, c); }

inline char32_t toutitle(char32_t c)
        { return lookup_alt_case(ucd::titlecase_index,
                                 ucd::titlecase_page, c); }

inline short digitval(char32_t c)
        { return ucd::lookup(ucd::digit_index, ucd::digit_page, c); }

inline short xdigitval(char32_t c)
        { return ucd::lookup(ucd::xdigit_index, ucd::xdigit_page, c); }

inline unsigned short udigitval(char32_t c)
        { return static_cast<uint8_t>(digitval(c)); }

inline unsigned short uxdigitval(char32_t c)
        { return static_cast<uint8_t>(xdigitval(c)); }


} // namespace wr

//-------------------------------------

namespace std {


template <>
class ctype<char32_t> :
        public ctype_base
#if !WR_DINKUM  // not Dinkumware (MSVC++ etc)
        , public locale::facet
#endif
{
public:
        using char_type = char32_t;

        using facet_base_t =
#if WR_DINKUM
                ctype_base
#else
                locale::facet
#endif
                ;
        

        ctype(size_t refs = 0) : facet_base_t(refs) {}

        ctype(std::locale loc, size_t refs = 0) :
                facet_base_t(refs), loc_(loc) {}

        bool is(mask m, char_type c) { return do_is(m, c); }

        const char_type *is(const char_type *low, const char_type *high,
                            mask *vec) const
                { return do_is(low, high, vec); }

        const char_type *scan_is(mask m, const char_type *begin,
                                 const char_type *end) const
                { return do_scan_is(m, begin, end); }

        const char_type *scan_not(mask m, const char_type *begin,
                                  const char_type *end) const
                { return do_scan_not(m, begin, end); }

        char_type toupper(char_type c) const { return do_toupper(c); }

        const char_type *toupper(char_type *begin, char_type *end) const
                { return do_toupper(begin, end); }

        char_type tolower(char_type c) const { return do_toupper(c); }

        const char_type *tolower(char_type *begin, char_type *end) const
                { return do_tolower(begin, end); }

        char_type widen(char c) const { return do_widen(c); }

        const char *widen(const char *begin,
                          const char *end, char_type *dest) const
                { return do_widen(begin, end, dest); }

        char narrow(char_type c, char dflt) const { return do_narrow(c, dflt); }

        const char_type *narrow(const char_type *begin, const char_type *end,
                                char dflt, char *dest) const
                { return do_narrow(begin, end, dflt, dest); }

        std::locale get_locale() const { return loc_; }

protected:
        virtual WRUTIL_API bool do_is(mask m, char_type c) const;

        virtual WRUTIL_API const char_type *do_is(const char_type *low,
                                                  const char_type *high,
                                                  mask *vec) const;
        virtual WRUTIL_API const char_type *
                do_scan_is(mask m, const char_type *begin,
                           const char_type *end) const;

        virtual WRUTIL_API const char_type *
                do_scan_not(mask m, const char_type *begin,
                            const char_type *end) const;

        virtual WRUTIL_API char_type do_toupper(char_type c) const;
        virtual WRUTIL_API const char_type *do_toupper(char_type *begin,
                                                       char_type *end) const;

        virtual WRUTIL_API char_type do_tolower(char_type c) const;
        virtual WRUTIL_API const char_type *do_tolower(char_type *begin,
                                                       char_type *end) const;

        virtual WRUTIL_API char_type do_widen(char c) const;

        virtual WRUTIL_API const char *do_widen(const char *begin,
                                                const char *end,
                                                char_type *dest) const;

        virtual WRUTIL_API char do_narrow(char_type c, char dflt) const;

        virtual WRUTIL_API const char_type *do_narrow(const char_type *begin,
                                                      const char_type *end,
                                                      char dflt,
                                                      char *dest) const;

private:
        std::locale loc_;
};

//-------------------------------------

template <> WRUTIL_API const ctype<char32_t> &
        use_facet<ctype<char32_t>>(const locale &loc);

template <> inline bool isspace(char32_t c, const locale &/* loc */)
        { return wr::isuspace(c); }

#if WR_HAVE_STD_ISBLANK_TEMPLATE
template <> inline bool isblank(char32_t c, const locale &/* loc */)
        { return wr::isublank(c); }
#endif

template <> inline bool iscntrl(char32_t c, const locale &/* loc */)
        { return wr::isucntrl(c); }

template <> inline bool isupper(char32_t c, const locale &/* loc */)
        { return wr::isuupper(c); }

template <> inline bool islower(char32_t c, const locale &/* loc */)
        { return wr::isulower(c); }

template <> inline bool isalpha(char32_t c, const locale &/* loc */)
        { return wr::isualpha(c); }

template <> inline bool isdigit(char32_t c, const locale &/* loc */)
        { return wr::isudigit(c); }

template <> inline bool ispunct(char32_t c, const locale &/* loc */)
        { return wr::isupunct(c); }

template <> inline bool isxdigit(char32_t c, const locale &/* loc */)
        { return wr::isuxdigit(c); }

template <> inline bool isalnum(char32_t c, const locale &/* loc */)
        { return wr::isualnum(c); }

template <> inline bool isprint(char32_t c, const locale &/* loc */)
        { return wr::isuprint(c); }

template <> inline bool isgraph(char32_t c, const locale &/* loc */)
        { return wr::isugraph(c); }

template <> inline char32_t toupper(char32_t c, const locale &/* loc */)
        { return wr::touupper(c); }

template <> inline char32_t tolower(char32_t c, const locale &/* loc */)
        { return wr::toulower(c); }


} // namespace std


#endif // !WRUTIL_CTYPE_H
