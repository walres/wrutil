/**
 * \file string_view.h
 *
 * \brief Lightweight immutable string template class wr::basic_string_view
 *        and non-member operator functions
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
 *
 * wr::basic_string_view provides a superset of the forthcoming C++17
 * std::basic_string_view class incorporating extensions such as
 * as tokenisation, case transformation, whitespace trimming and case-
 * insensitive comparisons.
 */
#ifndef WRUTIL_STRING_VIEW_H
#define WRUTIL_STRING_VIEW_H

#include <algorithm>
#include <ios>
#include <stdexcept>
#include <string>
#include <utility>
#include <wrutil/Config.h>
#if WR_HAVE_STD_STRING_VIEW
#       if WR_HAVE_STD_EXP_STRING_VIEW
#               include <experimental/string_view>
#       else
#               include <string_view>
#       endif
#endif


namespace wr {


#if WR_HAVE_STD_STRING_VIEW
#       if WR_HAVE_STD_EXP_STRING_VIEW
                template <typename CharT, typename Traits>
                using std_basic_string_view
                        = std::experimental::basic_string_view<CharT, Traits>;

                using std_string_view = std::experimental::string_view;
                using std_wstring_view = std::experimental::wstring_view;
                using std_u16string_view = std::experimental::u16string_view;
                using std_u32string_view = std::experimental::u32string_view;
#       else
                template <typename CharT, typename Traits>
                using std_basic_string_view
                        = std::basic_string_view<CharT, Traits>;

