//===-------------- char16.cxx (from libc++ original code) ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <wrutil/codecvt/char16.h>
#include <wrutil/codecvt/cvt_utf8.h>


namespace wr {


static std::codecvt_base::result
utf16_to_utf8(const uint16_t* frm, const uint16_t* frm_end,
              const uint16_t*& frm_nxt, uint8_t* to, uint8_t* to_end,
              uint8_t*& to_nxt, unsigned long Maxcode = 0x10ffff,
              codecvt_mode mode = static_cast<codecvt_mode>(0))
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
        uint16_t wc1 = *frm_nxt;
        if (wc1 > Maxcode)
            return std::codecvt_base::error;
        if (wc1 < 0x0080)
        {
            if (to_end-to_nxt < 1)
                return std::codecvt_base::partial;
            *to_nxt++ = static_cast<uint8_t>(wc1);
        }
        else if (wc1 < 0x0800)
        {
            if (to_end-to_nxt < 2)
                return std::codecvt_base::partial;
            *to_nxt++ = static_cast<uint8_t>(0xC0 | (wc1 >> 6));
            *to_nxt++ = static_cast<uint8_t>(0x80 | (wc1 & 0x03F));
        }
        else if (wc1 < 0xD800)
        {
            if (to_end-to_nxt < 3)
                return std::codecvt_base::partial;
            *to_nxt++ = static_cast<uint8_t>(0xE0 |  (wc1 >> 12));
            *to_nxt++ = static_cast<uint8_t>(0x80 | ((wc1 & 0x0FC0) >> 6));
            *to_nxt++ = static_cast<uint8_t>(0x80 |  (wc1 & 0x003F));
        }
        else if (wc1 < 0xDC00)
        {
            if (frm_end-frm_nxt < 2)
                return std::codecvt_base::partial;
            uint16_t wc2 = frm_nxt[1];
            if ((wc2 & 0xFC00) != 0xDC00)
                return std::codecvt_base::error;
            if (to_end-to_nxt < 4)
                return std::codecvt_base::partial;
            if (((((wc1 & 0x03C0UL) >> 6) + 1) << 16) +
                ((wc1 & 0x003FUL) << 10) + (wc2 & 0x03FF) > Maxcode)
                return std::codecvt_base::error;
            ++frm_nxt;
            uint8_t z = ((wc1 & 0x03C0) >> 6) + 1;
            *to_nxt++ = static_cast<uint8_t>(0xF0 | (z >> 2));
            *to_nxt++ = static_cast<uint8_t>(0x80 | ((z & 0x03) << 4)     | ((wc1 & 0x003C) >> 2));
            *to_nxt++ = static_cast<uint8_t>(0x80 | ((wc1 & 0x0003) << 4) | ((wc2 & 0x03C0) >> 6));
            *to_nxt++ = static_cast<uint8_t>(0x80 |  (wc2 & 0x003F));
        }
        else if (wc1 < 0xE000)
        {
            return std::codecvt_base::error;
        }
        else
        {
            if (to_end-to_nxt < 3)
                return std::codecvt_base::partial;
            *to_nxt++ = static_cast<uint8_t>(0xE0 |  (wc1 >> 12));
            *to_nxt++ = static_cast<uint8_t>(0x80 | ((wc1 & 0x0FC0) >> 6));
            *to_nxt++ = static_cast<uint8_t>(0x80 |  (wc1 & 0x003F));
        }
    }
    return std::codecvt_base::ok;
}

static std::codecvt_base::result
utf8_to_utf16(const uint8_t* frm, const uint8_t* frm_end,
              const uint8_t*& frm_nxt, uint16_t* to, uint16_t* to_end,
              uint16_t*& to_nxt, unsigned long Maxcode = 0x10ffff,
              codecvt_mode mode = static_cast<codecvt_mode>(0))
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
        uint8_t c1 = *frm_nxt;
        if (c1 > Maxcode)
            return std::codecvt_base::error;
        if (c1 < 0x80)
        {
            *to_nxt = static_cast<uint16_t>(c1);
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
            uint16_t t = static_cast<uint16_t>(((c1 & 0x1F) << 6) | (c2 & 0x3F));
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
            uint16_t t = static_cast<uint16_t>(((c1 & 0x0F) << 12)
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
            if (to_end-to_nxt < 2)
                return std::codecvt_base::partial;
            if ((((c1 & 7UL) << 18) +
                ((c2 & 0x3FUL) << 12) +
                ((c3 & 0x3FUL) << 6) + (c4 & 0x3F)) > Maxcode)
                return std::codecvt_base::error;
            *to_nxt = static_cast<uint16_t>(
                    0xD800
                  | (((((c1 & 0x07) << 2) | ((c2 & 0x30) >> 4)) - 1) << 6)
                  | ((c2 & 0x0F) << 2)
                  | ((c3 & 0x30) >> 4));
            *++to_nxt = static_cast<uint16_t>(
                    0xDC00
                  | ((c3 & 0x0F) << 6)
                  |  (c4 & 0x3F));
            frm_nxt += 4;
        }
        else
        {
            return std::codecvt_base::error;
        }
    }
    return frm_nxt < frm_end ? std::codecvt_base::partial
                             : std::codecvt_base::ok;
}

