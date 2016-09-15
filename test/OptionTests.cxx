/**
 * \file OptionTests.cxx
 *
 * \brief Unit tests for wr::Option::parse() and associated types/functions
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
#include <stdarg.h>
#include <string.h>
#include <wrutil/debug.h>  // add wrdebug library dependency
#include <wrutil/Option.h>
#include <wrutil/TestManager.h>

#include "OptionTestUtils.h"


static void nonOptArgTest();
static void toArgVectorTest(wr::string_view args, size_t expected_argc, ...);


int
main(
        int          argc,
        const char **argv
)
{
        using wr::Option;
        using wr::string_view;
        using wr::TestFailure;
        using wr::arraySize;

        wr::TestManager tester("Option", argc, argv);

        /*
         * Static Option::Name::prefix() tests
         */
        static const auto static_prefix_test = [](
                string_view input,
                string_view expected
        )
        {
                auto prefix = Option::prefix(input);
                if (prefix != expected) {
                        throw TestFailure("expected prefix \"%s\" from input \"%s\", got \"%s\"",
                                          expected, input, prefix);
                }
        };

        tester.run("StaticPrefix", 1, static_prefix_test, "", "");
        tester.run("StaticPrefix", 2, static_prefix_test, "foo", "");
        tester.run("StaticPrefix", 3, static_prefix_test, "-foo", "-");
        tester.run("StaticPrefix", 4, static_prefix_test, "--foo", "--");
        tester.run("StaticPrefix", 5, static_prefix_test, "---", "--");
        tester.run("StaticPrefix", 6, static_prefix_test, "+", "+");
        tester.run("StaticPrefix", 7, static_prefix_test, "+-", "+");
        tester.run("StaticPrefix", 8, static_prefix_test, "--", "--");
        tester.run("StaticPrefix", 9, static_prefix_test, "-###", "-");

        /*
         * Option construction tests
         */
        tester.run("Construct", 1, [] {
                Option  opt      { { "-i", "--ignore-case" } };
                auto    num_names = opt.names().size();
                auto   &name0     = opt.names()[0],
                       &name1     = opt.names()[1];

                if (num_names != 2) {
                        throw TestFailure("wrong number of names (%u), expected 2",
                                          num_names);
                } else if (name0 != "-i") {
                        throw TestFailure("wrong first option name \"%s\", expected \"-i\"",
                                          name0);
                } else if (name1 != "--ignore-case") {
                        throw TestFailure("wrong second option name \"%s\", expected \"--ignore-case\"",
                                          name1);
                }
        });

        /*
         * Option parsing tests
         */
        // simple: one option without an argument
        static const auto simple_parse_single_opt = [](
                const char * const opt_name
        )
        {
                const Option OPTIONS[] = {
                        { opt_name,
                                [opt_name](string_view opt, string_view arg) {
                                        verifyPassedOptArg(
                                                opt, arg, opt_name, "");
                                } }
                };
                const char *argv[] = { opt_name };
                Option::parse(OPTIONS,
                              static_cast<int>(arraySize(argv)), argv, 0);
        };

        tester.run("ParseSimple", 1, simple_parse_single_opt, "-o");
        tester.run("ParseSimple", 2, simple_parse_single_opt, "--foo");
        tester.run("ParseSimple", 3, simple_parse_single_opt, "-bar");
        tester.run("ParseSimple", 4, simple_parse_single_opt, "+w2");

        static const Option PARSE_ARG_OPTIONS_1[] = {
                { "-o", Option::NON_EMPTY_ARG_REQUIRED,
                        [](string_view opt, string_view arg) {
                                verifyPassedOptArg(opt, arg, "-o", "foo");
                        } },
                { "--directory", Option::NON_EMPTY_ARG_REQUIRED,
                        [](string_view opt, string_view arg) {
                                verifyPassedOptArg(opt, arg, "--directory",
                                                   "/foo/bar");
                        } },
                { "", [](string_view arg) {
                                throw TestFailure("unexpected non-option argument \"%s\"",
                                                  arg);
                        } }
        };

        // one single-char option with conjoined argument
        tester.run("ParseArg", 1, [] {
                const char *argv[] = { "-ofoo" };
                Option::parse(PARSE_ARG_OPTIONS_1,
                              static_cast<int>(arraySize(argv)), argv, 0);
        });

        // one single-char option with separate argument
        tester.run("ParseArg", 2, [] {
                const char *argv[] = { "-o", "foo" };
                Option::parse(PARSE_ARG_OPTIONS_1,
                              static_cast<int>(arraySize(argv)), argv, 0);
        });

        // one multi-char option with conjoined argument
        tester.run("ParseArg", 3, [] {
                const char *argv[] = { "--directory=/foo/bar" };
                Option::parse(PARSE_ARG_OPTIONS_1,
                              static_cast<int>(arraySize(argv)), argv, 0);
        });

        // one multi-char option with separate argument
        tester.run("ParseArg", 4, [] {
                const char *argv[] = { "--directory", "/foo/bar" };
                Option::parse(PARSE_ARG_OPTIONS_1,
                              static_cast<int>(arraySize(argv)), argv, 0);
        });

        // error handling: correctly report missing argument
        tester.run("ErrorHandling", 1, [] {
                const char *argv[] = { "-o" };
                try {
                        Option::parse(PARSE_ARG_OPTIONS_1,
                                      static_cast<int>(arraySize(argv)),
                                      argv, 0);
                        throw TestFailure("missing argument not reported");
                } catch (Option::MissingArgument &err) {
                        verifyMissingArgumentError(err, "-o");
                }
        });

        // error handling: correctly report unknown option
        tester.run("ErrorHandling", 2, [] {
                const char *argv[] = { "-ofoo", "-?" };
                try {
                        Option::parse(PARSE_ARG_OPTIONS_1,
                                      static_cast<int>(arraySize(argv)),
                                      argv, 0);
                        throw TestFailure("unknown option -? not reported");
                } catch (Option::UnknownOption &err) {
                        verifyUnknownOptionError(err, "-?");
                }
        });

        tester.run("ErrorHandling", 3, [] {
                const char *argv[] = { "-o", "foo", "--duff" };
                try {
                        Option::parse(PARSE_ARG_OPTIONS_1,
                                      static_cast<int>(arraySize(argv)),
                                      argv, 0);
                        throw TestFailure("unknown option --duff not reported");
                } catch (Option::UnknownOption &err) {
                        verifyUnknownOptionError(err, "--duff");
                }
        });

        // error handling: correctly report empty argument
        tester.run("ErrorHandling", 4, [] {
                const char *argv[] = { "-o", "" };
                try {
                        Option::parse(PARSE_ARG_OPTIONS_1,
                                      static_cast<int>(arraySize(argv)),
                                      argv, 0);
                        throw TestFailure("empty argument for -o not reported");
                } catch (Option::InvalidArgument &err) {
                        verifyInvalidArgumentError(err, "-o", {}, {});
                }
        });

        // parsing grouped single-letter options
        tester.run("Grouping", 1, [] {
                bool c = false, v = false;
                string_view f;
                const Option options[] = {
                        { "-c", [&c] { c = true; } },
                        { "-v", [&v] { v = true; } },
                        { "-f", Option::NON_EMPTY_ARG_REQUIRED,
                                [&f](string_view arg) { f = arg; } }
                };

                const char * const expected_f = "/foo/bar";
                const char *argv[] = { "-cvf", expected_f };
                Option::parse(options,
                              static_cast<int>(arraySize(argv)), argv, 0);
                if (!c) {
                        throw TestFailure("-c not parsed");
                }
                if (!v) {
                        throw TestFailure("-v not parsed");
                }
                if (f.empty()) {
                        throw TestFailure("-f not parsed");
                }
                if (f != expected_f) {
                        throw TestFailure("expected argument \"%s\" for -f, got \"%s\"",
                                          expected_f, f);
                }
        });

        /* single-character option grouping not allowed if one or more multi-
           character options have been registered under the same prefix */
        tester.run("Grouping", 2, [] {
                const Option options[] = {
                        { "-std", Option::NON_EMPTY_ARG_REQUIRED, {} },
                        { "-c" },
                        { "-v" }
                };

                try {
                        const char *argv[] = { "-cv", "-std=c++11" };
                        Option::parse(options,
                                    static_cast<int>(arraySize(argv)), argv, 0);
                        throw TestFailure("unknown option -cv not reported");
                } catch (Option::UnknownOption &err) {
                        verifyUnknownOptionError(err, "-cv");
                }
        });

        /* single-character option grouping still allowed if multi-character
           options have only been registered under a different prefix */
        tester.run("Grouping", 3, [] {
                bool c = false, v = false;
                string_view std;
                const Option options[] = {
                        { "--std", Option::NON_EMPTY_ARG_REQUIRED,
                                [&std](string_view arg) { std = arg; } },
                        { "-c", [&c] { c = true; } },
                        { "-v", [&v] { v = true; } }
                };

#               define EXPECTED_STD "c++11"
                const char *argv[] = { "-cv", "--std=" EXPECTED_STD };
                Option::parse(options,
                              static_cast<int>(arraySize(argv)), argv, 0);
                if (!c) {
                        throw TestFailure("-c not parsed");
                }
                if (!v) {
                        throw TestFailure("-v not parsed");
                }
                if (std.empty()) {
                        throw TestFailure("--std not parsed");
                }
                if (std != EXPECTED_STD) {
                        throw TestFailure("expected argument \"%s\" for option --std, got \"%s\"",
                                          EXPECTED_STD, std);
                }
#               undef EXPECTED_STD
        });

        /* grouped set of single-character options with one option taking an
           optional but separate argument (omitted in this test) must be parsed
           correctly */
        tester.run("Grouping", 4, [] {
                bool a = false, b = false, c = false;
                string_view b_arg;
                const Option options[] = {
                        { "-a", [&a] { a = true; } },
                        { "-b", Option::ARG_OPTIONAL
                                        | Option::SEPARATE_ARG_ONLY,
                                [&b, &b_arg](string_view arg) {
                                        b = true;
                                        b_arg = arg;
                                } },
                        { "-c", [&c] { c = true; } }
                };

                const char *argv[] = { "-abc" };
                Option::parse(options,
                              static_cast<int>(arraySize(argv)), argv, 0);
                if (!a) {
                        throw TestFailure("-a not parsed");
                }
                if (!b) {
                        throw TestFailure("-b not parsed");
                }
                if (!b_arg.empty()) {
                        throw TestFailure("expected empty argument for -b, got \"%s\"",
                                          b_arg);
                }
                if (!c) {
                        throw TestFailure("-c not parsed");
                }
        });

        /* single-character option requiring a separate argument placed in
           the middle of a grouping must trigger a MissingArgument error */
        tester.run("Grouping", 5, [] {
                static const Option OPTIONS[] = {
                        { "-a" },
                        { "-b", Option::ARG_REQUIRED
                                        | Option::SEPARATE_ARG_ONLY, {} },
                        { "-c" }
                };

                try {
                        const char *argv[] = { "-abc" };
                        Option::parse(OPTIONS,
                                    static_cast<int>(arraySize(argv)), argv, 0);
                } catch (Option::MissingArgument &err) {
                        verifyMissingArgumentError(err, "-b");
                }                
        });

        /* grouped set of single-character options with one option taking an
           optional but separate argument (omitted in this test) must be parsed
           correctly */
        tester.run("Grouping", 6, [] {
                bool a = false, b = false, c = false;
                string_view b_arg;
                const Option options[] = {
                        { "-a", [&a] { a = true; } },
                        { "-b", Option::ARG_OPTIONAL | Option::JOINED_ARG_ONLY,
                                [&b, &b_arg](string_view arg) {
                                        b = true;
                                        b_arg = arg;
                                } },
                        { "-c", [&c] { c = true; } }
                };

                const char *argv[] = { "-ab", "-c" };
                Option::parse(options,
                              static_cast<int>(arraySize(argv)), argv, 0);
                if (!a) {
                        throw TestFailure("-a not parsed");
                }
                if (!b) {
                        throw TestFailure("-b not parsed");
                }
                if (!b_arg.empty()) {
                        throw TestFailure("expected empty argument for -b, got \"%s\"",
                                          b_arg);
                }
                if (!c) {
                        throw TestFailure("-c not parsed");
                }
        });

        /* single-character option requiring a joined argument with nothing
           following before the next option must trigger a MissingArgument
           error */
        tester.run("Grouping", 7, [] {
                static const Option OPTIONS[] = {
                        { "-a" },
                        { "-b", Option::ARG_REQUIRED
                                        | Option::JOINED_ARG_ONLY, {} },
                        { "-c" }
                };

                try {
                        const char *argv[] = { "-ab -c" };
                        Option::parse(OPTIONS,
                                    static_cast<int>(arraySize(argv)), argv, 0);
                } catch (Option::MissingArgument &err) {
                        verifyMissingArgumentError(err, "-b");
                }                
        });

        /* long option requiring an argument, suffixed with punctuation mark
           so the combined option and argument appear as an option word */
        tester.run("CombinedOptArg", 1, [] {
                bool foo = false;
                string_view sdig_scan;
                const Option options[] = {
                        { "--plugin-arg-", Option::NON_EMPTY_ARG_REQUIRED,
                                [&sdig_scan](
                                        string_view         opt,
                                        string_view         arg,
                                        int                 argc,
                                        const char * const *argv
                                ) {
                                        verifyPassedOptArg(opt, arg,
                                                           "--plugin-arg-",
                                                           "sdig_scan");
                                        if (argc < 1) {
                                                throw Option::MissingArgument(
                                                      "--plugin-arg-sdig_scan");
                                        }

                                        if (string_view(argv[0]).empty()) {
                                                throw Option::InvalidArgument(
                                                      "--plugin-arg-sdig_scan",
                                                      argv[0],
                                                      "--plugin-arg-sdig_scan requires a non-empty argument");
                                        }

                                        sdig_scan = argv[0];
                                        return 1;
                                } },
                        { "--foo", [&foo] { foo = true; } }
                };

                const char * const expected_sdig_scan = "--tag=XYZ";
                const char *argv[] = { "--plugin-arg-sdig_scan",
                                       expected_sdig_scan, "--foo" };
                Option::parse(options,
                              static_cast<int>(arraySize(argv)), argv, 0);

                if (sdig_scan.empty()) {
                        throw TestFailure("--plugin-arg-sdig_scan not parsed");
                }
                if (sdig_scan != expected_sdig_scan) {
                        throw TestFailure("expected argument \"%s\" for --plugin-arg-sdig_scan, got \"%s\"",
                                          expected_sdig_scan, sdig_scan);
                }
                if (!foo) {
                        throw TestFailure("--foo not parsed");
                }
        });

        /* long option requiring an argument, suffixed with punctuation
           mark so the combined option and argument appear as an option word;
           ensure UnknownOption error is thrown if the argument is absent */
        tester.run("CombinedOptArg", 2, [] {
                static const Option OPTIONS[] = {
                        { "--plugin-arg-", Option::NON_EMPTY_ARG_REQUIRED, {} },
                        { "--foo" }
                };

                const char *argv[] = { "--plugin-arg-", "--tag=XYZ", "--foo" };

                try {
                        Option::parse(OPTIONS,
                                    static_cast<int>(arraySize(argv)), argv, 0);
                        throw TestFailure("unknown option \"%s\" not detected",
                                          argv[0]);
                } catch (Option::UnknownOption &err) {
                        verifyUnknownOptionError(err, "--plugin-arg-");
                }
        });

        /* parsing non-option argument(s) */
        tester.run("NonOptArg", 1, nonOptArgTest);

        /*
         * Option::toArgVector() tests
         */
        tester.run("toArgVector", 1, toArgVectorTest, "", 0, nullptr);
                // at least one argument required for lambda varargs
        tester.run("toArgVector", 2, toArgVectorTest,
                   "\t\n \t   ", 0, nullptr);

        tester.run("toArgVector", 3, toArgVectorTest,
                   " clang++  ", 1, "clang++");
        tester.run("toArgVector", 4, toArgVectorTest,
                   "clang++ \"\"", 2, "clang++", "");
        tester.run("toArgVector", 5, toArgVectorTest,
                   "g++ -DFOO=\"abc def\" -o foo foo.cxx", 5,
                   "g++", "-DFOO=\"abc def\"", "-o", "foo", "foo.cxx");
        tester.run("toArgVector", 6, toArgVectorTest,
                   "\"C:\\Program Files\\Microsoft SDKs\"",
                   1, "C:\\Program Files\\Microsoft SDKs");

        return !tester.failed() ? EXIT_SUCCESS : EXIT_FAILURE;
}

