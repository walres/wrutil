/**
 * \file Format.cxx
 *
 * \brief Implementation of formatted text output API
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
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <wrutil/Format.h>
#include <wrutil/numeric_cast.h>
#include <wrutil/utf8.h>


namespace wr {
namespace fmt {


static const char *convert(Target &target, const char *spec, const Arg *argv,
                           int argc, int &next_arg_ix);

//--------------------------------------

WRUTIL_API uintmax_t
Arg::toUInt(
        bool &ok
) const
{
        uintmax_t val = 0;

        switch (type) {
        case UINT_T:
                ok  = true;
                val = u;
                break;
        case INT_T:
                ok  = (i >= 0);
                if (ok) {
                        val = static_cast<uintmax_t>(i);
                } else {
                        errno = ERANGE;
                }
                break;
        case DBL_T:
                ok = (f >= 0.0) && (f <= UINTMAX_MAX);
                ok = ok && std::isfinite(f);

                if (ok) {
                        val = static_cast<uintmax_t>(f);
                } else {
                        errno = ERANGE;
                }
                break;
        case STR_T:
                if (s.data) {
                        try {
                                std::string tmp(s.data, s.length);
                                val = std::stoull(tmp);
                                ok = true;
                        } catch (const std::out_of_range &) {
                                ok = false;
                                errno = ERANGE;
                        } catch (const std::invalid_argument &) {
                                ok = false;
                                errno = EINVAL;
                        }
                } else {
                        ok = false;
                }
                break;
        case PINT16_T:
                ok = *pint16 >= 0;
                if (ok) {
                        val = static_cast<uintmax_t>(*pint16);
                } else {
                        val = ERANGE;
                }
                break;
        case PUINT16_T:
                ok  = true;
                val = *puint16;
                break;
        case PINT32_T:
                ok = *pint32 >= 0;
                if (ok) {
                        val = static_cast<uintmax_t>(*pint32);
                } else {
                        val = ERANGE;
                }
                break;
        case PUINT32_T:
                ok  = true;
                val = *puint32;
                break;
        case PINT64_T:
                ok = *pint64 >= 0;
                if (ok) {
                        val = static_cast<uintmax_t>(*pint64);
                } else {
                        val = ERANGE;
                }
                break;
        case PUINT64_T:
                ok  = true;
                val = *puint64;
                break;
        default:
                ok  = false;
                val = 0;
                break;
        }

        return val;
}

//--------------------------------------

namespace {


template <typename NumT, bool = std::is_signed<NumT>::value>
struct SetSignAndValue;

//--------------------------------------

template <typename NumT>
struct SetSignAndValue<NumT, false>
{
        static void
        apply(
                const Params &parms,
                NumT         &/* value */,
                char         &sign
        )
        {
                if (parms.flags & SHOW_POS_SIGN) {
                        sign = '+';
                } else if (parms.flags & PAD_POS_SIGN) {
                        sign = ' ';
                } else {
                        sign = '\0';
                }
        }        
};

//--------------------------------------

template <typename NumT>
struct SetSignAndValue<NumT, true>
{
        static void
        apply(
                const Params &parms,
                NumT         &value,
                char         &sign
        )
        {
                if (value >= 0) {
                        SetSignAndValue<NumT, false>::apply(parms, value, sign);
                } else {
                        sign = '-';
                        value = -value;
                }
        }
};


} // anonymous namespace

//--------------------------------------

template <typename NumT> WRUTIL_API char *
toDecStr(
        const Params   &params,
        NumT            value,
        char           *buf,
        uintmax_t       bufsize,
        NumConvResults &res
)
{
        if (bufsize == 0) {
                errno = EINVAL;
                return nullptr;
        }

        char       *p        = buf + bufsize,
                    sep      = '\0',
                    sign     = '\0';
        std::string grouping;
        size_t      gi       = 0,
                    glen     = 0,
                    n_digits = 0;
        int         grp_size = -1;

        SetSignAndValue<NumT>::apply(params, value, sign);

        if (params.flags & GROUP_THOU) {
                auto  locale   = params.target.locale();
                auto &loc_data = std::use_facet<std::numpunct<char>>(locale);
                grouping = loc_data.grouping();
                glen     = grouping.length();
                sep      = loc_data.thousands_sep();
        }

        do {
                if (!grouping.empty() && grp_size < 0) {
                        if (grouping[gi] == CHAR_MAX) {
                                grouping.erase();
                        } else {
                                if (gi >= glen || grouping[gi] == '\0') {
                                        --gi;
                                }
                                grp_size = grouping[gi++];
                        }
                }

                if (p-- == buf) {
                        errno = ENOSPC;
                        return nullptr;
                }

                if (grouping.empty() || grp_size-- > 0) {
                        *p = (char) (value % 10) + '0';
                        ++n_digits;
                        value /= 10;
                } else {
                        *p = sep;
                }
        } while (value != 0 || n_digits < params.precis);

        res.body = p;

        if (sign) {
                if (--p < buf) {
                        errno = ENOSPC;
                        return nullptr;
                }
                *p = sign;
        }

        res.prefix = p;
        res.len    = (buf + bufsize) - p;
        return p;
}

