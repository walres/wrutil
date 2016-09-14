/**
 * \file lexrel.cxx
 *
 * \brief Implementation of lexically_relative() function of filesystem
 *        module
 *
 * \note This file is used only when the underlying filesystem library
 *       (boost::filesystem or C++ standard library) does not provide
 *       path::lexically_relative()
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


WRUTIL_API path
lexically_relative(
        const path &p,
        const path &base
)
{
        path::const_iterator i_p      = p.begin(),
                             i_base   = base.begin(),
                             end_p    = p.end(),
                             end_base = base.end();
        path                 result;

        while ((i_p != end_p) && (i_base != end_base) && (*i_p == *i_base)) {
                ++i_p;
                ++i_base;
        }

        if ((i_p == end_p) && (i_base == end_base)) {
                result = DOT;
        } else if ((i_p != p.begin()) && (i_base != base.begin())) {
                for (; i_base != end_base; ++i_base) {
                        result /= DOTDOT;
                }
                for (; i_p != end_p; ++i_p) {
                        result /= *i_p;
                }
        }

        return result;
}


} // namespace wr
