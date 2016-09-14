// -*- C++ -*-
//===----------- wbuffer_convert.h (from libc++ original code) ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef WRUTIL_WBUFFER_CONVERT_H
#define WRUTIL_WBUFFER_CONVERT_H

#include <wrutil/Config.h>
#include <string.h>
#include <algorithm>
#include <streambuf>


namespace wr {


template <class _Codecvt, class _Elem = wchar_t,
          class _Tr = std::char_traits<_Elem> >
class wbuffer_convert
    : public std::basic_streambuf<_Elem, _Tr>
{
public:
    // types:
    typedef _Elem                          char_type;
    typedef _Tr                            traits_type;
    typedef typename traits_type::int_type int_type;
    typedef typename traits_type::pos_type pos_type;
    typedef typename traits_type::off_type off_type;
    typedef typename _Codecvt::state_type  state_type;

private:
    char*       __extbuf_;
    const char* __extbufnext_;
    const char* __extbufend_;
    char __extbuf_min_[8];
    size_t __ebs_;
    char_type* __intbuf_;
    size_t __ibs_;
    std::streambuf* __bufptr_;
    _Codecvt* __cv_;
    state_type __st_;
    std::ios_base::openmode __cm_;
    bool __owns_eb_;
    bool __owns_ib_;
    bool __always_noconv_;

    wbuffer_convert(const wbuffer_convert&);
    wbuffer_convert& operator=(const wbuffer_convert&);
public:
    wbuffer_convert(std::streambuf* __bytebuf = 0, 
            _Codecvt* __pcvt = new _Codecvt, state_type __state = state_type());
    ~wbuffer_convert();

    std::streambuf* rdbuf() const {return __bufptr_;}
    std::streambuf* rdbuf(std::streambuf* __bytebuf)
    {
        std::streambuf* __r = __bufptr_;
        __bufptr_ = __bytebuf;
        return __r;
    }

    state_type state() const {return __st_;}

protected:
    virtual int_type underflow();
    virtual int_type pbackfail(int_type __c = traits_type::eof());
    virtual int_type overflow (int_type __c = traits_type::eof());
    virtual std::basic_streambuf<char_type, traits_type>*
        setbuf(char_type* __s, std::streamsize __n);
    virtual pos_type seekoff(off_type __off, std::ios_base::seekdir __way,
                             std::ios_base::openmode __wch = std::ios_base::in | std::ios_base::out);
    virtual pos_type seekpos(pos_type __sp,
                             std::ios_base::openmode __wch = std::ios_base::in | std::ios_base::out);
    virtual int sync();

private:
    bool __read_mode();
    void __write_mode();
    wbuffer_convert* __close();
};

template <class _Codecvt, class _Elem, class _Tr>
wbuffer_convert<_Codecvt, _Elem, _Tr>::
    wbuffer_convert(std::streambuf* __bytebuf, _Codecvt* __pcvt,
                    state_type __state)
    : __extbuf_(0),
      __extbufnext_(0),
      __extbufend_(0),
      __ebs_(0),
      __intbuf_(0),
      __ibs_(0),
      __bufptr_(__bytebuf),
      __cv_(__pcvt),
      __st_(__state),
      __cm_(static_cast<std::ios_base::openmode>(0)),
      __owns_eb_(false),
      __owns_ib_(false),
      __always_noconv_(__cv_ ? __cv_->always_noconv() : false)
{
    setbuf(0, 4096);
}

template <class _Codecvt, class _Elem, class _Tr>
wbuffer_convert<_Codecvt, _Elem, _Tr>::~wbuffer_convert()
{
    __close();
    delete __cv_;
    if (__owns_eb_)
        delete [] __extbuf_;
    if (__owns_ib_)
        delete [] __intbuf_;
}

template <class _Codecvt, class _Elem, class _Tr>
typename wbuffer_convert<_Codecvt, _Elem, _Tr>::int_type
wbuffer_convert<_Codecvt, _Elem, _Tr>::underflow()
{
    if (__cv_ == 0 || __bufptr_ == 0)
        return traits_type::eof();
    bool __initial = __read_mode();
    char_type __1buf;
    if (this->gptr() == 0)
        this->setg(&__1buf, &__1buf+1, &__1buf+1);
    const size_t __unget_sz = __initial ? 0 : std::min<size_t>((this->egptr() - this->eback()) / 2, 4);
    int_type __c = traits_type::eof();
    if (this->gptr() == this->egptr())
    {
        memmove(this->eback(), this->egptr() - __unget_sz, __unget_sz * sizeof(char_type));
        if (__always_noconv_)
        {
            std::streamsize __nmemb = static_cast<std::streamsize>(this->egptr() - this->eback() - __unget_sz);
            __nmemb = __bufptr_->sgetn((char*)this->eback() + __unget_sz, __nmemb);
            if (__nmemb != 0)
            {
                this->setg(this->eback(),
                           this->eback() + __unget_sz,
                           this->eback() + __unget_sz + __nmemb);
                __c = *this->gptr();
            }
        }
        else
        {
            memmove(__extbuf_, __extbufnext_, __extbufend_ - __extbufnext_);
            __extbufnext_ = __extbuf_ + (__extbufend_ - __extbufnext_);
            __extbufend_ = __extbuf_ + (__extbuf_ == __extbuf_min_ ? sizeof(__extbuf_min_) : __ebs_);
            std::streamsize __nmemb = std::min(static_cast<std::streamsize>(this->egptr() - this->eback() - __unget_sz),
                                 static_cast<std::streamsize>(__extbufend_ - __extbufnext_));
            std::codecvt_base::result __r;
            std::streamsize __nr = __bufptr_->sgetn(const_cast<char*>(__extbufnext_), __nmemb);
            if (__nr != 0)
            {
                __extbufend_ = __extbufnext_ + __nr;
                char_type*  __inext;
                __r = __cv_->in(__st_, __extbuf_, __extbufend_, __extbufnext_,
                                       this->eback() + __unget_sz,
                                       this->egptr(), __inext);
                if (__r == std::codecvt_base::noconv)
                {
                    this->setg((char_type*)__extbuf_, (char_type*)__extbuf_, (char_type*)__extbufend_);
                    __c = *this->gptr();
                }
                else if (__inext != this->eback() + __unget_sz)
                {
                    this->setg(this->eback(), this->eback() + __unget_sz, __inext);
                    __c = *this->gptr();
                }
            }
        }
    }
    else
        __c = *this->gptr();
    if (this->eback() == &__1buf)
        this->setg(0, 0, 0);
    return __c;
}

template <class _Codecvt, class _Elem, class _Tr>
typename wbuffer_convert<_Codecvt, _Elem, _Tr>::int_type
wbuffer_convert<_Codecvt, _Elem, _Tr>::pbackfail(int_type __c)
{
    if (__cv_ != 0 && __bufptr_ != 0 && this->eback() < this->gptr())
    {
        if (traits_type::eq_int_type(__c, traits_type::eof()))
        {
            this->gbump(-1);
            return traits_type::not_eof(__c);
        }
        if (traits_type::eq(traits_type::to_char_type(__c), this->gptr()[-1]))
        {
            this->gbump(-1);
            *this->gptr() = traits_type::to_char_type(__c);
            return __c;
        }
    }
    return traits_type::eof();
}

template <class _Codecvt, class _Elem, class _Tr>
typename wbuffer_convert<_Codecvt, _Elem, _Tr>::int_type
wbuffer_convert<_Codecvt, _Elem, _Tr>::overflow(int_type __c)
{
    if (__cv_ == 0 || __bufptr_ == 0)
        return traits_type::eof();
    __write_mode();
    char_type __1buf;
    char_type* __pb_save = this->pbase();
    char_type* __epb_save = this->epptr();
    if (!traits_type::eq_int_type(__c, traits_type::eof()))
    {
        if (this->pptr() == 0)
            this->setp(&__1buf, &__1buf+1);
        *this->pptr() = traits_type::to_char_type(__c);
        this->pbump(1);
    }
    if (this->pptr() != this->pbase())
    {
        if (__always_noconv_)
        {
            auto __nmemb = static_cast<std::streamsize>(this->pptr() - this->pbase());
            if (__bufptr_->sputn((const char*)this->pbase(), __nmemb) != __nmemb)
                return traits_type::eof();
        }
        else
        {
            char* __extbe = __extbuf_;
            std::codecvt_base::result __r;
            do
            {
                const char_type* __e;
                __r = __cv_->out(__st_, this->pbase(), this->pptr(), __e,
                                        __extbuf_, __extbuf_ + __ebs_, __extbe);
                if (__e == this->pbase())
                    return traits_type::eof();
                if (__r == std::codecvt_base::noconv)
                {
                    std::streamsize __nmemb = static_cast<size_t>(this->pptr() - this->pbase());
                    if (__bufptr_->sputn((const char*)this->pbase(), __nmemb) != __nmemb)
                        return traits_type::eof();
                }
                else if (__r == std::codecvt_base::ok || __r == std::codecvt_base::partial)
                {
                    std::streamsize __nmemb = static_cast<size_t>(__extbe - __extbuf_);
                    if (__bufptr_->sputn(__extbuf_, __nmemb) != __nmemb)
                        return traits_type::eof();
                    if (__r == std::codecvt_base::partial)
                    {
                        this->setp((char_type*)__e, this->pptr());
                        this->pbump(static_cast<int>(this->epptr() - this->pbase()));
                    }
                }
                else
                    return traits_type::eof();
            } while (__r == std::codecvt_base::partial);
        }
        this->setp(__pb_save, __epb_save);
    }
    return traits_type::not_eof(__c);
}

template <class _Codecvt, class _Elem, class _Tr>
std::basic_streambuf<_Elem, _Tr>*
wbuffer_convert<_Codecvt, _Elem, _Tr>::setbuf(char_type* __s,
                                              std::streamsize __n)
{
    this->setg(0, 0, 0);
    this->setp(0, 0);
    if (__owns_eb_)
        delete [] __extbuf_;
    if (__owns_ib_)
        delete [] __intbuf_;
    __ebs_ = __n;
    if (__ebs_ > sizeof(__extbuf_min_))
    {
        if (__always_noconv_ && __s)
        {
            __extbuf_ = (char*)__s;
            __owns_eb_ = false;
        }
        else
        {
            __extbuf_ = new char[__ebs_];
            __owns_eb_ = true;
        }
    }
    else
    {
        __extbuf_ = __extbuf_min_;
        __ebs_ = sizeof(__extbuf_min_);
        __owns_eb_ = false;
    }
    if (!__always_noconv_)
    {
        __ibs_ = std::max<std::streamsize>(__n, sizeof(__extbuf_min_));
        if (__s && __ibs_ >= sizeof(__extbuf_min_))
        {
            __intbuf_ = __s;
            __owns_ib_ = false;
        }
        else
        {
            __intbuf_ = new char_type[__ibs_];
            __owns_ib_ = true;
        }
    }
    else
    {
        __ibs_ = 0;
        __intbuf_ = 0;
        __owns_ib_ = false;
    }
    return this;
}

template <class _Codecvt, class _Elem, class _Tr>
typename wbuffer_convert<_Codecvt, _Elem, _Tr>::pos_type
wbuffer_convert<_Codecvt, _Elem, _Tr>::seekoff(off_type __off, std::ios_base::seekdir __way,
                                        std::ios_base::openmode __om)
{
    int __width = __cv_->encoding();
    if (__cv_ == 0 || __bufptr_ == 0 || (__width <= 0 && __off != 0) || sync())
        return pos_type(off_type(-1));
    // __width > 0 || __off == 0
    switch (__way)
    {
    case std::ios_base::beg:
        break;
    case std::ios_base::cur:
        break;
    case std::ios_base::end:
        break;
    default:
        return pos_type(off_type(-1));
    }
    pos_type __r = __bufptr_->pubseekoff(__width * __off, __way, __om);
    __r.state(__st_);
    return __r;
}

template <class _Codecvt, class _Elem, class _Tr>
typename wbuffer_convert<_Codecvt, _Elem, _Tr>::pos_type
wbuffer_convert<_Codecvt, _Elem, _Tr>::seekpos(pos_type __sp, std::ios_base::openmode __wch)
{
    if (__cv_ == 0 || __bufptr_ == 0 || sync())
        return pos_type(off_type(-1));
    if (__bufptr_->pubseekpos(__sp, __wch) == pos_type(off_type(-1)))
        return pos_type(off_type(-1));
    return __sp;
}

template <class _Codecvt, class _Elem, class _Tr>
int
wbuffer_convert<_Codecvt, _Elem, _Tr>::sync()
{
    if (__cv_ == 0 || __bufptr_ == 0)
        return 0;
    if (__cm_ & std::ios_base::out)
    {
        if (this->pptr() != this->pbase())
            if (overflow() == traits_type::eof())
                return -1;
        std::codecvt_base::result __r;
        do
        {
            char* __extbe;
            __r = __cv_->unshift(__st_, __extbuf_, __extbuf_ + __ebs_, __extbe);
            auto __nmemb = static_cast<std::streamsize>(__extbe - __extbuf_);
            if (__bufptr_->sputn(__extbuf_, __nmemb) != __nmemb)
                return -1;
        } while (__r == std::codecvt_base::partial);
        if (__r == std::codecvt_base::error)
            return -1;
        if (__bufptr_->pubsync())
            return -1;
    }
    else if (__cm_ & std::ios_base::in)
    {
        off_type __c;
        if (__always_noconv_)
            __c = this->egptr() - this->gptr();
        else
        {
            int __width = __cv_->encoding();
            __c = __extbufend_ - __extbufnext_;
            if (__width > 0)
                __c += __width * (this->egptr() - this->gptr());
            else
            {
                if (this->gptr() != this->egptr())
                {
                    std::reverse(this->gptr(), this->egptr());
                    std::codecvt_base::result __r;
                    const char_type* __e = this->gptr();
                    char* __extbe;
                    do
                    {
                        __r = __cv_->out(__st_, __e, this->egptr(), __e,
                                         __extbuf_, __extbuf_ + __ebs_, __extbe);
                        switch (__r)
                        {
                        case std::codecvt_base::noconv:
                            __c += this->egptr() - this->gptr();
                            break;
                        case std::codecvt_base::ok:
                        case std::codecvt_base::partial:
                            __c += __extbe - __extbuf_;
                            break;
                        default:
                            return -1;
                        }
                    } while (__r == std::codecvt_base::partial);
                }
            }
        }
        if (__bufptr_->pubseekoff(-__c, std::ios_base::cur, __cm_) == pos_type(off_type(-1)))
            return -1;
        this->setg(0, 0, 0);
        __cm_ = static_cast<std::ios_base::openmode>(0);
    }
    return 0;
}

template <class _Codecvt, class _Elem, class _Tr>
bool
wbuffer_convert<_Codecvt, _Elem, _Tr>::__read_mode()
{
    if (!(__cm_ & std::ios_base::in))
    {
        this->setp(0, 0);
        if (__always_noconv_)
            this->setg((char_type*)__extbuf_,
                       (char_type*)__extbuf_ + __ebs_,
                       (char_type*)__extbuf_ + __ebs_);
        else
            this->setg(__intbuf_, __intbuf_ + __ibs_, __intbuf_ + __ibs_);
        __cm_ = std::ios_base::in;
        return true;
    }
    return false;
}

template <class _Codecvt, class _Elem, class _Tr>
void
wbuffer_convert<_Codecvt, _Elem, _Tr>::__write_mode()
{
    if (!(__cm_ & std::ios_base::out))
    {
        this->setg(0, 0, 0);
        if (__ebs_ > sizeof(__extbuf_min_))
        {
            if (__always_noconv_)
                this->setp((char_type*)__extbuf_,
                           (char_type*)__extbuf_ + (__ebs_ - 1));
            else
                this->setp(__intbuf_, __intbuf_ + (__ibs_ - 1));
        }
        else
            this->setp(0, 0);
        __cm_ = std::ios_base::out;
    }
}

template <class _Codecvt, class _Elem, class _Tr>
wbuffer_convert<_Codecvt, _Elem, _Tr>*
wbuffer_convert<_Codecvt, _Elem, _Tr>::__close()
{
    wbuffer_convert* __rt = 0;
    if (__cv_ != 0 && __bufptr_ != 0)
    {
        __rt = this;
        if ((__cm_ & std::ios_base::out) && sync())
            __rt = 0;
    }
    return __rt;
}


} // namespace wr


#endif // !WRUTIL_WBUFFER_CONVERT_H
