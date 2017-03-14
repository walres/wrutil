/**
 * \file SHA256.h
 *
 * \brief SHA256 hashing API
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
#ifndef WRUTIL_SHA256_H
#define WRUTIL_SHA256_H

#include <string.h>
#include <array>
#include <cstdint>
#include <string>

#include <wrutil/Config.h>
#include <wrutil/string_view.h>


namespace wr {


class WRUTIL_API SHA256
{
public:
        using Hash = std::array<uint32_t, 8>;

        SHA256() { reset(); }

        SHA256 &append(const void *data, size_t size);

        SHA256 &append(const string_view &str)
                { return append(str.data(), str.size()); }

        const Hash &hash();
        Hash chash() const;
        SHA256 &reset();

        SHA256 &operator+=(const string_view &str) { return append(str); }

        static std::string toString(const Hash &h);
        static Hash toHash(const string_view &str);

private:
        static Hash &hash(Hash &h, uint32_t (&w)[64], size_t total_length);
        static void computeBlock(Hash &h, uint32_t (&w)[64]);

        static const uint32_t K_[64];

        Hash     H_;
        uint32_t W_[64];
        size_t   total_length_;
};


} // namespace wr


#endif // !WRUTIL_SHA256_H
