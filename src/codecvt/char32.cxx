//===-------------- char32.cxx (from libc++ original code) ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <wrutil/codecvt/char32.h>
#include "utf8_ucs4.h"


namespace std {


// template <> class codecvt<char32_t, char, mbstate_t>

WRUTIL_API locale::id codecvt<char32_t, char, mbstate_t>::id;

WRUTIL_API 
codecvt<char32_t, char, mbstate_t>::~codecvt()
{
}

WRUTIL_API codecvt<char32_t, char, mbstate_t>::result
codecvt<char32_t, char, mbstate_t>::do_out(state_type&,
    const intern_type* frm, const intern_type* frm_end, const intern_type*& frm_nxt,
    extern_type* to, extern_type* to_end, extern_type*& to_nxt) const
{
    const uint32_t* _frm = reinterpret_cast<const uint32_t*>(frm);
    const uint32_t* _frm_end = reinterpret_cast<const uint32_t*>(frm_end);
    const uint32_t* _frm_nxt = _frm;
    uint8_t* _to = reinterpret_cast<uint8_t*>(to);
    uint8_t* _to_end = reinterpret_cast<uint8_t*>(to_end);
    uint8_t* _to_nxt = _to;
    result r = wr::ucs4_to_utf8(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt);
    frm_nxt = frm + (_frm_nxt - _frm);
    to_nxt = to + (_to_nxt - _to);
    return r;
}

WRUTIL_API codecvt<char32_t, char, mbstate_t>::result
codecvt<char32_t, char, mbstate_t>::do_in(state_type&,
    const extern_type* frm, const extern_type* frm_end, const extern_type*& frm_nxt,
    intern_type* to, intern_type* to_end, intern_type*& to_nxt) const
{
    const uint8_t* _frm = reinterpret_cast<const uint8_t*>(frm);
    const uint8_t* _frm_end = reinterpret_cast<const uint8_t*>(frm_end);
    const uint8_t* _frm_nxt = _frm;
    uint32_t* _to = reinterpret_cast<uint32_t*>(to);
    uint32_t* _to_end = reinterpret_cast<uint32_t*>(to_end);
    uint32_t* _to_nxt = _to;
    result r = wr::utf8_to_ucs4(_frm, _frm_end, _frm_nxt, _to, _to_end, _to_nxt);
    frm_nxt = frm + (_frm_nxt - _frm);
    to_nxt = to + (_to_nxt - _to);
    return r;
}

WRUTIL_API codecvt<char32_t, char, mbstate_t>::result
codecvt<char32_t, char, mbstate_t>::do_unshift(state_type&,
    extern_type* to, extern_type*, extern_type*& to_nxt) const
{
    to_nxt = to;
    return noconv;
}

WRUTIL_API int
codecvt<char32_t, char, mbstate_t>::do_encoding() const noexcept
{
    return 0;
}

WRUTIL_API bool
codecvt<char32_t, char, mbstate_t>::do_always_noconv() const noexcept
{
    return false;
}

WRUTIL_API int
codecvt<char32_t, char, mbstate_t>::do_length(state_type&,
    const extern_type* frm, const extern_type* frm_end, size_t mx) const
{
    const uint8_t* _frm = reinterpret_cast<const uint8_t*>(frm);
    const uint8_t* _frm_end = reinterpret_cast<const uint8_t*>(frm_end);
    return wr::utf8_to_ucs4_length(_frm, _frm_end, mx);
}

WRUTIL_API int
codecvt<char32_t, char, mbstate_t>::do_max_length() const noexcept
{
    return 4;
}


} // namespace std