//--------------------------------------

static void
nonOptArgTest()
{
        wr::string_view o, std, src[3], inc[2], lib[2];
        size_t n_src = 0, n_inc = 0, n_lib = 0;
        bool c = false;
        const wr::Option options[] = {
                { "-O", wr::Option::ARG_OPTIONAL,
                        [&o](wr::string_view arg) { o = arg; } },
                { "-c", [&c] { c = true; } },
                { "-std", wr::Option::NON_EMPTY_ARG_REQUIRED,
                        [&std](wr::string_view arg) { std = arg; } },
                { "-I", wr::Option::NON_EMPTY_ARG_REQUIRED,
                        [&inc, &n_inc](wr::string_view arg) {
                                if (n_inc >= arraySize(inc)) {
                                        throw wr::TestFailure("too many include paths");
                                }
                                inc[n_inc++] = arg;
                        } },
                { "-l", wr::Option::NON_EMPTY_ARG_REQUIRED,
                        [&lib, &n_lib](wr::string_view arg) {
                                if (n_lib >= arraySize(lib)) {
                                        throw wr::TestFailure("too many library names");
                                }
                                lib[n_lib++] = arg;
                        } },
                { "", [&src, &n_src](wr::string_view arg) {
                                if (n_src >= arraySize(src)) {
                                        throw wr::TestFailure("too many source file names");
                                }
                                src[n_src++] = arg;
                        } }
        };

#       define INC_0 "../.."
#       define INC_1 "/usr/pkg/sqlite-3.12.0"
#       define LIB_0 "sdigutil"
#       define LIB_1 "sqlite3"
#       define STD "c++11"
#       define SRC_0 "foo.cxx"
#       define SRC_1 "bar.cxx"
#       define SRC_2 "fred.cxx"
#       define O "2"

        const char *argv[] = { "clang++", "-std=c++11", "-I../..",
                               "-I/usr/pkg/sqlite-3.12.0", "-O2", "-c",
                               "foo.cxx", "bar.cxx", "fred.cxx",
                               "-lsdigutil", "-lsqlite3" };

        wr::Option::parse(options,
                          static_cast<int>(wr::arraySize(argv)), argv, 1);

        if (std != STD) {
                throw wr::TestFailure("expected argument \"%s\" for option -std, got \"%s\"",
                                      STD, std);
        }
        if (n_inc != 2) {
                throw wr::TestFailure("expected 2 include paths parsed; %s were actually parsed",
                                      n_inc);
        }
        if (inc[0] != INC_0) {
                throw wr::TestFailure("expected argument \"%s\" for first include path, got \"%s\"",
                                      INC_0, inc[0]);
        }
        if (inc[1] != INC_1) {
                throw wr::TestFailure("expected argument \"%s\" for second include path, got \"%s\"",
                                      INC_1, inc[1]);
        }
        if (o != O) {
                throw wr::TestFailure("expected argument \"%s\" for option -O, got \"%s\"",
                                      O, o);
        }
        if (!c) {
                throw wr::TestFailure("-c not parsed");
        }
        if (n_src != 3) {
                throw wr::TestFailure("expected 3 source files parsed; %s were actually parsed",
                                      n_src);
        }
        if (src[0] != SRC_0) {
                throw wr::TestFailure("expected argument \"%s\" for first source file, got \"%s\"",
                                      SRC_0, src[0]);
        }
        if (src[1] != SRC_1) {
                throw wr::TestFailure("expected argument \"%s\" for second source file, got \"%s\"",
                                      SRC_1, src[1]);
        }
        if (src[2] != SRC_2) {
                throw wr::TestFailure("expected argument \"%s\" for third source file, got \"%s\"",
                                      SRC_2, src[2]);
        }
        if (n_lib != 2) {
                throw wr::TestFailure("expected 2 library names parsed; %s were actually parsed",
                                      n_lib);
        }
        if (lib[0] != LIB_0) {
                throw wr::TestFailure("expected argument \"%s\" for first library name, got \"%s\"",
                                      LIB_0, lib[0]);
        }
        if (lib[1] != LIB_1) {
                throw wr::TestFailure("expected argument \"%s\" for second library name, got \"%s\"",
                                      LIB_1, lib[1]);
        }
#       undef O
#       undef SRC_2
#       undef SRC_1
#       undef SRC_0
#       undef STD
#       undef LIB_1
#       undef LIB_0
#       undef INC_1
#       undef INC_0
}

//--------------------------------------

static void
toArgVectorTest(
        wr::string_view   args,
        size_t            expected_argc,
        /* const char */  ... // expected_arg...
)
{
        auto result = wr::Option::toArgVector(args);

        if (result.first.size() != expected_argc) {
                throw wr::TestFailure("wrong no. of args (%u), %u expected",
                                      result.first.size(), expected_argc);
        }

        va_list ap;
        va_start(ap, expected_argc);

        for (size_t i = 0; i < expected_argc; ++i) {
                auto expected_arg = va_arg(ap, const char *);

                if (strcmp(result.first[i], expected_arg)) {
                        throw wr::TestFailure("wrong argument %u \"%s\" returned, expected \"%s\"",
                                              i, result.first[i],
                                              expected_arg);
                }
        }

        va_end(ap);
}
