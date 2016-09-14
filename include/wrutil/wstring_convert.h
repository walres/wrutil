// -*- C++ -*-
//===----------- wstring_convert.h (from libc++ original code) ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef WRUTIL_WSTRING_CONVERT_H
#define WRUTIL_WSTRING_CONVERT_H

#include <wrutil/Config.h>
#include <locale>
#include <memory>
#include <stdexcept>
#include <string>


namespace wr {


template<class _Codecvt, class _Elem = wchar_t,
         class _Wide_alloc = std::allocator<_Elem>,
         class _Byte_alloc = std::allocator<char> >
class wstring_convert
{
public:
    typedef std::basic_string<char, std::char_traits<char>, _Byte_alloc> byte_string;
    typedef std::basic_string<_Elem, std::char_traits<_Elem>, _Wide_alloc> wide_string;
    typedef typename _Codecvt::state_type                        state_type;
    typedef typename wide_string::traits_type::int_type          int_type;

private:
    byte_string __byte_err_string_;
    wide_string __wide_err_string_;
    _Codecvt* __cvtptr_;
    state_type __cvtstate_;
    size_t __cvtcount_;

    wstring_convert(const wstring_convert& __wc);
    wstring_convert& operator=(const wstring_convert& __wc);
public:
    wstring_convert(_Codecvt* __pcvt = new _Codecvt);
    wstring_convert(_Codecvt* __pcvt, state_type __state);
    wstring_convert(const byte_string& __byte_err,
                    const wide_string& __wide_err = wide_string());
    wstring_convert(wstring_convert&& __wc);
    ~wstring_convert();

    wide_string from_bytes(char __byte)
        {return from_bytes(&__byte, &__byte+1);}
    wide_string from_bytes(const char* __ptr)
        {return from_bytes(__ptr, __ptr + std::char_traits<char>::length(__ptr));}
    wide_string from_bytes(const byte_string& __str)
        {return from_bytes(__str.data(), __str.data() + __str.size());}
    wide_string from_bytes(const char* __first, const char* __last);

    byte_string to_bytes(_Elem __wchar)
        {return to_bytes(&__wchar, &__wchar+1);}
    byte_string to_bytes(const _Elem* __wptr)
        {return to_bytes(__wptr, __wptr + std::char_traits<_Elem>::length(__wptr));}
    byte_string to_bytes(const wide_string& __wstr)
        {return to_bytes(__wstr.data(), __wstr.data() + __wstr.size());}
    byte_string to_bytes(const _Elem* __first, const _Elem* __last);

