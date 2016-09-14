/**
 * \file utf16.cxx
 *
 * \brief Implementation of low-level UTF-16 character and string handling APIs
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
#include <wrutil/Config.h>
#include <wrutil/utf16.h>
#include <wrutil/ctype.h>  // for wr::INVALID_CHAR


namespace wr {


WRUTIL_API char32_t
utf16_char(
        const char16_t  *p,
        const char16_t  *end,
        const char16_t **next_p
)
{
        char32_t result;

        if (!is_surrogate(*p)) {
                result = *(p++);
        } else if (is_low_surrogate(*p)) {
                result = INVALID_CHAR;
                ++p;
        } else {  // high surrogate
                result = *p - 0xd800;
                if ((++p < end) && (is_low_surrogate(*p))) {
                        result = ((result << 10) + (*(p++) - 0xdc00)) + 0x10000;
                } else {
                        result = INVALID_CHAR;
                }
        }

        if (next_p) {
                *next_p = p;
        }

        return result;
}

//--------------------------------------

WRUTIL_API const char16_t *
utf16_dec(
        const char16_t *p,
        const char16_t *begin
)
{
        if (p > begin) {
                if (is_low_surrogate(*(--p)) && (p > begin)) {
                        --p;
                }
        }
        return p;
}

//--------------------------------------

WRUTIL_API uint8_t
utf16_seq(
        char32_t  in,
        char16_t *out
)
{
        if (in <= 0xffff) {
                *out = static_cast<char16_t>(in);
                return 1;
        } else {
                in -= 0x10000;
                out[1] = static_cast<char16_t>(in & 0x03ff);
                out[0] = static_cast<char16_t>((in >>= 10) & 0x03ff);
                return 2;
        }
}


} // namespace wr
