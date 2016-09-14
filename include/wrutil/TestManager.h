/**
 * \file TestManager.h
 *
 * \brief Unit testing API
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
#ifndef WRUTIL_TEST_MANAGER_H
#define WRUTIL_TEST_MANAGER_H

#include <fstream>
#include <functional>
#include <iosfwd>
#include <set>
#include <string>
#include <stdexcept>
#include <wrutil/Config.h>
#include <wrutil/filesystem.h>
#include <wrutil/Format.h>
#include <wrutil/string_view.h>


namespace wr {


template <typename T, size_t N> constexpr size_t arraySize(T (&)[N])
        { return N; }

template <typename T> constexpr size_t arraySize(T &&);

template <typename T> constexpr int arraySizeInt(T &&array)
        { return arraySize<T>(std::forward<T>(array)); }
        

//--------------------------------------

struct TestFailure :
        std::runtime_error
{
        TestFailure() : std::runtime_error("") {}
        TestFailure(const string_view &what) : std::runtime_error(what) {}

        template <typename ...Args>
        TestFailure(const char *fmt, Args &&...args) : std::runtime_error(
                printStr(fmt, std::forward<Args>(args)...)) {}
};

//--------------------------------------

class TestManager
{
public:
        WRUTIL_API TestManager(const string_view &group, int argc,
                               const char **argv);

        TestManager(const string_view &group, int argc, char **argv) :
                TestManager(group, argc, (const char **) argv) {}

        WRUTIL_API ~TestManager();

        template <typename Fn, typename ...Args> void
        run(
                const string_view &sub_group,
                unsigned           test_number,
                Fn               &&test_code,
                Args          &&...args
        )
        {
#if 0
                /* clang 3.5 infinite-loops consuming all RAM when compiling
                   the following code: */
                run(sub_group, test_number,
                    std::bind(std::forward<Fn>(test_code),
                              std::forward<Args>(args)...));
#endif
                if (run_selected_ && !to_run_.count({sub_group, test_number})) {
                        return;
                }
                std::function<void()> f(std::bind(std::forward<Fn>(test_code),
                                        std::forward<Args>(args)...));
                run_(sub_group, test_number, f);
        }

        size_t count() const  { return count_ + to_run_.size(); }
        size_t passed() const { return passed_; }
        size_t failed() const { return count() - passed_; }

private:
        void setUpChildProcessHandling();  // platform-specific
        void openLog();

        WRUTIL_API void run_(const string_view &sub_group, unsigned test_number,
                             const std::function<void()> &test_code);

        void runChildProcess(const string_view &sub_group, unsigned test_number,
                             const std::function<void()> &test_code);
                                                        // platform-specific

        int do_run(const string_view &sub_group, unsigned test_number,
                   const std::function<void()> &test_code);

        void output(const string_view &what);

        using TestSet = std::set<std::pair<string_view, unsigned>>;
        using DumpExceptionFn = void (*)(std::ostream &, const char *);

        std::string     group_;
        path            exec_path_,
                        log_name_;
        size_t          count_  = 0,
                        passed_ = 0;
        bool            run_selected_ = false,
                        run_directly_ = false;
        TestSet         to_run_,
                        have_run_;
        std::ofstream   log_;
        unsigned        timeout_ms_ = 5000;
        DumpExceptionFn dump_exception_ = nullptr;
                              // avoid hard-wired dependency on wrdebug library
};


} // namespace wr


#endif // !WRUTIL_TEST_MANAGER_H
