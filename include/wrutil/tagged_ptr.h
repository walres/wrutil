/**
 * \file tagged_ptr.h
 *
 * \brief \c tagged_ptr class combining an aligned pointer with an unsigned
 *      'tag' packed into the unused low bits
 *
 * \copyright
 * \parblock
 *
 *   Copyright 2017 James S. Waller
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
#ifndef WRUTIL_TAGGED_PTR_H__
#define WRUTIL_TAGGED_PTR_H__

#include <stdexcept>
#include <type_traits>
#include <wrutil/Config.h>


namespace wr {


template <typename Pointee, size_t N_TAG_BITS>
class tagged_ptr
{
public:
        using this_t = tagged_ptr;

        tagged_ptr() : ptr_(nullptr) {}
        tagged_ptr(const this_t &other) : ptr_(other.ptr_) {}
        tagged_ptr(std::nullptr_t) : this_t() {}

        template <typename Pointee2> explicit tagged_ptr(Pointee2 *p)
                { set(p, 0); }

        tagged_ptr(std::nullptr_t, uintptr_t tag) { set(nullptr, tag); }
        tagged_ptr(Pointee *p, uintptr_t tag) { set(p, tag); }

        this_t &operator=(const this_t &other)
                { ptr_ = other.ptr_; return *this; }

        /* support for setting tagged_ptr's with qualified Pointee from other
           tagged_ptr's with unqualified Pointee */
        template <typename Pointee2>
        tagged_ptr(const tagged_ptr<Pointee2, N_TAG_BITS> &other) :
                ptr_(other.ptr_) {}

        template <typename Pointee2>
        this_t &operator=(const tagged_ptr<Pointee2, N_TAG_BITS> &other)
                { ptr_ = other.ptr_; return *this; }

        template <typename Pointee2> this_t &operator=(Pointee2 *p)
                { return ptr(p); }

        template <typename Pointee2> explicit operator Pointee2 *() const
                { return static_cast<Pointee2 *>(ptr()); }

        explicit operator bool() const { return ptr() != nullptr; }

        Pointee *operator->() const { return ptr(); }

        typename std::add_lvalue_reference<Pointee>::type
                operator*() const  { return *ptr(); }

        Pointee *ptr() const
                { return reinterpret_cast<Pointee *>(bits_ & ptrMask()); }

        uintptr_t tag() const { return bits_ & tagMask(); }

        this_t &ptr(Pointee *p)
        {
                auto bits = reinterpret_cast<uintptr_t>(p);
                if ((bits & tagMask()) != 0) {
                        throw std::invalid_argument("tagged_ptr::ptr(): incorrectly aligned pointer");
                }
                bits_ = bits;
                return *this;
        }

        this_t &tag(uintptr_t t)
        {
                if ((t & ptrMask()) != 0) {
                       throw std::invalid_argument("tagged_ptr::tag(): tag too large");
                }
                bits_ = (bits_ & ptrMask()) | t;
                return *this;
        }

        this_t &set(Pointee *p, uintptr_t t) { return ptr(p).tag(t); }
        this_t &set(std::nullptr_t, uintptr_t t) { return ptr(nullptr).tag(t); }

        this_t &swap(this_t &other)
                { std::swap(ptr_, other.ptr_); return *this; }

        template <typename Pointee2>
        bool operator==(const tagged_ptr<Pointee2, N_TAG_BITS> &other) const
                { return bits_ == other.bits_; }

        template <typename Pointee2>
        bool operator!=(const tagged_ptr<Pointee2, N_TAG_BITS> &other) const
                { return bits_ != other.bits_; }

        template <typename Pointee2>
        bool operator<(const tagged_ptr<Pointee2, N_TAG_BITS> &other) const
                { return (ptr() < other.ptr())
                        || ((ptr() == other.ptr()) && (tag() < other.tag())); }

        template <typename Pointee2>
        bool operator<=(const tagged_ptr<Pointee2, N_TAG_BITS> &other) const
                { return (bits_ == other.bits_)
                        || ((ptr() == other.ptr()) && (tag() <= other.tag())); }

        template <typename Pointee2>
        bool operator>=(const tagged_ptr<Pointee2, N_TAG_BITS> &other) const
                { return (bits_ == other.bits_)
                        || ((ptr() == other.ptr()) && (tag() >= other.tag())); }

        template <typename Pointee2>
        bool operator>(const tagged_ptr<Pointee2, N_TAG_BITS> &other) const
                { return (ptr() > other.ptr())
                        || ((ptr() == other.ptr()) && (tag() > other.tag())); }


