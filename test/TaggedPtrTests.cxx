/**
 * \file TaggedPtrTests.cxx
 *
 * \brief Unit tests for `wr::tagged_ptr` class
 *
 * \copyright
 * \parblock
 *
 *   Copyright 2017 James S. Waller
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
#include <cstdint>
#include <wrutil/tagged_ptr.h>
#include <wrutil/TestManager.h>


int
main(
        int          argc,
        const char **argv
)
{
        wr::TestManager tests("tagged_ptr", argc, argv);

        using wr::TestFailure;

        tests.run("setPtr", 1, []{
                wr::tagged_ptr<void, 2> x;
                x.ptr(&x);
                x.tag(3);
                x.ptr(nullptr);
                if (x.tag() != 3) {
                        throw TestFailure("tagged_ptr::ptr(Pointee *) caused change to tag");
                }
        });

        tests.run("setPtr", 2, []{
                wr::tagged_ptr<void, 2> x;
                try {
                        x.ptr(reinterpret_cast<void *>(-1UL));
                        throw TestFailure("tagged_ptr::ptr(Pointee *) did not throw when given an incorrectly aligned pointer");
                } catch (std::invalid_argument &) {
                        // OK, expected
                }
        });

        return tests.failed() ? EXIT_FAILURE : EXIT_SUCCESS;
}
