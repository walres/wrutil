/**
 * \file mklexnorm.cxx
 *
 * \brief Internal function wr::make_lexically_normal()
 *
 * make_lexically_normal() is used to implement the functions
 * wr::lexically_normal() and wr::weakly_canonical() when either of
 * those are not provided by the underlying filesystem library.
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
 *
 * \see lexnorm.cxx, weakcanon.cxx
 */
#include "private.h"


namespace wr {


path &
make_lexically_normal(
        path                 &dst,
        const path           &src,
        path::const_iterator  src_i,
        path::const_iterator  src_end
)
{
        path::const_iterator src_next;
        path                 root = src.root_path();

        for (; src_i != src_end; src_i = src_next) {
                src_next = std::next(src_i);
                if ((*src_i == DOT) && (src_next != src_end)) {
                        continue;  // omit "/./" component
                } else if ((*src_i == DOTDOT) && !dst.empty()) {
                        if (dst == root) {  // "/.." == "/"
                                continue;
                        } else if (*std::prev(dst.end()) != DOTDOT) {
                                // omit "/../" component and strip previous one
                                dst.remove_filename();
                                continue;
                        }
                        }
                dst /= *src_i;
        }

        auto &s = dst.native();

        if (!s.empty() && is_separator(s.back())) {
                dst /= ".";
        }

        return dst;
}


} // namespace wr
