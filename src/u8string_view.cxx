/**
 * \file u8string_view.cxx
 *
 * \brief Implementation of wr::u8string_view class and associated operators
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
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <wrutil/codecvt.h>
#include <wrutil/ctype.h>
#include <wrutil/u8string_view.h>
#include <wrutil/utf16.h>


namespace wr {


WRUTIL_API
u8string_view::u8string_view(
        const char *s
) :
        begin_(reinterpret_cast<pointer>(s)),
        end_  (reinterpret_cast<pointer>(s + std::char_traits<char>::length(s)))
{
        ensure_is_safe();
}

//--------------------------------------

WRUTIL_API
u8string_view::u8string_view(
        const char *s,
        size_type   bytes
) :
        begin_(reinterpret_cast<pointer>(s)),
        end_  (begin_ + bytes)
{
        ensure_is_safe();
}

//--------------------------------------

void
u8string_view::ensure_is_safe()
{
        if (!empty()) {
                pointer next;

                /* begin_ must point at a valid UTF-8 head byte so that
                   backwards iteration from anywhere after begin_ is
                   guaranteed not to read memory before begin_ */
                while (utf8_char(begin_, end_, &next) == INVALID_CHAR) {
                        if (next >= end_) {
                                begin_ = end_;
                                return;
                        } else {
                                uint8_t head = (*begin_) >> 4;
                                if ((head < 8) || (head >= 12)) {
                                        break;  // head is valid
                                }
                        }
                        begin_ = next;
                }

                /* no head byte must exist within 4 bytes before end_ that
                   indicates a byte sequence overflowing end_, this means
                   forward iteration from anywhere before end_ is guaranteed
                   not to read memory at or beyond end_ */
                /* iterating forwards from invalid or non-head bytes only
                   cause movement of one byte at a time (returning
                   INVALID_CHAR on dereferencing), so those are
                   inconsequential */
                next = end_;

                while ((next > begin_) && ((end_ - next) < 4)) {
                        next = utf8_dec(next, begin_);
                        if (next < begin_) {
                                next = begin_ = end_;
                        } else if (utf8_inc(next) > end_) {
                                end_ = next;
                        }
                }
        }
}

//--------------------------------------

WRUTIL_API auto
u8string_view::back() const -> reference
{
        return *std::prev(end());
}

//--------------------------------------

WRUTIL_API auto
u8string_view::size() const -> size_type
{
        return std::distance(begin(), end());
}

//--------------------------------------

WRUTIL_API bool
u8string_view::has_min_size(
        size_type s
) const
{
        for (auto i = begin(), j = end(); s--; ++i) {
                if (i >= j) {
                        return false;
                }
        }
        return true;
}

//--------------------------------------

WRUTIL_API bool
u8string_view::has_max_size(
        size_type s
) const
{
        auto i = begin(), j = end();

        while (s--) {
                if (++i >= j) {
                        return true;
                }
        }

        return i >= j;
}

//--------------------------------------

WRUTIL_API auto
u8string_view::remove_prefix(
        size_type n
) -> this_t &
{
        iterator i = begin();

        while ((i < end_) && n--) {
                ++i;
        }

        begin_ = static_cast<pointer>(i);
        return *this;
}

//--------------------------------------

WRUTIL_API auto
u8string_view::remove_suffix(
        size_type n
) -> this_t &
{
        iterator i = end();

        while ((i > begin_) && n--) {
                --i;
        }

        end_ = static_cast<pointer>(i);
        return *this;
}

//--------------------------------------

WRUTIL_API auto
u8string_view::trim_left() -> this_t &
{
        auto i = begin();

        while ((i < end_) && (isuspace(*i))) {
                ++i;
        }

        begin_ = static_cast<pointer>(i);
        return *this;
}

//--------------------------------------

WRUTIL_API auto
u8string_view::trim_right() -> this_t &
{
        auto i = end();

        while (i > begin_) {
                --i;
                if (!isuspace(*i)) {
                        ++i;
                        break;
                }
        }

        end_ = static_cast<pointer>(i);
        return *this;
}