    size_t converted() const noexcept {return __cvtcount_;}
    state_type state() const {return __cvtstate_;}
};

template<class _Codecvt, class _Elem, class _Wide_alloc, class _Byte_alloc>
inline
wstring_convert<_Codecvt, _Elem, _Wide_alloc, _Byte_alloc>::
    wstring_convert(_Codecvt* __pcvt)
        : __cvtptr_(__pcvt), __cvtstate_(), __cvtcount_(0)
{
}

template<class _Codecvt, class _Elem, class _Wide_alloc, class _Byte_alloc>
inline
wstring_convert<_Codecvt, _Elem, _Wide_alloc, _Byte_alloc>::
    wstring_convert(_Codecvt* __pcvt, state_type __state)
        : __cvtptr_(__pcvt), __cvtstate_(__state), __cvtcount_(0)
{
}

template<class _Codecvt, class _Elem, class _Wide_alloc, class _Byte_alloc>
wstring_convert<_Codecvt, _Elem, _Wide_alloc, _Byte_alloc>::
    wstring_convert(const byte_string& __byte_err, const wide_string& __wide_err)
        : __byte_err_string_(__byte_err), __wide_err_string_(__wide_err),
          __cvtstate_(), __cvtcount_(0)
{
    __cvtptr_ = new _Codecvt;
}

template<class _Codecvt, class _Elem, class _Wide_alloc, class _Byte_alloc>
inline
wstring_convert<_Codecvt, _Elem, _Wide_alloc, _Byte_alloc>::
    wstring_convert(wstring_convert&& __wc)
        : __byte_err_string_(std::move(__wc.__byte_err_string_)),
          __wide_err_string_(std::move(__wc.__wide_err_string_)),
          __cvtptr_(__wc.__cvtptr_),
          __cvtstate_(__wc.__cvtstate_), __cvtcount_(__wc.__cvtstate_)
{
    __wc.__cvtptr_ = nullptr;
}

template<class _Codecvt, class _Elem, class _Wide_alloc, class _Byte_alloc>
wstring_convert<_Codecvt, _Elem, _Wide_alloc, _Byte_alloc>::~wstring_convert()
{
    delete __cvtptr_;
}

template<class _Codecvt, class _Elem, class _Wide_alloc, class _Byte_alloc>
typename wstring_convert<_Codecvt, _Elem, _Wide_alloc, _Byte_alloc>::wide_string
wstring_convert<_Codecvt, _Elem, _Wide_alloc, _Byte_alloc>::
    from_bytes(const char* __frm, const char* __frm_end)
{
    __cvtcount_ = 0;
    if (__cvtptr_ != nullptr)
    {
        wide_string __ws(2*(__frm_end - __frm), _Elem());
        if (__frm != __frm_end)
            __ws.resize(__ws.capacity());
        std::codecvt_base::result __r = std::codecvt_base::ok;
        state_type __st = __cvtstate_;
        if (__frm != __frm_end)
        {
            _Elem* __to = &__ws[0];
            _Elem* __to_end = __to + __ws.size();
            const char* __frm_nxt;
            do
            {
                _Elem* __to_nxt;
                __r = __cvtptr_->in(__st, __frm, __frm_end, __frm_nxt,
                                          __to, __to_end, __to_nxt);
                __cvtcount_ += __frm_nxt - __frm;
                if (__frm_nxt == __frm)
                {
                    __r = std::codecvt_base::error;
                }
                else if (__r == std::codecvt_base::noconv)
                {
                    __ws.resize(__to - &__ws[0]);
                    // This only gets executed if _Elem is char
                    __ws.append((const _Elem*)__frm, (const _Elem*)__frm_end);
                    __frm = __frm_nxt;
                    __r = std::codecvt_base::ok;
                }
                else if (__r == std::codecvt_base::ok)
                {
                    __ws.resize(__to_nxt - &__ws[0]);
                    __frm = __frm_nxt;
                }
                else if (__r == std::codecvt_base::partial)
                {
                    std::ptrdiff_t __s = __to_nxt - &__ws[0];
                    __ws.resize(2 * __s);
                    __to = &__ws[0] + __s;
                    __to_end = &__ws[0] + __ws.size();
                    __frm = __frm_nxt;
                }
            } while (__r == std::codecvt_base::partial && __frm_nxt < __frm_end);
        }
        if (__r == std::codecvt_base::ok)
            return __ws;
    }
    if (__wide_err_string_.empty())
        throw std::range_error("wstring_convert: from_bytes error");
    return __wide_err_string_;
}

template<class _Codecvt, class _Elem, class _Wide_alloc, class _Byte_alloc>
typename wstring_convert<_Codecvt, _Elem, _Wide_alloc, _Byte_alloc>::byte_string
wstring_convert<_Codecvt, _Elem, _Wide_alloc, _Byte_alloc>::
    to_bytes(const _Elem* __frm, const _Elem* __frm_end)
{
    __cvtcount_ = 0;
    if (__cvtptr_ != nullptr)
    {
        byte_string __bs(2*(__frm_end - __frm), char());
        if (__frm != __frm_end)
            __bs.resize(__bs.capacity());
        std::codecvt_base::result __r = std::codecvt_base::ok;
        state_type __st = __cvtstate_;
        if (__frm != __frm_end)
        {
            char* __to = &__bs[0];
            char* __to_end = __to + __bs.size();
            const _Elem* __frm_nxt;
            do
            {
                char* __to_nxt;
                __r = __cvtptr_->out(__st, __frm, __frm_end, __frm_nxt,
                                           __to, __to_end, __to_nxt);
                __cvtcount_ += __frm_nxt - __frm;
                if (__frm_nxt == __frm)
                {
                    __r = std::codecvt_base::error;
                }
                else if (__r == std::codecvt_base::noconv)
                {
                    __bs.resize(__to - &__bs[0]);
                    // This only gets executed if _Elem is char
                    __bs.append((const char*)__frm, (const char*)__frm_end);
                    __frm = __frm_nxt;
                    __r = std::codecvt_base::ok;
                }
                else if (__r == std::codecvt_base::ok)
                {
                    __bs.resize(__to_nxt - &__bs[0]);
                    __frm = __frm_nxt;
                }
                else if (__r == std::codecvt_base::partial)
                {
                    std::ptrdiff_t __s = __to_nxt - &__bs[0];
                    __bs.resize(2 * __s);
                    __to = &__bs[0] + __s;
                    __to_end = &__bs[0] + __bs.size();
                    __frm = __frm_nxt;
                }
            } while (__r == std::codecvt_base::partial && __frm_nxt < __frm_end);
        }
        if (__r == std::codecvt_base::ok)
        {
            size_t __s = __bs.size();
            __bs.resize(__bs.capacity());
            char* __to = &__bs[0] + __s;
            char* __to_end = __to + __bs.size();
            do
            {
                char* __to_nxt;
                __r = __cvtptr_->unshift(__st, __to, __to_end, __to_nxt);
                if (__r == std::codecvt_base::noconv)
                {
                    __bs.resize(__to - &__bs[0]);
                    __r = std::codecvt_base::ok;
                }
                else if (__r == std::codecvt_base::ok)
                {
                    __bs.resize(__to_nxt - &__bs[0]);
                }
                else if (__r == std::codecvt_base::partial)
                {
                    std::ptrdiff_t __sp = __to_nxt - &__bs[0];
                    __bs.resize(2 * __sp);
                    __to = &__bs[0] + __sp;
                    __to_end = &__bs[0] + __bs.size();
                }
            } while (__r == std::codecvt_base::partial);
            if (__r == std::codecvt_base::ok)
                return __bs;
        }
    }
    if (__byte_err_string_.empty())
        throw std::range_error("wstring_convert: to_bytes error");
    return __byte_err_string_;
}


} // namespace wr


#endif // !WRUTIL_WSTRING_CONVERT_H
