//===------------- cvt_utf8.cxx (from libc++ original code) ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <wrutil/codecvt/cvt_utf8.h>
#include "utf8_ucs4.h"


namespace wr {


std::codecvt_base::result
ucs2_to_utf8(const uint16_t* frm, const uint16_t* frm_end,
             const uint16_t*& frm_nxt, uint8_t* to, uint8_t* to_end,
             uint8_t*& to_nxt, unsigned long Maxcode = 0x10FFFF,
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
        uint16_t wc = *frm_nxt;
        if ((wc & 0xF800) == 0xD800 || wc > Maxcode)
            return std::codecvt_base::error;
        if (wc < 0x0080)
        {
            if (to_end-to_nxt < 1)
                return std::codecvt_base::partial;
            *to_nxt++ = static_cast<uint8_t>(wc);
        }
        else if (wc < 0x0800)
        {
            if (to_end-to_nxt < 2)
                return std::codecvt_base::partial;
            *to_nxt++ = static_cast<uint8_t>(0xC0 | (wc >> 6));
            *to_nxt++ = static_cast<uint8_t>(0x80 | (wc & 0x03F));
        }
        else // if (wc <= 0xFFFF)
        {
            if (to_end-to_nxt < 3)
                return std::codecvt_base::partial;
            *to_nxt++ = static_cast<uint8_t>(0xE0 |  (wc >> 12));
            *to_nxt++ = static_cast<uint8_t>(0x80 | ((wc & 0x0FC0) >> 6));
            *to_nxt++ = static_cast<uint8_t>(0x80 |  (wc & 0x003F));
        }
    }
    return std::codecvt_base::ok;
}

std::codecvt_base::result
utf8_to_ucs2(const uint8_t* frm, const uint8_t* frm_end,
             const uint8_t*& frm_nxt, uint16_t* to, uint16_t* to_end,
             uint16_t*& to_nxt, unsigned long Maxcode = 0x10FFFF,
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
        uint8_t c1 = static_cast<uint8_t>(*frm_nxt);
        if (c1 < 0x80)
        {
            if (c1 > Maxcode)
                return std::codecvt_base::error;
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
            uint16_t t = static_cast<uint16_t>(((c1 & 0x1F) << 6)
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
            uint16_t t = static_cast<uint16_t>(((c1 & 0x0F) << 12)
                                             | ((c2 & 0x3F) << 6)
                                             |  (c3 & 0x3F));
            if (t > Maxcode)
                return std::codecvt_base::error;
            *to_nxt = t;
            frm_nxt += 3;
        }
        else
        {
            return std::codecvt_base::error;
        }
    }
    return frm_nxt < frm_end ? std::codecvt_base::partial : std::codecvt_base::ok;
}

int
utf8_to_ucs2_length(const uint8_t* frm, const uint8_t* frm_end,
                    size_t mx, unsigned long Maxcode = 0x10FFFF,
                    codecvt_mode mode = static_cast<codecvt_mode>(0))
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
        else
        {
            break;
        }
    }
    return static_cast<int>(frm_nxt - frm);
}


// __codecvt_utf8<wchar_t>

WRUTIL_API __codecvt_utf8<wchar_t>::result
__codecvt_utf8<wchar_t>::do_out(state_type&,
    const intern_type* frm, const intern_type* frm_end, const intern_type*& frm_nxt,
    extern_type* to, extern_type* to_end, extern_type*& to_nxt) const
{
#if _WIN32
    const uint16_t* _frm = reinterpret_cast<const uint16_t*>(frm);
    const uint16_t* _frm_end = reinterpret_cast<const uint16_t*>(frm_end);
    const uint16_t* _frm_nxt = _frm;
#else
    const uint32_t* _frm = reinterpret_cast<const uint32_t*>(frm);
    const uint32_t* _frm_end = reinterpret_cast<const uint32_t*>(frm_end);
    const uint32_t* _frm_nxt = _frm;
#endif
    uint8_t* _to = reinterpret_cast<uint8_t*>(to);
    uint8_t* _to_end = reinterpret_cast<uint8_t*>(to_end);
    uint8_t* _to_nxt = _to;
#if _WIN32
    result r = ucs2_to_utf8(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt,
                            _Maxcode_, _Mode_);
#else
    result r = ucs4_to_utf8(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt,
                            _Maxcode_, _Mode_);
#endif
    frm_nxt = frm + (_frm_nxt - _frm);
    to_nxt = to + (_to_nxt - _to);
    return r;
}

