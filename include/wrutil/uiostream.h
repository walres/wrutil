/**
 * \file uiostream.h
 *
 * \brief std::iostream objects for input/output of Unicode text
 *
 * \copyright
 * \parblock
 *
 *   Copyright 2016 James S. Waller
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
#ifndef WRUTIL_UIOSTREAM_H
#define WRUTIL_UIOSTREAM_H

#include <iostream>
#include <wrutil/Config.h>


namespace wr {


extern WRUTIL_API std::istream &uin;
extern WRUTIL_API std::ostream &uout, &uerr, &ulog;

//--------------------------------------

static class WRUTIL_API uiostream_init
{
public:
        uiostream_init();
        ~uiostream_init();
} use_uio_;


} // namespace wr

#endif // !WRUTIL_UIOSTREAM_H
