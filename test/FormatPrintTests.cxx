/**
 * \file FormatPrintTests.cxx
 *
 * \brief Unit tests for wr::print() functions
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
#include <errno.h>
#include <locale>
#include <string>
#include <wrutil/debug.h>  // add wrdebug library dependency
#include <wrutil/Format.h>
#include <wrutil/TestManager.h>


using wr::TestFailure;


template <typename ...Args>
static void testPrint(const wr::string_view &expected,
                      const char *format, Args &&...args);


int
main(
        int          argc,
        const char **argv
)
{
        std::locale::global(std::locale(
#if WR_WINDOWS
                "en-US"
#else
                "en_US"
#endif
        ));

        wr::TestManager tester("Format", argc, argv);

        tester.run("print", 1, [] {
                testPrint("Hello *     World* ", "Hello *%10s* ", "World");
        });

        tester.run("print", 2, [] {
                testPrint("00,012,345s ", "%'.08ds ", 12345);
        });

        tester.run("print", 3, [] {
                errno = 0;
                auto result = wr::printStr("%3$=*1$.*02$d\n");
                if (errno != ERANGE) {
                        throw TestFailure("missing arguments not detected");
                }
                if (!result.empty()) {
                        throw TestFailure("result = \"%s\", expected empty",
                                          result);
                }
        });

        tester.run("print", 4, [] {
                errno = 0;
                auto result = wr::printStr("%3$=1$.02$d\n", 16, 8, 123);
                if (errno != EINVAL) {
                        throw TestFailure("invalid format string not detected\n");
                }
        });

        tester.run("print", 5, [] {
                testPrint("    00000123    ", "%3$=*1$.*02$d", 16, 8, 123);
        });

        tester.run("print", 6, [] {
                testPrint(" 123", "% d", 123);
        });

        tester.run("print", 7, [] {
                errno = 0;
                auto result = wr::printStr("%$1.8u %$1#.8o %$1#.8x %$1s\n",
                                           123);
                if (errno != EINVAL) {
                        throw TestFailure("invalid format string not detected\n");
                }
        });

        tester.run("print", 8, [] {
                testPrint("100,000,000.000000 ", "%'f ", 300000000.0 / 3.0);
        });

        tester.run("print", 9, [] {
                testPrint("-100,000,000.000000 ", "%'f ", 300000000.0 / -3.0);
        });

        tester.run("print", 10, [] {
                testPrint(" 100,000,000.000000 ", "% 'f ", 300000000.0 / 3.0);
        });

        tester.run("print", 11, [] {
                testPrint("+100,000,000. ", "%#+'.0f ", 300000000.0 / 3.0);
        });

        tester.run("print", 12, [] {
                testPrint("0x1.0p+0", "%.1a", 1);
        });

        tester.run("print", 13, [] {
                testPrint("0x2.00p+0", "%.2a", 1.999);
        });

        tester.run("print", 14, [] {
                testPrint("0x1.73p+1", "%.2a", 2.9);
        });

        tester.run("print", 15, [] {
                testPrint("0x1.p+0", "%#a", 1);
        });

        tester.run("print", 16, [] {
                testPrint("-000000456", "%010d", -456);
        });

        tester.run("print", 17, [] {
                testPrint("1c8       ", "%-010x", 456);
        });

        return !tester.failed() ? EXIT_SUCCESS : EXIT_FAILURE;
}

//--------------------------------------

template <typename ...Args> static void
testPrint(
        const wr::string_view &expected,
        const char            *format,
        Args              &&...args
)
{
        auto result = wr::printStr(format, std::forward<Args>(args)...);
        if (result != expected) {
                throw TestFailure("result was \"%s\" but expected \"%s\"",
                                  result, expected);
        }
}
