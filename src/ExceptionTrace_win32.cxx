/**
 * \file ExceptionTrace_win32.cxx
 *
 * \brief MS-Windows-specific part of exception tracing API implementation
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
 * References:
 *      http://www.drdobbs.com/visual-c-exception-handling-instrumentat/184416600
 *      http://www.codeproject.com/Articles/175482/Compiler-Internals-How-Try-Catch-Throw-are-Interpr
 *      https://msdn.microsoft.com/en-us/library/ff795730.aspx
 *      http://members.gamedev.net/sicrane/articles/exception.html
 *      http://stackoverflow.com/questions/39113168/c-rtti-in-a-windows-64-bit-vectoredexceptionhandler-ms-visual-studio-2015
 *
 * Many thanks to all authors of the above linked articles.
 */

#ifndef UNICODE
#       define UNICODE 1
#endif
#include <windows.h>
#if WINVER >= _WIN32_WINNT_WIN8
#       include <processthreadsapi.h>
#endif
// #include <imagehlp.h> clashes with dbghelp.h
#include <dbghelp.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <exception>
#include <iostream>
#include <mutex>
#include <string>
#include <wrutil/debug.h>


namespace wr {


static LONG CALLBACK onThrow(PEXCEPTION_POINTERS except_info);


namespace {


static struct Init
{
        Init()
        {
                SymInitialize(GetCurrentProcess(), nullptr, true);
                SymSetOptions(SYMOPT_UNDNAME);
                AddVectoredExceptionHandler(true, &onThrow);
        }

