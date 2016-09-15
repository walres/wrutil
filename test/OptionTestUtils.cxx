/**
 * \file OptionTestUtils.cxx
 *
 * \brief Helper functions shared between OptionTests and SuboptionTests
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
#include <wrutil/TestManager.h>

#include "OptionTestUtils.h"


void
verifyPassedOptArg(
        wr::string_view opt,
        wr::string_view arg,
        wr::string_view expected_opt,
        wr::string_view expected_arg
)
{
        if (opt != expected_opt) {
                throw wr::TestFailure("expected option name \"%s\", got \"%s\"",
                                      expected_opt, opt);
        } else if (arg != expected_arg) {
                throw wr::TestFailure("expected argument \"%s\", got \"%s\"",
                                      expected_arg, arg);
        }
}

//--------------------------------------

void
verifyOptionName(
        wr::string_view opt_name,
        wr::string_view sub_opt_name,
        wr::string_view expected_opt_name,
        wr::string_view expected_sub_opt_name
)
{
        if (opt_name != expected_opt_name) {
                throw wr::TestFailure("expected reported option name \"%s\", got \"%s\"",
                                      expected_opt_name, opt_name);
        }
        if (sub_opt_name != expected_sub_opt_name) {
                throw wr::TestFailure("expected reported sub-option name \"%s\", got \"%s\"",
                                      expected_sub_opt_name, sub_opt_name);
        }
}

//--------------------------------------

void
verifyUnknownOptionError(
        const wr::Option::UnknownOption &err,
        wr::string_view                  expected_opt_name,
        wr::string_view                  expected_sub_opt_name
)
{
        verifyOptionName(err.optionName(), err.subOptionName(),
                         expected_opt_name, expected_sub_opt_name);
}

//--------------------------------------

void
verifyMissingArgumentError(
        const wr::Option::MissingArgument &err,
        wr::string_view                    expected_opt_name,
        wr::string_view                    expected_sub_opt_name
)
{
        verifyOptionName(err.optionName(), err.subOptionName(),
                         expected_opt_name, expected_sub_opt_name);
}

//--------------------------------------

void
verifyInvalidArgumentError(
        const wr::Option::InvalidArgument &err,
        wr::string_view                    expected_opt_name,
        wr::string_view                    expected_sub_opt_name,
        wr::string_view                    expected_argument
)
{
        verifyOptionName(err.optionName(), err.subOptionName(),
                         expected_opt_name, expected_sub_opt_name);

        auto argument = err.argument();

        if (argument != expected_argument) {
                throw wr::TestFailure("expected reported argument \"%s\", got \"%s\"",
                                      expected_argument, argument);
        }
}
