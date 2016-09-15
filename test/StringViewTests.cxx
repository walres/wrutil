/**
 * \file StringViewTests.cxx
 *
 * \brief Unit tests for class wr::string_view
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
#include <locale.h>
#include <string.h>
#include <string>
#include <wrutil/debug.h>  // add wrdebug library dependency
#include <wrutil/Format.h>
#include <wrutil/TestManager.h>
#include <wrutil/string_view.h>


using wr::TestFailure;
using wr::string_view;


int
main(
        int          argc,
        const char **argv
)
{
        wr::TestManager tester("string_view", argc, argv);

        tester.run("has_min_size", 1, [] {
                string_view test("fn");
                if (!test.has_min_size(2)) {
                        throw TestFailure("has_min_size() returned false, expected true");
                }
        });

        tester.run("has_min_size", 2, [] {
                string_view test("fn");
                if (test.has_min_size(3)) {
                        throw TestFailure("has_min_size() returned true, expected false");
                }
        });

        tester.run("has_max_size", 1, [] {
                string_view test("abc");
                if (test.has_max_size(2)) {
                        throw TestFailure("has_max_size() returned true, expected false");
                }
        });

        tester.run("has_max_size", 2, [] {
                string_view test("abc");
                if (!test.has_max_size(3)) {
                        throw TestFailure("has_max_size() returned false, expected true");
                }
        });

        tester.run("has_max_size", 3, [] {
                string_view test("abc");
                if (!test.has_max_size(4)) {
                        throw TestFailure("has_max_size() returned false, expected true");
                }
        });

        tester.run("substr", 1, [] {
                string_view test("abc"), sub(test.substr(0, 3));
                if (sub != "abc") {
                        throw TestFailure("substr() returned \"%s\", expected \"abc\"",
                                          sub);
                }
        });

        tester.run("substr", 2, [] {
                string_view test("abc=def"), sub(test.substr(1, 2));
                if (sub != "bc") {
                        throw TestFailure("substr() returned \"%s\", expected \"bc\"",
                                          sub);
                }
        });

        tester.run("substr", 3, [] {
                string_view test(";"), sub(test.substr(1, 2));
                if (!sub.empty()) {
                        throw TestFailure("substr() returned \"%s\", expected empty",
                                          sub);
                }
        });

        tester.run("split", 1, [] {
                string_view abc("abc"), def("def");
                std::string test_str(printStr("%s=%s", abc, def));
                string_view test_view(test_str);

                auto split = test_view.split('=');

                if (split.first != abc) {
                        throw TestFailure("split.first == \"%s\", expected \"%s\"",
                                          split.first, abc);
                }
                if (split.second != def) {
                        throw TestFailure("split.second == \"%s\", expected \"%s\"",
                                          split.second, def);
                }
        });

        tester.run("split", 2, [] {
                auto split = string_view(";").split(';');

                if (!split.first.empty()) {
                        throw TestFailure("split.first == \"%s\", expected empty",
                                          split.first);
                }
                if (!split.second.empty()) {
                        throw TestFailure("split.second == \"%s\", expected empty",
                                          split.second);
                }
        });

        tester.run("find", 1, [] {
                string_view test("abcdefghi");
                auto        pos = test.find("def");

                if (pos != 3) {
                        throw TestFailure("pos == %u, expected 3", pos);
                }
        });

        tester.run("rfind", 1, [] {
                string_view test("abcdefghi");
                auto        pos = test.rfind("def");

                if (pos != 3) {
                        throw TestFailure("pos == %u, expected 3", pos);
                }
        });

        tester.run("find_first_of", 1, [] {
                string_view test("abcdefghi");
                auto        pos = test.find_first_of("fed");

                if (pos != 3) {
                        throw TestFailure("pos == %u, expected 3", pos);
                }
        });

        tester.run("find_last_of", 1, [] {
                string_view test("abcdefghi");
                auto        pos = test.find_last_of("edf");

                if (pos != 5) {
                        throw TestFailure("pos == %u, expected 5", pos);
                }
        });

        tester.run("find_first_not_of", 1, [] {
                string_view test("abcdefghi");
                auto        pos = test.find_first_not_of("daebfc");

                if (pos != 6) {
                        throw TestFailure("pos == %u, expected 6", pos);
                }
        });

        tester.run("find_last_not_of", 1, [] {
                string_view test("abcdefghi");
                auto        pos = test.find_last_not_of("figdhe");

                if (pos != 2) {
                        throw TestFailure("pos == %u, expected 2", pos);
                }
        });

        tester.run("trim", 1, [] {
                const char *TEST = "string_view";
                string_view test(TEST);
                if (test.trim() != TEST) {
                        throw TestFailure("test == \"%s\", expected \"%s\"",
                                          test, TEST);
                }
        });

        tester.run("has_prefix", 1, [] {
                string_view test("abcdefghi");
                if (!test.has_prefix("abc")) {
                        throw TestFailure("has_prefix() returned false, expected true");
                }
        });

        tester.run("has_prefix", 2, [] {
                string_view test("abcdefghi");
                if (test.has_prefix("def")) {
                        throw TestFailure("has_prefix() returned true, expected false");
                }
        });

        tester.run("has_suffix", 1, [] {
                string_view test("abcdefghi");
                if (!test.has_suffix("ghi")) {
                        throw TestFailure("has_suffix() returned false, expected true");
                }
        });

        tester.run("has_suffix", 2, [] {
                string_view test("abcdefghi");
                if (test.has_suffix("def")) {
                        throw TestFailure("has_suffix() returned true, expected false");
                }
        });

        return !tester.failed() ? EXIT_SUCCESS : EXIT_FAILURE;
}
