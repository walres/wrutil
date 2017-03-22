// -*- C++ -*-
//===-------------- optional.h (from libc++ original code)  ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef WRUTIL_OPTIONAL_H
#define WRUTIL_OPTIONAL_H

#include <wrutil/Config.h>
#include <wrutil/Format.h>


#if WR_HAVE_STD_OPTIONAL


#       if WR_HAVE_STD_EXP_OPTIONAL
#               include <experimental/optional>
#       else
#               include <optional>
#       endif


namespace wr {


#       if WR_HAVE_STD_EXP_OPTIONAL

using std::experimental::bad_optional_access;
using std::experimental::in_place_t;
using std::experimental::in_place;
using std::experimental::nullopt_t;
using std::experimental::nullopt;
using std::experimental::optional;
using std::experimental::make_optional;

#       else

using std::bad_optional_access;
using std::in_place_t;
using std::in_place;
using std::nullopt_t;
using std::nullopt;
using std::optional;
using std::make_optional;

#       endif


} // namespace wr


#else // WR_HAVE_STD_OPTIONAL


#include <wrutil/Config.h>
#include <cassert>
#include <functional>
#include <stdexcept>
#include <initializer_list>
#include <type_traits>
#if !WR_HAVE_STD_IS_TRIVIALLY_DESTRUCTIBLE \
                && !WR_HAVE_STD_HAS_TRIVIAL_DESTRUCTOR
#       include <boost/type_traits/has_trivial_destructor.hpp>
#endif
#include <new>


namespace wr {


class WRUTIL_API bad_optional_access
    : public std::exception
{
public:
    // Get the key function ~bad_optional_access() into the dylib
    virtual ~bad_optional_access() noexcept;
    virtual const char* what() const noexcept;
};


[[noreturn]] inline void __throw_bad_optional_access() {
        throw bad_optional_access();
        std::abort();
}

struct nullopt_t
{
    struct __secret_tag { explicit __secret_tag() = default; };
    constexpr explicit nullopt_t(__secret_tag, __secret_tag) noexcept {}
};

/* inline */ constexpr nullopt_t nullopt{nullopt_t::__secret_tag{}, nullopt_t::__secret_tag{}};

struct in_place_t {
  explicit in_place_t() = default;
};
constexpr in_place_t in_place{};

template <class _Tp, bool =
#if WR_HAVE_STD_IS_TRIVIALLY_DESTRUCTIBLE
                            std::is_trivially_destructible<_Tp>::
#elif WR_HAVE_STD_HAS_TRIVIAL_DESTRUCTOR
                            std::has_trivial_destructor<_Tp>::
#else
                            boost::has_trivial_destructor<_Tp>::
#endif
                                value>
struct __optional_destruct_base;

template <class _Tp>
struct __optional_destruct_base<_Tp, false>
{
    typedef _Tp value_type;
    static_assert(std::is_object<value_type>::value,
        "instantiation of optional with a non-object type is undefined behavior");
    union
    {
        char __null_state_;
        value_type __val_;
    };
    bool __engaged_;

    ~__optional_destruct_base()
    {
        if (__engaged_)
            __val_.~value_type();
    }

    constexpr __optional_destruct_base() noexcept
        :  __null_state_(),
           __engaged_(false) {}

    template <class... _Args>
    constexpr explicit __optional_destruct_base(in_place_t, _Args&&... __args)
        :  __val_(std::forward<_Args>(__args)...),
           __engaged_(true) {}

    void reset() noexcept
    {
        if (__engaged_)
        {
            __val_.~value_type();
            __engaged_ = false;
        }
    }
};

template <class _Tp>
struct __optional_destruct_base<_Tp, true>
{
    typedef _Tp value_type;
    static_assert(std::is_object<value_type>::value,
        "instantiation of optional with a non-object type is undefined behavior");
    union
    {
        char __null_state_;
        value_type __val_;
    };
    bool __engaged_;

    constexpr __optional_destruct_base() noexcept
        :  __null_state_(),
           __engaged_(false) {}

    template <class... _Args>
    constexpr explicit __optional_destruct_base(in_place_t, _Args&&... __args)
        :  __val_(std::forward<_Args>(__args)...),
           __engaged_(true) {}

