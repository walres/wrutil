/**
 * \file u8string_view.h
 *
 * \brief Present a UTF-8 encoded string as a sequence of 32-bit code points
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
#ifndef WRUTIL_U8STRING_VIEW_H
#define WRUTIL_U8STRING_VIEW_H

#include <algorithm>
#include <ios>
#include <iterator>
#include <locale>
#include <limits>
#include <string>
#include <wrutil/Config.h>
#include <wrutil/ctype.h>
#include <wrutil/utf8.h>
#include <wrutil/string_view.h>


namespace wr {


class u8string_view_iterator
{
public:
        using this_t = u8string_view_iterator;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = char32_t;
        using pointer = const uint8_t *;

        class reference
        {
        public:
                reference(pointer pos) : pos_(pos) {}
                operator char32_t() const { return utf8_char(pos_, pos_ + 4); }

        private:
                const char32_t *operator&() const;  // disallow address-of
                pointer pos_;
        };


        u8string_view_iterator() = default;
        u8string_view_iterator(const this_t &other) = default;
        u8string_view_iterator(const uint8_t *pos) : pos_(pos) {}
        u8string_view_iterator(const char *pos) :
                pos_(reinterpret_cast<const uint8_t *>(pos)) {}

        reference operator*() const { return pos_; }

        explicit operator pointer() const { return pos_; }
        explicit operator bool() const    { return (pos_ != nullptr); }

        this_t &
        operator++()
        {
                pos_ = utf8_inc(pos_);
                return *this;
        }

        this_t operator++(int) { this_t old(*this); ++(*this); return old; }

        this_t &
        operator--()
        {
                pos_ = utf8_dec(pos_);
                return *this;
        }

        this_t operator--(int) { this_t old(*this); --(*this); return old; }

        this_t &operator=(const this_t &other) = default;

        bool operator==(const this_t &other) { return pos_ == other.pos_; }
        bool operator!=(const this_t &other) { return pos_ != other.pos_; }
        bool operator<(const this_t &other) { return pos_ < other.pos_; }
        bool operator<=(const this_t &other) { return pos_ <= other.pos_; }
        bool operator>(const this_t &other) { return pos_ > other.pos_; }
        bool operator>=(const this_t &other) { return pos_ >= other.pos_; }

private:
        pointer pos_;
};

//--------------------------------------

class WRUTIL_API u8string_view
{
public:
        using this_t = u8string_view;
        using value_type = char32_t;
        using iterator = u8string_view_iterator;
        using const_iterator = iterator;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = reverse_iterator;
        using reference = iterator::reference;
        using const_reference = reference;
        using pointer = iterator::pointer;
        using const_pointer = pointer;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        static constexpr size_type npos = size_type(-1);


        u8string_view() : begin_(nullptr), end_(nullptr) {}
        u8string_view(const this_t &other) = default;
        u8string_view(const char *s);
        u8string_view(const char *s, size_type bytes);
        u8string_view(const uint8_t *s) :
                u8string_view(reinterpret_cast<const char *>(s)) {}
        u8string_view(const uint8_t *s, size_type bytes) :
                u8string_view(reinterpret_cast<const char *>(s), bytes) {}
        u8string_view(const_iterator b, const_iterator e) :
                begin_(b), end_(e) {}

        template <typename Traits, typename Alloc>
                u8string_view(const std::basic_string<char, Traits, Alloc> &s) :
                        u8string_view(s.data(), s.size()) {}

        template <typename Traits>
                u8string_view(const wr::basic_string_view<char, Traits> &s) :
                        u8string_view(s.data(), s.size()) {}

#if WR_HAVE_STD_STRING_VIEW
        template <typename Traits>
                u8string_view(const std_basic_string_view<char, Traits> &s) :
                        u8string_view(s.data(), s.size()) {}
#endif

        iterator begin() const          { return begin_; }
        const_iterator cbegin() const   { return begin(); }
        iterator end() const            { return end_; }
        const_iterator cend() const     { return end_; }
        reverse_iterator rbegin() const { return reverse_iterator(end_); }
        const_reverse_iterator crbegin() const { return rbegin(); }
        reverse_iterator rend() const   { return reverse_iterator(begin_); }
        const_reverse_iterator crend() const   { return rend(); }

        reference front() const    { return begin_; }
        reference back() const;
        const_pointer data() const { return begin_; }
        size_type bytes() const    { return end_ - begin_; }
        size_type size() const;
        size_type length() const   { return size(); }
        size_type max_size() const { return size_type(0) - 2; }
        bool empty() const         { return end_ <= begin_; }

        bool has_min_size(size_type s) const;
        bool has_max_size(size_type s) const;

        const char *char_data() const
                { return reinterpret_cast<const char *>(begin_); }

        this_t &remove_prefix(size_type n_code_points);
        this_t &remove_suffix(size_type n_code_points);

        this_t &trim_left();
        this_t &trim_right();
        this_t &trim();

        this_t &
        swap(
                u8string_view &other
        )
        {
                std::swap(begin_, other.begin_);
                std::swap(end_, other.end_);
                return *this;
        }

        template <typename Traits = std::char_traits<char>,
                  typename Alloc = std::allocator<char>>
        std::basic_string<char, Traits, Alloc>
        to_string(const Alloc &a = Alloc()) const
                { return decltype(to_string())(
                        reinterpret_cast<const char *>(data()), bytes(), a); }

        template <typename Traits = std::char_traits<char>,
                  typename Alloc = std::allocator<char>>
        operator std::basic_string<char, Traits, Alloc>() const
                { return to_string<Traits, Alloc>(); }

        template <typename Traits = std::char_traits<char16_t>,
                  typename Alloc = std::allocator<char16_t>>
        std::basic_string<char16_t, Traits, Alloc>
        to_u16string(const Alloc &a = Alloc()) const;

        template <typename Traits = std::char_traits<char16_t>,
                  typename Alloc = std::allocator<char16_t>>
        operator std::basic_string<char16_t, Traits, Alloc>() const
                { return to_u16string<Traits, Alloc>(); }

        template <typename Traits = std::char_traits<char>>
                basic_string_view<char, Traits> to_string_view() const
                        { return basic_string_view<char, Traits>(char_data(),
                                                                 bytes()); }

        template <typename Traits = std::char_traits<char>>
        operator string_view() const { return to_string_view<Traits>(); }

        template <typename Traits = std::char_traits<char>,
                  typename Alloc = std::allocator<char>>
        std::basic_string<char, Traits, Alloc>
        to_upper(
                const Alloc &a = Alloc()
        ) const
        {
                return change_case(a, &touupper);
        }

        template <typename Traits = std::char_traits<char>,
                  typename Alloc = std::allocator<char>>
        std::basic_string<char, Traits, Alloc>
        to_lower(
                const Alloc &a = Alloc()
        ) const
        {
                return change_case(a, &toulower);
        }

        this_t &operator=(const this_t &other) = default;

        this_t substr(const_iterator pos, size_type n_code_points = npos) const;
        this_t substr(const_iterator pos, const_iterator end_pos) const
                { return u8string_view(pos, end_pos); }

        std::pair<this_t, this_t> split(char32_t sep) const;
        std::pair<this_t, this_t> split(const this_t &sep) const;

        std::pair<this_t, this_t> rsplit(char32_t sep) const;
        std::pair<this_t, this_t> rsplit(const this_t &sep) const;

        static int compare(char32_t c1, char32_t c2)
                { return static_cast<int>(c1) - static_cast<int>(c2); }

        int compare(const this_t &s2) const;
        int compare(const_iterator pos1, size_type n_code_points1,
                    const this_t &s2) const;

        int compare(const_iterator pos1, size_type n_code_points1,
                    const this_t &s2, const_iterator pos2,
                    size_type n_code_points2 = npos) const;

        int compare(const char *s2) const;
        int compare(const_iterator pos1, size_type n_code_points1,
                    const char *s2) const;
        int compare(const_iterator pos1, size_type n_code_points1,
                    const char *s2, size_type n_bytes2) const;
        int compare(const char32_t *s2) const;
        int compare(const char32_t *s2, size_type num_code_points2) const;
        int compare(const_iterator pos1, size_type n_code_points1,
                    const char32_t *s2) const;
        int compare(const_iterator pos1, size_type n_code_points1,
                    const char32_t *s2, size_type n_code_points2) const;

        bool has_prefix(const this_t &s2) const;
        bool has_suffix(const this_t &s2) const;

        static int compare_nocase(char32_t c1, char32_t c2);

        int compare_nocase(const this_t &s2) const;
        int compare_nocase(const_iterator pos1, size_type n_code_points1,
                           const this_t &s2) const;
        int compare_nocase(const_iterator pos1, size_type n_code_points1,
                           const this_t &s2, const_iterator pos2,
                           size_type n_code_points2 = npos) const;
        int compare_nocase(const char *s2) const;
        int compare_nocase(const_iterator pos1, size_type n_code_points1,
                           const char *s2) const;
        int compare_nocase(const_iterator pos1, size_type n_code_points1,
                           const char *s2, size_type n_bytes2) const;
        int compare_nocase(const char32_t *s2) const;
        int compare_nocase(const char32_t *s2,
                           size_type num_code_points2) const;
        int compare_nocase(const_iterator pos1, size_type n_code_points1,
                           const char32_t *s2) const;
        int compare_nocase(const_iterator pos1, size_type n_code_points1,
                           const char32_t *s2, size_type n_code_points2) const;

        bool has_prefix_nocase(const this_t &s2) const;
        bool has_suffix_nocase(const this_t &s2) const;

        const_iterator find(const this_t &substr,
                            const_iterator pos = {}) const;
        const_iterator find(char32_t c, const_iterator pos = {}) const;
        const_iterator find(const char *substr, const_iterator pos,
                            size_type n_substr_bytes) const;
        const_iterator find(const char *substr, const_iterator pos = {}) const;

        const_iterator rfind(const this_t &substr,
                             const_iterator pos = {}) const;
        const_iterator rfind(char32_t c, const_iterator pos = {}) const;
        const_iterator rfind(const char *substr, const_iterator pos,
                             size_type n_substr_bytes) const;
        const_iterator rfind(const char *substr, const_iterator pos = {}) const;

        const_iterator find_first_of(const this_t &chars,
                                     const_iterator pos = {}) const;

        const_iterator find_first_of(char32_t c, const_iterator pos = {}) const
                { return find(c, pos); }

        const_iterator find_first_of(const char *u8chars, const_iterator pos,
                                     size_type n_u8chars_bytes) const;

        const_iterator find_first_of(const char *u8chars,
                                     const_iterator pos = {}) const;

        const_iterator find_last_of(const this_t &chars,
                                    const_iterator pos = {}) const;

        const_iterator find_last_of(char32_t c, const_iterator pos = {}) const
                { return rfind(c, pos); }

        const_iterator find_last_of(const char *u8chars, const_iterator pos,
                                    size_type n_u8chars_bytes) const;

        const_iterator find_last_of(const char *u8chars,
                                    const_iterator pos = {}) const;

        const_iterator find_first_not_of(const this_t &chars,
                                         const_iterator pos = {}) const;

        const_iterator find_first_not_of(char32_t c,
                                         const_iterator pos = {}) const;

        const_iterator find_first_not_of(const char *u8chars,
                                         const_iterator pos,
                                         size_type n_u8chars_bytes) const;

        const_iterator find_first_not_of(const char *u8chars,
                                         const_iterator pos = {}) const;

        const_iterator find_last_not_of(const this_t &chars,
                                        const_iterator pos = {}) const;

        const_iterator find_last_not_of(char32_t c,
                                        const_iterator pos = {}) const;

        const_iterator find_last_not_of(const char *u8chars, const_iterator pos,
                                        size_type n_u8chars_bytes) const;

        const_iterator find_last_not_of(const char *u8chars,
                                        const_iterator pos = {}) const;

private:
        void ensure_is_safe();

        template <typename Traits = std::char_traits<char>,
                  typename Alloc = std::allocator<char>>
        std::basic_string<char, Traits, Alloc>
        change_case(
                const Alloc &a,
                char32_t    (*change_char)(char32_t)
        ) const
        {
                std::basic_string<char, Traits, Alloc> result(a);
                result.reserve(bytes());
                for (char32_t c: *this) {
                        result += (*change_char)(c);
                }
                return result;
        }

        pointer begin_, end_;
};

//--------------------------------------

template <typename Traits = std::char_traits<char>,
          typename Alloc = std::allocator<char>>
inline std::basic_string<char, Traits, Alloc> &
assign(
        std::basic_string<char, Traits, Alloc> &dst,
        const u8string_view                    &src
)
{
        dst.assign(src.char_data(), src.bytes());
        return dst;
}

//--------------------------------------

inline bool operator==(const u8string_view &a, const u8string_view &b)
        { return a.compare(b) == 0; }

inline bool operator!=(const u8string_view &a, const u8string_view &b)
        { return a.compare(b) != 0; }

inline bool operator<(const u8string_view &a, const u8string_view &b)
        { return a.compare(b) < 0; }

inline bool operator<=(const u8string_view &a, const u8string_view &b)
        { return a.compare(b) <= 0; }

inline bool operator>(const u8string_view &a, const u8string_view &b)
        { return a.compare(b) > 0; }

inline bool operator>=(const u8string_view &a, const u8string_view &b)
        { return a.compare(b) >= 0; }

//--------------------------------------

/* directly using std::numeric_limits<T>::min()/max() as default arguments of
   to_int() causes error C2589/C2059 with Visual Studio 2015 even with NOMINMAX
   defined */
