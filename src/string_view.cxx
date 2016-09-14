/**
 * \file string_view.cxx
 *
 * \brief Implementation of wr::basic_string_view class template and
 *        associated operator functions
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
#include <ctype.h>
#include <wctype.h>
#include <wrutil/ctype.h>
#include <wrutil/string_view.h>


namespace wr {


template <> WRUTIL_API char
to_lower(
        char c
)
{
        return ::tolower(c);
}

//--------------------------------------

template <> WRUTIL_API char
to_upper(
        char c
)
{
        return ::toupper(c);
}

//--------------------------------------

template <> WRUTIL_API wchar_t
to_lower(
        wchar_t c
)
{
        return ::towlower(c);
}

//--------------------------------------

template <> WRUTIL_API wchar_t
to_upper(
        wchar_t c
)
{
        return ::towupper(c);
}

//--------------------------------------

template <> WRUTIL_API char32_t
to_lower(
        char32_t c
)
{
        return toulower(c);
}

//--------------------------------------

template <> WRUTIL_API char32_t
to_upper(
        char32_t c
)
{
        return touupper(c);
}

//--------------------------------------

template <> WRUTIL_API char16_t
to_lower(
        char16_t c
)
{
        return static_cast<char16_t>(to_lower(static_cast<char32_t>(c)));
}

//--------------------------------------

template <> WRUTIL_API char16_t
to_upper(
        char16_t c
)
{
        return static_cast<char16_t>(to_upper(static_cast<char32_t>(c)));
}


} // namespace wr
