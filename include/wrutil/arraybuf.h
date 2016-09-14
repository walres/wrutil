/**
 * \file arraybuf.h
 *
 * \brief std::basic_streambuf interface for reading/writing fixed-size
 *        character arrays
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
#ifndef WRUTIL_ARRAYBUF_H
#define WRUTIL_ARRAYBUF_H

#include <limits.h>
#include <streambuf>
#include <wrutil/numeric_cast.h>


namespace wr {


template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_arraybuf :
        public std::basic_streambuf<CharT, Traits>
{
public:
        using this_type = basic_arraybuf;
        using base_type = std::basic_streambuf<CharT, Traits>;
        using char_type = CharT;
        using traits_type = Traits;
        using int_type = typename traits_type::int_type;
        using pos_type = typename traits_type::pos_type;
        using off_type = typename traits_type::off_type;

        basic_arraybuf() = default;

        basic_arraybuf(const char_type *begin, std::streamsize size)
                { setbuf(begin, size); }

        basic_arraybuf(char_type *begin, std::streamsize size)
                { setbuf(begin, size); }

        basic_arraybuf(const this_type &) = default;
        virtual ~basic_arraybuf() = default;

        this_type &operator=(const this_type &) = default;

        void swap(this_type &other) { base_type::swap(*this); }

        this_type *
        setbuf(
                const char_type *begin,
                std::streamsize  size
        )
        {
                base_type::setg(const_cast<char_type *>(begin),
                                const_cast<char_type *>(begin),
                                const_cast<char_type *>(begin) + size);
                base_type::setp(nullptr, nullptr);
                return this;
        }

        virtual this_type *
        setbuf(
                char_type       *begin,
                std::streamsize  size
        )
        {
                base_type::setg(begin, begin, begin + size);
                base_type::setp(begin, begin + size);
                return this;
        }

protected:
#if 0
        char_type *eback() const { return base_type::eback(); }
        char_type *gptr() const  { return base_type::gptr(); }
        char_type *egptr() const { return base_type::egptr(); }
        char_type *pbase() const { return base_type::pbase(); }
        char_type *pptr() const  { return base_type::pptr(); }
        char_type *epptr() const { return base_type::epptr(); }
#endif

        virtual pos_type
        seekoff(
                off_type                off,
                std::ios_base::seekdir  dir,
                std::ios_base::openmode which
        ) override
        {
                static const pos_type FAIL = pos_type(off_type(-1));
                char_type *new_pos;

                switch (dir) {
                case std::ios_base::beg:
                        new_pos = eback() + off;
                        break;
                case std::ios_base::end:
                        new_pos = egptr() - off;
                        break;
                case std::ios_base::cur:
                        new_pos = gptr() + off;
                        break;
                default:
                        return FAIL;
                }

                if ((new_pos < eback()) || (new_pos > egptr())) {
                        return FAIL;
                }

                if (which & std::ios_base::out) {
                        if (!pbase()) {  // configured for reading only
                                return FAIL;
                        }
                        off = new_pos - pptr();
                        if ((off < INT_MIN) || (off > INT_MAX)) {
                                return FAIL;
                        }
                        base_type::pbump(int(off));
                }
                if (which & std::ios_base::in) {
                        base_type::setg(eback(), new_pos, egptr());
                }
                return pos_type(new_pos - eback());
        }

        virtual pos_type seekpos(pos_type pos,
                                 std::ios_base::openmode which) override
                { return seekoff(off_type(pos), std::ios_base::beg, which); }
#if 0
        virtual int_type
        underflow() override
        {
                if (gptr() < egptr()) {
                        return int_type(*gptr());
                } else {
                        return traits_type::eof();
                }
        }

        virtual std::streamsize
        xsgetn(
                char_type       *buf,
                std::streamsize  count
        ) override
        {
                if (count > 0) {
                        count = std::min(count, base_type::in_avail());
                        traits_type::copy(buf, gptr(),
                                          numeric_cast<size_t>(count));
                        /* don't use gbump() as it takes int which may be
                           narrower than std::streamsize */
                        base_type::setg(eback(), gptr() + count, egptr());
                        return count;
                } else {
                        return 0;
                }
        }

        virtual std::streamsize
        xsputn(
                const char_type *buf,
                std::streamsize  count
        ) override
        {
                if (count > 0) {
                        count = std::min(count,
                                         std::streamsize(epptr() - pptr()));
                        count = std::min(count, std::streamsize(INT_MAX));
                        traits_type::copy(pptr(), buf,
                                          numeric_cast<size_t>(count));
                        base_type::pbump(int(count));
                        return count;
                } else {
                        return 0;
                }
        }
#endif
};

using arraybuf = basic_arraybuf<char>;
using warraybuf = basic_arraybuf<wchar_t>;


} // namespace wr

//--------------------------------------

namespace std {


template <typename CharT, typename Traits> void
swap(
        wr::basic_arraybuf<CharT, Traits> &a,
        wr::basic_arraybuf<CharT, Traits> &b
)
{
        a.swap(b);
}


} // namespace std


#endif // !WRUTIL_ARRAYBUF_H