//--------------------------------------

template <> WRUTIL_API char *
toDecStr(
        const Params   &params,
        double          value,
        char           *buf,
        uintmax_t       bufsize,
        NumConvResults &res
)
{
        char  buf2   [80],
              fmt_buf[8],
             *fmt    = fmt_buf + sizeof(fmt_buf) - 1;

        /*
         * delegate to C printf() for basic conversion
         */
        int printed;

        *fmt = '\0';
        *(--fmt) = (params.conv == 'F') ? 'f' : params.conv;
                // some versions of printf() don't recognise 'F'

        if (params.flags & HAVE_PRECIS) {
                *(--fmt) = '*';
                *(--fmt) = '.';
        }

        if (params.flags & PAD_POS_SIGN) {
                *(--fmt) = ' ';
        } else if (params.flags & SHOW_POS_SIGN) {
                *(--fmt) = '+';
        }

        *(--fmt) = '%';

        if (params.flags & HAVE_PRECIS) {
                printed = ::snprintf(buf2,
                                     sizeof(buf2), fmt, params.precis, value);
        } else {
                printed = ::snprintf(buf2, sizeof(buf2), fmt, value);
        }

        if (printed < 0) {
                errno = ENOSPC;
                return nullptr;
        }

        if (params.conv == 'F' && !std::isfinite(value)) {
                for (char *p = buf2; *p; ++p) {
                        *p = toupper(*p);
                }
        }

        std::locale    locale     = params.target.locale();
        const auto    &loc_data   = std::use_facet<std::numpunct<char>>(locale);
        const ::lconv *c_loc_data = ::localeconv();
        const char    *dp         = strstr(buf2, c_loc_data->decimal_point),
                      *r          = buf2 + printed - 1;
        char          *w          = buf + bufsize - 1;

        *w = '\0';

        /*
         * output digits after the decimal point (if any)
         */
        if (dp) {
                uintmax_t dp_len = strlen(c_loc_data->decimal_point);

                for (const char *stop = dp + dp_len - 1; r > stop; --r) {
                        *(--w) = *r;
                }

                r -= dp_len;
                *(--w) = loc_data.decimal_point();
        } else if (params.flags & ALT_FORM) {  // always show decimal point
                *(--w) = loc_data.decimal_point();
        }

        std::string grouping;
        size_t      gi       = 0,
                    glen     = 0;
        char        sep      = '\0';
        int         grp_size = -1;

        if (std::isfinite(value) && (params.flags & GROUP_THOU)) {
                grouping = loc_data.grouping();
                glen     = grouping.length();
                sep      = loc_data.thousands_sep();
        }

        res.body = nullptr;

        while (r >= buf2) {
                if (!grouping.empty() && (grp_size < 0 || !isdigit(*r))) {
                        if (!isdigit(*r)) {  // reached sign
                                res.body = w;
                                grouping.erase();
                        } else if (grouping[gi] == CHAR_MAX) {
                                grouping.erase();
                        } else {
                                if (gi >= glen || grouping[gi] == '\0') {
                                        --gi;
                                }
                                grp_size = grouping[gi++];
                        }
                        continue;
                }

                if (w == buf) {
                        errno = ENOSPC;
                        return nullptr;
                }

                *(--w) = (grouping.empty() || grp_size-- > 0) ? *(r--) : sep;
        }

        res.prefix = w;
        res.len    = (buf + bufsize) - w - 1;
        return w;
}

//--------------------------------------

