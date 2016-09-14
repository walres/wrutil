/**
 * \file debug.h
 *
 * \brief wrdebug library API
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

#ifndef WRDEBUG_H
#define WRDEBUG_H

#include <iosfwd>
#include <typeinfo>
#include <wrutil/Config.h>

#if WR_WINDOWS
#       ifdef wrdebug_EXPORTS
#               define WRDEBUG_API __declspec(dllexport)
#       elif defined(wrdebug_IMPORTS)
#               define WRDEBUG_API __declspec(dllimport)
#       else
#               define WRDEBUG_API
#       endif
#elif WR_HAVE_ELF_VISIBILITY_ATTR
#       ifdef wrdebug_EXPORTS
#               define WRDEBUG_API __attribute__((visibility("default")))
#       else
#               define WRDEBUG_API
#       endif
#else
#       define WRDEBUG_API
#endif


// synonyms for wr::dumpException() functions below
extern "C" {

WRDEBUG_API void wr_dumpException(std::ostream &dest,
                                  const char *prefix = nullptr) noexcept;

WRDEBUG_API void wr_dumpExceptionFileLine(std::ostream &dest, const char *file,
                                         unsigned int line,
                                         const char *prefix = nullptr) noexcept;

} // extern "C"


namespace wr {


WRDEBUG_API const std::type_info *lastExceptionThrown() noexcept;
WRDEBUG_API const void * const *exceptionStackTrace(size_t &size_out) noexcept;

inline void dumpException(std::ostream &dest,
                          const char *prefix = nullptr) noexcept
        { return wr_dumpException(dest, prefix); }

inline void dumpException(std::ostream &dest, const char *file,
                          unsigned int line,
                          const char *prefix = nullptr) noexcept
        { return wr_dumpExceptionFileLine(dest, file, line, prefix); }


#ifndef wrutil_EXPORTS
static auto libwrdebug_ref = &wr_dumpException;
#endif

} // namespace wr


#endif // !WRDEBUG_H
