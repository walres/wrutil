/**
 * \file CircFwdListTests.cxx
 *
 * \brief Unit tests for intrusive (intrusive_circ_fwd_list) and
 *        nonintrusive (circ_fwd_list) class templates and operators
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
#include <memory>
#include <string>
#include <wrutil/circ_fwd_list.h>
#include <wrutil/debug.h>  // add wrdebug library dependency
#include <wrutil/Format.h>
#include <wrutil/string_view.h>
#include <wrutil/TestManager.h>

/*
 * Test case sources: www.cppreference.com
 */
template <typename ListType>
struct ListFormatter
{
        static void set(wr::fmt::Arg &arg, const ListType &val)
        {
                arg.type = wr::fmt::Arg::OTHER_T;
                arg.other = &val;
                arg.fmt_fn = &ListFormatter::format;
        }

        static bool format(const wr::fmt::Params &parms)
        {
                if (parms.conv != 's') {
                        errno = EINVAL;
                        return false;
                }

                std::string buf("[");
                const char *sep = "";
                auto &l = *static_cast<const ListType *>(parms.arg->other);

                for (const auto &i: l) {
                        buf += wr::printStr("%s%s", sep, i);
                        sep = ",";
                }

                buf += ']';
                wr::fmt::Arg arg2;
                arg2.type = wr::fmt::Arg::STR_T;
                arg2.s = { buf.data(), buf.size() };

                return parms.target.format(parms, &arg2);
        }
};

//--------------------------------------

struct node
{
        int x_;
        node *next_;

        node() = default;
        node(int x) : x_(x) {}
        node *next() { return next_; }
        void next(node *n) { next_ = n; }
        bool operator==(const node &n) const
                { return x_ == n.x_; }
        bool operator!=(const node &n) const
                { return x_ != n.x_; }
};

//--------------------------------------

namespace wr {
namespace fmt {


template <typename T, typename Alloc>
struct TypeHandler<circ_fwd_list<T, Alloc>> :
        ListFormatter<circ_fwd_list<T, Alloc>>
{
};

//--------------------------------------

template <typename T, typename Alloc>
struct TypeHandler<intrusive_circ_fwd_list<T, Alloc>> :
        ListFormatter<intrusive_circ_fwd_list<T, Alloc>>
{
};

//--------------------------------------

template <> struct TypeHandler<node>
{
        static void set(Arg &arg, const node &n)
                { arg.type = Arg::INT_T; arg.i = n.x_; }
};


} // namespace fmt
} // namespace wr

//--------------------------------------

