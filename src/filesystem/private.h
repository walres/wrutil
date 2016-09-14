/**
 * \file private.h
 *
 * \brief Shared internal function declarations for filesystem module
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
#ifndef WRUTIL_FS_PRIVATE_H
#define WRUTIL_FS_PRIVATE_H

#include <wrutil/filesystem.h>

#if WR_HAVE_STD_FILESYSTEM
#       include <system_error>
#else
#       include <boost/system/error_code.hpp>
#endif


namespace wr {


#if !WR_HAVE_FS_PATH_LEXICALLY_NORMAL || !WR_HAVE_FSIMPL_WEAKLY_CANONICAL

path &make_lexically_normal(path &dst, const path &src,
                            path::const_iterator src_i,
                            path::const_iterator src_end);

#endif


} // namespace wr


#endif // !WRUTIL_FS_PRIVATE_H
