/**
 * \file StdioFilePtr_posix.cxx
 *
 * \brief POSIX-specific implementation of wr::fopen(), wr::popen()
 *        and StdioPipeCloser
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
#include <wrutil/StdioFilePtr.h>


namespace wr {


WRUTIL_API UniqueFilePtr
fopen(
        const wr::path &file_name,
        const char     *mode
)
{
        return UniqueFilePtr(::fopen(file_name.c_str(), mode));
}

//--------------------------------------

WRUTIL_API UniquePipePtr
popen(
        const char *command,
        const char *mode
)
{
        return UniquePipePtr(::popen(command, mode));
}

//--------------------------------------

WRUTIL_API void
StdioPipeCloser::operator()(
        FILE *pipe
) const
{
        ::pclose(pipe);
}


} // namespace wr