template <typename T> T min_value() { return std::numeric_limits<T>::min(); }
template <typename T> T max_value() { return std::numeric_limits<T>::max(); }

template <typename T> WRUTIL_API T
        to_int(const u8string_view &s, size_t *end_code_point_offset = nullptr,
               int base = 10, T min_val = min_value<T>(),
               T max_val = max_value<T>());

template <> WRUTIL_API long long
        to_int<long long>(const u8string_view &s, size_t *end_code_point_offset,
                          int base, long long min_val, long long max_val);

template <> WRUTIL_API unsigned long long
        to_int<unsigned long long>(const u8string_view &s,
                                   size_t *end_code_point_offset, int base,
                                   unsigned long long min_val,
                                   unsigned long long max_val);

template <> inline short to_int<short>(const u8string_view &s,
                                       size_t *end_code_point_offset, int base,
                                       short min_val, short max_val)
        { return static_cast<short>(to_int<long long>(s, end_code_point_offset,
                                                     base, min_val, max_val)); }

template <> inline unsigned short
        to_int<unsigned short>(const u8string_view &s,
                               size_t *end_code_point_offset, int base,
                               unsigned short min_val, unsigned short max_val)
        { return static_cast<unsigned short>(
                to_int<unsigned long long>(s, end_code_point_offset,
                                           base, min_val, max_val)); }

