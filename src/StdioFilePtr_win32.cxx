/**
 * \file StdioFilePtr_win32.cxx
 *
 * \brief MS-Windows-specific implementation of wr::fopen(), wr::popen()
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
#ifndef UNICODE
#       define UNICODE
#endif
#include <errno.h>
#include <wchar.h>
#include <locale>
#include <string>
#include <wrutil/StdioFilePtr.h>


namespace wr {


WRUTIL_API UniqueFilePtr
fopen(
        const wr::path &path,
        const char     *mode
)
{
        std::wstring wmode;
        bool         restart_conv = false;

        for (mbstate_t mbstate = {0}; *mode; ++mode) {
                if (!restart_conv) {
                        wmode += L'\0';
                }
                switch (mbrtowc(&wmode.back(), mode, 1, &mbstate)) {
                case static_cast<size_t>(-2):
                        restart_conv = true;
                case static_cast<size_t>(-1):
                        errno = EINVAL;
                        return {};
                default:
                        restart_conv = false;
                        break;
                }
        }

        return UniqueFilePtr(::_wfopen(path.c_str(), wmode.c_str()));
}

//--------------------------------------

WRUTIL_API UniquePipePtr
popen(
        const char *command,
        const char *mode
)
{
        return UniquePipePtr(::_popen(command, mode));
}

//--------------------------------------

WRUTIL_API void
StdioPipeCloser::operator()(
        FILE *pipe
) const
{
        ::_pclose(pipe);
}


} // namespace wr