int
utf8_to_utf16_length(const uint8_t* frm, const uint8_t* frm_end,
                     size_t mx, unsigned long Maxcode = 0x10ffff,
                     codecvt_mode mode = static_cast<codecvt_mode>(0))
{
    const uint8_t* frm_nxt = frm;
    if (mode & consume_header)
    {
        if (frm_end-frm_nxt >= 3 && frm_nxt[0] == 0xEF && frm_nxt[1] == 0xBB &&
                                                          frm_nxt[2] == 0xBF)
            frm_nxt += 3;
    }
    for (size_t nchar16_t = 0; frm_nxt < frm_end && nchar16_t < mx; ++nchar16_t)
    {
        uint8_t c1 = *frm_nxt;
        if (c1 > Maxcode)
            break;
        if (c1 < 0x80)
        {
            ++frm_nxt;
        }
        else if (c1 < 0xC2)
        {
            break;
        }
        else if (c1 < 0xE0)
        {
            if ((frm_end-frm_nxt < 2) || (frm_nxt[1] & 0xC0) != 0x80)
                break;
            uint16_t t = static_cast<uint16_t>(((c1 & 0x1F) << 6) | (frm_nxt[1] & 0x3F));
            if (t > Maxcode)
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
            if (frm_end-frm_nxt < 4 || mx-nchar16_t < 2)
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
            if ((((c1 & 7UL) << 18) +
                ((c2 & 0x3FUL) << 12) +
                ((c3 & 0x3FUL) << 6) + (c4 & 0x3F)) > Maxcode)
                break;
            ++nchar16_t;
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


namespace std {


// template <> class codecvt<char16_t, char, mbstate_t>

WRUTIL_API locale::id codecvt<char16_t, char, mbstate_t>::id;

WRUTIL_API 
codecvt<char16_t, char, mbstate_t>::~codecvt()
{
}

WRUTIL_API codecvt<char16_t, char, mbstate_t>::result
codecvt<char16_t, char, mbstate_t>::do_out(state_type&,
    const intern_type* frm, const intern_type* frm_end, const intern_type*& frm_nxt,
    extern_type* to, extern_type* to_end, extern_type*& to_nxt) const
{
    const uint16_t* _frm = reinterpret_cast<const uint16_t*>(frm);
    const uint16_t* _frm_end = reinterpret_cast<const uint16_t*>(frm_end);
    const uint16_t* _frm_nxt = _frm;
    uint8_t* _to = reinterpret_cast<uint8_t*>(to);
    uint8_t* _to_end = reinterpret_cast<uint8_t*>(to_end);
    uint8_t* _to_nxt = _to;
    result r = wr::utf16_to_utf8(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt);
    frm_nxt = frm + (_frm_nxt - _frm);
    to_nxt = to + (_to_nxt - _to);
    return r;
}

WRUTIL_API codecvt<char16_t, char, mbstate_t>::result
codecvt<char16_t, char, mbstate_t>::do_in(state_type&,
    const extern_type* frm, const extern_type* frm_end, const extern_type*& frm_nxt,
    intern_type* to, intern_type* to_end, intern_type*& to_nxt) const
{
    const uint8_t* _frm = reinterpret_cast<const uint8_t*>(frm);
    const uint8_t* _frm_end = reinterpret_cast<const uint8_t*>(frm_end);
    const uint8_t* _frm_nxt = _frm;
    uint16_t* _to = reinterpret_cast<uint16_t*>(to);
    uint16_t* _to_end = reinterpret_cast<uint16_t*>(to_end);
    uint16_t* _to_nxt = _to;
    result r = wr::utf8_to_utf16(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt);
    frm_nxt = frm + (_frm_nxt - _frm);
    to_nxt = to + (_to_nxt - _to);
    return r;
}

WRUTIL_API codecvt<char16_t, char, mbstate_t>::result
codecvt<char16_t, char, mbstate_t>::do_unshift(state_type&,
    extern_type* to, extern_type*, extern_type*& to_nxt) const
{
    to_nxt = to;
    return noconv;
}

WRUTIL_API int
codecvt<char16_t, char, mbstate_t>::do_encoding() const noexcept
{
    return 0;
}

WRUTIL_API bool
codecvt<char16_t, char, mbstate_t>::do_always_noconv() const noexcept
{
    return false;
}

WRUTIL_API int
codecvt<char16_t, char, mbstate_t>::do_length(state_type&,
    const extern_type* frm, const extern_type* frm_end, size_t mx) const
{
    const uint8_t* _frm = reinterpret_cast<const uint8_t*>(frm);
    const uint8_t* _frm_end = reinterpret_cast<const uint8_t*>(frm_end);
    return wr::utf8_to_utf16_length(_frm, _frm_end, mx);
}

WRUTIL_API int
codecvt<char16_t, char, mbstate_t>::do_max_length() const noexcept
{
    return 4;
}


} // namespace std
