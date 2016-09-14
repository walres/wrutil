/**
 * \file arraystream.h
 *
 * \brief iostream classes for reading/writing fixed-size character arrays
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
#ifndef WRUTIL_ARRAYSTREAM_H
#define WRUTIL_ARRAYSTREAM_H

#include <iostream>
#include <wrutil/arraybuf.h>


namespace wr {


template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_iarraystream :
        public std::basic_istream<CharT, Traits>
{
public:
        using this_type = basic_iarraystream;
        using base_type = std::basic_istream<CharT, Traits>;
        using char_type = CharT;
        using traits_type = Traits;
        using int_type = typename traits_type::int_type;
        using pos_type = typename traits_type::pos_type;
        using off_type = typename traits_type::off_type;

        basic_iarraystream() : base_type(&buf_) {}

        basic_iarraystream(const char_type *buf, std::streamsize size) :
                base_type(&buf_), buf_(buf, size) {}

        basic_iarraystream(const this_type &) = delete;
        basic_iarraystream(this_type &&) = default;

        this_type &operator=(const this_type &) = delete;
        this_type &operator=(this_type &&) = default;

        void
        swap(
                this_type &other
        )
        {
                buf_.swap(other.buf_);
                base_type::swap(other);
        }

        basic_arraybuf<char_type, traits_type> *rdbuf() const
                { return static_cast<decltype(rdbuf())>(base_type::rdbuf()); }

private:
        basic_arraybuf<char_type, traits_type> buf_;
};

using iarraystream = basic_iarraystream<char>;
using wiarraystream = basic_iarraystream<wchar_t>;

//--------------------------------------

template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_oarraystream :
        public std::basic_ostream<CharT, Traits>
{
public:
        using this_type = basic_oarraystream;
        using base_type = std::basic_ostream<CharT, Traits>;
        using char_type = CharT;
        using traits_type = Traits;
        using int_type = typename traits_type::int_type;
        using pos_type = typename traits_type::pos_type;
        using off_type = typename traits_type::off_type;

        basic_oarraystream() : base_type(&buf_) {}

        basic_oarraystream(char_type *buf, std::streamsize size) :
                base_type(&buf_), buf_(buf, size) {}

        basic_oarraystream(const this_type &) = delete;
        basic_oarraystream(this_type &&) = default;

        this_type &operator=(const this_type &) = delete;
        this_type &operator=(this_type &&) = default;

        void
        swap(
                this_type &other
        )
        {
                buf_.swap(other.buf_);
                base_type::swap(other);
        }

        basic_arraybuf<char_type, traits_type> *rdbuf() const
                { return static_cast<decltype(rdbuf())>(base_type::rdbuf()); }

private:
        basic_arraybuf<char_type, traits_type> buf_;
};

using oarraystream = basic_oarraystream<char>;
using woarraystream = basic_oarraystream<wchar_t>;

//--------------------------------------

template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_arraystream :
        public std::basic_iostream<CharT, Traits>
{
public:
        using this_type = basic_arraystream;
        using base_type = std::basic_iostream<CharT, Traits>;
        using char_type = CharT;
        using traits_type = Traits;
        using int_type = typename traits_type::int_type;
        using pos_type = typename traits_type::pos_type;
        using off_type = typename traits_type::off_type;

        basic_arraystream() : base_type(&buf_) {}

        basic_arraystream(char_type *buf, std::streamsize size) :
                base_type(&buf_), buf_(buf, size) {}

        basic_arraystream(const this_type &) = delete;
        basic_arraystream(this_type &&) = default;

        this_type &operator=(const this_type &) = delete;
        this_type &operator=(this_type &&) = default;

        void
        swap(
                this_type &other
        )
        {
                buf_.swap(other.buf_);
                base_type::swap(other);
        }

        basic_arraybuf<char_type, traits_type> *rdbuf() const
                { return static_cast<decltype(rdbuf())>(base_type::rdbuf()); }

private:
        basic_arraybuf<char_type, traits_type> buf_;
};

using arraystream = basic_arraystream<char>;
using warraystream = basic_arraystream<wchar_t>;


} // namespace wr

//--------------------------------------

namespace std {


template <typename CharT, typename Traits> void
swap(
        wr::basic_iarraystream<CharT, Traits> &a,
        wr::basic_iarraystream<CharT, Traits> &b
)
{
        a.swap(b);
}

//--------------------------------------

template <typename CharT, typename Traits> void
swap(
        wr::basic_oarraystream<CharT, Traits> &a,
        wr::basic_oarraystream<CharT, Traits> &b
)
{
        a.swap(b);
}

//--------------------------------------

template <typename CharT, typename Traits> void
swap(
        wr::basic_arraystream<CharT, Traits> &a,
        wr::basic_arraystream<CharT, Traits> &b
)
{
        a.swap(b);
}


} // namespace std


#endif // !WRUTIL_ARRAYSTREAM_H