    void reset() noexcept
    {
        if (__engaged_)
        {
            __engaged_ = false;
        }
    }
};

template <class _Tp, bool = std::is_reference<_Tp>::value>
struct __optional_storage_base : __optional_destruct_base<_Tp>
{
    using __base = __optional_destruct_base<_Tp>;
    using value_type = _Tp;
    using __base::__base;

    constexpr bool has_value() const noexcept
    {
        return this->__engaged_;
    }

    WR_CXX14_CONSTEXPR value_type& __get() & noexcept
    {
        return this->__val_;
    }

    constexpr const value_type& __get() const & noexcept
    {
        return this->__val_;
    }

    WR_CXX14_CONSTEXPR value_type&& __get() && noexcept
    {
        return std::move(this->__val_);
    }

    constexpr const value_type&& __get() const && noexcept
    {
        return std::move(this->__val_);
    }

    template <class... _Args>
    void __construct(_Args&&... __args)
    {
        assert(!has_value() && "__construct called for engaged __optional_storage");
        ::new((void*)std::addressof(this->__val_)) value_type(std::forward<_Args>(__args)...);
        this->__engaged_ = true;
    }

    template <class _That>
    void __construct_from(_That&& __opt)
    {
        if (__opt.has_value())
            __construct(std::forward<_That>(__opt).__get());
    }

    template <class _That>
    void __assign_from(_That&& __opt)
    {
        if (this->__engaged_ == __opt.has_value())
        {
            if (this->__engaged_)
                this->__val_ = std::forward<_That>(__opt).__get();
        }
        else
        {
            if (this->__engaged_)
                this->reset();
            else
                __construct(std::forward<_That>(__opt).__get());
        }
    }
};

// optional<T&> is currently required ill-formed, however it may to be in the
// future. For this reason it has already been implemented to ensure we can
// make the change in an ABI compatible manner.
template <class _Tp>
struct __optional_storage_base<_Tp, true>
{
    using value_type = _Tp;
    using __raw_type = typename std::remove_reference<_Tp>::type;
    __raw_type* __value_;

    template <class _Up>
    static constexpr bool __can_bind_reference() {
        using _RawUp = typename std::remove_reference<_Up>::type;
        using _UpPtr = _RawUp*;
        using _RawTp = typename std::remove_reference<_Tp>::type;
        using _TpPtr = _RawTp*;
        using _CheckLValueArg = std::integral_constant<bool,
            (std::is_lvalue_reference<_Up>::value && std::is_convertible<_UpPtr, _TpPtr>::value)
        || std::is_same<_RawUp, std::reference_wrapper<_RawTp>>::value
        || std::is_same<_RawUp, std::reference_wrapper<typename std::remove_const<_RawTp>::type>>::value
        >;
        return (std::is_lvalue_reference<_Tp>::value && _CheckLValueArg::value)
            || (std::is_rvalue_reference<_Tp>::value && !std::is_lvalue_reference<_Up>::value &&
                std::is_convertible<_UpPtr, _TpPtr>::value);
    }

    constexpr __optional_storage_base() noexcept
        :  __value_(nullptr) {}

    template <class _UArg>
    constexpr explicit __optional_storage_base(in_place_t, _UArg&& __uarg)
        :  __value_(std::addressof(__uarg))
    {
      static_assert(__can_bind_reference<_UArg>(),
        "Attempted to construct a reference element in tuple from a "
        "possible temporary");
    }

    void reset() noexcept { __value_ = nullptr; }

    constexpr bool has_value() const noexcept
      { return __value_ != nullptr; }

    constexpr value_type& __get() const& noexcept
      { return *__value_; }

    constexpr value_type&& __get() const&& noexcept
      { return std::forward<value_type>(*__value_); }

    template <class _UArg>
    void __construct(_UArg&& __val)
    {
        assert(!has_value() && "__construct called for engaged __optional_storage");
        static_assert(__can_bind_reference<_UArg>(),
            "Attempted to construct a reference element in tuple from a "
            "possible temporary");
        __value_ = std::addressof(__val);
    }

    template <class _That>
    void __construct_from(_That&& __opt)
    {
        if (__opt.has_value())
            __construct(std::forward<_That>(__opt).__get());
    }

