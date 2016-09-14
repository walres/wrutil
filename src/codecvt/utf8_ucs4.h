/**
 * \file utf8_ucs4.cxx
 *
 * \brief Shared declarations for internal UTF-8 to UCS-4 transcoding
 *        functions
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
#ifndef WRUTIL_CODECVT_PRIVATE_H
#define WRUTIL_CODECVT_PRIVATE_H

#include <stdint.h>
#include <locale>
#include <wrutil/codecvt/cvt_utf8.h>


namespace wr {


std::codecvt_base::result
ucs4_to_utf8(const uint32_t *frm, const uint32_t *frm_end,
             const uint32_t *&frm_nxt, uint8_t *to, uint8_t *to_end,
             uint8_t *&to_nxt, unsigned long Maxcode = 0x10ffff,
             codecvt_mode mode = static_cast<codecvt_mode>(0));

std::codecvt_base::result
utf8_to_ucs4(const uint8_t *frm, const uint8_t *frm_end,
             const uint8_t *&frm_nxt, uint32_t *to, uint32_t *to_end,
             uint32_t *&to_nxt, unsigned long Maxcode = 0x10ffff,
             codecvt_mode mode = static_cast<codecvt_mode>(0));

int
utf8_to_ucs4_length(const uint8_t *frm, const uint8_t *frm_end,
                    size_t mx, unsigned long Maxcode = 0x10ffff,
                    codecvt_mode mode = static_cast<codecvt_mode>(0));


} // namespace wr


#endif // !WRUTIL_CODECVT_PRIVATE_H
