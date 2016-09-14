/**
 * \file codecvt.h
 *
 * \brief APIs for converting between UTF-8/16/32 and narrow strings
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
#ifndef WRUTIL_CODECVT_H
#define WRUTIL_CODECVT_H

#include <locale>
#include <string>

#include <wrutil/Config.h>
#include <wrutil/wbuffer_convert.h>
#include <wrutil/wstring_convert.h>
#include <wrutil/string_view.h>
#include <wrutil/u8string_view.h>

#if !WR_HAVE_STD_CODECVT_CHAR16
#       include <wrutil/codecvt/char16.h>
#endif
#if !WR_HAVE_STD_CODECVT_CHAR32
#       include <wrutil/codecvt/char32.h>
#endif
#include <wrutil/codecvt/cvt_utf8.h>

//--------------------------------------

namespace wr {


class WRUTIL_API codecvt_utf8_narrow :
        public std::codecvt<char, char, std::mbstate_t>
{
public:
        using base_type = std::codecvt<char, char, std::mbstate_t>;

        static std::locale::id id;

        codecvt_utf8_narrow(std::locale loc, std::size_t refs = 0);
        explicit codecvt_utf8_narrow(std::size_t refs = 0);

        virtual ~codecvt_utf8_narrow();

protected:
        virtual result do_out(state_type &state, const intern_type *from,
                              const intern_type *from_end,
                              const intern_type *&from_next, extern_type *to,
                              extern_type *to_end, extern_type *&to_next) const;

        virtual result do_in(state_type &state, const extern_type *from,
                             const extern_type *from_end,
                             const extern_type *&from_next, intern_type *to,
                             intern_type *to_end, intern_type *&to_next) const;

        virtual result do_unshift(state_type &state, extern_type *to,
                                  extern_type *to_end,
                                  extern_type *&to_next) const;

        virtual int do_encoding() const noexcept;
        virtual bool do_always_noconv() const noexcept;

        virtual int do_length(state_type &state, const extern_type *from,
                              const extern_type *from_end,
                              std::size_t max) const;

        virtual int do_max_length() const noexcept;

private:
        struct Body;
        Body *body_;
};

//--------------------------------------

struct codecvt_wide_narrow :
        public std::codecvt<wchar_t, char, std::mbstate_t>
{
        using codecvt::codecvt;
        ~codecvt_wide_narrow() = default;
};

//--------------------------------------

template <typename Alloc = std::allocator<char>>
class u8string_convert
{
public:
        using this_type = u8string_convert;
        using string_type
                = std::basic_string<char, std::char_traits<char>, Alloc>;

        u8string_convert(codecvt_utf8_narrow *cvt = new codecvt_utf8_narrow) :
                cnv_(cvt) {}

        u8string_convert(const string_type &local_err,
                         const string_type &utf8_err = string_type()) :
                cnv_(local_err, utf8_err) {}

        u8string_convert(const this_type &other) = delete;

        ~u8string_convert() = default;

        this_type &operator=(const this_type &other) = delete;

        string_type to_utf8(char c) { return cnv_.from_bytes(c); }
        string_type to_utf8(const string_view &s)
                { return cnv_.from_bytes(s.begin(), s.end()); }
        string_type to_utf8(const char *first, const char *last)
                { return cnv_.from_bytes(first, last); }

        string_type from_utf8(const u8string_view &s)
                { return cnv_.to_bytes(s.char_data(),
                                       s.char_data() + s.bytes()); }
        string_type from_utf8(const char *first, const char *last)
                { return cnv_.to_bytes(first, last); }

        size_t converted() const { return cnv_.converted(); }

private:
        wstring_convert<codecvt_utf8_narrow, char, Alloc, Alloc> cnv_;
};


WRUTIL_API u8string_convert<>                   &utf8_narrow_cvt();
WRUTIL_API wstring_convert<codecvt_wide_narrow> &wide_narrow_cvt();

//--------------------------------------

using u8buffer_convert = wbuffer_convert<codecvt_utf8_narrow, char>;


} // namespace wr


#endif // !WRUTIL_CODECVT_H