    template <class _That>
    void __assign_from(_That&& __opt)
    {
        if (has_value() == __opt.has_value())
        {
            if (has_value())
                *__value_ = std::forward<_That>(__opt).__get();
        }
        else
        {
            if (has_value())
                reset();
            else
                __construct(std::forward<_That>(__opt).__get());
        }
    }
};


template <class _Tp, bool =
#if WR_HAVE_STD_IS_TRIVIALLY_COPYABLE
                            std::is_trivially_copyable<_Tp>::value
#else
                            std::is_scalar<_Tp>::value
#endif
         >
struct __optional_storage;

template <class _Tp>
struct __optional_storage<_Tp, true> : __optional_storage_base<_Tp>
{
    using __optional_storage_base<_Tp>::__optional_storage_base;
};

template <class _Tp>
struct __optional_storage<_Tp, false> : __optional_storage_base<_Tp>
{
    using value_type = _Tp;
    using __optional_storage_base<_Tp>::__optional_storage_base;

    __optional_storage() = default;

    __optional_storage(const __optional_storage& __opt)
    {
        this->__construct_from(__opt);
    }

    __optional_storage(__optional_storage&& __opt)
        noexcept(std::is_nothrow_move_constructible<value_type>::value)
    {
        this->__construct_from(std::move(__opt));
    }

    __optional_storage& operator=(const __optional_storage& __opt)
    {
        this->__assign_from(__opt);
        return *this;
    }

