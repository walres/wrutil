//===------------- utf8_ucs4.cxx (from libc++ original code) --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "utf8_ucs4.h"


namespace wr {


std::codecvt_base::result
ucs4_to_utf8(const uint32_t* frm, const uint32_t* frm_end,
             const uint32_t*& frm_nxt, uint8_t* to, uint8_t* to_end,
             uint8_t*& to_nxt, unsigned long Maxcode, codecvt_mode mode)
{
    frm_nxt = frm;
    to_nxt = to;
    if (mode & generate_header)
    {
        if (to_end-to_nxt < 3)
            return std::codecvt_base::partial;
        *to_nxt++ = static_cast<uint8_t>(0xEF);
        *to_nxt++ = static_cast<uint8_t>(0xBB);
        *to_nxt++ = static_cast<uint8_t>(0xBF);
    }
    for (; frm_nxt < frm_end; ++frm_nxt)
    {
        uint32_t wc = *frm_nxt;
        if ((wc & 0xFFFFF800) == 0x00D800 || wc > Maxcode)
            return std::codecvt_base::error;
        if (wc < 0x000080)
        {
            if (to_end-to_nxt < 1)
                return std::codecvt_base::partial;
            *to_nxt++ = static_cast<uint8_t>(wc);
        }
        else if (wc < 0x000800)
        {
            if (to_end-to_nxt < 2)
                return std::codecvt_base::partial;
            *to_nxt++ = static_cast<uint8_t>(0xC0 | (wc >> 6));
            *to_nxt++ = static_cast<uint8_t>(0x80 | (wc & 0x03F));
        }
        else if (wc < 0x010000)
        {
            if (to_end-to_nxt < 3)
                return std::codecvt_base::partial;
            *to_nxt++ = static_cast<uint8_t>(0xE0 |  (wc >> 12));
            *to_nxt++ = static_cast<uint8_t>(0x80 | ((wc & 0x0FC0) >> 6));
            *to_nxt++ = static_cast<uint8_t>(0x80 |  (wc & 0x003F));
        }
        else // if (wc < 0x110000)
        {
            if (to_end-to_nxt < 4)
                return std::codecvt_base::partial;
            *to_nxt++ = static_cast<uint8_t>(0xF0 |  (wc >> 18));
            *to_nxt++ = static_cast<uint8_t>(0x80 | ((wc & 0x03F000) >> 12));
            *to_nxt++ = static_cast<uint8_t>(0x80 | ((wc & 0x000FC0) >> 6));
            *to_nxt++ = static_cast<uint8_t>(0x80 |  (wc & 0x00003F));
        }
    }
    return std::codecvt_base::ok;
}

std::codecvt_base::result
utf8_to_ucs4(const uint8_t* frm, const uint8_t* frm_end,
             const uint8_t*& frm_nxt, uint32_t* to, uint32_t* to_end,
             uint32_t*& to_nxt, unsigned long Maxcode, codecvt_mode mode)
{
    frm_nxt = frm;
    to_nxt = to;
    if (mode & consume_header)
    {
        if (frm_end-frm_nxt >= 3 && frm_nxt[0] == 0xEF && frm_nxt[1] == 0xBB &&
                                                          frm_nxt[2] == 0xBF)
            frm_nxt += 3;
    }
    for (; frm_nxt < frm_end && to_nxt < to_end; ++to_nxt)
    {
        uint8_t c1 = static_cast<uint8_t>(*frm_nxt);
        if (c1 < 0x80)
        {
            if (c1 > Maxcode)
                return std::codecvt_base::error;
            *to_nxt = static_cast<uint32_t>(c1);
            ++frm_nxt;
        }
        else if (c1 < 0xC2)
        {
            return std::codecvt_base::error;
        }
        else if (c1 < 0xE0)
        {
            if (frm_end-frm_nxt < 2)
                return std::codecvt_base::partial;
            uint8_t c2 = frm_nxt[1];
            if ((c2 & 0xC0) != 0x80)
                return std::codecvt_base::error;
            uint32_t t = static_cast<uint32_t>(((c1 & 0x1F) << 6)
                                              | (c2 & 0x3F));
            if (t > Maxcode)
                return std::codecvt_base::error;
            *to_nxt = t;
            frm_nxt += 2;
        }
        else if (c1 < 0xF0)
        {
            if (frm_end-frm_nxt < 3)
                return std::codecvt_base::partial;
            uint8_t c2 = frm_nxt[1];
            uint8_t c3 = frm_nxt[2];
            switch (c1)
            {
            case 0xE0:
                if ((c2 & 0xE0) != 0xA0)
                    return std::codecvt_base::error;
                 break;
            case 0xED:
                if ((c2 & 0xE0) != 0x80)
                    return std::codecvt_base::error;
                 break;
            default:
                if ((c2 & 0xC0) != 0x80)
                    return std::codecvt_base::error;
                 break;
            }
            if ((c3 & 0xC0) != 0x80)
                return std::codecvt_base::error;
            uint32_t t = static_cast<uint32_t>(((c1 & 0x0F) << 12)
                                             | ((c2 & 0x3F) << 6)
                                             |  (c3 & 0x3F));
            if (t > Maxcode)
                return std::codecvt_base::error;
            *to_nxt = t;
            frm_nxt += 3;
        }
        else if (c1 < 0xF5)
        {
            if (frm_end-frm_nxt < 4)
                return std::codecvt_base::partial;
            uint8_t c2 = frm_nxt[1];
            uint8_t c3 = frm_nxt[2];
            uint8_t c4 = frm_nxt[3];
            switch (c1)
            {
            case 0xF0:
                if (!(0x90 <= c2 && c2 <= 0xBF))
                    return std::codecvt_base::error;
                 break;
            case 0xF4:
                if ((c2 & 0xF0) != 0x80)
                    return std::codecvt_base::error;
                 break;
            default:
                if ((c2 & 0xC0) != 0x80)
                    return std::codecvt_base::error;
                 break;
            }
            if ((c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80)
                return std::codecvt_base::error;
            uint32_t t = static_cast<uint32_t>(((c1 & 0x07) << 18)
                                             | ((c2 & 0x3F) << 12)
                                             | ((c3 & 0x3F) << 6)
                                             |  (c4 & 0x3F));
            if (t > Maxcode)
                return std::codecvt_base::error;
            *to_nxt = t;
            frm_nxt += 4;
        }
        else
        {
            return std::codecvt_base::error;
        }
    }
    return frm_nxt < frm_end ? std::codecvt_base::partial : std::codecvt_base::ok;
}

int
utf8_to_ucs4_length(const uint8_t* frm, const uint8_t* frm_end,
                    size_t mx, unsigned long Maxcode, codecvt_mode mode)
{
    const uint8_t* frm_nxt = frm;
    if (mode & consume_header)
    {
        if (frm_end-frm_nxt >= 3 && frm_nxt[0] == 0xEF && frm_nxt[1] == 0xBB &&
                                                          frm_nxt[2] == 0xBF)
            frm_nxt += 3;
    }
    for (size_t nchar32_t = 0; frm_nxt < frm_end && nchar32_t < mx; ++nchar32_t)
    {
        uint8_t c1 = static_cast<uint8_t>(*frm_nxt);
        if (c1 < 0x80)
        {
            if (c1 > Maxcode)
                break;
            ++frm_nxt;
        }
        else if (c1 < 0xC2)
        {
            break;
        }
        else if (c1 < 0xE0)
        {
            if ((frm_end-frm_nxt < 2) || ((frm_nxt[1] & 0xC0) != 0x80))
                break;
            if ((((c1 & 0x1Fu) << 6) | (frm_nxt[1] & 0x3Fu)) > Maxcode)
                break;
            frm_nxt += 2;
        }
        else if (c1 < 0xF0)
        {
            if (frm_end-frm_nxt < 3)
                break;
            uint8_t c2 = frm_nxt[1];
            uint8_t c3 = frm_nxt[2];
            switch (c1)
            {
            case 0xE0:
                if ((c2 & 0xE0) != 0xA0)
                    return static_cast<int>(frm_nxt - frm);
                break;
            case 0xED:
                if ((c2 & 0xE0) != 0x80)
                    return static_cast<int>(frm_nxt - frm);
                 break;
            default:
                if ((c2 & 0xC0) != 0x80)
                    return static_cast<int>(frm_nxt - frm);
                 break;
            }
            if ((c3 & 0xC0) != 0x80)
                break;
            if ((((c1 & 0x0Fu) << 12) | ((c2 & 0x3Fu) << 6) | (c3 & 0x3Fu)) > Maxcode)
                break;
            frm_nxt += 3;
        }
        else if (c1 < 0xF5)
        {
            if (frm_end-frm_nxt < 4)
                break;
            uint8_t c2 = frm_nxt[1];
            uint8_t c3 = frm_nxt[2];
            uint8_t c4 = frm_nxt[3];
            switch (c1)
            {
            case 0xF0:
                if (!(0x90 <= c2 && c2 <= 0xBF))
                    return static_cast<int>(frm_nxt - frm);
                 break;
            case 0xF4:
                if ((c2 & 0xF0) != 0x80)
                    return static_cast<int>(frm_nxt - frm);
                 break;
            default:
                if ((c2 & 0xC0) != 0x80)
                    return static_cast<int>(frm_nxt - frm);
                 break;
            }
            if ((c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80)
                break;
            if ((((c1 & 0x07u) << 18) | ((c2 & 0x3Fu) << 12) |
                 ((c3 & 0x3Fu) << 6)  |  (c4 & 0x3Fu)) > Maxcode)
                break;
            frm_nxt += 4;
        }
        else
        {
            break;
        }
    }
    return static_cast<int>(frm_nxt - frm);
}


} // namespace wr
