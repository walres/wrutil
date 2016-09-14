/**
 * \file weakcanon.cxx
 *
 * \brief Implementation of the wr::weakly_canonical() filesystem functions
 *
 * \note This file is used only when the underlying filesystem library
 *       (boost::filesystem or C++ standard library) does not provide
 *       its own version of weakly_canonical()
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
weakly_canonical(
        const path &p
)
{
        fs_error_code ec;
        path result = weakly_canonical(p, ec);
        if (ec) {
                throw filesystem_error("error obtaining canonical path", p, ec);
        }
        return result;
}

//--------------------------------------

WRUTIL_API path
weakly_canonical(
        const path    &p,
        fs_error_code &ec
)
{
        bool           error = false;
        path           result;
        path::iterator i, j;

        for (i = p.begin(), j = p.end(); i != j; ++i) {
                path tmp = result / *i;
                if (!exists(tmp, ec)) {
                        break;
                } else {
                        error = !error && static_cast<bool>(ec);
                        result.swap(tmp);
                }
        }

        if (!error) {
                result = canonical(lexically_normal(result), ec);
                          /* boost::filesystem::canonical("/..") yields
                             an empty path! Result should be "/". */
                error = !error && static_cast<bool>(ec);
                if (!error) {
                        make_lexically_normal(result, p, i, j);
                }
        }

        if (error) {
                result.clear();
        } else {
                ec.clear();
        }

        return result;
}


} // namespace wr
