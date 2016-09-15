/**
 * \file U8StringViewTests.cxx
 *
 * \brief Unit tests for class wr::u8string_view
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
#include <locale>
#include <string>
#include <wrutil/debug.h>  // add wrdebug library dependency
#include <wrutil/TestManager.h>
#include <wrutil/u8string_view.h>


using wr::TestFailure;
using wr::u8string_view;


int
main(
        int          argc,
        const char **argv
)
{
        wr::TestManager tester("u8string_view", argc, argv);

        tester.run("ensure_is_safe", 1, [] {
                u8string_view test("\xf0");
                if (!test.empty()) {
                        throw TestFailure("empty() returned false, expected true");
                }
        });

        tester.run("ensure_is_safe", 2, [] {
                u8string_view test("\x80xyz");
                if (test != "xyz") {
                        throw TestFailure("resulting string is \"%s\", expected \"xyz\"",
                                          test);
                }
        });

        tester.run("ensure_is_safe", 3, [] {
                u8string_view test("abc\xf0");
                if (test != "abc") {
                        throw TestFailure("resulting string is \"%s\", expected \"abc\"",
                                          test);
                }
        });

        tester.run("has_min_size", 1, [] {
                u8string_view test("fn");
                if (!test.has_min_size(2)) {
                        throw TestFailure("has_min_size() returned false, expected true");
                }
        });

        tester.run("has_min_size", 2, [] {
                u8string_view test("fn");
                if (test.has_min_size(3)) {
                        throw TestFailure("has_min_size() returned true, expected false");
                }
        });

        tester.run("has_max_size", 1, [] {
                u8string_view test("abc");
                if (test.has_max_size(2)) {
                        throw TestFailure("has_max_size() returned true, expected false");
                }
        });

        tester.run("has_max_size", 2, [] {
                u8string_view test("abc");
                if (!test.has_max_size(3)) {
                        throw TestFailure("has_max_size() returned false, expected true");
                }
        });

        tester.run("has_max_size", 3, [] {
                u8string_view test("abc");
                if (!test.has_max_size(4)) {
                        throw TestFailure("has_max_size() returned false, expected true");
                }
        });

        tester.run("substr", 1, [] {
                u8string_view test("abc"),
                              sub (test.substr(test.begin(), 3));
                if (sub != "abc") {
                        throw TestFailure("substr() returned \"%s\", expected \"abc\"",
                                          sub);
                }
        });

        tester.run("substr", 2, [] {
                u8string_view test("abc=def"),
                              sub (test.substr(std::next(test.begin(), 1), 2));
                if (sub != "bc") {
                        throw TestFailure("substr() returned \"%s\", expected \"bc\"",
                                          sub);
                }
        });

        tester.run("substr", 3, [] {
                u8string_view test(";"), sub(test.substr(test.end(), 2));
                if (!sub.empty()) {
                        throw TestFailure("substr() returned \"%s\", expected empty",
                                          sub);
                }
        });

        tester.run("substr", 4, [] {
                u8string_view test(u8"i\u03c0j\u00dfk\U0001F603m"),
                              sub (test.substr(std::next(test.begin(), 2), 4)),
                              exp (u8"j\u00dfk\U0001F603");
                if (sub != exp) {
                        throw TestFailure("substr() returned \"%s\", expected \"%s\"",
                                          sub, exp);
                }
        });

        tester.run("split", 1, [] {
                u8string_view abc("abc"), def("def");
                std::string test_str(printStr("%s=%s", abc, def));
                u8string_view test_view(test_str);

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
                auto split = u8string_view(";").split(';');

                if (!split.first.empty()) {
                        throw TestFailure("split.first == \"%s\", expected empty",
                                          split.first);
                }
                if (!split.second.empty()) {
                        throw TestFailure("split.second == \"%s\", expected empty",
                                          split.second);
                }
        });

        return !tester.failed() ? EXIT_SUCCESS : EXIT_FAILURE;
}
