/**
 * \file ExceptionTrace_posix.cxx
 *
 * \brief POSIX-specific part of exception tracing API implementation
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
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <cxxabi.h>
#include <unwind.h>
#include <exception>
#include <iostream>
#include <thread>

#ifndef _GNU_SOURCE
#       define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <wrutil/debug.h>


namespace wr {


static _Unwind_Reason_Code buildStackTrace(struct _Unwind_Context *context,
                                           void *arg);

//--------------------------------------

enum { STACK_TRACE_MAX_DEPTH = 100 };

extern thread_local void                 *stack_trace[STACK_TRACE_MAX_DEPTH];
extern thread_local const std::type_info *thrown_type;
extern thread_local size_t                stack_trace_size;

static void (*actual_cxa_throw)(void *, std::type_info *, void (*)(void *))
        = reinterpret_cast<decltype(actual_cxa_throw)>(
                dlsym(RTLD_NEXT, "__cxa_throw"));

//--------------------------------------

extern "C" WRDEBUG_API void
__cxa_throw(
        void           *exception,
        std::type_info *exception_type,
        void           (*exception_dtor)(void *)
)
{
        stack_trace_size = 0;
        thrown_type = exception_type;
        size_t n = 0;
        _Unwind_Backtrace(&buildStackTrace, &n);
        (*actual_cxa_throw)(exception, exception_type, exception_dtor);
        abort();  // not reached
}

//--------------------------------------

static _Unwind_Reason_Code
buildStackTrace(
        struct _Unwind_Context *context,
        void                   *arg
)
{
        auto n = static_cast<size_t *>(arg);

        if ((*n) && (stack_trace_size < STACK_TRACE_MAX_DEPTH)) {
                stack_trace[stack_trace_size++]
                        = reinterpret_cast<void *>(_Unwind_GetIP(context));
        }

        ++(*n);
        return _URC_NO_REASON;
}

//--------------------------------------

extern "C" WRDEBUG_API void
wr_dumpExceptionFileLine(
        std::ostream &dest,
        const char   *file,
        unsigned int  line,
        const char   *prefix
)
{
        if (!stack_trace_size || !thrown_type) {
                return;
        }

        static const std::thread::id NULL_THREAD_ID;
        auto this_thread_id = std::this_thread::get_id();

        dest << '[' << getpid();

        if (this_thread_id != NULL_THREAD_ID) {
                dest << ':' << this_thread_id;
        }

        dest << "] ";

        if (file) {
                dest << file << ':' << line << ": ";
        }

        if (prefix) {
                dest << prefix << ' ';
        }

        char       *buf  = nullptr;
        const char *name = abi::__cxa_demangle(thrown_type->name(),
                                               buf, nullptr, nullptr);
        if (!name) {
                name = thrown_type->name();
        }

        dest << name << " thrown from:" << std::endl;

        for (size_t i = 0; i < stack_trace_size; ++i) {
                Dl_info info;

                if (!dladdr(stack_trace[i], &info)) {
                        break;
                }

                const char *func_name;

                if (info.dli_sname) {
                        func_name = abi::__cxa_demangle(
                                info.dli_sname, buf, nullptr, nullptr);

                        if (!func_name) {
                                func_name = info.dli_sname;
                        }
                } else {
                        func_name = "?";
                }

                const char *file_name;

                if (info.dli_fname) {
                        file_name = strrchr(info.dli_fname, '/');
                        if (file_name) {
                                ++file_name;
                        } else {
                                file_name = info.dli_fname;
                        }
                } else {
                        file_name = "?";
                }

                dest << '[' << getpid();

                if (this_thread_id != NULL_THREAD_ID) {
                        dest << ':' << this_thread_id;
                }

                dest << "]    " << func_name << " @ " << stack_trace[i] << " ("
                     << file_name << ')' << std::endl;

                if (!strcmp(func_name, "main")) {
                        break;
                }
        }

        free(buf);
}


} // namespace wr