    __optional_storage& operator=(__optional_storage&& __opt)
        noexcept(std::is_nothrow_move_assignable<value_type>::value &&
                 std::is_nothrow_move_constructible<value_type>::value)
    {
        this->__assign_from(std::move(__opt));
        return *this;
    }
};


template <class _Tp>
class optional
    : private __optional_storage<_Tp>
{
    using __base = __optional_storage<_Tp>;
public:
    using value_type = _Tp;

private:
     // Disable the reference extension using this static assert.
    static_assert(!std::is_same<value_type, in_place_t>::value,
        "instantiation of optional with in_place_t is ill-formed");
    static_assert(!std::is_same<value_type, nullopt_t>::value,
        "instantiation of optional with nullopt_t is ill-formed");
    static_assert(!std::is_reference<value_type>::value,
        "instantiation of optional with a reference type is ill-formed");
    static_assert(std::is_destructible<value_type>::value,
        "instantiation of optional with a non-destructible type is ill-formed");

    struct __check_tuple_constructor_fail {
        template <class ...>
        static constexpr bool __enable_default() { return false; }
        template <class ...>
        static constexpr bool __enable_explicit() { return false; }
        template <class ...>
        static constexpr bool __enable_implicit() { return false; }
        template <class ...>
        static constexpr bool __enable_assign() { return false; }
    };

    // LWG2756: conditionally explicit conversion from _Up
    struct _CheckOptionalArgsConstructor {
      template <class _Up>
      static constexpr bool __enable_implicit() {
          return std::is_constructible<_Tp, _Up&&>::value &&
                 std::is_convertible<_Up&&, _Tp>::value;
      }

      template <class _Up>
      static constexpr bool __enable_explicit() {
          return std::is_constructible<_Tp, _Up&&>::value &&
                 !std::is_convertible<_Up&&, _Tp>::value;
      }
    };
    template <class _Up>
    using _CheckOptionalArgsCtor = typename std::conditional<
        !std::is_same<in_place_t, _Up>::value &&
        !std::is_same<typename std::decay<_Up>::type, optional>::value,
        _CheckOptionalArgsConstructor,
        __check_tuple_constructor_fail
    >::type;
    template <class _QualUp>
    struct _CheckOptionalLikeConstructor {
      template <class _Up, class _Opt = optional<_Up>>
      using __check_constructible_from_opt = std::integral_constant<bool,
          std::is_constructible<_Tp, _Opt&>::value ||
          std::is_constructible<_Tp, _Opt const&>::value ||
          std::is_constructible<_Tp, _Opt&&>::value ||
          std::is_constructible<_Tp, _Opt const&&>::value ||
          std::is_convertible<_Opt&, _Tp>::value ||
          std::is_convertible<_Opt const&, _Tp>::value ||
          std::is_convertible<_Opt&&, _Tp>::value ||
          std::is_convertible<_Opt const&&, _Tp>::value
      >;
      template <class _Up, class _Opt = optional<_Up>>
      using __check_assignable_from_opt = std::integral_constant<bool,
          std::is_assignable<_Tp&, _Opt&>::value ||
          std::is_assignable<_Tp&, _Opt const&>::value ||
          std::is_assignable<_Tp&, _Opt&&>::value ||
          std::is_assignable<_Tp&, _Opt const&&>::value
      >;
      template <class _Up, class _QUp = _QualUp>
      static constexpr bool __enable_implicit() {
          return std::is_convertible<_QUp, _Tp>::value &&
              !__check_constructible_from_opt<_Up>::value;
      }
      template <class _Up, class _QUp = _QualUp>
      static constexpr bool __enable_explicit() {
          return !std::is_convertible<_QUp, _Tp>::value &&
              !__check_constructible_from_opt<_Up>::value;
      }
      template <class _Up, class _QUp = _QualUp>
      static constexpr bool __enable_assign() {
          // Construction and assignability of _Qup to _Tp has already been
          // checked.
          return !__check_constructible_from_opt<_Up>::value &&
              !__check_assignable_from_opt<_Up>::value;
      }
    };

    template <class _Up, class _QualUp>
    using _CheckOptionalLikeCtor = typename std::conditional<
      std::integral_constant<bool,
          !std::is_same<_Up, _Tp>::value &&
          std::is_constructible<_Tp, _QualUp>::value
      >::value,
      _CheckOptionalLikeConstructor<_QualUp>,
      __check_tuple_constructor_fail
    >::type;
    template <class _Up, class _QualUp>
    using _CheckOptionalLikeAssign = typename std::conditional<
      std::integral_constant<bool,
          !std::is_same<_Up, _Tp>::value &&
          std::is_constructible<_Tp, _QualUp>::value &&
          std::is_assignable<_Tp&, _QualUp>::value
      >::value,
      _CheckOptionalLikeConstructor<_QualUp>,
      __check_tuple_constructor_fail
    >::type;
public:

    constexpr optional() noexcept {}
    optional(const optional&) = default;
    optional(optional&&) = default;
    constexpr optional(nullopt_t) noexcept {}

    template <class... _Args, class = typename std::enable_if<
        std::is_constructible<value_type, _Args...>::value>::type
    >
    constexpr explicit optional(in_place_t, _Args&&... __args)
        : __base(in_place, std::forward<_Args>(__args)...) {}

    template <class _Up, class... _Args, class = typename std::enable_if<
        std::is_constructible<value_type, std::initializer_list<_Up>&,
                              _Args...>::value>::type
    >
    constexpr explicit optional(in_place_t, std::initializer_list<_Up> __il,
                                _Args&&... __args)
        : __base(in_place, __il, std::forward<_Args>(__args)...) {}

    template <class _Up = value_type, typename std::enable_if<
        _CheckOptionalArgsCtor<_Up>::template __enable_implicit<_Up>()
    , int>::type = 0>
    constexpr optional(_Up&& __v)
        : __base(in_place, std::forward<_Up>(__v)) {}

    template <class _Up, typename std::enable_if<
        _CheckOptionalArgsCtor<_Up>::template __enable_explicit<_Up>()
    , int>::type = 0>
    constexpr explicit optional(_Up&& __v)
        : __base(in_place, std::forward<_Up>(__v)) {}

    // LWG2756: conditionally explicit conversion from const optional<_Up>&
    template <class _Up, typename std::enable_if<
        _CheckOptionalLikeCtor<_Up, _Up const&>::template __enable_implicit<_Up>()
    , int>::type = 0>
    optional(const optional<_Up>& __v)
    {
        this->__construct_from(__v);
    }
    template <class _Up, typename std::enable_if<
        _CheckOptionalLikeCtor<_Up, _Up const&>::template __enable_explicit<_Up>()
    , int>::type = 0>
    explicit optional(const optional<_Up>& __v)
    {
        this->__construct_from(__v);
    }

    // LWG2756: conditionally explicit conversion from optional<_Up>&&
    template <class _Up, typename std::enable_if<
        _CheckOptionalLikeCtor<_Up, _Up &&>::template __enable_implicit<_Up>()
    , int>::type = 0>
    optional(optional<_Up>&& __v)
    {
        this->__construct_from(std::move(__v));
    }
    template <class _Up, typename std::enable_if<
        _CheckOptionalLikeCtor<_Up, _Up &&>::template __enable_explicit<_Up>()
    , int>::type = 0>
    explicit optional(optional<_Up>&& __v)
    {
        this->__construct_from(std::move(__v));
    }

    optional& operator=(nullopt_t) noexcept
    {
        reset();
        return *this;
    }

    optional& operator=(const optional&) = default;
    optional& operator=(optional&&) = default;

    // LWG2756
    template <class _Up = value_type,
              class = typename std::enable_if
                      <std::integral_constant<bool,
                          std::integral_constant<bool,
                              !std::is_same<typename std::decay<_Up>::type,
                                            optional>::value &&
                              !(std::is_same<_Up, value_type>::value
                                        && std::is_scalar<value_type>::value)
                          >::value &&
                          std::is_constructible<value_type, _Up>::value &&
                          std::is_assignable<value_type&, _Up>::value
                      >::value>::type
             >
    optional&
    operator=(_Up&& __v)
    {
        if (this->has_value())
            this->__get() = std::forward<_Up>(__v);
        else
            this->__construct(std::forward<_Up>(__v));
        return *this;
    }

    // LWG2756
    template <class _Up, typename std::enable_if<
        _CheckOptionalLikeAssign<_Up, _Up const&>::template __enable_assign<_Up>()
    , int>::type = 0>
    optional&
    operator=(const optional<_Up>& __v)
    {
        this->__assign_from(__v);
        return *this;
    }

    // LWG2756
    template <class _Up, typename std::enable_if<
        _CheckOptionalLikeCtor<_Up, _Up &&>::template __enable_assign<_Up>()
    , int>::type = 0>
    optional&
    operator=(optional<_Up>&& __v)
    {
        this->__assign_from(std::move(__v));
        return *this;
    }

    template <class... _Args,
              class = typename std::enable_if
                      <
                          std::is_constructible<value_type, _Args...>::value
                      >::type
             >
    void
    emplace(_Args&&... __args)
    {
        reset();
        this->__construct(std::forward<_Args>(__args)...);
    }

    template <class _Up, class... _Args,
              class = typename std::enable_if
                      <
                          std::is_constructible<value_type,
                                                std::initializer_list<_Up>&,
                                                _Args...>::value
                      >::type
             >
    void
    emplace(std::initializer_list<_Up> __il, _Args&&... __args)
    {
        reset();
        this->__construct(__il, std::forward<_Args>(__args)...);
    }

    void swap(optional& __opt)
        noexcept(std::is_nothrow_move_constructible<value_type>::value /*&&
                 std::is_nothrow_swappable<value_type>::value*/)
    {
        if (this->has_value() == __opt.has_value())
        {
            using std::swap;
            if (this->has_value())
                swap(this->__get(), __opt.__get());
        }
        else
        {
            if (this->has_value())
            {
                __opt.__construct(std::move(this->__get()));
                reset();
            }
            else
            {
                this->__construct(std::move(__opt.__get()));
                __opt.reset();
            }
        }
    }

    WR_CXX14_CONSTEXPR
    typename std::add_pointer<const value_type>::type
    operator->() const
    {
        assert(this->has_value() && "optional operator-> called for disengaged value");
        return __operator_arrow(
            std::true_type() /*__has_operator_addressof<value_type>{}*/,
            this->__get());
    }

    WR_CXX14_CONSTEXPR
    typename std::add_pointer<value_type>::type
    operator->()
    {
        assert(this->has_value() && "optional operator-> called for disengaged value");
        return __operator_arrow(
            std::true_type() /*__has_operator_addressof<value_type>{}*/,
            this->__get());
    }

    WR_CXX14_CONSTEXPR
    const value_type&
    operator*() const&
    {
        assert(this->has_value() && "optional operator* called for disengaged value");
        return this->__get();
    }

    WR_CXX14_CONSTEXPR
    value_type&
    operator*() &
    {
        assert(this->has_value() && "optional operator* called for disengaged value");
        return this->__get();
    }

    WR_CXX14_CONSTEXPR
    value_type&&
    operator*() &&
    {
        assert(this->has_value() && "optional operator* called for disengaged value");
        return std::move(this->__get());
    }

    WR_CXX14_CONSTEXPR
    const value_type&&
    operator*() const&&
    {
        assert(this->has_value() && "optional operator* called for disengaged value");
        return std::move(this->__get());
    }

    constexpr explicit operator bool() const noexcept { return has_value(); }

    using __base::has_value;
    using __base::__get;

    WR_CXX14_CONSTEXPR value_type const& value() const&
    {
        if (!this->has_value())
            __throw_bad_optional_access();
        return this->__get();
    }

    WR_CXX14_CONSTEXPR value_type& value() &
    {
        if (!this->has_value())
            __throw_bad_optional_access();
        return this->__get();
    }

    WR_CXX14_CONSTEXPR value_type&& value() &&
    {
        if (!this->has_value())
            __throw_bad_optional_access();
        return std::move(this->__get());
    }

    WR_CXX14_CONSTEXPR value_type const&& value() const&&
    {
        if (!this->has_value())
            __throw_bad_optional_access();
        return std::move(this->__get());
    }

    template <class _Up>
    constexpr value_type value_or(_Up&& __v) const&
    {
        static_assert(std::is_copy_constructible<value_type>::value,
                      "optional<T>::value_or: T must be copy constructible");
        static_assert(std::is_convertible<_Up, value_type>::value,
                      "optional<T>::value_or: U must be convertible to T");
        return this->has_value() ? this->__get() :
                                  static_cast<value_type>(std::forward<_Up>(__v));
    }

    template <class _Up>
    value_type value_or(_Up&& __v) &&
    {
        static_assert(std::is_move_constructible<value_type>::value,
                      "optional<T>::value_or: T must be move constructible");
        static_assert(std::is_convertible<_Up, value_type>::value,
                      "optional<T>::value_or: U must be convertible to T");
        return this->has_value() ? std::move(this->__get()) :
                                  static_cast<value_type>(std::forward<_Up>(__v));
    }

    using __base::reset;

private:
    template <class _Up>
    static _Up*
    __operator_arrow(std::true_type, _Up& __x)
    {
        return std::addressof(__x);
    }

    template <class _Up>
    static constexpr _Up*
    __operator_arrow(std::false_type, _Up& __x)
    {
        return &__x;
    }
};

// Comparisons between optionals
template <class _Tp>
WR_CXX14_CONSTEXPR
typename std::enable_if<
    std::is_convertible<decltype(std::declval<const _Tp&>() ==
        std::declval<const _Tp&>()), bool>::value,
    bool
>::type
operator==(const optional<_Tp>& __x, const optional<_Tp>& __y)
{
    if (static_cast<bool>(__x) != static_cast<bool>(__y))
        return false;
    if (!static_cast<bool>(__x))
        return true;
    return *__x == *__y;
}

template <class _Tp>
WR_CXX14_CONSTEXPR
typename std::enable_if<
    std::is_convertible<decltype(std::declval<const _Tp&>() !=
        std::declval<const _Tp&>()), bool>::value,
    bool
>::type
operator!=(const optional<_Tp>& __x, const optional<_Tp>& __y)
{
    if (static_cast<bool>(__x) != static_cast<bool>(__y))
        return true;
    if (!static_cast<bool>(__x))
        return false;
    return *__x != *__y;
}

template <class _Tp>
WR_CXX14_CONSTEXPR
typename std::enable_if<
    std::is_convertible<decltype(std::declval<const _Tp&>() <
        std::declval<const _Tp&>()), bool>::value,
    bool
>::type
operator<(const optional<_Tp>& __x, const optional<_Tp>& __y)
{
    if (!static_cast<bool>(__y))
        return false;
    if (!static_cast<bool>(__x))
        return true;
    return *__x < *__y;
}

template <class _Tp>
WR_CXX14_CONSTEXPR
typename std::enable_if<
    std::is_convertible<decltype(std::declval<const _Tp&>() >
        std::declval<const _Tp&>()), bool>::value,
    bool
>::type
operator>(const optional<_Tp>& __x, const optional<_Tp>& __y)
{
    if (!static_cast<bool>(__x))
        return false;
    if (!static_cast<bool>(__y))
        return true;
    return *__x > *__y;
}

template <class _Tp>
WR_CXX14_CONSTEXPR
typename std::enable_if<
    std::is_convertible<decltype(std::declval<const _Tp&>() <=
        std::declval<const _Tp&>()), bool>::value,
    bool
>::type
operator<=(const optional<_Tp>& __x, const optional<_Tp>& __y)
{
    if (!static_cast<bool>(__x))
        return true;
    if (!static_cast<bool>(__y))
        return false;
    return *__x <= *__y;
}

template <class _Tp>
WR_CXX14_CONSTEXPR
typename std::enable_if<
    std::is_convertible<decltype(std::declval<const _Tp&>() >=
        std::declval<const _Tp&>()), bool>::value,
    bool
>::type
operator>=(const optional<_Tp>& __x, const optional<_Tp>& __y)
{
    if (!static_cast<bool>(__y))
        return true;
    if (!static_cast<bool>(__x))
        return false;
    return *__x >= *__y;
}

// Comparisons with nullopt
template <class _Tp>
constexpr
bool
operator==(const optional<_Tp>& __x, nullopt_t) noexcept
{
    return !static_cast<bool>(__x);
}

template <class _Tp>
constexpr
bool
operator==(nullopt_t, const optional<_Tp>& __x) noexcept
{
    return !static_cast<bool>(__x);
}

template <class _Tp>
constexpr
bool
operator!=(const optional<_Tp>& __x, nullopt_t) noexcept
{
    return static_cast<bool>(__x);
}

template <class _Tp>
constexpr
bool
operator!=(nullopt_t, const optional<_Tp>& __x) noexcept
{
    return static_cast<bool>(__x);
}

template <class _Tp>
constexpr
bool
operator<(const optional<_Tp>&, nullopt_t) noexcept
{
    return false;
}

template <class _Tp>
constexpr
bool
operator<(nullopt_t, const optional<_Tp>& __x) noexcept
{
    return static_cast<bool>(__x);
}

template <class _Tp>
constexpr
bool
operator<=(const optional<_Tp>& __x, nullopt_t) noexcept
{
    return !static_cast<bool>(__x);
}

template <class _Tp>
constexpr
bool
operator<=(nullopt_t, const optional<_Tp>&) noexcept
{
    return true;
}

template <class _Tp>
constexpr
bool
operator>(const optional<_Tp>& __x, nullopt_t) noexcept
{
    return static_cast<bool>(__x);
}

template <class _Tp>
constexpr
bool
operator>(nullopt_t, const optional<_Tp>&) noexcept
{
    return false;
}

template <class _Tp>
constexpr
bool
operator>=(const optional<_Tp>&, nullopt_t) noexcept
{
    return true;
}

template <class _Tp>
constexpr
bool
operator>=(nullopt_t, const optional<_Tp>& __x) noexcept
{
    return !static_cast<bool>(__x);
}

// Comparisons with T
template <class _Tp>
constexpr
typename std::enable_if<
    std::is_convertible<decltype(std::declval<const _Tp&>() ==
        std::declval<const _Tp&>()), bool>::value,
    bool
>::type
operator==(const optional<_Tp>& __x, const _Tp& __v)
{
    return static_cast<bool>(__x) ? *__x == __v : false;
}

template <class _Tp>
constexpr
typename std::enable_if<
    std::is_convertible<decltype(std::declval<const _Tp&>() ==
        std::declval<const _Tp&>()), bool>::value,
    bool
>::type
operator==(const _Tp& __v, const optional<_Tp>& __x)
{
    return static_cast<bool>(__x) ? __v == *__x : false;
}

template <class _Tp>
constexpr
typename std::enable_if<
    std::is_convertible<decltype(std::declval<const _Tp&>() !=
        std::declval<const _Tp&>()), bool>::value,
    bool
>::type
operator!=(const optional<_Tp>& __x, const _Tp& __v)
{
    return static_cast<bool>(__x) ? *__x != __v : true;
}

template <class _Tp>
constexpr
typename std::enable_if<
    std::is_convertible<decltype(std::declval<const _Tp&>() !=
        std::declval<const _Tp&>()), bool>::value,
    bool
>::type
operator!=(const _Tp& __v, const optional<_Tp>& __x)
{
    return static_cast<bool>(__x) ? __v != *__x : true;
}

template <class _Tp>
constexpr
typename std::enable_if<
    std::is_convertible<decltype(std::declval<const _Tp&>() <
        std::declval<const _Tp&>()), bool>::value,
    bool
>::type
operator<(const optional<_Tp>& __x, const _Tp& __v)
{
    return static_cast<bool>(__x) ? *__x < __v : true;
}

template <class _Tp>
constexpr
typename std::enable_if<
    std::is_convertible<decltype(std::declval<const _Tp&>() <
        std::declval<const _Tp&>()), bool>::value,
    bool
>::type
operator<(const _Tp& __v, const optional<_Tp>& __x)
{
    return static_cast<bool>(__x) ? __v < *__x : false;
}

template <class _Tp>
constexpr
typename std::enable_if<
    std::is_convertible<decltype(std::declval<const _Tp&>() <=
        std::declval<const _Tp&>()), bool>::value,
    bool
>::type
operator<=(const optional<_Tp>& __x, const _Tp& __v)
{
    return static_cast<bool>(__x) ? *__x <= __v : true;
}

template <class _Tp>
constexpr
typename std::enable_if<
    std::is_convertible<decltype(std::declval<const _Tp&>() <=
        std::declval<const _Tp&>()), bool>::value,
    bool
>::type
operator<=(const _Tp& __v, const optional<_Tp>& __x)
{
    return static_cast<bool>(__x) ? __v <= *__x : false;
}

template <class _Tp>
constexpr
typename std::enable_if<
    std::is_convertible<decltype(std::declval<const _Tp&>() >
        std::declval<const _Tp&>()), bool>::value,
    bool
>::type
operator>(const optional<_Tp>& __x, const _Tp& __v)
{
    return static_cast<bool>(__x) ? *__x > __v : false;
}

template <class _Tp>
constexpr
typename std::enable_if<
    std::is_convertible<decltype(std::declval<const _Tp&>() >
        std::declval<const _Tp&>()), bool>::value,
    bool
>::type
operator>(const _Tp& __v, const optional<_Tp>& __x)
{
    return static_cast<bool>(__x) ? __v > *__x : true;
}

template <class _Tp>
constexpr
typename std::enable_if<
    std::is_convertible<decltype(std::declval<const _Tp&>() >=
        std::declval<const _Tp&>()), bool>::value,
    bool
>::type
operator>=(const optional<_Tp>& __x, const _Tp& __v)
{
    return static_cast<bool>(__x) ? *__x >= __v : false;
}

template <class _Tp>
constexpr
typename std::enable_if<
    std::is_convertible<decltype(std::declval<const _Tp&>() >=
        std::declval<const _Tp&>()), bool>::value,
    bool
>::type
operator>=(const _Tp& __v, const optional<_Tp>& __x)
{
    return static_cast<bool>(__x) ? __v >= *__x : true;
}


template <class _Tp>
inline
typename std::enable_if<
    std::is_move_constructible<_Tp>::value /*&& std::is_swappable<_Tp>::value*/,
    void
>::type
swap(optional<_Tp>& __x, optional<_Tp>& __y) noexcept(noexcept(__x.swap(__y)))
{
    __x.swap(__y);
}

template <class _Tp>
constexpr
optional<typename std::decay<_Tp>::type> make_optional(_Tp&& __v)
{
    return optional<typename std::decay<_Tp>::type>(std::forward<_Tp>(__v));
}

template <class _Tp, class... _Args>
constexpr
optional<_Tp> make_optional(_Args&&... __args)
{
    return optional<_Tp>(in_place, std::forward<_Args>(__args)...);
}

template <class _Tp, class _Up, class... _Args>
constexpr
optional<_Tp> make_optional(std::initializer_list<_Up> __il,  _Args&&... __args)
{
    return optional<_Tp>(in_place, __il, std::forward<_Args>(__args)...);
}


} // namespace wr


namespace std {


template <class _Tp>
struct hash<wr::optional<_Tp> >
{
    typedef wr::optional<_Tp> argument_type;
    typedef size_t            result_type;

    result_type operator()(const argument_type& __opt) const noexcept
    {
        return static_cast<bool>(__opt) ? hash<_Tp>()(*__opt) : 0;
    }
};


} // namespace std


#endif // WR_HAVE_STD_OPTIONAL


namespace wr {
namespace fmt {


template <typename T> struct TypeHandler<optional<T>>
{
        static void set(Arg &arg, const optional<T> &data)
        {
                if (data.has_value()) {
                        arg.set(data.value());
                } else {
                        arg.set("(null)");
                }
        }
};


} // namespace fmt
} // namespace wr


#endif // !WRUTIL_OPTIONAL_H