template <> inline int to_int<int>(const u8string_view &s,
                                   size_t *end_code_point_offset, int base,
                                   int min_val, int max_val)
        { return static_cast<int>(to_int<long long>(s, end_code_point_offset,
                                                    base, min_val, max_val)); }

template <> inline unsigned int
        to_int<unsigned int>(const u8string_view &s,
                             size_t *end_code_point_offset, int base,
                             unsigned int min_val, unsigned int max_val)
        { return static_cast<unsigned int>(
                to_int<unsigned long long>(s, end_code_point_offset,
                                           base, min_val, max_val)); }

template <> inline long to_int<long>(const u8string_view &s,
                                     size_t *end_code_point_offset, int base,
                                     long min_val, long max_val)
        { return static_cast<long>(to_int<long long>(s, end_code_point_offset,
                                                     base, min_val, max_val)); }

template <> inline unsigned long
        to_int<unsigned long>(const u8string_view &s,
                              size_t *end_code_point_offset, int base,
                              unsigned long min_val, unsigned long max_val)
        { return static_cast<unsigned long>(
                to_int<unsigned long long>(s, end_code_point_offset,
                                           base, min_val, max_val)); }

//--------------------------------------

template <typename T> WRUTIL_API T
        to_float(const u8string_view &s,
                 size_t *end_code_point_offset = nullptr,
                 T min_val = min_value<T>(), T max_val = max_value<T>());

