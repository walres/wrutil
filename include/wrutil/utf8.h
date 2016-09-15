/**
 * \file utf8.h
 *
 * \brief Low-level UTF-8 character and string handling API
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
#ifndef WRUTIL_UTF8_H
#define WRUTIL_UTF8_H

#include <stddef.h>
#include <locale>
#include <string>
#include <wrutil/Config.h>
#include <wrutil/string_view.h>


namespace wr {


enum
{
        UTF8_SEQ_MAX = 4
};

//--------------------------------------

WRUTIL_API char32_t utf8_char(const uint8_t *p, const uint8_t *end,
                              const uint8_t **next_p);

WRUTIL_API const uint8_t *utf8_inc_(const uint8_t *p);

inline char32_t utf8_char(const uint8_t *p, const uint8_t *end)
        { return ((*p) & 0x80) ? utf8_char(p, end, nullptr) : *(p++); }

inline const uint8_t *utf8_inc(const uint8_t *p)
        { return ((*p) & 0x80) ? utf8_inc_(p) : p + 1; }

inline uint8_t *utf8_inc(uint8_t *p)
        { return const_cast<uint8_t *>(
                utf8_inc(static_cast<const uint8_t *>(p))); }

WRUTIL_API const uint8_t *utf8_dec(const uint8_t *p);
WRUTIL_API const uint8_t *utf8_dec(const uint8_t *p, const uint8_t *begin);

inline uint8_t *utf8_dec(uint8_t *p)
        { return const_cast<uint8_t *>(
                utf8_dec(static_cast<const uint8_t *>(p))); }

inline uint8_t *utf8_dec(uint8_t *p, const uint8_t *begin)
        { return const_cast<uint8_t *>(
                utf8_dec(static_cast<const uint8_t *>(p), begin)); }

WRUTIL_API uint8_t utf8_seq_size(char32_t c);
WRUTIL_API uint8_t utf8_seq_size(const uint8_t *p);

WRUTIL_API uint8_t utf8_seq(char32_t in, uint8_t *out);

WRUTIL_API bool is_utf8(const std::locale &loc);
WRUTIL_API bool is_utf8(string_view charset_name);

WRUTIL_API std::string u8strerror(int errnum);

//--------------------------------------

template <typename Traits = std::char_traits<char>,
          typename Alloc = std::allocator<char>>
std::basic_string<char, Traits, Alloc> &
utf8_append(
        std::basic_string<char, Traits, Alloc> &str,
        char32_t                                c
)
{
        if (c < 0x80) {
                return str += static_cast<char>(c);
        } else {
                uint8_t seq[UTF8_SEQ_MAX];
                size_t  seq_size = utf8_seq(c, seq);
                str.append(reinterpret_cast<char *>(seq), seq_size);
                return str;
        }
}


} // namespace wr


#endif // !WRUTIL_UTF8_H