private:
        static constexpr uintptr_t ptrMask()
                { return uintptr_t(-1L) << N_TAG_BITS; }

        static constexpr uintptr_t tagMask() { return ~ptrMask(); }

        union
        {
                Pointee   *ptr_;
                uintptr_t  bits_;
        };
};

//--------------------------------------

template <typename Pointee, size_t N_TAG_BITS, typename RawPtr> inline bool
operator==(
        const tagged_ptr<Pointee, N_TAG_BITS> &a,
        RawPtr                                 b
)
{
        return a.ptr() == b;
}

template <typename RawPtr, typename Pointee, size_t N_TAG_BITS> inline bool
operator==(
        RawPtr                                 a,
        const tagged_ptr<Pointee, N_TAG_BITS> &b
)
{
        return a == b.ptr();
}

template <typename Pointee, size_t N_TAG_BITS, typename RawPtr> inline bool
operator!=(
        const tagged_ptr<Pointee, N_TAG_BITS> &a,
        RawPtr                                 b
)
{
        return a.ptr() != b;
}

template <typename RawPtr, typename Pointee, size_t N_TAG_BITS> inline bool
operator!=(
        RawPtr                                 a,
        const tagged_ptr<Pointee, N_TAG_BITS> &b
)
{
        return a != b.ptr();
}

template <typename RawPtr, typename Pointee, size_t N_TAG_BITS> inline bool
operator<(
        RawPtr                                 a,
        const tagged_ptr<Pointee, N_TAG_BITS> &b
)
{
        return a < b.ptr();
}

template <typename Pointee, size_t N_TAG_BITS, typename RawPtr> inline bool
operator<(
        const tagged_ptr<Pointee, N_TAG_BITS> &a,
        RawPtr                                 b
)
{
        return a.ptr() < b;
}

template <typename RawPtr, typename Pointee, size_t N_TAG_BITS> inline bool
operator<=(
        RawPtr                                 a,
        const tagged_ptr<Pointee, N_TAG_BITS> &b
)
{
        return a <= b.ptr();
}

template <typename Pointee, size_t N_TAG_BITS, typename RawPtr> inline bool
operator<=(
        const tagged_ptr<Pointee, N_TAG_BITS> &a,
        RawPtr                                 b
)
{
        return a.ptr() <= b;
}

template <typename RawPtr, typename Pointee, size_t N_TAG_BITS> inline bool
operator>=(
        RawPtr                                 a,
        const tagged_ptr<Pointee, N_TAG_BITS> &b
)
{
        return a >= b.ptr();
}

template <typename Pointee, size_t N_TAG_BITS, typename RawPtr> inline bool
operator>=(
        const tagged_ptr<Pointee, N_TAG_BITS> &a,
        RawPtr                                 b
)
{
        return a.ptr() >= b;
}

template <typename RawPtr, typename Pointee, size_t N_TAG_BITS> inline bool
operator>(
        RawPtr                                 a,
        const tagged_ptr<Pointee, N_TAG_BITS> &b
)
{
        return a > b.ptr();
}

template <typename Pointee, size_t N_TAG_BITS, typename RawPtr> inline bool
operator>(
        const tagged_ptr<Pointee, N_TAG_BITS> &a,
        RawPtr                                 b
)
{
        return a.ptr() > b;
}

//--------------------------------------
/*
 * wr::print*() support
 */
namespace fmt {


struct Arg;
struct Params;
template <typename> struct TypeHandler;


/* place implementation in tagged_ptr_format.cxx so programs linking with the
   static library won't have this code copied in unless they actually use it */
struct TaggedPtrHandlerBase
{
        static void set(Arg &arg, void *ptr, uintptr_t tag);
        static bool format(const Params &parms);
};


template <typename Pointee, size_t N_TAG_BITS>
struct WRUTIL_API TypeHandler<tagged_ptr<Pointee, N_TAG_BITS>> :
        TaggedPtrHandlerBase
{
        static void set(Arg &arg, const tagged_ptr<Pointee, N_TAG_BITS> &val)
                { TaggedPtrHandlerBase::set(arg, val.ptr(), val.tag()); }
};



} // namespace fmt

} // namespace wr

//------------------------------

namespace std {


template <typename Pointee, size_t N_TAG_BITS> void
swap(
        wr::tagged_ptr<Pointee, N_TAG_BITS> &a,
        wr::tagged_ptr<Pointee, N_TAG_BITS> &b
)
{
        a.swap(b);
}


} // namespace std


#endif // !WRUTIL_TAGGED_PTR_H__