                using std_string_view = std::string_view;
                using std_wstring_view = std::wstring_view;
                using std_u16string_view = std::u16string_view;
                using std_u32string_view = std::u32string_view;
#       endif
#endif

//--------------------------------------

template <typename CharT> CharT to_lower(CharT c);
template <typename CharT> CharT to_upper(CharT c);
        /* specialized for char, wchar_t, char16_t and char32_t;
           must be specialized by user for other character types */

//--------------------------------------

template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_string_view
{
public:
        using this_t = basic_string_view;
        using traits_type = Traits;
        using value_type = CharT;
        using pointer = CharT *;
        using const_pointer = const CharT *;
        using reference = CharT &;
        using const_reference = const CharT &;
        using const_iterator = const CharT *;
        using iterator = const_iterator;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using reverse_iterator = const_reverse_iterator;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        static const size_type npos = static_cast<size_type>(-1);


        basic_string_view() : begin_(nullptr), end_(nullptr) {}

        basic_string_view(const this_t &other) = default;

        template <typename Traits2>
        basic_string_view(const basic_string_view<CharT, Traits2> &str) :
                begin_(str.begin_), end_(str.end_) {}

        template <typename Traits2, typename Alloc>
        basic_string_view(const std::basic_string<CharT, Traits2, Alloc> &str) :
                begin_(str.data()), end_(begin_ + str.size()) {}

#if WR_HAVE_STD_STRING_VIEW
        template <typename Traits2> explicit
        basic_string_view(const std_basic_string_view<CharT, Traits2> &str) :
                begin_(str.data()), end_(begin_ + str.size()) {}
#endif

        basic_string_view(const CharT *s, size_type count) :
                begin_(s), end_(begin_ + count) {}

        basic_string_view(const CharT *s) :
                begin_(s), end_(s + Traits::length(s)) {}

        basic_string_view(const_iterator b, const_iterator e) :
                begin_(b), end_(e) {}

        const_iterator begin() const { return begin_; }
        const_iterator cbegin() const { return begin_; }
        const_iterator end() const { return end_; }
        const_iterator cend() const { return end_; }

        const_reverse_iterator rbegin() const
                { return const_reverse_iterator(end_); }

        const_reverse_iterator crbegin() const
                { return const_reverse_iterator(end_); }

        const_reverse_iterator rend() const
                { return const_reverse_iterator(begin_); }

        const_reverse_iterator crend() const
                { return const_reverse_iterator(begin_); }

        const_reference operator[](size_type pos) const { return begin_[pos]; }

        const_reference
        at(
                size_type pos
        ) const
        {
                if (pos >= size()) {
                        throw std::out_of_range(
                               "wr::basic_string_view::at(): pos out of range");
                }
                return begin_[pos];
        }

        const_reference front() const { return *begin_; }
        const_reference back() const { return end_[-1]; }
        const_pointer data() const { return begin_; }
        size_type size() const { return end_ - begin_; }
        size_type length() const { return size(); }
        size_type max_size() const { return size_type(-2); }
        bool empty() const { return begin_ == end_; }

        bool has_min_size(size_type s) const { return size() >= s; }
        bool has_max_size(size_type s) const { return size() <= s; }

        this_t &remove_prefix(size_type n)
                { begin_ += std::min(n, size()); return *this; }

        this_t &remove_suffix(size_type n)
                { end_ -= std::min(n, size()); return *this; }

        this_t &trim_left()
                { return remove_prefix(find_first_not_of(" \t\n\r\f\v")); }

        this_t &trim_right()
                { return remove_suffix(
                                size() - find_last_not_of(" \t\n\r\f\v") - 1); }

        this_t &trim() { return trim_left().trim_right(); }

        void
        swap(
                this_t &v
        )
        {
                std::swap(begin_, v.begin_);
                std::swap(end_, v.end_);
        }

        template <typename Alloc = std::allocator<CharT>>
        std::basic_string<CharT, Traits, Alloc>
        to_string(
                const Alloc &a = {}
        ) const
        {
                return std::basic_string<CharT, Traits, Alloc>(
                        data(), size(), a);
        }

        template <typename Alloc>
        operator std::basic_string<CharT, Traits, Alloc>() const
                { return to_string<Alloc>(); }

#if WR_HAVE_STD_STRING_VIEW
        explicit operator std_basic_string_view<CharT, Traits>() const
                { return { data(), size() }; }
#endif

        template <typename Alloc = std::allocator<char>>
        std::basic_string<char, Traits, Alloc>
        to_upper(
                const Alloc &a = Alloc()
        ) const
        {
                auto result = to_string(a);
                for (auto i = result.begin(), j = result.end(); i != j; ++i) {
                        *i = wr::to_upper(*i);
                }
                return result;
        }

        template <typename Alloc = std::allocator<char>>
        std::basic_string<char, Traits, Alloc>
        to_lower(
                const Alloc &a = Alloc()
        ) const
        {
                auto result = to_string(a);
                for (auto i = result.begin(), j = result.end(); i != j; ++i) {
                        *i = wr::to_lower(*i);
                }
                return result;
        }

        size_type
        copy(
                CharT     *dest,
                size_type  count,
                size_type  pos = 0
        ) const
        {
                if (pos >= size()) {
                        throw std::out_of_range(
                             "wr::basic_string_view::copy(): pos out of range");
                }
                return std::copy_n(begin_ + pos,
                                   std::min(count, size() - pos), dest) - dest;
        }

        this_t
        substr(
                size_type pos   = 0,
                size_type count = npos
        ) const
        {
                if (pos > size()) {
                        throw std::out_of_range(
                           "wr::basic_string_view::substr(): pos out of range");
                }
                return { begin_ + pos, std::min(count, size() - pos) };
        }

        std::pair<this_t, this_t>
        split(
                value_type sep
        ) const
        {
                auto i = find(sep);
                if (i == npos) {
                        return std::make_pair(*this, this_t());
                } else {
                        return std::make_pair(substr(0, i),
                                              substr(i + 1, size() - i - 1));
                }
        }

        std::pair<this_t, this_t>
        split(
                const this_t &sep
        ) const
        {
                auto i = find(sep);
                if (i == npos) {
                        return std::make_pair(*this, this_t());
                } else {
                        return std::make_pair(substr(0, i),
                                              substr(i + sep.size(),
                                                     size() - i - sep.size()));
                }
        }

        std::pair<this_t, this_t>
        rsplit(
                value_type sep
        ) const
        {
                auto i = rfind(sep);
                if (i == npos) {
                        return std::make_pair(*this, this_t());
                } else {
                        return std::make_pair(substr(0, i),
                                              substr(i + 1, size() - i - 1));
                }
        }

        std::pair<this_t, this_t>
        rsplit(
                const this_t &sep
        ) const
        {
                auto i = rfind(sep);
                if (i == npos) {
                        return std::make_pair(*this, this_t());
                } else {
                        return std::make_pair(substr(0, i),
                                              substr(i + sep.size(),
                                                     size() - i - sep.size()));
                }
        }

        int
        compare(
                const this_t &v
        ) const
        {
                int difference = Traits::compare(data(), v.data(),
                                                 std::min(size(), v.size()));
                if (!difference && (size() != v.size())) {
                        difference = (size() < v.size()) ? -1 : 1;
                }
                return difference;
        }

        int compare(size_type pos, size_type count, const this_t &v) const
                { return substr(pos, count).compare(v); }

        int compare(size_type pos1, size_type count1, const this_t &v,
                    size_type pos2, size_type count2) const
                { return substr(pos1, count1).compare(v.substr(pos2, count2)); }

        int compare(const CharT *s) const
                { return compare(this_t(s)); }

        int compare(size_type pos, size_type count, const CharT *s) const
                { return substr(pos, count).compare(s); }

        int compare(size_type pos1, size_type count1,
                    const CharT *s, size_type count2) const
                { return substr(pos1, count1).compare({ s, count2 }); }

        bool has_prefix(const this_t &s2) const
                { return (size() >= s2.size()) && !compare(0, s2.size(), s2); }

        bool has_suffix(const this_t &s2) const
                { return (size() >= s2.size())
                          && !compare(size() - s2.size(), s2.size(), s2); }

        int
        compare_nocase(
                const this_t &v
        ) const
        {
                auto begin1 = begin(), begin2 = v.begin(),
                     end1 = end(), end2 = v.end();

                for (; (begin1 < end1) && (begin2 < end2); ++begin1, ++begin2) {
                        int c = static_cast<int>(wr::to_lower(*begin1))
                                - static_cast<int>(wr::to_lower(*begin2));
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

        int compare_nocase(size_type pos, size_type count,
                           const this_t &v) const
                { return substr(pos, count).compare_nocase(v); }

        int compare_nocase(size_type pos1, size_type count1, const this_t &v,
                           size_type pos2, size_type count2) const
                { return substr(pos1, count1)
                                .compare_nocase(v.substr(pos2, count2)); }

        int compare_nocase(const CharT *s) const
                { return compare_nocase({ s }); }

        int compare_nocase(size_type pos, size_type count, const CharT *s) const
                { return substr(pos, count).compare_nocase(s); }

        int compare_nocase(size_type pos1, size_type count1,
                           const CharT *s, size_type count2) const
                { return substr(pos1, count1).compare_nocase({ s, count2 }); }

        bool has_prefix_nocase(const this_t &s2) const
                { return (size() >= s2.size())
                                && !compare_nocase(0, s2.size(), s2); }

        bool has_suffix_nocase(const this_t &s2) const
                { return (size() >= s2.size()) &&
                          !compare_nocase(size() - s2.size(), s2.size(), s2); }

        size_type
        find(
                const this_t &v,
                size_type     pos = 0
        ) const
        {
                auto stop = std::search(begin_ + pos, end_, v.begin_, v.end_);
                return (stop != end_) ? stop - begin_ : npos;
        }

        size_type
        find(
                CharT     c,
                size_type pos = 0
        ) const
        {
                auto stop = std::find(begin_ + pos, end_, c);
                return (stop != end_) ? (stop - begin_) : npos;
        }

        size_type find(const CharT *s, size_type pos, size_type count) const
                { return find({ s, count }, pos); }

        size_type find(const CharT *s, size_type pos = 0) const
                { return find(this_t(s), pos); }

        size_type
        rfind(
                const this_t &v,
                size_type     pos = 0
        ) const
        {
                auto stop = std::find_end(begin_ + pos, end_, v.begin_, v.end_);
                return (stop != end_) ? stop - begin_ : npos;
        }

        size_type
        rfind(
                CharT     c,
                size_type pos = npos
        ) const
        {
                pos = std::min(pos, size() - 1);
                const_reverse_iterator i(begin_ + pos + 1);
                i = std::find(i, rend(), c);
                return (i != rend()) ? (i.base() - begin_) - 1 : npos;
        }

        size_type rfind(const CharT *s, size_type pos, size_type count) const
                { return rfind({ s, count }, pos); }

        size_type rfind(const CharT *s, size_type pos = npos) const
                { return rfind(this_t(s), pos); }

        size_type
        find_first_of(
                const this_t &v,
                size_type     pos = 0
        ) const
        {
                auto stop = std::find_first_of(begin_ + pos, end_,
                                               v.begin_, v.end_);
                return (stop != end_) ? (stop - begin_) : npos;
        }

        size_type find_first_of(CharT c, size_type pos = 0) const
                { return find(c, pos); }

        size_type find_first_of(const CharT *s, size_type pos,
                                size_type count) const
                { return find_first_of({ s, count }, pos); }

        size_type find_first_of(const CharT *s, size_type pos = 0) const
                { return find_first_of(this_t(s), pos); }

        size_type
        find_last_of(
                const this_t &v,
                size_type     pos = npos
        ) const
        {
                pos = std::min(pos, size() - 1);
                const_reverse_iterator i(begin_ + pos + 1);
                i = std::find_first_of(i, rend(), v.begin_, v.end_);
                return (i != rend()) ? (i.base() - begin_) - 1 : npos;
        }

        size_type find_last_of(CharT c, size_type pos = npos) const
                { return rfind(c, pos); }

        size_type find_last_of(const CharT *s, size_type pos,
                               size_type count) const
                { return find_last_of({ s, count }, pos); }

        size_type find_last_of(const CharT *s, size_type pos = npos) const
                { return find_last_of(this_t(s), pos); }

        size_type
        find_first_not_of(
                const this_t &v,
                size_type     pos = 0
        ) const
        {
                auto i = begin_ + pos;
                if (i >= end_) {
                        return npos;
                }
                i = std::find_if_not(i, end_,
                                    [&v](CharT c){ return v.find(c) != npos; });
                return (i != end_) ? (i - begin_) : npos;
        }

        size_type find_first_not_of(CharT c, size_type pos = 0) const
                { return find_first_not_of({ &c, 1 }, pos); }

        size_type find_first_not_of(const CharT *s, size_type pos,
                                    size_type count) const
                { return find_first_not_of({ s, count }, pos); }

        size_type find_first_not_of(const CharT *s, size_type pos = 0) const
                { return find_first_not_of(this_t(s), pos); }

        size_type
        find_last_not_of(
                const this_t &v,
                size_type     pos = npos
        ) const
        {
                pos = std::min(pos, size() - 1);
                const_reverse_iterator i(begin_ + pos + 1), j = rend();
                i = std::find_if_not(i, j,
                                   [&v](CharT c) { return v.find(c) != npos; });
                return (i != j) ? (i.base() - begin_) - 1 : npos;
        }

        size_type find_last_not_of(CharT c, size_type pos = npos) const
                { return find_last_not_of({ &c, 1 }, pos); }

        size_type find_last_not_of(const CharT *s, size_type pos,
                                   size_type count) const
                { return find_last_not_of({ s, count }, pos); }

        size_type find_last_not_of(const CharT *s, size_type pos = npos) const
                { return find_last_not_of(this_t(s), pos); }

        this_t& operator=(const this_t &view) = default;

private:
        const CharT *begin_, *end_;
};

//--------------------------------------

typedef basic_string_view<char> string_view;
typedef basic_string_view<wchar_t> wstring_view;
typedef basic_string_view<char16_t> u16string_view;
typedef basic_string_view<char32_t> u32string_view;

//--------------------------------------

template <typename CharT, typename Traits>
inline bool operator==(basic_string_view<CharT, Traits> a,
                       basic_string_view<CharT, Traits> b)
        { return !a.compare(b); }

template <typename CharT, typename Traits, typename T>
inline bool operator==(basic_string_view<CharT, Traits> a, T b)
        { return !a.compare(b); }

template <typename T, typename CharT, typename Traits>
inline bool operator==(T a, basic_string_view<CharT, Traits> b)
        { return !b.compare(a); }

template <typename CharT, typename Traits>
inline bool operator!=(basic_string_view<CharT, Traits> a,
                       basic_string_view<CharT, Traits> b)
        { return a.compare(b) != 0; }

template <typename CharT, typename Traits, typename T>
inline bool operator!=(basic_string_view<CharT, Traits> a, T b)
        { return a.compare(b) != 0; }

template <typename T, typename CharT, typename Traits>
inline bool operator!=(T a, basic_string_view<CharT, Traits> b)
        { return b.compare(a) != 0; }

template <typename CharT, typename Traits>
inline bool operator<(basic_string_view<CharT, Traits> a,
                      basic_string_view<CharT, Traits> b)
        { return a.compare(b) < 0; }

template <typename CharT, typename Traits, typename T>
inline bool operator<(basic_string_view<CharT, Traits> a, T b)
        { return a.compare(b) < 0; }

template <typename T, typename CharT, typename Traits>
inline bool operator<(T a, basic_string_view<CharT, Traits> b)
        { return b.compare(a) > 0; }

template <typename CharT, typename Traits>
inline bool operator<=(basic_string_view<CharT, Traits> a,
                       basic_string_view<CharT, Traits> b)
        { return a.compare(b) <= 0; }

template <typename CharT, typename Traits, typename T>
inline bool operator<=(basic_string_view<CharT, Traits> a, T b)
        { return a.compare(b) <= 0; }

template <typename T, typename CharT, typename Traits>
inline bool operator<=(T a, basic_string_view<CharT, Traits> b)
        { return b.compare(a) >= 0; }

template <typename CharT, typename Traits>
inline bool operator>(basic_string_view<CharT, Traits> a,
                      basic_string_view<CharT, Traits> b)
        { return a.compare(b) > 0; }

template <typename CharT, typename Traits, typename T>
inline bool operator>(basic_string_view<CharT, Traits> a, T b)
        { return a.compare(b) > 0; }

template <typename T, typename CharT, typename Traits>
inline bool operator>(T a, basic_string_view<CharT, Traits> b)
        { return b.compare(a) < 0; }

template <typename CharT, typename Traits>
inline bool operator>=(basic_string_view<CharT, Traits> a,
                       basic_string_view<CharT, Traits> b)
        { return a.compare(b) >= 0; }

template <typename CharT, typename Traits, typename T>
inline bool operator>=(basic_string_view<CharT, Traits> a, T b)
        { return a.compare(b) >= 0; }

template <typename T, typename CharT, typename Traits>
inline bool operator>=(T a, basic_string_view<CharT, Traits> b)
        { return b.compare(a) <= 0; }

//--------------------------------------

template <typename CharT, typename Traits>
inline std::basic_ostream<CharT, Traits> &
operator<<(
        std::basic_ostream<CharT, Traits>      &os,
        const basic_string_view<CharT, Traits> &v
)
{
        os.write(v.data(), v.length());
        return os;
}

//--------------------------------------

template <typename CharT, typename Traits, typename Alloc>
inline std::basic_string<CharT, Traits, Alloc>
operator+(
        const std::basic_string<CharT, Traits, Alloc> &a,
        const basic_string_view<CharT, Traits>        &b
)
{
        return std::basic_string<CharT, Traits, Alloc>(a, b.get_allocator())
                .append(b.data(), b.size());
}

//--------------------------------------

template <typename CharT, typename Traits, typename Alloc>
inline std::basic_string<CharT, Traits, Alloc>
operator+(
        const basic_string_view<CharT, Traits>        &a,
        const std::basic_string<CharT, Traits, Alloc> &b
)
{
        auto result = a.to_string(b.get_allocator());
        return result += b;
}

//--------------------------------------

template <typename CharT, typename Traits, typename Alloc>
inline std::basic_string<CharT, Traits, Alloc> &
operator+=(
        std::basic_string<CharT, Traits, Alloc> &a,
        const basic_string_view<CharT, Traits>  &b
)
{
        a.append(b.data(), b.size());
        return a;
}


extern WRUTIL_API size_t stdHash(const void *k, size_t len);  // from CityHash.h

//--------------------------------------

template <typename CharT, typename Traits> inline void
swap(
        basic_string_view<CharT, Traits> &a,
        basic_string_view<CharT, Traits> &b
)
{
        a.swap(b);
}

//--------------------------------------
/*
 * wr::print() support
 */
namespace fmt {


struct Arg;
template <typename> struct TypeHandler;

template <> struct WRUTIL_API TypeHandler<string_view>
{
        static void set(Arg &arg, const string_view &val);
};


} // namespace fmt


} // namespace wr

//--------------------------------------

namespace std {


template <typename CharT, typename Traits>
struct hash<wr::basic_string_view<CharT, Traits>>
{
        size_t operator()(const wr::basic_string_view<CharT, Traits> &val) const
                { return wr::stdHash(val.data(), val.size()); }
};


} // namespace std


#endif // !WRUTIL_STRING_VIEW_H