template <typename NumT> WRUTIL_API char *
toOctStr(
        const Params   &params,
        NumT            value,
        char           *buf,
        uintmax_t       bufsize,
        NumConvResults &res
)
{
        if (bufsize == 0) {
                errno = EINVAL;
                return nullptr;
        }

        char      *p        = buf + bufsize;
        uintmax_t  n_digits = 0;

        do {
                if (--p == buf) {
                        errno = ENOSPC;
                        return nullptr;
                }

                *p = (char) (value & 7) + '0';
                ++n_digits;
                value >>= 3;
        } while (value != 0 || n_digits < params.precis);

        res.body = p;

        if ((params.flags & ALT_FORM) && *p != '0') {
                if (--p < buf) {
                        errno = ENOSPC;
                        return nullptr;
                }
                *p = '0';
        }

        res.prefix = p;
        res.len    = (buf + bufsize) - p;
        return p;
}

//--------------------------------------

static const char UPPER_DIGITS[] = "0123456789ABCDEFXP",
                  LOWER_DIGITS[] = "0123456789abcdefxp";

//--------------------------------------

template <typename NumT> WRUTIL_API char *
toHexBinStr(
        const Params   &params,
        NumT            value,
        char           *buf,
        uintmax_t       bufsize,
        NumConvResults &res
)
{
        NumT     mask;
        unsigned shift,
                 base_char_ix;

        if (bufsize == 0) {
                errno = EINVAL;
                return nullptr;
        } else {
                switch (tolower(params.conv)) {
                case 'b': case 'B':     // binary
                        mask = 1;
                        shift = 1;
                        base_char_ix = 11;
                        break;
                default:                // hex
                        mask = 0xf;
                        shift = 4;
                        base_char_ix = 16;
                        break;
                }
        }

        char       *p        = buf + bufsize;
        uintmax_t   n_digits = 0;
        const char *digits = isupper(params.conv) ? UPPER_DIGITS : LOWER_DIGITS;

        do {
                if (p-- == buf) {
                        errno = ENOSPC;
                        return nullptr;
                }

                *p = digits[value & mask];
                ++n_digits;
                value >>= shift;
        } while (value != 0 || n_digits < params.precis);

        res.body = p;

        if (params.flags & ALT_FORM) {
                if ((p - buf) < 1) {
                        errno = ENOSPC;
                        return nullptr;
                }

                *(--p) = digits[base_char_ix];
                *(--p) = '0';
        }

        res.prefix = p;
        res.len    = (buf + bufsize) - p;
        return p;
}

//--------------------------------------

WRUTIL_API char *
toHexStr(
        const Params   &params,
        double          value,
        char           *buf,
        uintmax_t       bufsize,
        NumConvResults &res
)
{
        if (bufsize < 4) {      // definitely not enough room
                errno = ENOSPC;
                return nullptr;
        }

        /*
         * gather numeric data and handle output for NaN/infinity cases
         */
        struct {
                uint64_t mantissa : 52,
                         exponent : 11,
                         sign     : 1;
        } *raw = reinterpret_cast<decltype(raw)>(&value);

        intmax_t exponent;
        uint64_t mantissa = raw->mantissa;

        if (raw->exponent == 0) {  // zero or subnormal
                exponent = 0;
        } else if (raw->exponent == 0x7ff) {  // infinity or not-a-number
                if (raw->mantissa != 0) {  // not-a-number
                        memcpy(buf, isupper(params.conv) ? "NAN" : "nan", 3);
                        res.body = res.prefix = buf;
                        res.len  = 3;
                        return buf;
                }
                // else handle infinity case below after handling sign
        } else {
                exponent = raw->exponent - 0x3ff;
                mantissa |= 0x0010000000000000;   // tack on implicit 1
        }

        res.prefix = buf;

        char *       w   = buf,
             * const end = buf + bufsize;

        /*
         * output sign and radix prefix
         */
        if (raw->sign) {
                *(w++) = '-';
        } else if (params.flags & PAD_POS_SIGN) {
                *(w++) = ' ';
        } else if (params.flags & SHOW_POS_SIGN) {
                *(w++) = '+';
        }

        if (raw->exponent == 0x7ff) {  // infinity
                memcpy(w, isupper(params.conv) ? "INF" : "inf", 3);
                res.body = res.prefix = buf;
                res.len  = (w - buf) + 3;
                return buf;
        }

        const char *digits = isupper(params.conv) ? UPPER_DIGITS : LOWER_DIGITS;

        *(w++) = '0';
        *(w++) = digits[16];

        /*
         * round mantissa up if necessary to suit precision
         */
        bool have_precis = (params.flags & HAVE_PRECIS) != 0;

        if (have_precis && params.precis < 14) {
                uintmax_t shift = ((13 - (params.precis + 1)) << 2);

                mantissa >>= shift;
                        // 1st digit to lose is now the least-significant

                if ((mantissa & 0xf) >= 8) {  // round up
                        mantissa += 0x10;
                }

                mantissa = (mantissa & (~0xf)) << shift;
        }

        /*
         * output mantissa
         */
        uintmax_t num_digits = 0;
        bool      point      = false;

        res.body = w;

        while (true) {
                if (w >= end) {
                        errno = ENOSPC;
                        return nullptr;
                }

                if (num_digits == 1 && !point) {
                        std::locale locale = params.target.locale();
                        *w = std::use_facet<std::numpunct<char>>(locale)
                                .decimal_point();
                        point = true;
                } else {
                        *w       = digits[(mantissa >> 52) & 0xf];
                        mantissa = (mantissa << 4) & 0x00ffffffffffffff;
                        ++num_digits;
                }

                ++w;

                if ((mantissa == 0 && num_digits >= 1)
                    && (!have_precis || num_digits > params.precis)) {
                        if (point || !(params.flags & ALT_FORM)) {
                                break;
                        } /* else '#' flag used so point always wanted;
                             break on next iteration */
                }
        }

        /*
         * output 'p'/'P' followed by exponent
         */
        *(w++) = digits[17];

        NumConvResults exp_res;
        const char *exp = toDecStr(Params { params.target, nullptr,
                                            SHOW_POS_SIGN, 0, 0, 'd' },
                                   exponent, w, bufsize - (w - buf), exp_res);
        if (!exp) {
                return nullptr;
        }

        for (const char *end = exp + exp_res.len; exp != end; ++exp, ++w) {
                *w = *exp;
        }

        res.len = w - buf;
        return buf;
}

