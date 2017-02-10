/**
 * \file Format.h
 *
 * \brief Formatted output API
 *
 * \copyright
 * \parblock
 *
 *   Copyright 2013-2016 James S. Waller
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
#ifndef WRUTIL_FORMAT_H
#define WRUTIL_FORMAT_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <iostream>
#include <locale>
#include <string>
#include <type_traits>

#include <wrutil/Config.h>


namespace wr {
namespace fmt {


enum
{
        ALT_FORM        = 1 << 0,
        ZERO_PAD        = 1 << 1,
        LEFT_ALIGN      = 1 << 2,
        CENTRE_ALIGN    = 1 << 3,
        PAD_POS_SIGN    = 1 << 4,
        SHOW_POS_SIGN   = 1 << 5,
        GROUP_THOU      = 1 << 6,
        HAVE_WIDTH      = 1 << 7,
        HAVE_PRECIS     = 1 << 8
};

//--------------------------------------

struct Arg;
class Target;

//--------------------------------------

struct Params
{
        Target    &target;
        const Arg *arg;
        uintmax_t  flags,
                   width,
                   precis;
        char       conv;
};

//--------------------------------------

template <typename T> struct TypeHandler;

using FormatFn = bool (*)(const Params &);

//--------------------------------------

struct Arg
{
        enum Type
        {
                VOID_T = 0,
                INT_T,
                UINT_T,
                DBL_T,
                STR_T,
                PINT16_T,
                PUINT16_T,
                PINT32_T,
                PUINT32_T,
                PINT64_T,
                PUINT64_T,
                OTHER_T
        };

        Type type;

        union
        {
                intmax_t    i;
                uintmax_t   u;
                double      f;

                struct
                {
                        const char *data;
                        size_t      length;
                } s;

                int16_t    *pint16;
                uint16_t   *puint16;
                int32_t    *pint32;
                uint32_t   *puint32;
                int64_t    *pint64;
                uint64_t   *puint64;
                const void *other;
        };

        FormatFn fmt_fn = nullptr;


        Arg() : type(VOID_T) {}

        template <typename T> void set(T &&val)
                { TypeHandler<typename std::decay<T>::type>
                        ::set(*this, std::forward<T>(val)); }

        WRUTIL_API uintmax_t toUInt(bool &ok) const;

        template <typename Next, typename ...Rest>
        static void setArray(Arg *next_out, Next &&next_in, Rest &&...rest)
        {
                TypeHandler<typename std::decay<Next>::type>
                        ::set(*next_out, std::forward<Next>(next_in));
                setArray(++next_out, std::forward<Rest>(rest)...);
        }

        static void setArray(Arg *) {}  /**< sentinel version of setArray(),
                                             does nothing */
};

//--------------------------------------

class WRUTIL_API Target
{
public:
        virtual ~Target();

        virtual void begin();
        virtual void put(char c) = 0;
        virtual void put(const char *chars, uintmax_t count);
        void put(const char *chars);
        virtual intmax_t end();
        virtual std::locale locale() const;
        virtual uintmax_t count() const = 0;

        bool format(const Params &params,
                    const Arg *arg = nullptr, char conv = 0);
};

//--------------------------------------

WRUTIL_API intmax_t print(Target &target, const char *fmt, const Arg *argv,
                          int argc);

//--------------------------------------

struct NumConvResults
{
        uintmax_t   len;
        const char *prefix,
                   *body;
};

//--------------------------------------

template <typename NumT> WRUTIL_API
char *toDecStr(const Params &params, NumT value,
               char *buf, uintmax_t bufsize, NumConvResults &res);

template <typename NumT> WRUTIL_API
char *toOctStr(const Params &params, NumT value,
               char *buf, uintmax_t bufsize, NumConvResults &res);

template <typename NumT> WRUTIL_API
char *toHexStr(const Params &params, NumT value,
               char *buf, uintmax_t bufsize, NumConvResults &res);

//--------------------------------------