//--------------------------------------

WRUTIL_API auto
u8string_view::trim() -> this_t &
{
        return trim_left().trim_right();
}

//--------------------------------------

template <typename Traits, typename Alloc>
std::basic_string<char16_t, Traits, Alloc>
u8string_view::to_u16string(
        const Alloc &a
) const
{
        std::basic_string<char16_t, Traits, Alloc> result;
        for (char32_t c: *this) {
                append(result, c);
        }
        return result;
}

//--------------------------------------

WRUTIL_API auto
u8string_view::substr(
        const_iterator pos,
        size_type      n_code_points
) const -> this_t
{
        if (n_code_points == npos) {
                return u8string_view(pos, end_);
        }

        const_iterator e(pos);

        for (; n_code_points--; ++e) {
                if (e >= end_) {
                        e = end_;
                        break;
                }
        }

        return u8string_view(pos, e);
}

//--------------------------------------

WRUTIL_API auto
u8string_view::split(
        char32_t sep
) const -> std::pair<this_t, this_t>
{
        auto i = find(sep);
        if (i == end()) {
                return std::make_pair(*this, this_t());
        } else {
                return std::make_pair(this_t(begin(), i),
                                      this_t(std::next(i), end()));
        }
}

//--------------------------------------

WRUTIL_API auto
u8string_view::split(
        const this_t &sep
) const -> std::pair<this_t, this_t>
{
        auto i = find(sep);
        if (i == end()) {
                return std::make_pair(*this, this_t());
        } else {
                return std::make_pair(this_t(begin(), i),
                                      this_t(std::next(i), end()));
        }
}

//--------------------------------------

WRUTIL_API auto
u8string_view::rsplit(
        char32_t sep
) const -> std::pair<this_t, this_t>
{
        auto i = rfind(sep);
        if (i == end()) {
                return std::make_pair(*this, this_t());
        } else {
                return std::make_pair(this_t(begin(), i),
                                      this_t(std::next(i), end()));
        }
}

//--------------------------------------

WRUTIL_API auto
u8string_view::rsplit(
        const this_t &sep
) const -> std::pair<this_t, this_t>
{
        auto i = rfind(sep);
        if (i == end()) {
                return std::make_pair(*this, this_t());
        } else {
                return std::make_pair(this_t(begin(), i),
                                      this_t(std::next(i), end()));
        }
}

//--------------------------------------

