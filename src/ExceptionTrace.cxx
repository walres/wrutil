/**
 * \file ExceptionTrace.cxx
 *
 * \brief Platform-independent part of exception tracing API implementation
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
#include <stdlib.h>
#include <exception>
#include <iostream>
#include <wrutil/debug.h>


namespace wr {


static void terminate();

//--------------------------------------

thread_local void                 *stack_trace[100];
thread_local const std::type_info *thrown_type      = nullptr;
thread_local size_t                stack_trace_size = 0;

static auto orig_terminate = std::set_terminate(wr::terminate);

//--------------------------------------

static void
terminate()
{
        dumpException(std::cerr, "Unhandled exception");
        orig_terminate();
}

//-------------------------------------

WRDEBUG_API const std::type_info *
lastExceptionThrown() noexcept
{
        return thrown_type;
}

//--------------------------------------

WRDEBUG_API const void * const *
exceptionStackTrace(
        size_t &size_out
) noexcept
{
        size_out = stack_trace_size;
        return stack_trace;
}

//--------------------------------------

extern "C" WRDEBUG_API void
wr_dumpException(std::ostream &dest, const char *prefix)
{
        dumpException(dest, nullptr, 0, prefix);
}


} // namespace wr
