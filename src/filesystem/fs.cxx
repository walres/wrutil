/**
 * \file fs.cxx
 *
 * \brief Extra filesystem module functions not provided by underlying
 *        boost::filesystem or std library
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
#       include <unistd.h>
#endif
#include <errno.h>

#include <wrutil/string_view.h>

#include "private.h"


namespace wr {


#if WR_HAVE_STD_FILESYSTEM
using std::system_category;
#else
using boost::system_category;
#endif


const path DOT("."), DOTDOT("..");

//--------------------------------------

WRUTIL_API bool
path_has_prefix(
        const path &p,
        const path &prefix
)
{
        path::const_iterator i_p        = p.begin(),
                             end_p      = p.end(),
                             i_prefix   = prefix.begin(),
                             end_prefix = prefix.end();

        for (; (i_p != end_p) && (i_prefix != end_prefix); ++i_p, ++i_prefix) {
                if (*i_p != *i_prefix) {
                        return false;
                }
        }

        return (i_prefix == end_prefix);
}

//--------------------------------------

WRUTIL_API bool
is_separator(
        path::value_type c
)
{
        return (c == path::preferred_separator) || (c == '/');
}

//--------------------------------------
#if WR_WINAPI

static std::vector<string_view>
getExecSuffixes(std::string &path_ext_var)
{
        const char *val = getenv("PATHEXT");

        if (val) {
                path_ext_var = val;
        } else {
                path_ext_var = ".EXE;.COM;.BAT;.CMD;.VBS;.VBE;.JS;.JSE;.WSF;"
                               ".WSH;.PSC1;.SCR";
        }

        std::vector<string_view> results;
        std::pair<string_view>   split({}, path_ext_var);
        split.second.trim();

        while (true) {
                split = split.second.split(';');
                split.first = split.first.trim_right();
                split.second = split.second.trim_left();

                if (split.first.empty()) {
                        if (split.second.empty()) {
                                break;
                        } else {
                                continue;
                        }
                }

                results.push_back(split.first);
        }

        return results;
}

#endif // WR_WINAPI
//--------------------------------------

WRUTIL_API bool
is_executable(
        const fs_impl::path &p,
        fs_error_code &ec
) noexcept
{
        bool result = false;

        if (is_regular_file(p, ec)) {
#if WR_POSIX
                if (::access(p.c_str(), X_OK) == 0) {
                        result = true;
                } else if (errno != EACCES) {
                        ec.assign(errno, system_category());
                }
#elif WR_WINAPI
                static const std::string pathext;
                static const auto        exe_suffixes
                                                = getExecSuffixes(pathext);
                auto                     ext = p.extension();

                for (const auto &suffix: exe_suffixes) {
                        if (ext.native() == suffix) {
                                result = true;
                                break;
                        }
                }
#else
#       error Unsupported platform
#endif
        }

        return result;
}

//--------------------------------------

WRUTIL_API bool
is_executable(
        const fs_impl::path &p
)
{
        fs_error_code ec;
        bool result = is_executable(p, ec);
        if (ec) {
                throw filesystem_error("problem checking file executability",
                                       p, ec);
        }
        return result;
}


} // namespace wr
