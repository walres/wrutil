/**
 * \file utf16.h
 *
 * \brief Low-level UTF-16 character and string handling API
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
#ifndef WRUTIL_UTF16_H
#define WRUTIL_UTF16_H

#include <stddef.h>
#include <string>
#include <wrutil/Config.h>


namespace wr {


enum
{
        UTF16_SEQ_MAX       = 2,
        UTF16_SEQ_MAX_BYTES = 4
};

//--------------------------------------

WRUTIL_API char32_t utf16_char(const char16_t *p, const char16_t *end,
                               const char16_t **next_p);

inline bool is_surrogate(char16_t c) { return (c >= 0xd800) && (c <= 0xdfff); }

inline bool is_high_surrogate(char16_t c)
        { return (c >= 0xd800) && (c <= 0xdbff); }

inline bool is_low_surrogate(char16_t c)
        { return (c >= 0xdc00) && (c <= 0xdfff); }

inline char32_t utf16_char(const char16_t *p, const char16_t *end)
        { return is_surrogate(*p) ? utf16_char(p, end, nullptr) : *(p++); }

inline const char16_t *utf16_inc(const char16_t *p)
        { return is_high_surrogate(*p) ? p + 2 : p + 1; }

inline char16_t *utf16_inc(char16_t *p)
        { return const_cast<char16_t *>(
                utf16_inc(static_cast<const char16_t *>(p))); }

inline const char16_t *utf16_dec(const char16_t *p)
        { return is_low_surrogate(*(--p)) ? --p : p; }

WRUTIL_API const char16_t *utf16_dec(const char16_t *p, const char16_t *begin);

inline char16_t *utf16_dec(char16_t *p)
        { return const_cast<char16_t *>(
                utf16_dec(static_cast<const char16_t *>(p))); }

inline char16_t *utf16_dec(char16_t *p, const char16_t *begin)
        { return const_cast<char16_t *>(
                utf16_dec(static_cast<const char16_t *>(p), begin)); }

inline uint8_t utf16_seq_size(char32_t c) { return (c > 0xffff) ? 2 : 1; }

inline uint8_t utf16_seq_size(const char16_t *p)
        { return is_high_surrogate(*p) ? 2 : 1; }

WRUTIL_API uint8_t utf16_seq(char32_t in, char16_t *out);

//--------------------------------------

template <typename Traits = std::char_traits<char16_t>,
          typename Alloc = std::allocator<char16_t>>
std::basic_string<char16_t, Traits, Alloc> &
operator+=(
        std::basic_string<char16_t, Traits, Alloc> &str,
        char32_t                                    c
)
{
        if (c <= 0xffff) {
                str += static_cast<char16_t>(c);
        } else {
                str += static_cast<char16_t>((c -= 0x10000) & 0x03ff);
                str += static_cast<char16_t>((c >>= 10) & 0x03ff);
        }

        return str;
}

//--------------------------------------

template <typename Traits = std::char_traits<wchar_t>,
          typename Alloc = std::allocator<wchar_t>>
std::basic_string<wchar_t, Traits, Alloc> &
operator+=(
        std::basic_string<wchar_t, Traits, Alloc> &str,
        char32_t                                   c
)
{
#if WR_SIZEOF_WCHAR_T == 2
        if (c <= 0xffff) {
                return str += static_cast<wchar_t>(c);
        } else {
                str += static_cast<wchar_t>((c -= 0x10000) & 0x03ff);
                str += static_cast<wchar_t>((c >>= 10) & 0x03ff);
                return str;
        }
#else
        return str += static_cast<wchar_t>(c);
#endif
}


} // namespace wr


#endif // !WRUTIL_UTF16_H