//--------------------------------------

template <typename IntT> bool
convIntPtr(
        const Params &params,
        IntT         *value
)
{
        if (!value) {
                errno = EINVAL;
                return false;
        }

        Arg       tmp;
        uintmax_t count;

        switch (params.conv) {
        case 'p':
                tmp.set(reinterpret_cast<uintptr_t>(value));
                return params.target.format(params, &tmp, 0);
        case 'n':
                count = params.target.count();
                if (count <= static_cast<uintmax_t>(
                                        std::numeric_limits<IntT>::max())) {
                        *value = static_cast<IntT>(count);
                        return true;
                } else {
                        errno = ERANGE;
                        return false;
                }
                break;
        default:
                tmp.set(*value);
                return params.target.format(params, &tmp, 0);
        }
}

//--------------------------------------

WRUTIL_API void
Target::put(
        const char *chars
)
{
        put(chars, strlen(chars));
}

//--------------------------------------

WRUTIL_API bool
Target::format(
        const Params &params,
        const Arg    *arg,
        char          conv
)
{
        uintmax_t      flags    = params.flags,
                       width    = params.width,
                       precis   = params.precis;
        char           buf[80];
        bool           ok       = false;
        NumConvResults res      = { 0, nullptr, nullptr };

        if (!arg) {
                arg = params.arg;
        }
        if (!conv) {
                conv = params.conv;
        }

        if (conv == 'm') {  // needs no argument
                Arg tmp;
                tmp.set(strerror(errno));
                return format(params, &tmp, 's');
        } else if (!arg) {
                ok = (conv == 'n');

                if (ok) {
                        ;  // XXX to be implemented
                } else {
                        errno = EINVAL;  // invalid conversion
                }

                return ok;
        }

        Arg::Type type = arg->type;

        switch (type) {
        case Arg::INT_T: case Arg::UINT_T:
                switch (conv) {
                case 'd': case 'i':
                        if (type == Arg::INT_T) {
                                toDecStr(params, arg->i, buf, sizeof(buf), res);
                        } else {
                                toDecStr(params, arg->u, buf, sizeof(buf), res);
                        }
                        break;
                case 'o':
                        toOctStr(params, arg->u, buf, sizeof(buf), res);
                        break;
                case 'u':
                        toDecStr(params, arg->u, buf, sizeof(buf), res);
                        break;
                case 'p':
                        flags |= ALT_FORM;
                case 'X': case 'x': case 'B': case 'b':
                        {
                                Params params2(params);
                                params2.flags = flags;
                                toHexBinStr(params2,arg->u, buf, sizeof(buf), res);
                        }
                        break;
                case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
                case 'a': case 'A':
                        {
                                Arg tmp;
                                if (type == Arg::INT_T) {
                                        tmp.set(static_cast<double>(arg->i));
                                } else {
                                        tmp.set(static_cast<double>(arg->u));
                                }
                                return format(params, &tmp, 0);
                        }
                        break;  // not reached
                case 's': case 'S':
                        {
                                Params params2(params);
                                params2.flags  = 0;
                                params2.precis = 1;

                                if (type == Arg::INT_T) {
                                        toDecStr(params2,
                                                 arg->i, buf, sizeof(buf), res);
                                } else {
                                        toDecStr(params2,
                                                 arg->u, buf, sizeof(buf), res);
                                }
                        }
                        break;  // not reached
                case 'c':
                        if ((arg->i < 0xd800) || ((arg->i >= 0xe000)
                                                  && (arg->i <= 0x10ffff))) {
                                res.body = buf;
                                res.len = utf8_seq(
                                            static_cast<char32_t>(arg->i),
                                            reinterpret_cast<uint8_t *>(buf));
                        } else {
                                errno = EILSEQ;  // invalid character
                                return false;
                        }
                        break;
                case 'C':
                        ok = (arg->u <= WCHAR_MAX);

                        if (ok) {
                                using Converter = std::codecvt<wchar_t, char,
                                                               mbstate_t>;
                                const Converter &cvt
                                        = std::use_facet<Converter>(locale());

                                wchar_t        from      = (wchar_t) arg->u;
                                const wchar_t *next_from = &from;
                                char          *next_to   = buf;

                                std::mbstate_t state;
                                memset(&state, '\0', sizeof(state));

                                Converter::result result = cvt.out(state,
                                        &from, &from + 1, next_from,
                                        buf, buf + sizeof(buf), next_to);

                                ok = (result == cvt.ok);

                                if (ok) {
                                        res.body = buf;
                                        res.len  = static_cast<uintmax_t>(
                                                                next_to - buf);
                                }
                        }

                        if (!ok) {
                                errno = EILSEQ;
                                return false;
                        }
                        break;
                case 'n':  // no effect
                        break;
                default:
                        errno = EINVAL;  // unrecognised conversion specifier
                        return false;
                }
                break;
        case Arg::DBL_T:
                switch (conv) {
                case 'd': case 'i':
                        toDecStr(params, (long long) round(arg->f),
                                 buf, sizeof(buf), res);
                        break;
                case 'o':
                        toOctStr(params, (unsigned long long) round(arg->f),
                                 buf, sizeof(buf), res);
                        break;
                case 'u':
                        toDecStr(params, (unsigned long long) round(arg->f),
                                 buf, sizeof(buf), res);
                        break;
                case 'X': case 'x': case 'B': case 'b':
                        toHexBinStr(params, (unsigned long long) round(arg->f),
                                    buf, sizeof(buf), res);
                        break;
                case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
                        toDecStr(params, arg->f, buf, sizeof(buf), res);
                        break;
                case 'a': case 'A':
                        toHexStr(params, arg->f, buf, sizeof(buf), res);
                        break;
                case 's': case 'S':
                        {
                                int n = ::snprintf(buf,
                                                   sizeof(buf), "%f", arg->f);
                                if (n < 0) {
                                        return false;
                                }
                                res.len = static_cast<uintmax_t>(n);
                        }
                        res.body = buf;
                        break;
                case 'c': case 'C':
                        if (std::isfinite(arg->f) && (arg->f >= INTMAX_MIN)
                                                  && (arg->f <= INTMAX_MAX)) {
                                Arg tmp;
                                tmp.set(static_cast<intmax_t>(arg->f));
                                ok = format(params, &tmp, 0);
                        } else {
                                errno = EILSEQ;  // invalid character
                        }
                        return ok;
                case 'n':  // no effect
                        break;
                case 'p':
                        errno = EINVAL;  // inappropriate conversion specifier
                        return false;
                }
                break;
        case Arg::STR_T:
                switch (conv) {
                case 'd': case 'i':
                        {
                                long val;
                                try {
                                        std::string tmp(arg->s.data,
                                                        arg->s.length);
                                        val = std::stol(tmp);
                                } catch (...) {
                                        val = 0;
                                }
                                Arg tmp;
                                tmp.set(val);
                                return format(params, &tmp, 0);
                        }
                        break;  // not reached
                case 'o': case 'u': case 'x': case 'X':
                case 'b': case 'B': case 'p':
                        {
                                unsigned long val;
                                try {
                                        std::string tmp(arg->s.data,
                                                        arg->s.length);
                                        val = std::stoul(tmp);
                                } catch (...) {
                                        val = 0;
                                }
                                Arg tmp;
                                tmp.set(val);
                                return format(params, &tmp, 0);
                        }
                        break;  // not reached
                case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
                case 'a': case 'A':
                        {
                                double val;
                                try {
                                        std::string tmp(arg->s.data,
                                                        arg->s.length);
                                        val = stod(tmp);
                                } catch (...) {
                                        val = std::numeric_limits<double>::
                                                quiet_NaN();
                                }
                                Arg tmp;
                                tmp.set(val);
                                return format(params, &tmp, 0);
                        }
                        break;  // not reached
                case 'c': case 'C':
                        errno = EINVAL;  // inappropriate conversion specifier
                        return false;
                case 's': case 'S':
                        res.body = arg->s.data;
                        if (flags & HAVE_PRECIS) {
                                res.len = std::min<uintmax_t>(precis,
                                                              arg->s.length);
                        } else {
                                res.len = arg->s.length;
                        }
                        break;
                case 'n':  // no effect
                        break;
                }
                break;
        case Arg::PINT16_T:
                return convIntPtr(params, arg->pint16);
        case Arg::PUINT16_T:
                return convIntPtr(params, arg->puint16);
        case Arg::PINT32_T:
                return convIntPtr(params, arg->pint32);
        case Arg::PUINT32_T:
                return convIntPtr(params, arg->puint32);
        case Arg::PINT64_T:
                return convIntPtr(params, arg->pint64);
        case Arg::PUINT64_T:
                return convIntPtr(params, arg->puint64);
        case Arg::OTHER_T:
                switch (conv) {
                case 'p':
                        {
                                Params params2(params);
                                params2.flags = flags | ALT_FORM;
                                toHexBinStr(params2,
                                        reinterpret_cast<uintmax_t>(arg->other),
                                        buf, sizeof(buf), res);
                        }
                        break;
                default:
                        if (arg->fmt_fn) {
                                ok = (*arg->fmt_fn)(params);
                        } else {
                                errno = EINVAL;  // no format function defined
                        }
                        return ok;
                }
                break;
        default:
                errno = EINVAL;  // type not supported
                return false;
        }

        char fill;

        switch (conv) {
        case 'd': case 'i': case 'o': case 'u': case 'x': case 'X': case 'e':
        case 'E': case 'f': case 'F': case 'g': case 'G': case 'a': case 'A':
                if (flags & (HAVE_PRECIS | CENTRE_ALIGN | LEFT_ALIGN)) {
                        ;  // above flags cancel the zero-pad flag
                } else if (flags & ZERO_PAD) {
                        fill = '0';
                        break;
                }
                // fall through
        default:
                fill = ' ';
                break;
        }

        const char *contents = res.prefix ? res.prefix : res.body;
        uintmax_t   gap;

        if ((flags & HAVE_WIDTH) && (width > res.len)) {
                gap = width - res.len;
        } else {
                gap = 0;
        }

        if (gap && !(flags & LEFT_ALIGN)) {
                if (fill == '0' && res.prefix && res.prefix < res.body) {
                        uintmax_t pfx_len;
                        pfx_len = static_cast<uintmax_t>(res.body - res.prefix);
                        put(res.prefix, pfx_len);
                        contents = res.body;
                        res.len -= pfx_len;
                }

                uintmax_t left_gap = gap;

                if (flags & CENTRE_ALIGN) {
                        left_gap >>= 1;
                }

                gap -= left_gap;

                for (; left_gap > 0; --left_gap) {
                        put(fill);
                }
        }

        put(contents, res.len);

        for (; gap > 0; --gap) {
                put(fill);
        }

        return true;
}

