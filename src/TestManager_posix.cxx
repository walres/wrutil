/**
 * \file TestManager_posix.cxx
 *
 * \brief POSIX-specific part of unit testing API implementation
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
#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <wrutil/TestManager.h>


namespace wr {


void
TestManager::setUpChildProcessHandling()
{
        dump_exception_ = reinterpret_cast<DumpExceptionFn>(
                                      dlsym(RTLD_DEFAULT, "wr_dumpException"));
        sigset_t signals;
        sigemptyset(&signals);
        sigaddset(&signals, SIGCHLD);
        if (sigprocmask(SIG_BLOCK, &signals, nullptr) != 0) {
                throw std::system_error(errno, std::system_category(),
                                        "sigprocmask() failed");
        }
}

//--------------------------------------

void
TestManager::runChildProcess(
        const string_view           &sub_group,
        unsigned                     test_number,
        const std::function<void()> &test_code
)
{
        if (log_.is_open()) {
                log_.flush();
        }

        pid_t          child_pid = fork();
        int            child_status;
        sigset_t       signals;
        struct timeval start_time, end_time;
        bool           timed_out = false;

        switch (child_pid) {
        default:  // parent process
                sigemptyset(&signals);
                sigaddset(&signals, SIGCHLD);
                gettimeofday(&start_time, nullptr);
                end_time = start_time;
                end_time.tv_sec += timeout_ms_ / 1000;
                end_time.tv_usec += (timeout_ms_ % 1000) * 1000;
                if (end_time.tv_usec > 1000000) {
                        ++end_time.tv_sec;
                        end_time.tv_usec -= 1000000;
                }

                while (true) {
                        struct timespec timeout;

                        if (timed_out) {
                                timeout.tv_sec = timeout.tv_nsec = 0;
                        } else {
                                timeout.tv_sec = end_time.tv_sec
                                                        - start_time.tv_sec;
                                timeout.tv_nsec =
                                        (end_time.tv_usec - start_time.tv_usec)
                                        * 1000;

                                if (timeout.tv_nsec < 0) {
                                        --timeout.tv_sec;
                                        timeout.tv_nsec += 1000000000;
                                }
                        }

                        int s = sigtimedwait(&signals, nullptr, &timeout);

                        if (timed_out || (s == SIGCHLD)) {
                                int waitpid_options = 0;

                                if (!timed_out) {  // don't block
                                        waitpid_options |= WNOHANG;
                                }

                                pid_t waited = waitpid(child_pid,
                                                &child_status, waitpid_options);

                                if (waited == child_pid) {
                                        if (WIFEXITED(child_status) &&
                                            WEXITSTATUS(child_status) == 0) {
                                                ++passed_;
                                        }
                                        break;
                                } else if ((waited == -1) && (errno != EINTR)) {
                                        throw std::system_error(errno,
                                                std::system_category(),
                                                "waitpid() failed");
                                }
                        } else if (s == -1) switch (errno) {
                        case EAGAIN:
                                timed_out = true;
                                kill(child_pid, SIGKILL);
                                break;
                        case EINTR:
                                break;  // carry on
                        default:
                                throw std::system_error(errno,
                                                std::system_category(),
                                                "sigtimedwait() failed");
                        }

                        if (!timed_out) {
                                gettimeofday(&start_time, nullptr);
                        }
                }

                if (timed_out) {
                        output("FAIL (timed out)\n");
                } else if (WIFSIGNALED(child_status)) {
                        output(printStr("FAIL (%s)\n",
                                        strsignal(WTERMSIG(child_status))));
                }
                break;
        case 0:   // child process
                if (log_.is_open()) {
                        // log file won't automatically be flushed on exit
                        static struct LogFileFlusher
                        {
                                std::ostream &log_;
                                LogFileFlusher(std::ostream &log) : log_(log) {}
                                ~LogFileFlusher() { log_.flush(); }
                        }
                        flush_log_on_exit(log_);
                }
                exit(do_run(sub_group, test_number, test_code));
                break;  // not reached
        case -1:  // error
                throw std::system_error(errno, std::system_category(),
                                        "fork() failed");
        }
}


} // namespace wr