class WRUTIL_API IOStreamTarget :
        public Target
{
public:
        IOStreamTarget(std::ostream &s);

        virtual void begin();
        virtual void put(char c);
        virtual void put(const char *chars, uintmax_t count);
        virtual std::locale locale() const;
        virtual uintmax_t count() const;

private:
        std::ostream &stream_;
        uintmax_t     count_;
};

//--------------------------------------

class WRUTIL_API CStreamTarget :
        public Target
{
public:
        CStreamTarget(FILE *s);

        virtual void begin();
        virtual void put(char c);
        virtual void put(const char *chars, uintmax_t count);
        virtual uintmax_t count() const;

private:
        FILE      *stream_;
        uintmax_t  count_;
};

//--------------------------------------

class WRUTIL_API FixedBufferTarget :
        public Target
{
public:
        FixedBufferTarget(char *buf, uintmax_t capacity);

        virtual void begin();
        virtual void put(char c);
        virtual void put(const char *chars, uintmax_t count);
        virtual intmax_t end();
        virtual uintmax_t count() const;

private:
        char *buf_, *stop_, *pos_;
};

//--------------------------------------

class WRUTIL_API StringTarget :
        public Target
{
public:
        StringTarget(std::string &s);

        virtual void begin();
        virtual void put(char c);
        virtual void put(const char *chars, uintmax_t count);
        virtual uintmax_t count() const;

private:
        std::string &str_;
        uintmax_t    initial_len_;
};

//--------------------------------------

