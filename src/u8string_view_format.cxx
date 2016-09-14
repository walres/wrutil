/**
 * \file u8string_view_format.cxx
 *
 * \brief format() argument support for wr::u8string_view objects
 *
 * \note This file exists so that users of the static wrutil library
 *       do not require to link the format() object code if they use
 *       wr::u8string_view without the format() APIs
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
#include <wrutil/Format.h>
#include <wrutil/u8string_view.h>


namespace wr {


WRUTIL_API void
fmt::TypeHandler<u8string_view>::set(
        Arg                 &arg,
        const u8string_view &val
)
{
        arg.type = Arg::STR_T;
        arg.s = { val.char_data(), val.bytes() };
}


} // namespace wr
