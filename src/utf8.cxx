/**
 * \file utf8.cxx
 *
 * \brief Implementation of low-level UTF-8 character and string handling APIs
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
#if WR_POSIX
#       include <langinfo.h>
#elif WR_WINAPI
#       include <windows.h>
#endif
#include <errno.h>
#include <string.h>
#include <thread>
#include <wrutil/utf8.h>
#include <wrutil/ctype.h>  // for wr::INVALID_CHAR
#include <wrutil/codecvt.h>


namespace wr {


WRUTIL_API bool
is_utf8(
        const std::locale &loc
)
{
        std::string name = loc.name();
        size_t      i    = name.rfind('.');

        if (i == name.npos) {
                auto &cvt = std::use_facet<std::codecvt<wchar_t, char,
                                                        std::mbstate_t>>(loc);
                static const char * const in = u8"\u2323";
                const char *end_in;
                wchar_t out, *end_out;
                std::mbstate_t state = { 0 };

                if (cvt.in(state, in,
                      in + utf8_seq_size(reinterpret_cast<const uint8_t *>(in)),
                      end_in, &out, &out + 1, end_out) == cvt.ok) {
                        return (end_out == (&out + 1)) && (out == 0x2323);
                }

                return false;
        }

        for (size_t j = ++i, n = name.size(); j < n; ++j) {
                if (isalnum(name[j])) {
                        name[i++] = tolower(name[j]);
                }
        }

        name.resize(i);
        return is_utf8(string_view(name).rsplit('.').second);
}

//--------------------------------------

WRUTIL_API bool
is_utf8(
        string_view charset_name
)
{
        charset_name.trim();

        for (const string_view &name: { "utf8", "utf-8", "ibm-1208", "ibm-1209",
                                        "ibm-5304", "ibm-5305", "ibm-13496",
                                        "ibm-13497", "ibm-17592", "ibm-17593",
                                        "windows-65001", "65001", "cp65001",
                                        "cp1208", "x-utf_8j",
                                        "unicode-1-1-utf-8",
                                        "unicode-2-0-utf-8" }) {
                if (!charset_name.compare_nocase(name)) {
                        return true;
                }
        }

        return false;
}

//--------------------------------------

WRUTIL_API char32_t
utf8_char(
        const uint8_t  *p,
        const uint8_t  *end,
        const uint8_t **next_p
)
{
        char32_t result;

        if (p < end) switch ((*p) >> 4) {
        case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
                result = *(p++);
                break;
        case 12: case 13:       // 110xxxxx 10xxxxxx
                result = char32_t(*(p++) & 0b00011111);
                if ((p < end) && (((*p) & 0xc0) == 0x80)) {
                        result = (result << 6) | (*(p++) & 0x3f);
                } else {
                        result = INVALID_CHAR;
                        break;
                }
                break;
        case 14:                // 1110xxxx 10xxxxxx 10xxxxxx
                result = char32_t(*(p++) & 0b00001111);
                if ((p < end) && (((*p) & 0xc0) == 0x80)) {
                        result = (result << 6) | (*(p++) & 0x3f);
                } else {
                        result = INVALID_CHAR;
                        break;
                }
                if ((p < end) && (((*p) & 0xc0) == 0x80)) {
                        result = (result << 6) | (*(p++) & 0x3f);
                } else {
                        result = INVALID_CHAR;
                        break;
                }
                break;
        case 15:                // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
                result = char32_t(*(p++) & 0b00000111);
                if ((p < end) && (((*p) & 0xc0) == 0x80)) {
                        result = (result << 6) | (*(p++) & 0x3f);
                } else {
                        result = INVALID_CHAR;
                        break;
                }
                if ((p < end) && (((*p) & 0xc0) == 0x80)) {
                        result = (result << 6) | (*(p++) & 0x3f);
                } else {
                        result = INVALID_CHAR;
                        break;
                }
                if ((p < end) && (((*p) & 0xc0) == 0x80)) {
                        result = (result << 6) | (*(p++) & 0x3f);
                } else {
                        result = INVALID_CHAR;
                        break;
                }
                break;
        default:
                ++p;
                result = INVALID_CHAR;
                break;
        } else {
                result = INVALID_CHAR;
        }

        if (next_p) {
                *next_p = p;
        }

        return result;
}

//--------------------------------------

WRUTIL_API const uint8_t *
utf8_inc_(
        const uint8_t *p
)
{
        return p + utf8_seq_size(p);
}

//--------------------------------------

WRUTIL_API const uint8_t *
utf8_dec(
        const uint8_t *p
)
{
        --p;
        if (((*p) & 0xc0) == 0x80) {
                --p;
        }
        if (((*p) & 0xc0) == 0x80) {
                --p;
        }
        if (((*p) & 0xc0) == 0x80) {
                --p;
        }
        return p;
}

//--------------------------------------

WRUTIL_API const uint8_t *
utf8_dec(
        const uint8_t *p,
        const uint8_t *begin
)
{
        if (--p >= begin) {
                if (((*p) & 0xc0) == 0x80) {
                        --p;
                }
                if (p >= begin) {
                        if (((*p) & 0xc0) == 0x80) {
                                --p;
                        }
                        if (p >= begin) {
                                if (((*p) & 0xc0) == 0x80) {
                                        --p;
                                }
                        }
                }
        }
        return p;
}

//--------------------------------------

WRUTIL_API uint8_t
utf8_seq_size(
        char32_t c
)
{
        if (c < 0x80) {
                return 1;
        } else if (c < 0x800) {
                return 2;
        } else if (c < 0x200000) {
                return 3;
        } else {
                return 4;
        }
}

//--------------------------------------

WRUTIL_API uint8_t
utf8_seq_size(
        const uint8_t *p
)
{
        static const uint8_t SIZE[] = {
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 4
        };
        return SIZE[*p >> 4];
}

//--------------------------------------

WRUTIL_API uint8_t
utf8_seq(
        char32_t  in,
        uint8_t  *out
)
{
        if (in < 0x80) {
                out[0] = static_cast<uint8_t>(in);
                return 1;
        } else if (in < 0x800) {
                out[1] = 0x80 | static_cast<uint8_t>(in & 0x3f);
                in >>= 6;
                out[0] = 0xc0 | static_cast<uint8_t>(in & 0x1f);
                return 2;
        } else if (in < 0x200000) {
                out[2] = 0x80 | static_cast<uint8_t>(in & 0x3f);
                in >>= 6;
                out[1] = 0x80 | static_cast<uint8_t>(in & 0x3f);
                in >>= 6;
                out[0] = 0xe0 | static_cast<uint8_t>(in & 0xf);
                return 3;
        } else if (in < 0x4000000) {
                out[3] = 0x80 | static_cast<uint8_t>(in & 0x3f);
                in >>= 6;
                out[2] = 0x80 | static_cast<uint8_t>(in & 0x3f);
                in >>= 6;
                out[1] = 0x80 | static_cast<uint8_t>(in & 0x3f);
                in >>= 6;
                out[0] = 0xf0 | static_cast<uint8_t>(in & 0x7);
                return 4;
        } else {
                return utf8_seq(INVALID_CHAR, out);
        }
}

//--------------------------------------

WRUTIL_API std::string
u8strerror(
        int errnum
)
{
#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !_GNU_SOURCE
        std::string text(256, '\0');
        while (true) {
                if (strerror_r(errnum, &text[0], text.size()) == 0) {
                        text = utf8_narrow_cvt().to_utf8(text.data());
                        break;
                } else if (errno == ERANGE) {
                        text.resize(text.size() * 2);
                } else {  // EINVAL
                        text = "Unknown system error code "
                                + std::to_string(errnum);
                        break;
                }
        }
        return text;
#elif _GNU_SOURCE
        std::string text(512, '\0');
        char *msg = strerror_r(errnum, &text[0], text.size());
        if (msg == &text[0]) {
                text = utf8_narrow_cvt().to_utf8(text.data());
        } else {
                text = utf8_narrow_cvt().to_utf8(msg);
        }
        return text;
#elif WR_WINDOWS && defined(_UCRT)
        std::string text(512, '\0');
        if (strerror_s(&text[0], text.size(), errnum) == 0) {
                size_t pos = text.find('\n');
                if (pos != text.npos) {
                        text.resize(pos);
                }
                text = utf8_narrow_cvt().to_utf8(text);
        } else {
                text = "Unknown system error code " + std::to_string(errnum);
        }
        return text;
#else
        static std::mutex mutex;
        std::lock_guard lock(mutex);
        return utf8_narrow_cvt().to_utf8(strerror(errnum));
#endif
}


} // namespace wr