template <>
struct TypeHandler<bool>
{
        static void set(Arg &arg, bool val)
        {
                arg.type = Arg::INT_T;
                arg.i = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<char>
{
        static void set(Arg &arg, char val)
        {
                arg.type = Arg::INT_T;
                arg.i = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<signed char>
{
        static void set(Arg &arg, signed char val)
        {
                arg.type = Arg::INT_T;
                arg.i = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<unsigned char>
{
        static void set(Arg &arg, unsigned char val)
        {
                arg.type = Arg::UINT_T;
                arg.u = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<wchar_t>
{
        static void set(Arg &arg, wchar_t val)
        {
                arg.type = Arg::INT_T;
                arg.i = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<char16_t>
{
        static void set(Arg &arg, char16_t val)
        {
                arg.type = Arg::INT_T;
                arg.i = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<char32_t>
{
        static void set(Arg &arg, char32_t val)
        {
                arg.type = Arg::INT_T;
                arg.i = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<short>
{
        static void set(Arg &arg, short val)
        {
                arg.type = Arg::INT_T;
                arg.i = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<unsigned short>
{
        static void set(Arg &arg, unsigned short val)
        {
                arg.type = Arg::UINT_T;
                arg.u = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<int>
{
        static void set(Arg &arg, int val)
        {
                arg.type = Arg::INT_T;
                arg.i = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<unsigned int>
{
        static void set(Arg &arg, unsigned int val)
        {
                arg.type = Arg::UINT_T;
                arg.u = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<long>
{
        static void set(Arg &arg, long val)
        {
                arg.type = Arg::INT_T;
                arg.i = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<unsigned long>
{
        static void set(Arg &arg, unsigned long val)
        {
                arg.type = Arg::UINT_T;
                arg.u = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<long long>
{
        static void set(Arg &arg, long long val)
        {
                arg.type = Arg::INT_T;
                arg.i = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<unsigned long long>
{
        static void set(Arg &arg, unsigned long long val)
        {
                arg.type = Arg::UINT_T;
                arg.u = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<float>
{
        static void set(Arg &arg, float val)
        {
                arg.type = Arg::DBL_T;
                arg.f = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<double>
{
        static void set(Arg &arg, double val)
        {
                arg.type = Arg::DBL_T;
                arg.f = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<std::string>
{
        static void set(Arg &arg, const std::string &val)
        {
                arg.type = Arg::STR_T;
                arg.s.data = val.data();
                arg.s.length = val.size();
        }
};

//--------------------------------------

template <>
struct TypeHandler<char *>
{
        static void set(Arg &arg, const char *val)
        {
                arg.type = Arg::STR_T;
                arg.s.data = val;
                arg.s.length = strlen(val);
        }
};

template <> struct TypeHandler<const char *> : TypeHandler<char *> {};

//--------------------------------------

template <>
struct TypeHandler<int16_t *>
{
        static void set(Arg &arg, int16_t *val)
        {
                arg.type = Arg::PINT16_T;
                arg.pint16 = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<uint16_t *>
{
        static void set(Arg &arg, uint16_t *val)
        {
                arg.type = Arg::PUINT16_T;
                arg.puint16 = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<int32_t *>
{
        static void set(Arg &arg, int32_t *val)
        {
                arg.type = Arg::PINT32_T;
                arg.pint32 = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<uint32_t *>
{
        static void set(Arg &arg, uint32_t *val)
        {
                arg.type = Arg::PUINT32_T;
                arg.puint32 = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<int64_t *>
{
        static void set(Arg &arg, int64_t *val)
        {
                arg.type = Arg::PINT64_T;
                arg.pint64 = val;
        }
};

//--------------------------------------

template <>
struct TypeHandler<uint64_t *>
{
        static void set(Arg &arg, uint64_t *val)
        {
                arg.type = Arg::PUINT64_T;
                arg.puint64 = val;
        }
};

//--------------------------------------

template <typename T>
struct TypeHandler<T *>
{
        static void set(Arg &arg, const T *val)
        {
                arg.type = Arg::OTHER_T;
                arg.other = val;
        }
};


} // namespace fmt


//--------------------------------------

template <typename ...Args> intmax_t
print(
        fmt::Target     &target,
        const char      *fmt,
        Args        &&...in_args
)
{
        fmt::Arg argv[sizeof...(in_args) ? sizeof...(in_args) : 1];
        fmt::Arg::setArray(argv, std::forward<Args>(in_args)...);
        return fmt::print(target, fmt, argv, sizeof...(in_args));
}

//--------------------------------------

template <typename ...Args> intmax_t
print(
        std::string &str,
        const char  *fmt,
        Args    &&...in_args
)
{
        std::string       tmp;
        fmt::StringTarget target(tmp);
        intmax_t result = print(target, fmt, std::forward<Args>(in_args)...);
        str = std::move(tmp);
        return result;
}

//--------------------------------------

template <typename ...Args> intmax_t
print(
        std::ostream &stream,
        const char   *fmt,
        Args     &&...in_args
)
{
        fmt::IOStreamTarget target(stream);
        return print(target, fmt, std::forward<Args>(in_args)...);
}

//--------------------------------------

template <typename ...Args> intmax_t
print(
        FILE       *stream,
        const char *fmt,
        Args   &&...in_args
)
{
        fmt::CStreamTarget target(stream);
        return print(target, fmt, std::forward<Args>(in_args)...);
}

//--------------------------------------

template <typename ...Args> intmax_t
print(
        char       *buf,
        uintmax_t   capacity,
        const char *fmt,
        Args   &&...in_args
)
{
        fmt::FixedBufferTarget target(buf, capacity);
        return print(target, fmt, std::forward<Args>(in_args)...);
}

//--------------------------------------

template <uintmax_t capacity, typename ...Args> intmax_t
print(
        char      (&buf)[capacity],
        const char *fmt,
        Args   &&...in_args
)
{
        fmt::FixedBufferTarget target(buf, capacity);
        return print(target, fmt, std::forward<Args>(in_args)...);
}

//--------------------------------------

template <typename ...Args> std::string
printStr(
        const char *fmt,
        Args   &&...in_args
)
{
        std::string       result;
        fmt::StringTarget target(result);
        print(target, fmt, std::forward<Args>(in_args)...);
        return result;
}


} // namespace wr


#endif // !WRUTIL_FORMAT_H
