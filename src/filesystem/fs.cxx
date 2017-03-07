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
#include "private.h"


namespace wr {


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


} // namespace wr
