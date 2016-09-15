/**
 * \file ArraybufTests.cxx
 *
 * \brief Unit tests for wr::basic_arraybuf and wr::*arraystream classes
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
#include <string>
#include <wrutil/arraystream.h>
#include <wrutil/debug.h>  // add wrdebug library dependency
#include <wrutil/TestManager.h>


using wr::TestFailure;


int
main(
        int          argc,
        const char **argv
)
{
        wr::TestManager tester("Arraybuf", argc, argv);

        tester.run("Read", 1, [] {
                static const char * const TEXT = "Hello World",
                                  * const WORD1 = "Hello",
                                  * const WORD2 = "World";
                std::string      word;
                wr::iarraystream in(TEXT, strlen(TEXT));

                if (size_t(in.rdbuf()->in_avail()) != strlen(TEXT)) {
                        throw TestFailure("in_avail() returned %u, expected %u\n",
                                          in.rdbuf()->in_avail(), strlen(TEXT));
                }
                in >> word;
                if (word != WORD1) {
                        throw TestFailure("read \"%s\", expected \"%s\"",
                                          word, WORD1);
                }
                if (size_t(in.rdbuf()->in_avail())
                                        != (strlen(TEXT) - strlen(WORD1))) {
                        throw TestFailure("in_avail() returned %u, expected %u\n",
                                          in.rdbuf()->in_avail(),
                                          strlen(TEXT) - strlen(WORD1));
                }
                in >> word;
                if (word != WORD2) {
                        throw TestFailure("read \"%s\", expected \"%s\"",
                                          word, WORD2);
                }
                if (in.rdbuf()->in_avail() != 0) {
                        throw TestFailure("in_avail() returned %u, expected 0\n",
                                          in.rdbuf()->in_avail());
                }
                in >> word;
                if (!in.eof()) {
                        throw TestFailure("in.eof() returned false");
                }
        });

        tester.run("Read", 2, [] {
                wr::iarraystream in("", 0);
                std::string      word;

                if (in.rdbuf()->in_avail() != 0) {
                        throw TestFailure("in_avail() returned %u, expected 0",
                                          in.rdbuf()->in_avail());
                }
                in >> word;
                if (!in.eof()) {
                        throw TestFailure("in.eof() returned false");
                }
        });

        tester.run("Putback", 1, [] {
                static const char * const TEXT = "abc";
                wr::iarraystream in(TEXT, strlen(TEXT));
                int              i;

                if (in.putback('#')) {  // buf must refuse this operation
                        throw TestFailure("in.putback('#') succeeded, expected failure");
                }
                in.clear();
                for (i = 0; i < int(strlen(TEXT)); ++i) {
                        auto ch = in.get();
                        if (ch != TEXT[i]) {
                                throw TestFailure("in.get() returned '%c' on i=%d, expected '%c'",
                                                  ch, i, TEXT[0]);
                        }
                }
                for (--i; i >= 0; --i) {
                        if (!in.unget()) {
                                throw TestFailure("in.unget() failed on i=%d, expected success",
                                                  i);
                        }
                        auto ch = in.peek();
                        if (ch != TEXT[i]) {
                                throw TestFailure("in.peek() returned '%c' on i=%d, expected '%c'",
                                                  ch, i, TEXT[i]);
                        }                        
                }
                if (in.unget()) {
                        throw TestFailure("in.unget() at start of buffer succeeded, expected failure");
                }
        });

        tester.run("Write", 1, [] {
                static const char * const TEXT = "abc";
                char buf[8];
                std::fill_n(buf, wr::arraySize(buf), '\0');
                wr::arraystream out(buf, wr::arraySize(buf) - 1);
                out << TEXT;
                if (strcmp(buf, TEXT)) {
                        throw TestFailure("buf = \"%s\", expected \"%s\"",
                                          buf, TEXT);
                }
                out << TEXT << TEXT;
                if (strcmp(buf, "abcabca")) {
                        throw TestFailure("buf = \"%s\", expected \"%s\"",
                                          buf, "abcabca");
                }
        });

        return !tester.failed() ? EXIT_SUCCESS : EXIT_FAILURE;
}