int
main(
        int          argc,
        const char **argv
)
{
        using wr::circ_fwd_list;
        using wr::intrusive_circ_fwd_list;
        using wr::TestFailure;

        wr::TestManager tester("CircularList", argc, argv);

        tester.run("Construct", 1, [] {
                circ_fwd_list<std::string> words1
                        = { "the", "frogurt", "is", "also", "cursed" };

                static const char * const EXPECTED_OUTPUT
                                          = "[the,frogurt,is,also,cursed]";

                std::string s(wr::printStr("%s", words1));

                if (s != EXPECTED_OUTPUT) {
                        throw TestFailure("words1 = %s, expected %s",
                                          s, EXPECTED_OUTPUT);
                } else if (words1.front() != "the") {
                        throw TestFailure("front() returned \"%s\", expected \"%s\"",
                                          words1.front(), "the");
                } else if (words1.back() != "cursed") {
                        throw TestFailure("back() returned \"%s\", expected \"%s\"",
                                          words1.back(), "cursed");
                }

                circ_fwd_list<std::string> words2(words1.begin(), words1.end());
                if (words2 != words1) {
                        throw TestFailure("words2 != words1");
                }

                circ_fwd_list<std::string> words3(words1);
                if (words3 != words1) {
                        throw TestFailure("words3 != words1");
                }

                circ_fwd_list<std::string> words4(5, "Mo");
                size_t i = 0;
                for (auto &s: words4) {
                        if (s != "Mo") {
                                throw TestFailure("words4 = %s, expected [Mo,Mo,Mo,Mo,Mo]",
                                                  words4);
                        }
                        ++i;
                }
                if (i != 5) {
                        throw TestFailure("i = %u, expected 5", i);
                }
        });

        tester.run("Compare", 1, [] {
                circ_fwd_list<int> l1 = { 1, 2 }, l2 = { 1, 3 }, l3 = { 1, 2 },
                                   l4 = { 1, 2, 3 }, l5 = { 1 }, l6 = {},
                                   l7 = {};
                if (l1 == l2) {
                        throw TestFailure("(l1 == l2) returned true, expected false");
                } else if (l1 == l3) {
                        if (l1 == l4) {
                                throw TestFailure("(l1 == l4) returned true, expected false");
                        } else if (l1 == l5) {
                                throw TestFailure("(l1 == l5) returned true, expected false");
                        } else if (l1 == l6) {
                                throw TestFailure("(l1 == l6) returned true, expected false");
                        } else if (l6 == l7) {
                                ;
                        } else {
                                throw TestFailure("(l6 == l7) returned false, expected true");
                        }
                } else {
                        throw TestFailure("(l1 == l3) returned false, expected true");
                }
        });

        tester.run("Compare", 2, [] {
                circ_fwd_list<int> l1 = { 1, 2 }, l2 = { 1, 3 }, l3 = { 1, 2 },
                                   l4 = { 1, 2, 3 }, l5 = { 1 }, l6 = {},
                                   l7 = {};
                if (l1 != l2) {
                        if (l1 != l3) {
                                throw TestFailure("(l1 != l3) returned true, expected false");
                        } else if (l1 != l4) {
                                if (l1 != l5) {
                                        ;
                                } else {
                                        throw TestFailure("(l1 != l5) returned false, expected true");
                                }
                        } else {
                                throw TestFailure("(l1 != l4) returned false, expected true");
                        }
                } else {
                        throw TestFailure("(l1 != l2) returned false, expected true");
                }

                if (l1 != l6) {
                        if (l6 != l7) {
                                throw TestFailure("(l6 != l7) returned true, expected false");
                        }
                } else {
                        throw TestFailure("(l1 != l6) returned false, expected true");
                }
        });

        tester.run("AssignOp", 1, [] {
                circ_fwd_list<int> nums1 { 3, 1, 4, 6, 5, 9 }, nums2, nums3;
                nums2 = nums1;

                if (nums2 != nums1) {
                        throw TestFailure("nums2 != nums1");
                }

                circ_fwd_list<int>::difference_type
                        size1 = std::distance(nums1.begin(), nums1.end()),
                        size2 = std::distance(nums2.begin(), nums2.end()),
                        size3 = std::distance(nums3.begin(), nums3.end());

                if (size1 != 6) {
                        throw TestFailure("size1 = %u, expected 6", size1);
                } else if (size2 != size1) {
                        throw TestFailure("size2 = %u, expected %u",
                                          size2, size1);
                } else if (size3 != 0) {
                        throw TestFailure("size3 = %u, expected 0", size3);
                }

                nums3 = std::move(nums1);
                size1 = std::distance(nums1.begin(), nums1.end());
                size3 = std::distance(nums3.begin(), nums3.end());

                if (size1 != 0) {
                        throw TestFailure("size1 = %u, expected 0", size1);
                } else if (size3 != 6) {
                        throw TestFailure("size3 = %u, expected 6", size3);
                } else if (nums3 != nums2) {
                        throw TestFailure("nums3 != nums2");
                }
        });

        tester.run("EraseAfter", 1, [] {
                circ_fwd_list<int> l         = { 1, 2, 3, 4, 5, 6, 7, 8, 9 },
                                   expected1 = { 2, 3, 4, 5, 6, 7, 8, 9 },
                                   expected2 = { 2, 3, 6, 7, 8, 9 };
                l.erase_after(l.before_begin());
                if (l != expected1) {
                        throw TestFailure("l = %s, expected %s", l, expected1);
                }
                auto first = std::next(l.begin()), last = std::next(first, 3);
                l.erase_after(first, last);
                if (l != expected2) {
                        throw TestFailure("l = %s, expected %s", l, expected2);
                }
        });

        tester.run("EraseAfter", 2, [] {
                circ_fwd_list<int> l = { 1, 2, 3 }, expected = l;
                l.erase_after(l.before_begin(), l.begin());
                if (l != expected) {
                        throw TestFailure("l = %s, expected %s", l, expected);
                }
        });

        tester.run("PopFront", 1, [] {
                circ_fwd_list<int> l = { 1, 2 }, expected = { 2 };
                l.pop_front();
                if (l != expected) {
                        throw TestFailure("l = %s, expected %s", l, expected);
                }
        });

        tester.run("PopFront", 2, [] {
                circ_fwd_list<int> l = { 1 };
                l.pop_front();
                if (!l.empty()) {
                        throw TestFailure("l = %s, expected empty", l);
                }
        });

        tester.run("DetachFront", 1, [] {
                intrusive_circ_fwd_list<node> l = { 1, 2 },
                                              expected = { 2 };
                std::unique_ptr<node> detached(l.detach_front());
                if (l != expected) {
                        throw TestFailure("l = %s, expected %s", l, expected);
                } else if (!detached) {
                        throw TestFailure("detach_front() returned null");
                } else if (detached->x_ != 1) {
                        throw TestFailure("detached wrong node %d, expected 1",
                                          detached->x_);
                }
        });

        tester.run("DetachFront", 2, [] {
                intrusive_circ_fwd_list<node> l = { 1 };
                std::unique_ptr<node> detached(l.detach_front());
                if (!l.empty()) {
                        throw TestFailure("l = %s, expected empty", l);
                } else if (!detached) {
                        throw TestFailure("detach_front() returned null");
                } else if (detached->x_ != 1) {
                        throw TestFailure("detached wrong node %d, expected 1",
                                          detached->x_);
                }
        });

        tester.run("SpliceAfter", 1, [] {
                circ_fwd_list<int> l1 = { 1, 2, 3, 4, 5 }, l2 = { 10, 11, 12 },
                                   expected1 = { 1 },
                                   expected2 = { 10, 2, 3, 4, 5, 11, 12 };
                l2.splice_after(l2.cbegin(), l1, l1.cbegin(), l1.cend());
                if (l1 != expected1) {
                        throw TestFailure("l1 = %s, expected %s",
                                          l1, expected1);
                }
                if (l2 != expected2) {
                        throw TestFailure("l2 = %s, expected %s",
                                          l2, expected2);
                }
        });

        tester.run("SpliceAfter", 2, [] {
                circ_fwd_list<int> l1 = { 1, 2, 3, 4, 5 }, l2 = { 10, 11, 12 },
                                   expected = { 1, 2, 3, 4, 5, 10, 11, 12 };
                l2.splice_after(l2.cbefore_begin(), l1);
                if (l2 != expected) {
                        throw TestFailure("l2 = %s, expected %s", l2, expected);
                }
        });

        tester.run("Remove", 1, [] {
                circ_fwd_list<int> l = { 1, 100, 2, 3, 10, 1, 11, -1, 12 },
                        expected1 = { 100, 2, 3, 10, 11, -1, 12 },
                        expected2 = { 2, 3, 10, -1 };
                l.remove(1);
                if (l != expected1) {
                        throw TestFailure("l = %s, expected %s", l, expected1);
                }
                l.remove_if([](int n) { return n > 10; });
                if (l != expected2) {
                        throw TestFailure("l = %s, expected %s", l, expected2);
                }
        });

        tester.run("Sort", 1, [] {
                circ_fwd_list<int> l         = { 8, 7, 5, 9, 0, 1, 3, 2, 6, 4 },
                                   expected1 = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
                                   expected2 = { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
                l.sort();
                if (l != expected1) {
                        throw TestFailure("l = %s, expected %s", l, expected1);
                }
                l.sort(std::greater<int>());
                if (l != expected2) {
                        throw TestFailure("l = %s, expected %s", l, expected2);
                }
        });

        tester.run("Reverse", 1, [] {
                circ_fwd_list<int> l         = { 8, 7, 5, 9, 0, 1, 3, 2, 6, 4 },
                                   expected1 = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 },
                                   expected2 = { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
                l.sort();
                if (l != expected1) {
                        throw TestFailure("l = %s, expected %s", l, expected1);
                }
                l.reverse();
                if (l != expected2) {
                        throw TestFailure("l = %s, expected %s", l, expected2);
                }
        });

        tester.run("Merge", 1, [] {
                circ_fwd_list<int> l1 = { 5, 9, 0, 1, 3 },
                                   l2 = { 8, 7, 2, 6, 4 },
                                   expected1 = { 0, 1, 3, 5, 9 },
                                   expected2 = { 2, 4, 6, 7, 8 },
                                   expected3 = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
                l1.sort();
                if (l1 != expected1) {
                        throw TestFailure("l1 = %s, expected %s",
                                          l1, expected1);
                }
                l2.sort();
                if (l2 != expected2) {
                        throw TestFailure("l1 = %s, expected %s",
                                          l1, expected2);
                }
                l1.merge(l2);
                if (l1 != expected3) {
                        throw TestFailure("l1 = %s, expected %s",
                                          l1, expected3);
                }
        });

        tester.run("Unique", 1, [] {
                circ_fwd_list<int> l        = { 1, 2, 2, 3, 3, 2, 1, 1, 2 },
                                   expected = { 1, 2, 3, 2, 1, 2 };
                l.unique();
                if (l != expected) {
                        throw TestFailure("l = %s, expected %s", l, expected);
                }
        });

        tester.run("IterWrap", 1, [] {
                std::string                  s;
                circ_fwd_list<int>           l        = { 1, 2, 3 };
                static const wr::string_view expected = "1 2 3 1 2 3 ";

                auto i = l.begin();
                for (int n = 2; n > 0; --n, ++i) {
                        for (; i != l.end(); ++i) {
                                s += std::to_string(*i);
                                s += ' ';
                        }
                }

                if (s != expected) {
                        throw TestFailure("s = \"%s\", expected \"%s\"",
                                          s, expected);
                }
        });

        tester.run("IterWrap", 2, [] {  // test iterator wrap on empty list
                circ_fwd_list<int> l = {};

                auto i = l.begin();
                for (int n = 2; n > 0; --n, ++i) {
                        while (i != l.end()) {
                                ++i;
                        }
                }

                if (i != l.begin()) {
                        throw TestFailure("i != l.begin()");
                }
        });

        return !tester.failed() ? EXIT_SUCCESS : EXIT_FAILURE;
}
