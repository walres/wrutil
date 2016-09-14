/**
 * \file StdioFilePtr.h
 *
 * \brief Wrapper API for opening/closing C stdio FILE objects
 *
 * Convenience functions and types for opening and automatic closure
 * of C stdio FILE objects representing files and process pipes.
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
#ifndef WRUTIL_STDIO_FILE_PTR_H
#define WRUTIL_STDIO_FILE_PTR_H

#include <stdio.h>
#include <memory>
#include <wrutil/Config.h>
#include <wrutil/filesystem.h>


namespace wr {

struct StdioFileCloser
{
        void operator()(FILE *file) const { ::fclose(file); }
};

struct StdioPipeCloser
{
        WRUTIL_API void operator()(FILE *pipe) const;
};


using UniqueFilePtr = std::unique_ptr<FILE, StdioFileCloser>;
using UniquePipePtr = std::unique_ptr<FILE, StdioPipeCloser>;

//--------------------------------------

WRUTIL_API UniqueFilePtr fopen(const wr::path &file_name, const char *mode);
WRUTIL_API UniquePipePtr popen(const char *command, const char *mode);


} // namespace wr


#endif // !WRUTIL_STDIO_FILE_PTR_H