//--------------------------------------

WRUTIL_API intmax_t
print(
        Target     &target,
        const char *fmt,
        const Arg  *argv,
        int         argc
)
{
        int next_arg_ix = 0;  // will set -ve if using indexes in format spec

        target.begin();

        for (const char *p = fmt, *q = p; *p != '\0'; p = q) {
                while (*q != '\0' && *q != '%') {
                        ++q;
                }

                if (q > p) {
                        target.put(p, static_cast<uintmax_t>(q - p));
                }

                if (*q == '%') {
                        ++q;

                        switch (*q) {
                        default:
                                q = convert(target, q, argv, argc, next_arg_ix);
                                if (!q) {
                                        return -1;
                                }
                                break;
                        case '%':
                                target.put('%');
                        case '\0':
                                break;
                        }
                }
        }

        return target.end();
}

//--------------------------------------

static const char *
convert(
        Target     &target,
        const char *spec,    // = character after '%' on entry
        const Arg  *argv,
        int         argc,
        int        &next_arg_ix
)
{
        enum State { FLAGS, WIDTH, PRECIS, LEN, CONV } state = FLAGS;

        const Arg *val_arg = nullptr;
        uintmax_t  flags   = 0,
                   width   = 0,
                   precis  = 0;
        char      *end;

        while (state == FLAGS) {
                switch (*spec) {
                case '#':  flags |= ALT_FORM; ++spec; break;
                case '0':  flags |= ZERO_PAD; ++spec; break;
                case '-':  flags |= LEFT_ALIGN; ++spec; break;
                case '=':  flags |= CENTRE_ALIGN; ++spec; break;
                case ' ':  flags |= PAD_POS_SIGN; ++spec; break;
                case '+':  flags |= SHOW_POS_SIGN; ++spec; break;
                case '\'': flags |= GROUP_THOU; ++spec; break;
                default:
                        if (isdigit(*spec) && !flags && !val_arg) {
                                uintmax_t ix = strtoul(spec, &end, 10) - 1;

                                if (*end != '$') {
                                        state = WIDTH;
                                        break;
                                } else if (ix >= static_cast<uintmax_t>(argc)) {
                                        // explicit arg index out of range
                                        errno = ERANGE;
                                        return nullptr;
                                } else if (next_arg_ix > 0) {
                                        // inconsistent use of explicit indices
                                        errno = EINVAL;
                                        return nullptr;
                                }

                                next_arg_ix = -1;
                                val_arg     = &argv[ix];
                                spec        = ++end;
                        } else {
                                state = WIDTH;
                        }
                        break;
                }
        }

        if (next_arg_ix < 0 && !val_arg) {
                errno = EINVAL;  // inconsistent use of explicit indices
                return nullptr;
        }

        while (state == WIDTH || state == PRECIS) {
                const Arg *arg = nullptr;
                uintmax_t  n;
                bool       ok;

                switch (*spec) {
                case '*':
                        ++spec;
                        if (isdigit(*spec)) {
                                uintmax_t ix = strtoul(spec, &end, 10) - 1;

                                if (*end != '$') {
                                        // bad conversion spec syntax
                                        errno = EINVAL;
                                        return nullptr;
                                } else if (ix >= static_cast<uintmax_t>(argc)) {
                                        // explicit arg index out of range
                                        errno = ERANGE;
                                        return nullptr;
                                } else if (next_arg_ix > 0) {
                                        // inconsistent use of explicit indices
                                        errno = EINVAL;
                                        return nullptr;
                                }

                                next_arg_ix = -1;
                                arg         = &argv[ix];
                                spec        = ++end;
                        } else if (next_arg_ix >= 0 && next_arg_ix < argc) {
                                arg = &argv[next_arg_ix++];
                        } else {
                                /* inconsistent use of explicit indices
                                   or not enough args */
                                errno = EINVAL;
                                return nullptr;
                        }

                        n = arg->toUInt(ok);

                        if (state == WIDTH) {
                                width = n, flags |= HAVE_WIDTH;
                        } else {
                                precis = n, flags |= HAVE_PRECIS;
                        }
                        break;
                case '.':
                        if (state == WIDTH) {
                                state = PRECIS;
                                ++spec;
                                break;
                        }
                        // else fall through and eventually error
                default:
                        if (isdigit(*spec)) {
                                uintmax_t x = strtoul(spec, &end, 10);

                                if (state == WIDTH) {
                                        width = x, flags |= HAVE_WIDTH;
                                } else {
                                        precis = x, flags |= HAVE_PRECIS;
                                }
                                spec = end;
                        } else {
                                state = LEN;
                        }
                        break;
                }
        }

        // length modifiers are accepted for compatibility but not actually used
        if (state == LEN) {
                switch (*spec) {
                case 'h': case 'l':
                        ++spec;
                        if (*spec == *(spec - 1)) {
                                ++spec;
                        }
                        break;
                case 'L': case 'q': case 'j': case 'z': case 't':
                        ++spec;
                        break;
                default:
                        break;
                }

                state = CONV;
        }

        if (state == CONV) {
                if (!val_arg && *spec != 'm') {
                        if (next_arg_ix >= 0 && next_arg_ix < argc) {
                                // take next arg (not using explicit indices)
                                val_arg = &argv[next_arg_ix++];
                        } else {
                                errno = EINVAL;  // not enough args
                                return nullptr;
                        }
                }

                if (target.format(Params { target, val_arg, flags, width,
                                           precis, *spec }, nullptr, 0)) {
                        ++spec;
                } else {
                        spec = nullptr;
                }
        }

        return spec;
}

