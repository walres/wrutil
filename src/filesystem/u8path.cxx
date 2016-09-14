/**
 * \file u8path.cxx
 *
 * \brief Implementation of wr::u8path() and wr::to_*u8string()
 *        filesystem functions
 *
 * \note This file is used only when the underlying filesystem library
 *       (boost::filesystem or C++ standard library) does not provide
 *       the functions path::u8string() or u8string()
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
#include <wrutil/codecvt.h>
#include "private.h"


namespace wr {


namespace {


template <typename> struct u8convert;


template <>
struct u8convert<char>
{
        std::string to_utf8(const string_view &in)
                { return utf8_narrow_cvt().to_utf8(in); }

        std::string from_utf8(const u8string_view &in)
                { return utf8_narrow_cvt().from_utf8(in); }
};

//--------------------------------------

template <>
struct u8convert<wchar_t>
{
        std::string to_utf8(const wstring_view &in)
                { return cvt_.to_bytes(in.data(), in.data() + in.size()); }

        std::wstring from_utf8(const u8string_view &in)
                { return cvt_.from_bytes(in.char_data(),
                                         in.char_data() + in.bytes()); }

        wstring_convert<codecvt_utf8<wchar_t>> cvt_;
};

//--------------------------------------

static u8convert<path::value_type> &
utf8_converter()
{
        static thread_local u8convert<path::value_type> converter;
        return converter;
}


} // anonymous namespace

//--------------------------------------

#if !WR_HAVE_FS_PATH_TO_U8STRING

WRUTIL_API std::string
to_u8string(
        const path &p
)
{
        return utf8_converter().to_utf8(p.native());
}

//--------------------------------------

WRUTIL_API std::string
to_generic_u8string(
        const path &p
)
{
#if WR_HAVE_STD_FILESYSTEM
        auto s = p.generic_string<path::value_type>();
#else
        auto s = p.generic_string<path::string_type>();
#endif
        return utf8_converter().to_utf8(s);
}

#endif // !WR_HAVE_FS_PATH_TO_U8STRING

//--------------------------------------

#if !WR_HAVE_FSIMPL_U8PATH

template <> WRUTIL_API path
u8path(const wr::u8string_view &s)
{
        return path(utf8_converter().from_utf8(s));
}

//--------------------------------------

template <> WRUTIL_API path
u8path<char *>(char * const &s)
{
        return u8path(u8string_view(s));
}

//--------------------------------------

template <> WRUTIL_API path
u8path<const char *>(const char * const &s)
{
        return u8path(u8string_view(s));
}

//--------------------------------------

template <> WRUTIL_API path
u8path(const std::string &s)
{
        return u8path(u8string_view(s));
}

//--------------------------------------

template <> WRUTIL_API path
u8path(const wr::string_view &s)
{
        return u8path(u8string_view(s));
}

//--------------------------------------

#if WR_HAVE_STD_STRING_VIEW

template <> WRUTIL_API path
u8path(const std_string_view &s)
{
        return u8path(u8string_view(s));
}

#endif // WR_HAVE_STD_STRING_VIEW

//--------------------------------------

template <> WRUTIL_API path
u8path(
        const char *first,
        const char *last
)
{
        return u8path(u8string_view(first, last - first));
}

//--------------------------------------

template <> WRUTIL_API path
u8path(
        char *first,
        char *last
)
{
        return u8path(u8string_view(first, last - first));
}

//--------------------------------------

template <> WRUTIL_API path
u8path(
        u8string_view::const_iterator first,
        u8string_view::const_iterator last
)
{
        return u8path(u8string_view(first, last));
}

//--------------------------------------

template <> WRUTIL_API path
u8path(
        std::string::const_iterator first,
        std::string::const_iterator last
)
{
        return u8path(u8string_view(&(*first), last - first));
}

//--------------------------------------

template <> WRUTIL_API path
u8path(
        std::string::iterator first,
        std::string::iterator last
)
{
        return u8path(u8string_view(&(*first), last - first));
}

#endif // !WR_HAVE_FSIMPL_U8PATH


} // namespace wr
