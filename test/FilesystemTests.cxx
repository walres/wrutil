/**
 * \file FilesystemTests.cxx
 *
 * \brief Unit tests for filesystem classes and functions
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
#include <stdlib.h>
#include <wrutil/debug.h>  // add wrdebug library dependency
#include <wrutil/filesystem.h>
#include <wrutil/TestManager.h>


using wr::TestFailure;


int
main(
        int          argc,
        const char **argv
)
{
        wr::TestManager tester("filesystem", argc, argv);

        tester.run("lexically_normal", 1, [] {
                wr::path input ("/usr/lib64/gcc/x86_64-slackware-linux/4.8.2/../../../../x86_64-slackware-linux/include"),
                         normal(wr::lexically_normal(input)),
                         expect("/usr/x86_64-slackware-linux/include");

                if (normal != expect) {
                        throw TestFailure("lexically_normal(%s) returned \"%s\", expected \"%s\"",
                                          input, normal, expect);
                }
        });

        tester.run("lexically_normal", 2, [] {
                wr::path input ("/../test/canonpath"),
                         normal(wr::lexically_normal(input)),
                         expect("/test/canonpath");

                if (normal != expect) {
                        throw TestFailure("lexically_normal(%s) returned \"%s\", expected \"%s\"",
                                          input, normal, expect);
                }
        });

        tester.run("weakly_canonical", 1, [] {
                wr::path input  = wr::current_path().root_name()
                                  / wr::u8path(u8"/does/not/exist"),
                         wcanon = wr::weakly_canonical(input);

                if (wcanon != input) {
                        throw TestFailure("weakly_canonical(%s) returned \"%s\", expected \"%s\"",
                                          input, wcanon, input);
                }
        });

        tester.run("path_has_prefix", 1, [] {
                wr::path p1("one/two/three"), p2("one/two");
                if (!wr::path_has_prefix(p1, p2)) {
                        throw TestFailure("path_has_prefix(\"%s\", \"%s\") returned false, expected true",
                                          p1, p2);
                }
        });

        tester.run("path_has_prefix", 2, [] {
                wr::path p1("one/two"), p2("one/two/three");
                if (wr::path_has_prefix(p1, p2)) {
                        throw TestFailure("path_has_prefix(\"%s\", \"%s\") returned true, expected false",
                                          p1, p2);
                }
        });

        return tester.failed() ? EXIT_FAILURE : EXIT_SUCCESS;
}