WRUTIL_API __codecvt_utf8<wchar_t>::result
__codecvt_utf8<wchar_t>::do_in(state_type&,
    const extern_type* frm, const extern_type* frm_end, const extern_type*& frm_nxt,
    intern_type* to, intern_type* to_end, intern_type*& to_nxt) const
{
    const uint8_t* _frm = reinterpret_cast<const uint8_t*>(frm);
    const uint8_t* _frm_end = reinterpret_cast<const uint8_t*>(frm_end);
    const uint8_t* _frm_nxt = _frm;
#if _WIN32
    uint16_t* _to = reinterpret_cast<uint16_t*>(to);
    uint16_t* _to_end = reinterpret_cast<uint16_t*>(to_end);
    uint16_t* _to_nxt = _to;
    result r = utf8_to_ucs2(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt,
                            _Maxcode_, _Mode_);
#else
    uint32_t* _to = reinterpret_cast<uint32_t*>(to);
    uint32_t* _to_end = reinterpret_cast<uint32_t*>(to_end);
    uint32_t* _to_nxt = _to;
    result r = utf8_to_ucs4(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt,
                            _Maxcode_, _Mode_);
#endif
    frm_nxt = frm + (_frm_nxt - _frm);
    to_nxt = to + (_to_nxt - _to);
    return r;
}

WRUTIL_API __codecvt_utf8<wchar_t>::result
__codecvt_utf8<wchar_t>::do_unshift(state_type&,
    extern_type* to, extern_type*, extern_type*& to_nxt) const
{
    to_nxt = to;
    return noconv;
}

WRUTIL_API int
__codecvt_utf8<wchar_t>::do_encoding() const noexcept
{
    return 0;
}

WRUTIL_API bool
__codecvt_utf8<wchar_t>::do_always_noconv() const noexcept
{
    return false;
}

WRUTIL_API int
__codecvt_utf8<wchar_t>::do_length(state_type&,
    const extern_type* frm, const extern_type* frm_end, size_t mx) const
{
    const uint8_t* _frm = reinterpret_cast<const uint8_t*>(frm);
    const uint8_t* _frm_end = reinterpret_cast<const uint8_t*>(frm_end);
    return utf8_to_ucs4_length(_frm, _frm_end, mx, _Maxcode_, _Mode_);
}

WRUTIL_API int
__codecvt_utf8<wchar_t>::do_max_length() const noexcept
{
    if (_Mode_ & consume_header)
        return 7;
    return 4;
}

// __codecvt_utf8<char16_t>

WRUTIL_API __codecvt_utf8<char16_t>::result
__codecvt_utf8<char16_t>::do_out(state_type&,
    const intern_type* frm, const intern_type* frm_end, const intern_type*& frm_nxt,
    extern_type* to, extern_type* to_end, extern_type*& to_nxt) const
{
    const uint16_t* _frm = reinterpret_cast<const uint16_t*>(frm);
    const uint16_t* _frm_end = reinterpret_cast<const uint16_t*>(frm_end);
    const uint16_t* _frm_nxt = _frm;
    uint8_t* _to = reinterpret_cast<uint8_t*>(to);
    uint8_t* _to_end = reinterpret_cast<uint8_t*>(to_end);
    uint8_t* _to_nxt = _to;
    result r = ucs2_to_utf8(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt,
                            _Maxcode_, _Mode_);
    frm_nxt = frm + (_frm_nxt - _frm);
    to_nxt = to + (_to_nxt - _to);
    return r;
}

WRUTIL_API __codecvt_utf8<char16_t>::result
__codecvt_utf8<char16_t>::do_in(state_type&,
    const extern_type* frm, const extern_type* frm_end, const extern_type*& frm_nxt,
    intern_type* to, intern_type* to_end, intern_type*& to_nxt) const
{
    const uint8_t* _frm = reinterpret_cast<const uint8_t*>(frm);
    const uint8_t* _frm_end = reinterpret_cast<const uint8_t*>(frm_end);
    const uint8_t* _frm_nxt = _frm;
    uint16_t* _to = reinterpret_cast<uint16_t*>(to);
    uint16_t* _to_end = reinterpret_cast<uint16_t*>(to_end);
    uint16_t* _to_nxt = _to;
    result r = utf8_to_ucs2(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt,
                            _Maxcode_, _Mode_);
    frm_nxt = frm + (_frm_nxt - _frm);
    to_nxt = to + (_to_nxt - _to);
    return r;
}

