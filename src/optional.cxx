//===------------- optional.cxx (from libc++ original code) ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <wrutil/optional.h>


namespace wr {


WRUTIL_API bad_optional_access::~bad_optional_access() noexcept {}

WRUTIL_API const char *bad_optional_access::what() const noexcept
        { return "null optional object"; }


} // namespace wr
