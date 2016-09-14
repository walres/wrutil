/**
 * \file SHA256.cxx
 *
 * \brief SHA-256 hashing implementation
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
#include <algorithm>
#include <stdexcept>
#include <wrutil/Config.h>

#if WR_WINAPI
#       include <winsock2.h>
#else
#       include <arpa/inet.h>
#endif

#include <wrutil/Format.h>
#include <wrutil/SHA256.h>


namespace wr {


const uint32_t SHA256::K_[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

//--------------------------------------

WRUTIL_API SHA256 &
SHA256::append(
        const void *data,
        size_t      size
)
{
        while (size) {
                size_t pos = size_t(total_length_ & 63),
                       bytes_this_block = 64 - pos;

                if (size < bytes_this_block) {
                        memcpy(reinterpret_cast<uint8_t *>(W_) + pos, data,
                               size);
                        total_length_ += size;
                        break;
                } else {
                        memcpy(reinterpret_cast<uint8_t *>(W_) + pos, data,
                               bytes_this_block);
                        computeBlock(H_, W_);
                        size -= bytes_this_block;
                        total_length_ += bytes_this_block;
                }
        }

        return *this;
}

//--------------------------------------

WRUTIL_API const SHA256::Hash &
SHA256::hash()
{
        return hash(H_, W_, total_length_);
}

//--------------------------------------

WRUTIL_API SHA256::Hash
SHA256::chash() const
{
        Hash     h = H_;
        uint32_t w[64];
        std::copy_n(W_, 64, w);
        hash(h, w, total_length_);
        return h;
}

//--------------------------------------

SHA256::Hash &
SHA256::hash(
        Hash      &h,
        uint32_t (&w)[64],
        size_t     total_length
) // static
{
        size_t pos = size_t(total_length & 63),
               pad = 64 - pos - 1;

        auto dst = reinterpret_cast<uint8_t *>(w) + pos;
        *dst = 0x80;
        ++dst;

        if (pad > 8) {
                memset(dst, 0, pad - 8);
        } else if (pad < 8) {
                memset(dst, 0, pad);
                computeBlock(h, w);
                dst = reinterpret_cast<uint8_t *>(w);
                pos = 0;
                memset(dst, 0, pad = 56);
        }

        uint64_t total_length_bits = uint64_t(total_length) << 3;

        if (total_length_bits < total_length) {
                throw std::overflow_error("message too long");
        }

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        reinterpret_cast<uint64_t *>(w)[7] = total_length_bits;
#else
        w[15] = ntohl(uint32_t(total_length_bits));
        w[14] = ntohl(uint32_t(total_length_bits >>= 32));
#endif
        computeBlock(h, w);
        return h;
}

//--------------------------------------

WRUTIL_API SHA256 &
SHA256::reset()
{
        total_length_ = 0;
        H_ = {{ 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 }};
        return *this;
}

//--------------------------------------

inline uint32_t rotateRight(uint32_t x, unsigned bits)
        { return (x >> bits) | (x << (32 - bits)); }

inline uint32_t Ch(uint32_t x, uint32_t y, uint32_t z)
        { return (x & y) ^ ((~x) & z); }

inline uint32_t Maj(uint32_t x, uint32_t y, uint32_t z)
        { return (x & y) ^ (x & z) ^ (y & z); }

inline uint32_t SIGMA0(uint32_t x)
        { return rotateRight(x, 2) ^ rotateRight(x, 13) ^ rotateRight(x, 22); }

inline uint32_t SIGMA1(uint32_t x)
        { return rotateRight(x, 6) ^ rotateRight(x, 11) ^ rotateRight(x, 25); }

inline uint32_t sigma0(uint32_t x)
        { return rotateRight(x, 7) ^ rotateRight(x, 18) ^ (x >> 3); }

inline uint32_t sigma1(uint32_t x)
        { return rotateRight(x, 17) ^ rotateRight(x, 19) ^ (x >> 10); }

//--------------------------------------

void
SHA256::computeBlock(
        Hash      &h,
        uint32_t (&w)[64]
) // static
{
#if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
        for (size_t i = 0; i < 16; ++i) {
                w[i] = htonl(w[i]);
        }
#endif
        for (size_t j = 16; j < 64; ++j) {
                w[j] = sigma1(w[j - 2]) + w[j - 7] + sigma0(w[j - 15])
                                        + w[j - 16];
        }

        union
        {
                struct
                {
                        uint32_t a, b, c, d, e, f, g, h;
                };
                Hash hash;
        } reg;

        reg.hash = h;

        for (size_t j = 0; j < 64; ++j) {
                uint32_t T1 = reg.h + SIGMA1(reg.e) + Ch(reg.e, reg.f, reg.g)
                                    + K_[j] + w[j],
                         T2 = SIGMA0(reg.a) + Maj(reg.a, reg.b, reg.c);
                reg.h = reg.g;
                reg.g = reg.f;
                reg.f = reg.e;
                reg.e = reg.d + T1;
                reg.d = reg.c;
                reg.c = reg.b;
                reg.b = reg.a;
                reg.a = T1 + T2;
        }

        for (size_t i = 0; i < reg.hash.size(); ++i) {
                h[i] += reg.hash[i];
        }
}

//--------------------------------------

WRUTIL_API std::string
SHA256::toString(
        const Hash &h
)
{
        
        return printStr("%.08x%.08x%.08x%.08x%.08x%.08x%.08x%.08x",
                        h[0], h[1], h[2], h[3], h[4], h[5], h[6], h[7]);
}

//--------------------------------------

SHA256::Hash
WRUTIL_API SHA256::toHash(
        const string_view &str
)
{
        Hash      h;
        uint32_t *element;
        size_t    i, n = std::min(str.size(), size_t(64));

        for (i = 0, element = &h[-1]; i < n; ++i) {
                if (i & 7) {
                        *element <<= 4;
                } else {  // i is a multiple of 8
                        *(++element) = 0;
                }

                char ch = str[i];

                if (isdigit(ch)) {
                        *element |= ch - '0';
                } else {
                        ch = tolower(ch);
                        if ((ch >= 'a') && (ch <= 'f')) {
                                *element |= ch - 'a';
                        }
                }
        }

        return h;
}


} // namespace wr