template <> WRUTIL_API double to_float<double>(const u8string_view &s,
                                               size_t *end_code_point_offset,
                                               double min_val, double max_val);

template <> WRUTIL_API long double
        to_float<long double>(const u8string_view &s,
                              size_t *end_code_point_offset,
                              long double min_val, long double max_val);

template <> inline float to_float<float>(const u8string_view &s,
                                         size_t *end_code_point_offset,
                                         float min_val, float max_val)
        { return static_cast<float>(
                to_float<double>(s, end_code_point_offset, min_val, max_val)); }

//--------------------------------------

WRUTIL_API std::string &per_thread_tmp_string_buffer();

//--------------------------------------

WRUTIL_API std::ostream &operator<<(std::ostream &out,
                                    const u8string_view &in);

//--------------------------------------
/*
 * wr::print() support
 */
namespace fmt {


struct Arg;
template <typename> struct TypeHandler;

template <> struct WRUTIL_API TypeHandler<u8string_view>
{
        static void set(Arg &arg, const u8string_view &val);
};


} // namespace fmt


} // namespace wr

//--------------------------------------

namespace std {


template <> inline void swap(wr::u8string_view &a,
                             wr::u8string_view &b) noexcept
        { a.swap(b); }


template <>
struct hash<wr::u8string_view>
{
        size_t operator()(const wr::u8string_view &val) const
                { return wr::stdHash(val.char_data(), val.bytes()); }
};


} // namespace std


#endif // !WRUTIL_U8STRING_VIEW_H
