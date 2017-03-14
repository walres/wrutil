// -*- C++ -*-
//===-------------- cvt_utf8.h (from libc++ original code) ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef WRUTIL_CODECVT_UTF8_H
#define WRUTIL_CODECVT_UTF8_H

#include <locale>
#if WR_HAVE_STD_CODECVT_UTF8
#       include <codecvt>
#endif
#include <wrutil/Config.h>


namespace wr {


#if WR_HAVE_STD_CODECVT_UTF8


using codecvt_mode = std::codecvt_mode;

template <typename Elem, unsigned long Maxcode = 0x10ffff,
          codecvt_mode Mode = static_cast<codecvt_mode>(0)>
        using codecvt_utf8 = std::codecvt_utf8<Elem, Maxcode, Mode>;


#else // WR_HAVE_STD_CODECVT_UTF8


enum codecvt_mode
{
    consume_header = 4,
    generate_header = 2,
    little_endian = 1
};

// codecvt_utf8

template <class _Elem> class __codecvt_utf8;

template <>
class WRUTIL_API __codecvt_utf8<wchar_t>
    : public std::codecvt<wchar_t, char, std::mbstate_t>
{
    unsigned long _Maxcode_;
    codecvt_mode _Mode_;

public:
    typedef wchar_t   intern_type;
    typedef char      extern_type;
    typedef mbstate_t state_type;

    explicit __codecvt_utf8(size_t __refs, unsigned long _Maxcode,
                            codecvt_mode _Mode)
        : codecvt<wchar_t, char, mbstate_t>(__refs), _Maxcode_(_Maxcode),
          _Mode_(_Mode) {}

protected:
    virtual result do_out(state_type& __st,
                          const intern_type* __frm,
                          const intern_type* __frm_end,
                          const intern_type*& __frm_nxt, extern_type* __to,
                          extern_type* __to_end, extern_type*& __to_nxt) const;

    virtual result do_in(state_type& __st, const extern_type* __frm,
                         const extern_type* __frm_end,
                         const extern_type*& __frm_nxt, intern_type* __to,
                         intern_type* __to_end, intern_type*& __to_nxt) const;

    virtual result do_unshift(state_type& __st, extern_type* __to,
                              extern_type* __to_end,
                              extern_type*& __to_nxt) const;

    virtual int do_encoding() const noexcept;
    virtual bool do_always_noconv() const noexcept;
    virtual int do_length(state_type&, const extern_type* __frm,
                          const extern_type* __end, size_t __mx) const;
    virtual int do_max_length() const noexcept;
};

template <>
class WRUTIL_API __codecvt_utf8<char16_t>
    : public std::codecvt<char16_t, char, std::mbstate_t>
{
    unsigned long _Maxcode_;
    codecvt_mode _Mode_;

public:
    typedef char16_t  intern_type;
    typedef char      extern_type;
    typedef mbstate_t state_type;

    explicit __codecvt_utf8(size_t __refs, unsigned long _Maxcode,
                            codecvt_mode _Mode)
        : codecvt<char16_t, char, mbstate_t>(__refs), _Maxcode_(_Maxcode),
          _Mode_(_Mode) {}

protected:
    virtual result do_out(state_type& __st, const intern_type* __frm,
                          const intern_type* __frm_end,
                          const intern_type*& __frm_nxt, extern_type* __to,
                          extern_type* __to_end, extern_type*& __to_nxt) const;

    virtual result do_in(state_type& __st, const extern_type* __frm,
                         const extern_type* __frm_end,
                         const extern_type*& __frm_nxt, intern_type* __to,
                         intern_type* __to_end, intern_type*& __to_nxt) const;

    virtual result do_unshift(state_type& __st, extern_type* __to,
                              extern_type* __to_end,
                              extern_type*& __to_nxt) const;

    virtual int do_encoding() const noexcept;
    virtual bool do_always_noconv() const noexcept;
    virtual int do_length(state_type&, const extern_type* __frm,
                          const extern_type* __end, size_t __mx) const;
    virtual int do_max_length() const noexcept;
};

template <>
class WRUTIL_API __codecvt_utf8<char32_t>
    : public std::codecvt<char32_t, char, std::mbstate_t>
{
    unsigned long _Maxcode_;
    codecvt_mode _Mode_;
public:
    typedef char32_t  intern_type;
    typedef char      extern_type;
    typedef mbstate_t state_type;

    explicit __codecvt_utf8(size_t __refs, unsigned long _Maxcode,
                            codecvt_mode _Mode)
        : codecvt<char32_t, char, mbstate_t>(__refs), _Maxcode_(_Maxcode),
          _Mode_(_Mode) {}
protected:
    virtual result do_out(state_type& __st, const intern_type* __frm,
                          const intern_type* __frm_end,
                          const intern_type*& __frm_nxt, extern_type* __to,
                          extern_type* __to_end, extern_type*& __to_nxt) const;

    virtual result do_in(state_type& __st, const extern_type* __frm,
                         const extern_type* __frm_end,
                         const extern_type*& __frm_nxt, intern_type* __to,
                         intern_type* __to_end, intern_type*& __to_nxt) const;

    virtual result do_unshift(state_type& __st, extern_type* __to,
                              extern_type* __to_end,
                              extern_type*& __to_nxt) const;

    virtual int do_encoding() const noexcept;
    virtual bool do_always_noconv() const noexcept;
    virtual int do_length(state_type&, const extern_type* __frm,
                          const extern_type* __end, size_t __mx) const;
    virtual int do_max_length() const noexcept;
};

template <class _Elem, unsigned long _Maxcode = 0x10ffff,
          codecvt_mode _Mode = (codecvt_mode)0>
class codecvt_utf8
    : public __codecvt_utf8<_Elem>
{
public:
    explicit codecvt_utf8(size_t __refs = 0)
        : __codecvt_utf8<_Elem>(__refs, _Maxcode, _Mode) {}

    ~codecvt_utf8() {}
};


#endif // WR_HAVE_STD_CODECVT_UTF8


} // namespace wr


#endif // !WRUTIL_CODECVT_UTF8_LIBCXX_H
