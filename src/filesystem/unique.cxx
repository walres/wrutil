/**
 * \file unique.cxx
 *
 * \brief Implementation of the wr::unique_path() filesystem functions
 *
 * \note This file is used only when the underlying filesystem library
 *       (boost::filesystem or C++ standard library) does not provide
 *       its own version of unique_path()
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
#include <wrutil/Config.h>
#if WR_POSIX
#       include <unistd.h>
#elif WR_WINAPI
#       include <windows.h>
#endif
#include <stdio.h>
#include <wchar.h>
#include <chrono>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <wrutil/SHA256.h>
#include "private.h"


namespace wr {


WRUTIL_API path
unique_path(
        const path &pattern
)
{
        fs_error_code ec;
        path result = unique_path(pattern, ec);
        if (ec) {
                throw filesystem_error("error obtaining unique path",
                                       pattern, ec);
        }
        return result;
}

//--------------------------------------

WRUTIL_API path
unique_path(
        const path    &pattern,
        fs_error_code &ec
)
{
        SHA256       hasher;
        std::wstring result = pattern.wstring(), hash_str(64, 0);

        std::thread::id this_thread_id = std::this_thread::get_id();
        hasher.append(&this_thread_id, sizeof(this_thread_id));

        auto pid =
#if WR_POSIX
                getpid()
#elif WR_WINAPI
                GetCurrentProcessId()
#endif
                ;
        hasher.append(&pid, sizeof(pid));

        const void *some_addr = &hasher;  // mix in a stack address
        hasher.append(&some_addr, sizeof(some_addr));

        // mix in a code address (take advantage of any address randomization)
        some_addr = reinterpret_cast<void *>(&path_has_prefix);
        hasher.append(&some_addr, sizeof(some_addr));

        // mix in address of hash_str's data (probably heap)
        some_addr = hash_str.data();
        hasher.append(&some_addr, sizeof(some_addr));

        for (auto i = result.begin(), j = result.end(); i < j; ) {
                auto time = std::chrono::high_resolution_clock::now()
                                                        .time_since_epoch();
                hasher.append(&time, sizeof(time));
                std::this_thread::yield();  // introduce some scheduler delay

                {
                        // mix in a heap address
                        std::unique_ptr<char> addr(new char);
                        some_addr = addr.get();
                        hasher.append(&some_addr, sizeof(some_addr));
                }

                time = std::chrono::high_resolution_clock::now()
                                                        .time_since_epoch();
                hasher.append(&time, sizeof(time));
#if WR_POSIX
                {
                        std::ifstream random("/dev/urandom");
                        char          bytes[16];
                        if (random.is_open()) {
                                random.read(bytes, sizeof(bytes));
                                hasher.append(bytes, sizeof(bytes));
                        }
                }
#endif
                time = std::chrono::high_resolution_clock::now()
                                                        .time_since_epoch();
                hasher.append(&time, sizeof(time));

                const SHA256::Hash &h = hasher.hash();
                swprintf(&hash_str[0], hash_str.size(),
                         L"%.08x%.08x%.08x%.08x%.08x%.08x%.08x%.08x",
                         h[0], h[1], h[2], h[3], h[4], h[5], h[6], h[7]);

                for (auto x = hash_str.begin(), y = hash_str.end();
                                                (i < j) && (x < y); ++i) {
                        if (*i == '%') {
                                *i = *x++;
                        }
                }

                if (i < j) {
                        hasher.append(hash_str.data(),
                                      sizeof(wchar_t) * hash_str.size());
                }
        }

        return result;
}


} // namespace wr