//--------------------------------------

Target::~Target()
{
}

//--------------------------------------

void
Target::begin()
{
}

//--------------------------------------

std::locale
Target::locale() const
{
        return std::locale();
}

//--------------------------------------

void
Target::put(
        const char *chars,
        uintmax_t   count
)
{
        for (; count > 0; ++chars, --count) {
                put(*chars);
        }
}

//--------------------------------------

intmax_t
Target::end()
{
        return static_cast<intmax_t>(
                std::min(count(), static_cast<uintmax_t>(INTMAX_MAX)));
}

//--------------------------------------

IOStreamTarget::IOStreamTarget(
        std::ostream &s
) :
        stream_(s)
{
}

//--------------------------------------

void
IOStreamTarget::begin()
{
        count_ = 0;
}

//--------------------------------------

void
IOStreamTarget::put(
        char c
)
{
        stream_.put(c);
        ++count_;
}

//--------------------------------------

void
IOStreamTarget::put(
        const char *chars,
        uintmax_t   count
)
{
        stream_.write(chars, count);
        count_ += count;
}

//--------------------------------------

std::locale
IOStreamTarget::locale() const
{
        return stream_.getloc();
}

//--------------------------------------

uintmax_t
IOStreamTarget::count() const
{
        return count_;
}

//--------------------------------------

