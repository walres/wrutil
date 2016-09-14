/**
 * \file lexnorm.cxx
 *
 * \brief Implementation of wr::lexically_normal() filesystem function
 *
 * \note This file is used only when the underlying filesystem library
 *       (boost::filesystem or C++ standard library) does not provide
 *       path::lexically_normal()
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
lexically_normal(
        const path &p
)
{
        path result;
        make_lexically_normal(result, p, p.begin(), p.end());
        return result;
}


} // namespace wr
