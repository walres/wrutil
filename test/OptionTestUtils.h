/**
 * \file OptionTestUtils.h
 *
 * \brief Declarations of functions shared between OptionTests and
 *        SuboptionTests
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
#ifndef WRUTIL_OPTION_TEST_UTILS_H
#define WRUTIL_OPTION_TEST_UTILS_H

#include <wrutil/Option.h>
#include <wrutil/string_view.h>


void verifyPassedOptArg(wr::string_view opt, wr::string_view arg,
                        wr::string_view expected_opt,
                        wr::string_view expected_arg);

void verifyOptionName(wr::string_view opt_name,
                      wr::string_view sub_opt_name,
                      wr::string_view expected_opt_name,
                      wr::string_view expected_sub_opt_name);

void verifyUnknownOptionError(const wr::Option::UnknownOption &err,
                              wr::string_view expected_opt_name,
                              wr::string_view expected_sub_opt_name = {});

void verifyMissingArgumentError(const wr::Option::MissingArgument &err,
                                wr::string_view expected_opt_name,
                                wr::string_view expected_sub_opt_name = {});

void verifyInvalidArgumentError(const wr::Option::InvalidArgument &err,
                                wr::string_view expected_opt_name,
                                wr::string_view expected_sub_opt_name,
                                wr::string_view expected_argument);

#endif // !WRUTIL_OPTION_TEST_UTILS_H
