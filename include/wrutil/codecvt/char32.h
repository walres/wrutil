// -*- C++ -*-
//===--------------- char32.h (from libc++ original code) -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef WRUTIL_CODECVT_CHAR32_H
#define WRUTIL_CODECVT_CHAR32_H

#include <locale>
#include <wrutil/Config.h>


#if !WR_HAVE_STD_CODECVT_CHAR32


namespace std {


template <>
class codecvt<char32_t, char, mbstate_t>
    : public locale::facet,
      public codecvt_base
{
public:
    typedef char32_t  intern_type;
    typedef char      extern_type;
    typedef mbstate_t state_type;

    explicit codecvt(size_t __refs = 0)
        : locale::facet(__refs) {}

    result out(state_type& __st, const intern_type* __frm,
               const intern_type* __frm_end, const intern_type*& __frm_nxt,
               extern_type* __to, extern_type* __to_end,
               extern_type*& __to_nxt) const
    {
        return do_out(__st, __frm, __frm_end, __frm_nxt, __to, __to_end,
                      __to_nxt);
    }

    result unshift(state_type& __st, extern_type* __to, extern_type* __to_end,
                   extern_type*& __to_nxt) const
    {
        return do_unshift(__st, __to, __to_end, __to_nxt);
    }

    result in(state_type& __st, const extern_type* __frm,
              const extern_type* __frm_end, const extern_type*& __frm_nxt,
              intern_type* __to, intern_type* __to_end,
              intern_type*& __to_nxt) const
    {
        return do_in(__st, __frm, __frm_end, __frm_nxt, __to, __to_end,
                     __to_nxt);
    }

    int encoding() const noexcept
    {
        return do_encoding();
    }

    bool always_noconv() const noexcept
    {
        return do_always_noconv();
    }

    int length(state_type& __st, const extern_type* __frm,
               const extern_type* __end, size_t __mx) const
    {
        return do_length(__st, __frm, __end, __mx);
    }

    int max_length() const noexcept
    {
        return do_max_length();
    }

    static locale::id id;

protected:
    explicit codecvt(const char*, size_t __refs = 0)
        : locale::facet(__refs) {}

    WRUTIL_API ~codecvt();

    virtual WRUTIL_API result do_out(state_type& __st, const intern_type* __frm,
                                     const intern_type* __frm_end,
                                     const intern_type*& __frm_nxt,
                                     extern_type* __to, extern_type* __to_end,
                                     extern_type*& __to_nxt) const;

    virtual WRUTIL_API result do_in(state_type& __st, const extern_type* __frm,
                                    const extern_type* __frm_end,
                                    const extern_type*& __frm_nxt,
                                    intern_type* __to, intern_type* __to_end,
                                    intern_type*& __to_nxt) const;

    virtual WRUTIL_API result do_unshift(state_type& __st, extern_type* __to,
                                         extern_type* __to_end,
                                         extern_type*& __to_nxt) const;

    virtual WRUTIL_API int do_encoding() const noexcept;
    virtual WRUTIL_API bool do_always_noconv() const noexcept;
    virtual WRUTIL_API int do_length(state_type&, const extern_type* __frm,
                                     const extern_type* __end,
                                     size_t __mx) const;
    virtual WRUTIL_API int do_max_length() const noexcept;
};


} // namespace std


#endif // !WR_HAVE_STD_CODECVT_CHAR32
#endif // !WRUTIL_CODECVT_CHAR32_H
