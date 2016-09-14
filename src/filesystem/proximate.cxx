/**
 * \file proximate.cxx
 *
 * \brief Implementation of wr::proximate() filesystem functions
 *
 * \note This file is used only when the underlying filesystem library
 *       (boost::filesystem or C++ standard library) does not provide
 *       its own version of proximate()
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
proximate(
        const path    &p,
        fs_error_code &ec
)
{
        return proximate(p, current_path(), ec);
}

//--------------------------------------

WRUTIL_API path
proximate(
        const path &p,
        const path &base
)
{
        fs_error_code ec;
        path result = proximate(p, base, ec);
        if (ec) {
                throw filesystem_error("error obtaining proximate path",
                                       p, base, ec);
        }
        return result;
}

//--------------------------------------

WRUTIL_API path
proximate(
        const path    &p,
        const path    &base,
        fs_error_code &ec
)
{
        path p_canon = weakly_canonical(p, ec),
             result;

        if (!ec) {
                path base_canon = weakly_canonical(base, ec);

                if (!ec) {
                        result = lexically_proximate(p_canon, base_canon);
                }
        }

        return result;
}


} // namespace wr