WRUTIL_API __codecvt_utf8<char16_t>::result
__codecvt_utf8<char16_t>::do_unshift(state_type&,
    extern_type* to, extern_type*, extern_type*& to_nxt) const
{
    to_nxt = to;
    return noconv;
}

WRUTIL_API int
__codecvt_utf8<char16_t>::do_encoding() const noexcept
{
    return 0;
}

WRUTIL_API bool
__codecvt_utf8<char16_t>::do_always_noconv() const noexcept
{
    return false;
}

WRUTIL_API int
__codecvt_utf8<char16_t>::do_length(state_type&,
    const extern_type* frm, const extern_type* frm_end, size_t mx) const
{
    const uint8_t* _frm = reinterpret_cast<const uint8_t*>(frm);
    const uint8_t* _frm_end = reinterpret_cast<const uint8_t*>(frm_end);
    return utf8_to_ucs2_length(_frm, _frm_end, mx, _Maxcode_, _Mode_);
}

WRUTIL_API int
__codecvt_utf8<char16_t>::do_max_length() const noexcept
{
    if (_Mode_ & consume_header)
        return 6;
    return 3;
}

// __codecvt_utf8<char32_t>

WRUTIL_API __codecvt_utf8<char32_t>::result
__codecvt_utf8<char32_t>::do_out(state_type&,
    const intern_type* frm, const intern_type* frm_end, const intern_type*& frm_nxt,
    extern_type* to, extern_type* to_end, extern_type*& to_nxt) const
{
    const uint32_t* _frm = reinterpret_cast<const uint32_t*>(frm);
    const uint32_t* _frm_end = reinterpret_cast<const uint32_t*>(frm_end);
    const uint32_t* _frm_nxt = _frm;
    uint8_t* _to = reinterpret_cast<uint8_t*>(to);
    uint8_t* _to_end = reinterpret_cast<uint8_t*>(to_end);
    uint8_t* _to_nxt = _to;
    result r = ucs4_to_utf8(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt,
                            _Maxcode_, _Mode_);
    frm_nxt = frm + (_frm_nxt - _frm);
    to_nxt = to + (_to_nxt - _to);
    return r;
}

WRUTIL_API __codecvt_utf8<char32_t>::result
__codecvt_utf8<char32_t>::do_in(state_type&,
    const extern_type* frm, const extern_type* frm_end, const extern_type*& frm_nxt,
    intern_type* to, intern_type* to_end, intern_type*& to_nxt) const
{
    const uint8_t* _frm = reinterpret_cast<const uint8_t*>(frm);
    const uint8_t* _frm_end = reinterpret_cast<const uint8_t*>(frm_end);
    const uint8_t* _frm_nxt = _frm;
    uint32_t* _to = reinterpret_cast<uint32_t*>(to);
    uint32_t* _to_end = reinterpret_cast<uint32_t*>(to_end);
    uint32_t* _to_nxt = _to;
    result r = utf8_to_ucs4(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt,
                            _Maxcode_, _Mode_);
    frm_nxt = frm + (_frm_nxt - _frm);
    to_nxt = to + (_to_nxt - _to);
    return r;
}

WRUTIL_API __codecvt_utf8<char32_t>::result
__codecvt_utf8<char32_t>::do_unshift(state_type&,
    extern_type* to, extern_type*, extern_type*& to_nxt) const
{
    to_nxt = to;
    return noconv;
}

WRUTIL_API int
__codecvt_utf8<char32_t>::do_encoding() const noexcept
{
    return 0;
}

WRUTIL_API bool
__codecvt_utf8<char32_t>::do_always_noconv() const noexcept
{
    return false;
}

WRUTIL_API int
__codecvt_utf8<char32_t>::do_length(state_type&,
    const extern_type* frm, const extern_type* frm_end, size_t mx) const
{
    const uint8_t* _frm = reinterpret_cast<const uint8_t*>(frm);
    const uint8_t* _frm_end = reinterpret_cast<const uint8_t*>(frm_end);
    return utf8_to_ucs4_length(_frm, _frm_end, mx, _Maxcode_, _Mode_);
}

WRUTIL_API int
__codecvt_utf8<char32_t>::do_max_length() const noexcept
{
    if (_Mode_ & consume_header)
        return 7;
    return 4;
}


} // namespace wr