template <typename Iter1, typename Iter2> static int
compare_(
        Iter1 begin1,
        Iter1 end1,
        Iter2 begin2,
        Iter2 end2,
        int   (*cmp)(char32_t c1, char32_t c2)
)
{
        for (; (begin1 < end1) && (begin2 < end2); ++begin1, ++begin2) {
                int c = cmp(*begin1, *begin2);
                if (c != 0) {
                        return c;
                }
        }

        if (begin1 >= end1) {
                if (begin2 >= end2) {
                        return 0;
                } else {
                        return -1;  // (begin2, end2) is longer
                }
        } else {
                return 1;  // (begin1, end1) is longer
        }
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare(
        const this_t &s2
) const
{
        return compare_(begin(), end(), s2.begin(), s2.end(), &this_t::compare);
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare(
        const_iterator pos1,
        size_type      n_code_points1,
        const this_t  &s2
) const
{
        return substr(pos1, n_code_points1).compare(s2);
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare(
        const_iterator  pos1,
        size_type       n_code_points1,
        const this_t   &s2,
        const_iterator  pos2,
        size_type       n_code_points2
) const
{
        return substr(pos1, n_code_points1)
                .compare(s2.substr(pos2, n_code_points2));
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare(
        const char *s2
) const
{
        return compare(this_t(s2));
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare(
        const_iterator pos1,
        size_type      n_code_points1,
        const char    *s2
) const
{
        return substr(pos1, n_code_points1).compare(s2);
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare(
        const_iterator pos1,
        size_type      n_code_points1,
        const char    *s2,
        size_type      n_bytes2
) const
{
        return substr(pos1, n_code_points1).compare(this_t(s2, n_bytes2));
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare(
        const char32_t *s2,
        size_type       num_code_points2
) const
{
        return compare_(
                begin(), end(), s2, s2 + num_code_points2, &this_t::compare);
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare(
        const char32_t *s2
) const
{
        return compare(s2, std::char_traits<char32_t>::length(s2));
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare(
        const_iterator  pos1,
        size_type       n_code_points1,
        const char32_t *s2
) const
{
        return substr(pos1, n_code_points1).compare(s2);
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare(
        const_iterator  pos1,
        size_type       n_code_points1,
        const char32_t *s2,
        size_type       n_code_points2
) const
{
        return substr(pos1, n_code_points1).compare(s2, n_code_points2);
}

//--------------------------------------

template <typename Iter1, typename Iter2> static bool
hasPrefix_(
        Iter1 begin_str,
        Iter1 end_str,
        Iter2 begin_pfx,
        Iter2 end_pfx,
        int   (*compare)(char32_t c1, char32_t c2)
)
{
        for (; (begin_str < end_str) && (begin_pfx < end_pfx);
                                                ++begin_str, ++begin_pfx) {
                int c = compare(*begin_str, *begin_pfx);
                if (c != 0) {
                        return false;
                }
        }

        return begin_pfx >= end_pfx;
}

//--------------------------------------

WRUTIL_API bool
u8string_view::has_prefix(
        const this_t &s2
) const
{
        return hasPrefix_(
                begin(), end(), s2.begin(), s2.end(), &this_t::compare);
}

//--------------------------------------

WRUTIL_API bool
u8string_view::has_suffix(
        const this_t &s2
) const
{
        return hasPrefix_(
                rbegin(), rend(), s2.rbegin(), s2.rend(), &this_t::compare);
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare_nocase(
        char32_t c1,
        char32_t c2
) /* static */
{
        return toulower(c1) - toulower(c2);
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare_nocase(
        const this_t &s2
) const
{
        return compare_(
                begin(), end(), s2.begin(), s2.end(), &this_t::compare_nocase);
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare_nocase(
        const_iterator pos1,
        size_type      n_code_points1,
        const this_t  &s2
) const
{
        return substr(pos1, n_code_points1).compare_nocase(s2);
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare_nocase(
        const_iterator  pos1,
        size_type       n_code_points1,
        const this_t   &s2,
        const_iterator  pos2,
        size_type       n_code_points2
) const
{
        return substr(pos1, n_code_points1)
                .compare_nocase(s2.substr(pos2, n_code_points2));
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare_nocase(
        const char *s2
) const
{
        return compare_nocase(this_t(s2));
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare_nocase(
        const_iterator pos1,
        size_type      n_code_points1,
        const char    *s2
) const
{
        return substr(pos1, n_code_points1).compare_nocase(s2);
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare_nocase(
        const_iterator pos1,
        size_type      n_code_points1,
        const char    *s2,
        size_type      n_bytes2
) const
{
        return substr(pos1, n_code_points1)
                .compare_nocase(this_t(s2, n_bytes2));
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare_nocase(
        const char32_t *s2,
        size_type       num_code_points2
) const
{
        return compare_(begin(), end(),
                        s2, s2 + num_code_points2, &this_t::compare_nocase);
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare_nocase(
        const char32_t *s2
) const
{
        return compare_nocase(s2, std::char_traits<char32_t>::length(s2));
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare_nocase(
        const_iterator  pos1,
        size_type       n_code_points1,
        const char32_t *s2
) const
{
        return substr(pos1, n_code_points1).compare_nocase(s2);
}

//--------------------------------------

WRUTIL_API int
u8string_view::compare_nocase(
        const_iterator  pos1,
        size_type       n_code_points1,
        const char32_t *s2,
        size_type       n_code_points2
) const
{
        return substr(pos1, n_code_points1).compare_nocase(s2, n_code_points2);
}

//--------------------------------------

WRUTIL_API bool
u8string_view::has_prefix_nocase(
        const this_t &s2
) const
{
        return hasPrefix_(
                begin(), end(), s2.begin(), s2.end(), &this_t::compare_nocase);
}

//--------------------------------------

WRUTIL_API bool
u8string_view::has_suffix_nocase(
        const this_t &s2
) const
{
        return hasPrefix_(rbegin(), rend(),
                          s2.rbegin(), s2.rend(), &this_t::compare_nocase);
}

//--------------------------------------

WRUTIL_API auto
u8string_view::find(
        const this_t   &substr,
        const_iterator  pos
) const -> const_iterator
{
        if (!pos) {
                pos = begin();
        }
        return std::search(pos, end(), substr.begin(), substr.end());
}

//--------------------------------------

WRUTIL_API auto
u8string_view::find(
        char32_t       c,
        const_iterator pos
) const -> const_iterator
{
        if (!pos) {
                pos = begin();
        }
        return std::find(pos, end(), c);
}

//--------------------------------------

WRUTIL_API auto
u8string_view::find(
        const char     *substr,
        const_iterator  pos,
        size_type       n_substr_bytes
) const -> const_iterator
{
        return find(this_t(substr, n_substr_bytes), pos);
}

//--------------------------------------

WRUTIL_API auto
u8string_view::find(
        const char     *substr,
        const_iterator  pos
) const -> const_iterator
{
        return find(this_t(substr), pos);
}

//--------------------------------------

WRUTIL_API auto
u8string_view::rfind(
        const this_t   &substr,
        const_iterator  pos
) const -> const_iterator
{
        if (!pos) {
                pos = end();
        }
        auto i = std::search(const_reverse_iterator(pos), rend(),
                             substr.rbegin(), substr.rend());
        if (i == rend()) {
                return end();
        } else {
                return std::prev(i.base(), substr.size() + 1);
        }
}

//--------------------------------------

WRUTIL_API auto
u8string_view::rfind(
        char32_t       c,
        const_iterator pos
) const -> const_iterator
{
        if (!pos) {
                pos = end();
        }
        auto i = std::find(const_reverse_iterator(pos), rend(), c);
        return (i >= rend()) ? end() : std::prev(i.base());
}

//--------------------------------------

WRUTIL_API auto
u8string_view::rfind(
        const char     *substr,
        const_iterator  pos,
        size_type       n_substr_bytes
) const -> const_iterator
{
        return rfind(this_t(substr, n_substr_bytes), pos);
}

//--------------------------------------

WRUTIL_API auto
u8string_view::rfind(
        const char     *substr,
        const_iterator  pos
) const -> const_iterator
{
        return rfind(this_t(substr), pos);
}

//--------------------------------------

WRUTIL_API auto
u8string_view::find_first_of(
        const this_t   &chars,
        const_iterator  pos
) const -> const_iterator
{
        if (!pos) {
                pos = begin();
        }
        return std::find_first_of(pos, end(), chars.begin(), chars.end());
}

//--------------------------------------

WRUTIL_API auto
u8string_view::find_first_of(
        const char     *u8chars,
        const_iterator  pos,
        size_type       n_u8chars_bytes
) const -> const_iterator
{
        return find_first_of(this_t(u8chars, n_u8chars_bytes), pos);
}

//--------------------------------------

WRUTIL_API auto
u8string_view::find_first_of(
        const char     *u8chars,
        const_iterator  pos
) const -> const_iterator
{
        return find_first_of(this_t(u8chars), pos);
}

//--------------------------------------

WRUTIL_API auto
u8string_view::find_last_of(
        const this_t   &chars,
        const_iterator  pos
) const -> const_iterator
{
        if (!pos) {
                pos = end();
        }
        const_reverse_iterator i(pos);
        i = std::find_first_of(i, rend(), chars.begin(), chars.end());
        return (i >= rend()) ? end() : std::prev(i.base());
}

//--------------------------------------

WRUTIL_API auto
u8string_view::find_last_of(
        const char     *u8chars,
        const_iterator  pos,
        size_type       n_u8chars_bytes
) const -> const_iterator
{
        return find_last_of(this_t(u8chars, n_u8chars_bytes), pos);
}

//--------------------------------------

WRUTIL_API auto
u8string_view::find_last_of(
        const char     *u8chars,
        const_iterator  pos
) const -> const_iterator
{
        return find_last_of(this_t(u8chars), pos);
}

//--------------------------------------

WRUTIL_API auto
u8string_view::find_first_not_of(
        const this_t   &chars,
        const_iterator  pos
) const -> const_iterator
{
        if (!pos) {
                pos = begin();
        }
        return std::find_if(pos, end(),
                [&chars](char32_t c) { return chars.find(c) == chars.end(); });
}

//--------------------------------------

WRUTIL_API auto
u8string_view::find_first_not_of(
        char32_t       c,
        const_iterator pos
) const -> const_iterator
{
        if (!pos) {
                pos = begin();
        }
        return std::find_first_of(pos, end(), &c, (&c) + 1,
                                  std::not_equal_to<char32_t>());
}

//--------------------------------------

WRUTIL_API auto
u8string_view::find_first_not_of(
        const char     *u8chars,
        const_iterator  pos,
        size_type       n_u8chars_bytes
) const -> const_iterator
{
        return find_first_not_of(this_t(u8chars, n_u8chars_bytes), pos);
}

//--------------------------------------

WRUTIL_API auto
u8string_view::find_first_not_of(
        const char     *u8chars,
        const_iterator  pos
) const -> const_iterator
{
        return find_first_not_of(this_t(u8chars), pos);
}

//--------------------------------------

WRUTIL_API auto
u8string_view::find_last_not_of(
        const this_t   &chars,
        const_iterator  pos
) const -> const_iterator
{
        if (!pos) {
                pos = end();
        }
        auto i = std::find_if(const_reverse_iterator(pos), rend(),
                [&chars](char32_t c) { return chars.find(c) != chars.end(); });
        return (i == rend()) ? end() : std::prev(i.base());
}

//--------------------------------------

WRUTIL_API auto
u8string_view::find_last_not_of(
        char32_t       c,
        const_iterator pos
) const -> const_iterator
{
        if (!pos) {
                pos = end();
        }
        auto i = std::find_first_of(const_reverse_iterator(pos), rend(), &c,
                                    (&c) + 1, std::not_equal_to<char32_t>());
        return (i == rend()) ? end() : std::prev(i.base());
}

//--------------------------------------

WRUTIL_API auto
u8string_view::find_last_not_of(
        const char     *u8chars,
        const_iterator  pos,
        size_type       n_u8chars_bytes
) const -> const_iterator
{
        return find_last_not_of(this_t(u8chars, n_u8chars_bytes), pos);
}

//--------------------------------------

WRUTIL_API auto
u8string_view::find_last_not_of(
        const char     *u8chars,
        const_iterator  pos
) const -> const_iterator
{
        return find_last_not_of(this_t(u8chars), pos);
}

//--------------------------------------

WRUTIL_API std::string &
per_thread_tmp_string_buffer()
{
        static thread_local std::string buf(32, '\0');
        return buf;
}

//--------------------------------------

template <typename NumType> static void
check_to_num_result(
        NumType     result,
        NumType     min_val,
        NumType     max_val,
        NumType     min_representable,
        NumType     max_representable,
        const char *kind,
        const char *endptr,
        int         saved_errno,
        size_t     *end_code_point_offset
)
try {
        std::string &buf = per_thread_tmp_string_buffer();

        switch (errno) {
        case 0:
                errno = saved_errno;
                if (result < min_val) {
                        throw std::out_of_range(
                                buf + " exceeds minimum allowed value "
                                + std::to_string(min_val));
                } else if (result > max_val) {
                        throw std::out_of_range(
                                buf + " exceeds maximum allowed value "
                                + std::to_string(max_val));
                }

                if ((endptr == buf.c_str())
                             || (!end_code_point_offset && (*endptr != '\0'))) {
                        throw std::invalid_argument(
                                "value \"" + buf + "\" not valid, "
                                + std::string(kind) + " number expected");
                }
                break;
        case ERANGE:
                if (result == min_representable) {
                        throw std::out_of_range(
                                buf + " exceeds minimum allowed value "
                                + std::to_string(min_representable));
                } else {
                        throw std::out_of_range(
                                buf + " exceeds maximum allowed value "
                                + std::to_string(max_representable));
                }
        default: case EINVAL:
                throw std::invalid_argument("value \"" + buf + "\" not valid, "
                        + std::string(kind) + " number expected");
        }

        if (end_code_point_offset) {
                *end_code_point_offset
                        = u8string_view(buf.data(), endptr - buf.data()).size();
        }

        buf.clear();
} catch (...) {
        per_thread_tmp_string_buffer().clear();
        throw;
}

//--------------------------------------

template <> WRUTIL_API long long
to_int<long long>(
        const u8string_view &s,
        size_t              *end_code_point_offset,
        int                  base,
        long long            min_val,
        long long            max_val
)
{
        std::string &buf = per_thread_tmp_string_buffer();
        buf.assign(s.char_data(), s.bytes());

        int saved_errno = errno;
        errno = 0;

        char      *endptr;
        long long  result = strtoll(buf.c_str(), &endptr, base);

        check_to_num_result(result, min_val, max_val, LLONG_MIN, LLONG_MAX,
                            "integral", endptr, saved_errno,
                            end_code_point_offset);
        return result;
}

//--------------------------------------

template <> WRUTIL_API unsigned long long
to_int<unsigned long long>(
        const u8string_view &s,
        size_t              *end_code_point_offset,
        int                  base,
        unsigned long long   min_val,
        unsigned long long   max_val
)
{
        std::string &buf = per_thread_tmp_string_buffer();
        buf.assign(s.char_data(), s.bytes());

        int saved_errno = errno;
        errno = 0;

        char               *endptr;
        unsigned long long  result = strtoull(buf.c_str(), &endptr, base);

        check_to_num_result(result, min_val, max_val, 0ULL, ULLONG_MAX,
                            "unsigned integral", endptr,
                            saved_errno, end_code_point_offset);
        return result;
}

//--------------------------------------

template <> WRUTIL_API double
to_float<double>(
        const u8string_view &s,
        size_t              *end_code_point_offset,
        double               min_val,
        double               max_val
)
{
        std::string &buf = per_thread_tmp_string_buffer();
        buf.assign(s.char_data(), s.bytes());

        int saved_errno = errno;
        errno = 0;

        char   *endptr;
        double  result = strtod(buf.c_str(), &endptr);

        check_to_num_result(result, min_val, max_val, -HUGE_VAL, +HUGE_VAL,
                            "real", endptr, saved_errno, end_code_point_offset);
        return result;
}

//--------------------------------------

template <> WRUTIL_API long double
to_float<long double>(
        const u8string_view &s,
        size_t              *end_code_point_offset,
        long double          min_val,
        long double          max_val
)
{
        std::string &buf = per_thread_tmp_string_buffer();
        buf.assign(s.char_data(), s.bytes());

        int saved_errno = errno;
        errno = 0;

        char        *endptr;
        long double  result = strtold(buf.c_str(), &endptr);

        check_to_num_result(result, min_val, max_val, -HUGE_VALL, +HUGE_VALL,
                            "real", endptr, saved_errno, end_code_point_offset);
        return result;
}

//--------------------------------------

WRUTIL_API std::ostream &
operator<<(
        std::ostream        &out,
        const u8string_view &in
)
{
        out.write(in.char_data(), in.bytes());
        return out;
}


} // namespace wr
