/**
 * \file TestManager_win32.cxx
 *
 * \brief MS-Windows-specific part of unit testing API implementation
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
#       define UNICODE 1
#endif
#include <windows.h>
#include <dbghelp.h>
#include <wrutil/TestManager.h>
#include <wrutil/codecvt.h>


namespace wr {


void
TestManager::setUpChildProcessHandling()
{
        dump_exception_ = reinterpret_cast<DumpExceptionFn>(
                                GetProcAddress(GetModuleHandleW(L"wrdebug.dll"),
                                               "wr_dumpException"));

        // disable message boxes on program crash
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX
                                            | SEM_NOOPENFILEERRORBOX);
}

//--------------------------------------

void
TestManager::runChildProcess(
        const string_view           &sub_group,
        unsigned                     test_number,
        const std::function<void()> &test_code
)
{
        auto exec_path_str = exec_path_.wstring();

        if (!wstring_view(exec_path_str).has_suffix(L".exe")) {
                exec_path_str += L".exe";
        }

        auto cmd_line = exec_path_str;

        if (!log_name_.empty()) {
                cmd_line += L" -l \"" + log_name_.wstring() + L'"';
        }

        cmd_line += L" -d -r \""
                    + wide_narrow_cvt().from_bytes(sub_group.to_string())
                    + L'.' + std::to_wstring(test_number) + L"\"\0";

        std::wstring desktop_name;
        DWORD        req_length;
        HDESK        desktop_handle = GetThreadDesktop(GetCurrentThreadId());

        GetUserObjectInformationW(desktop_handle, UOI_NAME, nullptr,
                                  0, &req_length);
        desktop_name.resize(req_length);
        GetUserObjectInformationW(desktop_handle, UOI_NAME, &desktop_name[0],
                                  req_length, nullptr);

        STARTUPINFOW test_process_settings = {
                sizeof(STARTUPINFOW),
                nullptr,  // reserved
                &desktop_name[0],
                nullptr,  // use default title
                0, 0,     // default window position; not used
                0, 0,     // default window size; not used
                0, 0, 0,  // console window size & colours; not used
                0,        // flags
                0,        /* nCmdShow override for child's first call to
                             ShowWindow(); not used */
                0, nullptr,  // reserved
                0,
                0,
                0
        };

        PROCESS_INFORMATION test_process;

        if (!CreateProcessW(exec_path_str.c_str(), &cmd_line[0], nullptr,
                            nullptr, false, 0, nullptr, nullptr,
                            &test_process_settings, &test_process)) try {
                /* using GetLastError() directly in the system_error
                   constructor doesn't work (always yields zero) */
                auto error_code = GetLastError();
                throw std::system_error(error_code, std::system_category(),
                        printStr("failed to create process for test %s.%s.%u",
                                 group_, sub_group, test_number));
        } catch (const std::exception &e) {
                std::cerr << "error: " << e.what() << '\n';
                throw;
        }

        struct CloseProcessHandles
        {
                CloseProcessHandles(PROCESS_INFORMATION &process_info) :
                        process_info_(process_info) {}

                ~CloseProcessHandles()
                {
                        CloseHandle(process_info_.hProcess);
                        CloseHandle(process_info_.hThread);
                }

                PROCESS_INFORMATION &process_info_;
        }
        close_process_handles(test_process);

        DWORD       child_status;
        const char *exception_descr = nullptr;

        switch (WaitForSingleObject(test_process.hProcess, timeout_ms_)) {
        case WAIT_OBJECT_0:
                if (!GetExitCodeProcess(test_process.hProcess, &child_status)) {
                        throw std::system_error(GetLastError(),
                                                std::system_category(),
                                                "failed to obtain test process' exit status");
                }
                switch (child_status) {
                case EXIT_SUCCESS:
                        ++passed_;
                        break;
                case STILL_ACTIVE:
                        output(printStr("warning: child process %#x running test %s.%s.%u still active\n",
                                        test_process.hProcess, group_,
                                        sub_group, test_number));
                        break;
                case STATUS_ACCESS_VIOLATION:
                        exception_descr = "Access violation";
                        break;
                case STATUS_ARRAY_BOUNDS_EXCEEDED:
                        exception_descr = "Array bounds exceeded";
                        break;
                case STATUS_BREAKPOINT:
                        exception_descr = "Breakpoint encountered";
                        break;
                case STATUS_DATATYPE_MISALIGNMENT:
                        exception_descr = "Data type misalignment";
                        break;
                case STATUS_FLOAT_DENORMAL_OPERAND:
                        exception_descr = "Denormal floating-point operand";
                        break;
                case STATUS_FLOAT_DIVIDE_BY_ZERO:
                        exception_descr = "Floating-point division by zero";
                        break;
                case STATUS_FLOAT_INEXACT_RESULT:
                        exception_descr = "Inexact floating-point result";
                        break;
                case STATUS_FLOAT_INVALID_OPERATION:
                        exception_descr = "Invalid floating-point operation";
                        break;
                case STATUS_FLOAT_OVERFLOW:
                        exception_descr = "Floating-point overflow";
                        break;
                case STATUS_FLOAT_STACK_CHECK:
                        exception_descr = "Floating-point stack overflow/underflow";
                        break;
                case STATUS_FLOAT_UNDERFLOW:
                        exception_descr = "Floating-point underflow";
                        break;
                case STATUS_GUARD_PAGE_VIOLATION:
                        exception_descr = "Guard page violation";
                        break;
                case STATUS_ILLEGAL_INSTRUCTION:
                        exception_descr = "Illegal instruction";
                        break;
                case STATUS_IN_PAGE_ERROR:
                        exception_descr = "Memory page no longer present";
                        break;
                case STATUS_INTEGER_DIVIDE_BY_ZERO:
                        exception_descr = "Integer division by zero";
                        break;
                case STATUS_INTEGER_OVERFLOW:
                        exception_descr = "Integer overflow";
                        break;
                case STATUS_INVALID_DISPOSITION:
                        exception_descr = "Invalid exception disposition";
                        break;
                case STATUS_INVALID_HANDLE:
                        exception_descr = "Invalid handle";
                        break;
                case STATUS_NONCONTINUABLE_EXCEPTION:
                        exception_descr = "Non-continuable exception";
                        break;
                case STATUS_PRIVILEGED_INSTRUCTION:
                        exception_descr = "Privileged instruction";
                        break;
                case STATUS_SINGLE_STEP:
                        exception_descr = "Debug trap";
                        break;
                case STATUS_STACK_OVERFLOW:
                        exception_descr = "Stack overflow";
                        break;
                case STATUS_UNWIND_CONSOLIDATE:
                        exception_descr = "Frame consolidation";
                        break;
                }

                if (exception_descr) {
                        output(printStr("FAIL (%s)\n", exception_descr));
                }
                break;
        case WAIT_TIMEOUT:
                output("FAIL (timed out)\n");
                TerminateProcess(test_process.hProcess, EXIT_FAILURE);
                break;
        case WAIT_FAILED: default:
                throw std::system_error(GetLastError(), std::system_category(),
                                        "WaitForSingleObject() failed");
        }
}


} // namespace wr