        ~Init()
        {
                RemoveVectoredExceptionHandler(&onThrow);
                SymCleanup(GetCurrentProcess());
        }
} init;


} // anonymous namespace

//--------------------------------------

enum
{
        STACK_TRACE_MAX_DEPTH =
#if WINVER >= 0x0600  // Windows Vista and later
                                100,
#else
                                57,
#endif
        MS_CXX_EH_EXCEPTION_CODE = 0xe06d7363,
        MS_CXX_EH_MAGIC_NUMBER   = 0x19930520
};

extern thread_local void                 *stack_trace[STACK_TRACE_MAX_DEPTH];
extern thread_local const std::type_info *thrown_type;
extern thread_local size_t                stack_trace_size;

static std::mutex                         dbghelp_mutex;

//--------------------------------------

#if WINVER < 0x0600  // Windows XP, Server 2003, or earlier

NTSYSAPI USHORT NTAPI RtlCaptureStackBackTrace(DWORD n_skip_frames,
                                               DWORD max_frames,
                                               PVOID *back_trace,
                                               PULONG *back_trace_hash);

#       define CaptureStackBackTrace RtlCaptureStackBackTrace

#endif

//--------------------------------------

static LONG CALLBACK
onThrow(
        PEXCEPTION_POINTERS except_info
)
{
        thrown_type = nullptr;

        if (except_info->ExceptionRecord
                       ->ExceptionCode != MS_CXX_EH_EXCEPTION_CODE) {
                return EXCEPTION_CONTINUE_SEARCH;
        }
        if (except_info->ExceptionRecord
                       ->ExceptionInformation[0] != MS_CXX_EH_MAGIC_NUMBER) {
                return EXCEPTION_CONTINUE_SEARCH;
        }

        stack_trace_size = CaptureStackBackTrace(6, STACK_TRACE_MAX_DEPTH,
                                                 stack_trace, nullptr);
#if WR_WIN64
        if (except_info->ExceptionRecord->NumberParameters < 4) {
                thrown_type = nullptr;  // module unavailable
                return EXCEPTION_CONTINUE_SEARCH;
        }

#       pragma pack(push, 4)

        /* built-in _ThrowInfo and _CatchableType structures contain pointers
           where there should be 32-bit offsets (from base address of module
           in 64-bit mode) */
        struct ThrowInfo64
        {
                int32_t attributes;
                int32_t pmfnUnwind;
                int32_t pForwardCompat;
                int32_t pCatchableTypeArray;
        };

        struct CatchableTypeArray64
        {
                int     nCatchableTypes;
                int32_t arrayOfCatchableTypes[1];
        };

        struct CatchableType64
        {
                int32_t properties;
                int32_t pType;
                _PMD    thisDisplacement;
                int32_t sizeOrOffset;
                int32_t copyFunction;
        };

#       pragma pack(pop)

        using ThrowInfo = ThrowInfo64;
        using CatchableType = CatchableType64;
#else
        using ThrowInfo = _ThrowInfo;
#endif

        const auto *throw_info = reinterpret_cast<const ThrowInfo *>(
                                        except_info->ExceptionRecord
                                                   ->ExceptionInformation[2]);

#if WR_WIN64
        const char *module_base = reinterpret_cast<const char *>(
                                        except_info->ExceptionRecord
                                                   ->ExceptionInformation[3]);
        if (!module_base) {
                return EXCEPTION_CONTINUE_SEARCH;
        }

        const auto *types = reinterpret_cast<const CatchableTypeArray64 *>(
                                module_base + throw_info->pCatchableTypeArray);

        const auto *type1 = reinterpret_cast<const CatchableType64 *>(
                                module_base + types->arrayOfCatchableTypes[0]);

        thrown_type = reinterpret_cast<const std::type_info *>(
                                                module_base + type1->pType);
#else
        thrown_type = reinterpret_cast<const std::type_info *>(
                                        throw_info->pCatchableTypeArray
                                                  ->arrayOfCatchableTypes[0]
                                                  ->pType);
#endif
        return EXCEPTION_CONTINUE_SEARCH;  // we don't handle the exception
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
        if (!stack_trace_size) {
                return;
        }

        auto this_process_handle = GetCurrentProcess();
        auto this_process_id     = GetCurrentProcessId();
        auto this_thread_handle  = GetCurrentThread();
        auto this_thread_id      = GetCurrentThreadId();

        dest << '[' << this_process_id << ':' << this_thread_id << "] ";

        if (file) {
                dest << file << ':' << line << ": ";
        }

        if (prefix) {
                dest << prefix << ' ';
        }

        union
        {
                SYMBOL_INFO sym_info;
                char        buf[sizeof(SYMBOL_INFO) + 256];
        };

        const char *name;

        if (thrown_type) {
                name = thrown_type->name();
        } else {
                name = "Unknown exception";
        }

        static const DWORD demangle_flags = UNDNAME_NO_ACCESS_SPECIFIERS
                                            | UNDNAME_NO_ALLOCATION_LANGUAGE
                                            | UNDNAME_NO_ALLOCATION_MODEL
                                            | UNDNAME_NO_FUNCTION_RETURNS
                                            | UNDNAME_NO_LEADING_UNDERSCORES
                                            | UNDNAME_NO_MEMBER_TYPE
                                            | UNDNAME_NO_MS_KEYWORDS
                                            | UNDNAME_NO_THROW_SIGNATURES
                                            | UNDNAME_32_BIT_DECODE
                                            | UNDNAME_NAME_ONLY;

        std::lock_guard<std::mutex> lock(dbghelp_mutex);

        if ((name[0] == '?') && UnDecorateSymbolName(name, buf, sizeof(buf),
                                                     demangle_flags)) {
                name = buf;
        }

        if (!strncmp(name, "class ", 6) || !strncmp(name, "union ", 6)) {
                name += 6;
        } else if (!strncmp(name, "struct ", 7)) {
                name += 7;
        } else if (!strncmp(name, "enum ", 5)) {
                name += 5;
        }

        dest << name << " thrown from:" << std::endl;

        sym_info.SizeOfStruct = sizeof(sym_info);
        sym_info.MaxNameLen   = sizeof(buf) - sizeof(sym_info);

        for (size_t i = 0; i < stack_trace_size; ++i) {
                const char        *func_name = "?", *file_name = "?";
                IMAGEHLP_MODULE64  mod_info;

                mod_info.SizeOfStruct = sizeof(mod_info);

                if (SymFromAddr(this_process_handle,
                                reinterpret_cast<DWORD64>(stack_trace[i]),
                                nullptr, &sym_info)) {
                        func_name = sym_info.Name;
                        if (SymGetModuleInfo64(this_process_handle,
                                               sym_info.ModBase, &mod_info)) {
                                file_name = mod_info.ImageName;
                        }
                }

                dest << '[' << this_process_id << ':' << this_thread_id
                     << "]    " << func_name << " @ " << stack_trace[i] << " ("
                     << file_name << ')' << std::endl;

                if (!strcmp(func_name, "main")) {
                        break;
                }
        }
}


} // namespace wr
