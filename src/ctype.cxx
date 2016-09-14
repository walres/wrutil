/**
 * \file ctype.cxx
 *
 * \brief Implementation of std::ctype<char32_t> specialization for
 *        32-bit Unicode characters
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
#include <wrutil/ctype.h>
#include <wrutil/codecvt.h>
#include <wrutil/utf8.h>


namespace wr {


WRUTIL_API char32_t
lookup_alt_case(
        const int16_t       *page_index,
        const ucd::CasePage *pages,
        char32_t             c
)
{
        auto i_page = page_index[c >> 8];
        if (i_page >= 0) {
                c = pages[i_page][c & 0xff];
        }
        return c;
}


} // namespace wr

//--------------------------------------

namespace std {


WRUTIL_API bool
ctype<char32_t>::do_is(
        mask      m,
        char_type c
) const
{
        if (!wr::ucd::print_bits && ((m & print) == print) && wr::isuprint(c)) {
                return true;
        }

        if (!wr::ucd::alpha_bits && ((m & alpha) == alpha) && wr::isualpha(c)) {
                return true;
        }

        if (!wr::ucd::xdigit_bits && ((m & xdigit) == xdigit)) {
                if (wr::isuxdigit(c)) {
                        return true;
                }
        }

#if WR_HAVE_STD_CTYPE_BLANK
        if (!wr::ucd::blank_bits && ((m & blank) == blank) && wr::isublank(c)) {
                return true;
        }
#endif

        return (wr::ucd::class_(c) & m) != 0;
}

//--------------------------------------

WRUTIL_API auto
ctype<char32_t>::do_is(
        const char_type *low,
        const char_type *high,
        mask            *vec
) const -> const char_type *
{
        for (; low != high; ++low, ++vec) {
                *vec = wr::ucd::class_(*low);
                if (!wr::ucd::print_bits && wr::isuprint(*low)) {
                        *vec |= print;
                }
                if (!wr::ucd::alpha_bits && wr::isualpha(*low)) {
                        *vec |= alpha;
                }
                if (!wr::ucd::xdigit_bits && wr::isuxdigit(*low)) {
                        *vec |= xdigit;
                }
#if WR_HAVE_STD_CTYPE_BLANK
                if (!wr::ucd::blank_bits && wr::isublank(*low)) {
                        *vec |= blank;
                }
#endif
        }

        return high;
}

//--------------------------------------

WRUTIL_API auto
ctype<char32_t>::do_scan_is(
        mask             m,
        const char_type *begin,
        const char_type *end
) const -> const char_type *
{
        char_type prev = -1;

        for (; begin < end; ++begin) {
                if ((*begin != prev) && do_is(m, *begin)) {
                        break;
                }
        }

        return begin;
}

//--------------------------------------

WRUTIL_API auto
ctype<char32_t>::do_scan_not(
        mask             m,
        const char_type *begin,
        const char_type *end
) const -> const char_type *
{
        char_type prev = -1;

        for (; begin < end; ++begin) {
                if ((*begin != prev) && !do_is(m, *begin)) {
                        break;
                }
        }

        return begin;
}

//--------------------------------------

WRUTIL_API auto
ctype<char32_t>::do_toupper(
        char_type c
) const -> char_type
{
        return wr::touupper(c);
}

//--------------------------------------

WRUTIL_API auto
ctype<char32_t>::do_toupper(
        char_type *begin,
        char_type *end
) const -> const char_type *
{
        for (; begin < end; ++begin) {
                *begin = do_toupper(*begin);
        }

        return begin;
}

//--------------------------------------

WRUTIL_API auto
ctype<char32_t>::do_tolower(
        char_type c
) const -> char_type
{
        return wr::toulower(c);
}

//--------------------------------------

WRUTIL_API auto
ctype<char32_t>::do_tolower(
        char_type *begin,
        char_type *end
) const -> const char_type *
{
        for (; begin < end; ++begin) {
                *begin = do_tolower(*begin);
        }

        return begin;
}

//--------------------------------------

WRUTIL_API auto
ctype<char32_t>::do_widen(
        char c
) const -> char_type
{
        char32_t result;
        do_widen(&c, &c + 1, &result);
        return result;
}

//--------------------------------------

WRUTIL_API const char *
ctype<char32_t>::do_widen(
        const char *begin,
        const char *end,
        char_type  *dest
) const
{
        if (begin >= end) {
                return end;
        }

#if __STDC_ISO_10646__  // wchar_t is UTF-32
        auto &wctype = use_facet<ctype<wchar_t>>(loc_);
        return wctype.widen(begin, end, reinterpret_cast<wchar_t *>(dest));
#else
        auto   &to_wide = use_facet<codecvt<wchar_t, char, mbstate_t>>(loc_);
#   if !WR_WINDOWS
        auto   *to_utf8 = new codecvt_byname<wchar_t, char,
                                             mbstate_t>("en_US.utf8");
        locale  loc2(loc_, to_utf8);  // required to destroy to_utf8
#   endif
        wchar_t buf[2];  /* use 2 in case wchar_t is UTF-16 (Windows)
                            and result is outside Basic Multilingual Plane */

        for (; begin < end; ++begin, ++dest) {
                mbstate_t   state = { 0 };
                union
                {
                        const char *c_cend;
                        char       *c_end;
                };
                union
                {
                        const wchar_t *w_cend;
                        wchar_t       *w_end;
                };

                if (to_wide.in(state, begin, begin + 1,
                               c_cend, buf, &buf[2], w_end) != to_wide.ok) {
                        *dest = static_cast<char32_t>(-1);
                        continue;
                }

                // at this point buf contains wchar_t encoding of *begin

#   if WR_WINDOWS
                switch (w_end - buf) {
                case 1:
                        *dest = static_cast<char32_t>(buf[0]);
                        break;
                case 2:  // result outside BMP, should be surrogate pair
                        if ((buf[0] >= 0xd800) && (buf[0] <= 0xdbff)
                            && (buf[1] >= 0xdc00) && (buf[1] <= 0xdfff)) {
                                // valid (high, low) surrogate pair
                                *dest = (buf[0] - 0xd800);
                                *dest <<= 10;
                                *dest |= (buf[1] - 0xdc00);
                                *dest += 0x10000;
                                break;
                        } // else fall through
                default:
                        *dest = static_cast<char32_t>(-1);
                        break;
                }
#   else
                char utf8[4];

                if (to_utf8->out(state, buf, &buf[2], w_cend,
                                 utf8, &utf8[4], c_end) == to_utf8->ok) {
                        *dest = wr::utf8_char(
                                        reinterpret_cast<uint8_t *>(utf8),
                                        reinterpret_cast<uint8_t *>(c_end));
                } else {
                        *dest = static_cast<char32_t>(-1);
                }
#   endif
        }

        return end;
