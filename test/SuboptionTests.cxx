/**
 * \file SuboptionTests.cxx
 *
 * \brief Unit tests for wr::Option::parseSubOptions()
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
#include <string.h>
#include <wrutil/debug.h>  // add wrdebug library dependency
#include <wrutil/Option.h>
#include <wrutil/TestManager.h>

#include "OptionTestUtils.h"


int
main(
        int          argc,
        const char **argv
)
{
        using wr::Option;
        using wr::string_view;
        using wr::TestFailure;

        wr::TestManager tester("Suboption", argc, argv);

        // one sub-option, no argument
        tester.run("Basic", 1, [] {
                bool foo = false;
                const Option options[] = {
                        { "foo", [&foo](string_view opt, string_view arg) {
                                        verifyPassedOptArg(opt, arg, "foo", {});
                                        foo = true;
                                } }
                };

                Option::parseSubOptions(options, {}, "foo");

                if (!foo) {
                        throw TestFailure("foo not parsed");
                }
        });

        // two sub-options, no arguments
        tester.run("Basic", 2, [] {
                bool foo = false, bar = false;
                const Option options[] = {
                        { "foo", [&foo](string_view opt, string_view arg) {
                                        verifyPassedOptArg(opt, arg, "foo", {});
                                        foo = true;
                                } },
                        { "bar", [&bar](string_view opt, string_view arg) {
                                        verifyPassedOptArg(opt, arg, "bar", {});
                                        bar = true;
                                } }
                };

                Option::parseSubOptions(options, "(test)", "bar,foo");

                if (!foo) {
                        throw TestFailure("foo not parsed");
                }
                if (!bar) {
                        throw TestFailure("bar not parsed");
                }
        });

        // two sub-options, with an argument for one option
        tester.run("Basic", 3, [] {
                string_view foo;
                bool      bar = false;
                const Option options[] = {
                        { "foo", [&foo](string_view opt, string_view arg) {
                                        if (opt != "foo") {
                                                throw TestFailure("expected option name \"foo\", got \"%s\"",
                                                                  opt);
                                        }
                                        foo = arg;
                                } },
                        { "bar", [&bar] { bar = true; } }
                };

#               define EXPECTED_FOO "xyz"
                Option::parseSubOptions(
                        options, "(test)", "foo=" EXPECTED_FOO ",bar");

                if (foo != EXPECTED_FOO) {
                        throw TestFailure("expected argument \"" EXPECTED_FOO "\" for option foo, got \"%s\"",
                                          foo);
                }
                if (!bar) {
                        throw TestFailure("bar not parsed");
                }
#               undef EXPECTED_FOO
        });

        // error handling: check for correct reporting of unknown option
        tester.run("ErrorHandling", 1, [] {
                static const Option OPTIONS[] = {
                        { "foo" },
                        { "bar" }
                };
                try {
                        Option::parseSubOptions(
                                OPTIONS, "(test)", "foo, bar, duff");
                        throw TestFailure("unknown option \"duff\" not reported");
                } catch (Option::UnknownOption &err) {
                        verifyUnknownOptionError(err, "(test)", "duff");
                }
        });

        // error handling: check for correct reporting of empty argument
        tester.run("ErrorHandling", 2, [] {
                static const Option OPTIONS[] = {
                        { "foo" },
                        { "bar", Option::NON_EMPTY_ARG_OPTIONAL, {} }
                };
                try {
                        Option::parseSubOptions(OPTIONS, "(test)", "foo, bar=");
                        throw TestFailure("empty argument not reported");
                } catch (Option::InvalidArgument &err) {
                        verifyInvalidArgumentError(err, "(test)", "bar", {});
                }
        });

        // error handling: check omitted optional argument is allowed
        tester.run("ErrorHandling", 3, [] {
                static const Option OPTIONS[] = {
                        { "foo" },
                        { "bar", Option::NON_EMPTY_ARG_OPTIONAL, {} }
                };
                Option::parseSubOptions(OPTIONS, "(test)", "bar , foo");
        });

        // error handling: check for correct reporting of missing argument
        tester.run("ErrorHandling", 4, [] {
                static const Option OPTIONS[] = {
                        { "foo" },
                        { "bar", Option::ARG_REQUIRED, {} }
                };
                try {
                        Option::parseSubOptions(OPTIONS, "(test)", "bar,foo");
                        throw TestFailure("empty argument not reported");
                } catch (Option::MissingArgument &err) {
                        verifyMissingArgumentError(err, "(test)", "bar");
                }
        });

        // advanced parsing: handler skips commas in argument
        tester.run("SelfParseArg", 1, [] {
                bool pub = false, virt = false;
                string_view rettype;
#               define TYPE         "map<string_view, SomeType>"
#               define EXPECTED_ARG TYPE ",virtual"
                const Option options[] = {
                        { "public", [&pub] { pub = true; } },
                        { "rettype", Option::NON_EMPTY_ARG_REQUIRED
                                        | Option::SUB_OPT_SELF_PARSE_ARG,
                                [&rettype](
                                        string_view opt,
                                        string_view arg
                                ) {
                                        verifyPassedOptArg(opt, arg, "rettype",
                                                           EXPECTED_ARG);
                                        return int(strlen(TYPE));
                                } },
                        { "virtual", [&virt] { virt = true; } }
                };

                Option::parseSubOptions(options, "(test)",
                        "public,rettype=" EXPECTED_ARG);

                if (!pub) {
                        throw TestFailure("public not parsed");
                }
                if (!virt) {
                        throw TestFailure("virtual not parsed");
                }
#               undef EXPECTED_ARG
        });        

        return !tester.failed() ? EXIT_SUCCESS : EXIT_FAILURE;
}
