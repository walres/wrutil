/**
 * \file TestManager.cxx
 *
 * \brief Platform-independent part of unit testing API implementation
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
#include <wrutil/Config.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <system_error>
#include <wrutil/Option.h>
#include <wrutil/TestManager.h>


namespace wr {


WRUTIL_API TestManager::TestManager() = default;

//--------------------------------------

WRUTIL_API
TestManager::TestManager(
        const string_view &group,
        int                argc,
        const char       **argv
) :
        TestManager()
{
        init(group, argc, argv);
}

//--------------------------------------

WRUTIL_API void
TestManager::init(
        const string_view  &group,
        int                 argc,
        const char        **argv
)
{
        group_ = group;
        exec_path_ = argv[0];

        static const Option OPTIONS[] = {
                { "-A", Option::NON_EMPTY_ARG_REQUIRED,
                        [this](string_view arg) {
                                auto split = arg.split('=');
                                args_[split.first] = split.second;
                        } },

                { { "-d", "--debug", "--run-directly" },
                        [this]() { run_directly_ = true; } },

                { { "-l", "--log-file" }, Option::NON_EMPTY_ARG_REQUIRED,
                        [this](string_view arg) {
                                log_name_ = u8path(arg);
                                openLog();
                        } },

                { { "-r", "--run" }, Option::NON_EMPTY_ARG_REQUIRED,
                        [this](string_view arg) {
                                run_selected_ = true;
                                auto fields = arg.split('.');
                                if (fields.second.trim().empty()) {
                                        throw Option::InvalidArgument(
                                                "no test number specified");
                                }
                                to_run_.insert(
                                        { fields.first.trim(),
                                          to_int<unsigned>(fields.second) });
                        } },

                { { "-t", "--timeout" }, Option::NON_EMPTY_ARG_REQUIRED,
                        [this](string_view arg) {
                                timeout_ms_ = to_int<unsigned>(arg);
                        } }
        };

        Option::parse(OPTIONS, argc, argv, 1);
        setUpChildProcessHandling();
}

//--------------------------------------

WRUTIL_API
TestManager::~TestManager()
{
        for (const auto &not_run: to_run_) {
                print(std::cerr, "no such test %s.%s.%u\n",
                      group_, not_run.first, not_run.second);
        }
}

//--------------------------------------

WRUTIL_API std::string &
TestManager::operator[](
        const string_view &arg_name
)
{
        return args_[arg_name.to_string()];
}

//--------------------------------------

WRUTIL_API optional<string_view>
TestManager::value(
        const string_view &arg_name
) const
{
        auto i = args_.find(arg_name.to_string());
        optional<string_view> result;

        if (i != args_.end()) {
                result = string_view(i->second);
        }

        return result;
}

//--------------------------------------

WRUTIL_API string_view
TestManager::valueOr(
        const string_view &arg_name,
        const string_view &or_value
) const
{
        auto i = args_.find(arg_name.to_string());
        return i != args_.end() ? string_view(i->second) : or_value;
}

//--------------------------------------

WRUTIL_API void
TestManager::clearArgs()
{
        args_.clear();
}

//--------------------------------------

void
TestManager::openLog()
{
        log_.open(log_name_.native(), log_.out | log_.app);
        if (!log_.is_open()) {
                throw std::runtime_error(
                        printStr("cannot open test log file \"%s\"",
                                 log_name_));
        }
}

//--------------------------------------

WRUTIL_API void
TestManager::run_(
        const string_view           &sub_group,
        unsigned                     test_number,
        const std::function<void()> &test_code
)
{
        if (run_selected_ && !to_run_.erase({ sub_group, test_number })) {
                return;
        } else if (!have_run_.insert({ sub_group, test_number }).second) {
                throw std::invalid_argument(
                        printStr("duplicate test ID %s.%s.%u",
                                 group_, sub_group, test_number));
        }

        ++count_;

        if (run_directly_) {
                do_run(sub_group, test_number, test_code);
        } else {
                runChildProcess(sub_group, test_number, test_code);
        }
}

//--------------------------------------

int
TestManager::do_run(
        const string_view           &sub_group,
        unsigned                     test_number,
        const std::function<void()> &test_code
)
{
        output(printStr("%s.%s.%u: ", group_, sub_group, test_number));
        std::clog.flush();
        if (log_.is_open()) {
                log_.flush();
        }

        int                status = EXIT_FAILURE;
        std::ostringstream messages;

        try {
                test_code();
                messages << "PASS\n";
                ++passed_;
                status = EXIT_SUCCESS;
        } catch (TestFailure &err) {
                if (err.what()[0]) {
                        print(messages, "FAIL (%s)\n", err.what());
                } else {
                        messages << "FAIL\n";
                }
        } catch (std::exception &e) {
                print(messages, "FAIL with exception (%s):\n", e.what());
                if (dump_exception_) {
                        (*dump_exception_)(messages, nullptr);
                }
        } catch (...) {
                messages << "FAIL with exception:\n";
                if (dump_exception_) {
                        (*dump_exception_)(messages, nullptr);
                }
        }

        output(messages.str());
        return status;
}

//--------------------------------------

void
TestManager::output(
        const string_view &what
)
{
        std::clog << what;
        if (log_.is_open()) {
                log_ << what;
        }
}


} // namespace wr
