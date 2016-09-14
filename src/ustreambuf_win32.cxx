/**
 * \file ustreambuf_win32.cxx
 *
 * \brief MS-Windows-specific implementation of stream buffer for Unicode
 *        iostreams wr::ucin, wr::ucout, wr::ucerr and wr::uclog
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
#ifndef UNICODE
#       define UNICODE 1
#endif
#include <assert.h>
#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <stdexcept>
#include <streambuf>
#include <wrutil/numeric_cast.h>
#include <wrutil/u8string_view.h>
#include <wrutil/uiostream.h>
#include <wrutil/utf16.h>


namespace wr {


class winconsolebuf :
        public std::streambuf
{
        enum
        {
                BUF_SIZE = 1024
        };

public:
        using this_t = winconsolebuf;

        winconsolebuf(
                HANDLE                  console,
                std::ios_base::openmode mode
        ) :
                console_ (console),
                user_buf_(false)
        {
                if ((mode & std::ios_base::in) == std::ios_base::in) {
                        is_input_ = true;
                } else if ((mode & std::ios_base::out) == std::ios_base::out) {
                        is_input_ = false;
                } else {
                        throw std::invalid_argument(
                                "mode not ios_base::in or ios_base::out");
                }

                char *buf = new char[BUF_SIZE];
                try {
                        wbuf_ = new wchar_t[BUF_SIZE];
                        buf_ = buf;
                } catch (std::bad_alloc &) {
                        delete [] buf;
                        throw;
                }
        }

        virtual
        ~winconsolebuf() override
        {
                if (!user_buf_) {
                        delete [] buf_;
                        delete [] wbuf_;
                }
        }

protected:
        virtual int
        sync() override
        {
                if (is_input_) {
                        if (!FlushConsoleInputBuffer(console_)) {
                                return -1;
                        }
                        setg(nullptr, nullptr, nullptr);
                } else {
                        if (pptr() != pbase()) {
                                if (overflow() == traits_type::eof()) {
                                        return -1;
                                }
                        }
                        setp(buf_, buf_ + BUF_SIZE);
                }
                return 0;
        }

        virtual int_type
        underflow() override
        {
                if (!is_input_) {
                        return traits_type::eof();
                } else if (gptr() < egptr()) {
                        return *gptr();
                } else if (!eback()) {
                        setg(buf_, buf_, buf_);
                }

                char *buf_end = &buf_[BUF_SIZE];

                if ((buf_end - egptr()) < 4) {
                        /* leave 4 bytes' worth of putback space
                           (enough for largest poss. UTF-8 sequence) */
                        if ((egptr() - eback()) < 4) {  // buffer too small
                                return traits_type::eof();
                        }
                        memmove(buf_, egptr() - 4, 4);
                        setg(buf_, buf_ + 4, buf_ + 4);
                }

                DWORD read_max = numeric_cast<DWORD>(buf_end - egptr()) / 4,
                      read_count;

                if (!read_max) {  // buffer too small
                        return traits_type::eof();
                }

                if (!ReadConsoleW(console_, &wbuf_[0], read_max,
                                  &read_count, nullptr)) {
                        return traits_type::eof();
                }

                const auto *in     = reinterpret_cast<char16_t *>(&wbuf_[0]),
                           *in_end = in + read_count;
                auto       *out    = egptr();

                while (in < in_end) {
                        char32_t c = utf16_char(in, in_end);
                        ++in;
                        if (c > 0xffff) {  // surrogate pair
                                ++in;
                        }
                        out += utf8_seq(c, reinterpret_cast<uint8_t *>(out));
                }

                setg(buf_, egptr(), out);
                return *gptr();
        }

        virtual int_type
        overflow(
                int_type c = traits_type::eof()
        ) override
        {
                if (is_input_) {
                        return traits_type::eof();
                }

                if ((c != traits_type::eof()) && (pptr() < epptr())) {
                        *pptr() = c;
                        pbump(1);
                        c = traits_type::eof();
                }

                char *save_pptr = pptr();

                u8string_view chars(pbase(), pptr());
                        // will truncate any partial UTF-8 sequence at the end
                setp(pbase(), epptr());
                pbump(numeric_cast<int>(chars.bytes()));

                while (!chars.empty()) {
                        union
                        {
                                wchar_t  *w;
                                char16_t *u16;
                        } wbuf_pos;

                        wchar_t *wbuf_beg = &wbuf_[0],
                                *wbuf_end = &wbuf_[BUF_SIZE];

                        wbuf_pos.w = wbuf_beg;

                        for (char32_t c: chars) {
                                if (wbuf_pos.w >= wbuf_end) {
                                        break;
                                } else if ((wbuf_end - wbuf_pos.w) == 1) {
                                        if (c > 0xffff) {
                                                // no room for surrogate pair
                                                break;
                                        }
                                }
                                wbuf_pos.w += utf16_seq(c, wbuf_pos.u16);
                        }

                        DWORD write_count = 0;

                        while (true) {
                                if (!WriteConsoleW(console_, wbuf_beg,
                                                   numeric_cast<DWORD>(
                                                        wbuf_pos.w - wbuf_beg),
                                                   &write_count, nullptr)) {
                                        return traits_type::eof();
                                }

                                if (write_count >= static_cast<DWORD>
                                                      (wbuf_pos.w - wbuf_beg)) {
                                        chars = {};
                                        break;
                                } else if (write_count > 0) {
                                        /* write_count is in UTF-16 code units
                                           not code points */
                                        auto i = chars.begin();
                                        for (DWORD j = 0; j < write_count; ) {
                                                ++j;
                                                if (*i > 0xffff) {
                                                        ++j;
                                                }
                                                ++i;
                                        }
                                        chars = chars.substr(i);
                                        wbuf_beg = wbuf_pos.w;
                                }
                        }
                }

                setp(buf_, buf_ + BUF_SIZE);

                if (c != traits_type::eof()) {
                        *pptr() = c;
                        pbump(1);
                }

                return 0;
        }

        virtual int_type
        pbackfail(
                int_type c
        ) override
        {
                if (c != traits_type::eof()) {
                        if (gptr() > eback()) {
                                gbump(-1);
                                *gptr() = c;
                        } else {
                                c = traits_type::eof();
                        }
                }
                return c;
        }

private:
        HANDLE   console_;
        bool     is_input_ : 1,
                 user_buf_ : 1;
        char    *buf_;
        wchar_t *wbuf_;
};

//--------------------------------------

std::streambuf *
getUStreamBuf(
        std::streambuf           *underlying_streambuf,
        FILE                     *c_stream,
        std::ios_base::openmode   mode,
        std::streambuf          *&overlying_streambuf
)
{
        int fd = _fileno(c_stream);

        if (fd >= 0) {
                auto h = reinterpret_cast<HANDLE>(_get_osfhandle(fd));

                if (h != INVALID_HANDLE_VALUE) {
                        DWORD dummy;
                        if (GetConsoleMode(h, &dummy)) {
                                return overlying_streambuf
                                        = new winconsolebuf(h, mode);
                        }
                }
        }

        return underlying_streambuf;  // file redirection, exchange raw UTF-8
}


} // namespace wr