CStreamTarget::CStreamTarget(
        FILE *s
) :
        stream_(s)
{
}

//--------------------------------------

void
CStreamTarget::begin()
{
        count_ = 0;
}

//--------------------------------------

void
CStreamTarget::put(
        char c
)
{
        fputc(c, stream_);
        ++count_;
}

//--------------------------------------

void
CStreamTarget::put(
        const char *chars,
        uintmax_t   count
)
{
        count_ += fwrite(chars, 1, numeric_cast<size_t>(count), stream_);
}

//--------------------------------------

uintmax_t
CStreamTarget::count() const
{
        return count_;
}

//--------------------------------------

FixedBufferTarget::FixedBufferTarget(
        char      *buf,
        uintmax_t  capacity
) :
        buf_ (buf),
        stop_(buf_ + capacity - 1)
{
}

//--------------------------------------

void
FixedBufferTarget::begin()
{
        pos_ = buf_;
}

//--------------------------------------

void
FixedBufferTarget::put(
        char c
)
{
        if (pos_ < stop_) {
                *(pos_++) = c;
        }
}

//--------------------------------------

void
FixedBufferTarget::put(
        const char *chars,
        uintmax_t   count
)
{
        size_t final_count = std::min(numeric_cast<size_t>(count),
                                      numeric_cast<size_t>(stop_ - pos_));
        memcpy(pos_, chars, final_count);
        pos_ += count;
}

//--------------------------------------

intmax_t
FixedBufferTarget::end()
{
        *pos_ = '\0';
        return Target::end();
}

//--------------------------------------

uintmax_t
FixedBufferTarget::count() const
{
        return static_cast<uintmax_t>(pos_ - buf_);
}

//--------------------------------------

StringTarget::StringTarget(
        std::string &s
) :
        str_(s)
{
}

//--------------------------------------

void
StringTarget::begin()
{
        initial_len_ = str_.length();
}

//--------------------------------------

void
StringTarget::put(
        char c
)
{
        str_ += c;
}

//--------------------------------------

void
StringTarget::put(
        const char *chars,
        uintmax_t   count
)
{
        str_.append(chars, numeric_cast<size_t>(count));
}

//--------------------------------------

uintmax_t
StringTarget::count() const
{
        return str_.length() - initial_len_;
}


} // namespace fmt
} // namespace wr