#endif
}

//--------------------------------------

WRUTIL_API char
ctype<char32_t>::do_narrow(
        char_type c,
        char      dflt
) const
{
        char result;
        do_narrow(&c, &c + 1, dflt, &result);
        return result;
}

//--------------------------------------

WRUTIL_API auto
ctype<char32_t>::do_narrow(
        const char_type *begin,
        const char_type *end,
        char             dflt,
        char            *dest
) const -> const char_type *
{
        if (begin >= end) {
                return end;
        }

#if __STDC_ISO_10646__  // wchar_t is UTF-32
        auto &wctype = use_facet<ctype<wchar_t>>(loc_);
        return reinterpret_cast<const char_type *>(
                wctype.narrow(reinterpret_cast<const wchar_t *>(begin),
                              reinterpret_cast<const wchar_t *>(end),
                              dflt, dest));
#else
#   if !WR_WINDOWS
        auto    *to_wide = new codecvt_byname<wchar_t, char,
                                              mbstate_t>("en_US.utf8");
        locale   loc2(loc_, to_utf8);  // required to destroy to_wide
#   endif
        auto    &to_narrow = use_facet<codecvt<wchar_t, char, mbstate_t>>(loc_);
        wchar_t  buf[2];  /* use 2 in case wchar_t is UTF-16 (Windows)
                             and result is outside Basic Multilingual Plane */

        for (; begin < end; ++begin, ++dest) {
                mbstate_t  state = { 0 };
                union
                {
                        const wchar_t *w_cend;
                        wchar_t       *w_end;
                };
                union
                {
                        const char *c_cend;
                        char       *c_end;
                };

#   if WR_WINDOWS
                if ((*begin) <= 0xffff) {
                        buf[0] = static_cast<wchar_t>(*begin);
                        w_end = &buf[1];
                } else if (*begin <= 0x10ffff) {
                        // character outside BMP, encode as surrogate pair
                        buf[0] = static_cast<wchar_t>(((*begin) - 0x10000)
                                                      >> 10);
                                // make high surrogate from top 10 bits
                        buf[1] = static_cast<wchar_t>(((*begin) - 0x10000)
                                                      & 0x3ff);
                                // and low surrogate from bottom 10 bits
                        w_end = &buf[2];
                } else {
                        *dest = dflt;
                        continue;
                }
#   else
                char utf8[4];
                c_cend = &utf8[0]
                         + wr::utf8_seq(*begin,
                                        reinterpret_cast<uint8_t *>(&utf8[0]));

                if (to_wide->out(state, utf8, c_cend, c_cend,
                                 buf, w_end, w_end) != to_wide->ok) {
                        *dest = dflt;
                        continue;
                }
#   endif
                // at this point buf contains wchar_t encoding of *begin

                if (to_narrow.out(state, buf, w_cend, w_cend,
                                  dest, dest + 1, c_end) != to_narrow.ok) {
                        *dest = dflt;
                }
        }
#endif
        return end;
}

//--------------------------------------

template <> WRUTIL_API const ctype<char32_t> &
use_facet<ctype<char32_t>>(const locale &/* loc */)
{ 
        static thread_local ctype<char32_t> instance;
        return instance;
}


} // namespace std
