/**
 * \file codecvt_utf8_narrow.cxx
 *
 * \brief Implementation of conversion between UTF-8 and locale-specific
 *        narrow text encodings
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
#include <ctype.h>
#include <algorithm>
#include <type_traits>
#include <wrutil/codecvt.h>


namespace wr {


struct codecvt_utf8_narrow::Body
{
#ifdef __STDC_ISO_10646__
        codecvt_utf8<wchar_t>                              utf8_;
#elif WR_WINDOWS
        std::codecvt_utf8_utf16<wchar_t>                   utf8_;
#else
        std::locale                                        utf8_loc_;
        const std::codecvt<wchar_t, char, std::mbstate_t> &utf8_;
#endif
        std::locale                                        narrow_loc_;
        const std::codecvt<wchar_t, char, std::mbstate_t> &narrow_;

        Body(std::locale loc);

        template <typename FromCvt, typename ToCvt>
                auto do_inout(state_type &state, const FromCvt &from_cvt,
                              const intern_type *from,
                              const intern_type *from_end,
                              const intern_type *&from_next,
                              const ToCvt &to_cvt, extern_type *to,
                              extern_type *to_end,
                              extern_type *&to_next) -> result;
};

//-------------------------------------

WRUTIL_API std::locale::id codecvt_utf8_narrow::id;

//-------------------------------------

WRUTIL_API
codecvt_utf8_narrow::codecvt_utf8_narrow(
        std::locale loc,
        std::size_t refs
) :
        base_type(refs),
        body_    (is_utf8(loc) ? nullptr: new Body(loc))
{
}

//--------------------------------------

WRUTIL_API
codecvt_utf8_narrow::codecvt_utf8_narrow(
        std::size_t refs
) :
        codecvt_utf8_narrow(std::locale(""), refs)
{
}

//--------------------------------------

codecvt_utf8_narrow::Body::Body(
        std::locale loc
) :
#if !WR_WINDOWS && !defined(__STDC_ISO_10646__)
        utf8_loc_  ("en_US.utf8"),
        utf8_      (std::use_facet<std::codecvt<wchar_t, char,
                                                std::mbstate_t>>(utf8_loc_)),
#endif
        narrow_loc_(loc),
        narrow_    (std::use_facet<std::codecvt<wchar_t, char,
                                                std::mbstate_t>>(narrow_loc_))
{
}

//--------------------------------------

codecvt_utf8_narrow::~codecvt_utf8_narrow()
{
        delete body_;
}

//--------------------------------------

template <typename FromCvt, typename ToCvt> auto
codecvt_utf8_narrow::Body::do_inout(
        state_type         &state,
        const FromCvt      &from_cvt,
        const intern_type  *from,
        const intern_type  *from_end,
        const intern_type *&from_next,
        const ToCvt        &to_cvt,
        extern_type        *to,
        extern_type        *to_end,
        extern_type       *&to_next
) -> result
{
        from_next = from;
        to_next = to;

        wchar_t  pivot[2],  // array size of 2 for platforms with 16-bit wchar_t
                *pivot_pos = &pivot[0];

        while ((from_next < from_end) && (to_next < to_end)) {
                from = from_next;

                result res = from_cvt.in(state, from, from_end, from_next,
                                         pivot_pos, pivot_pos + 1, pivot_pos);
                switch (res) {
                case error: case noconv:
                        return res;
                case partial:
                        if (from_next == from_end) {
                                return res;     // need more input
                        } else if ((pivot_pos == &pivot[1])
                                        && (pivot[0] >= 0xd800)
                                        && (pivot[0] <= 0xdbff)) {
                                continue;       /* two-part UTF-16 sequence;
                                                   get low surrogate */
                        } else {
                                break;          // got a full character
                        }
                case ok:
                        break;
                }

                const wchar_t *pivot_cend;
                std::mbstate_t to_state = { 0 };
                res = to_cvt.out(to_state, &pivot[0], pivot_pos, pivot_cend,
                                 to_next, to_end, to_next);
                pivot_pos = &pivot[0];

                switch (res) {
                case noconv:
                        return res;
                case error:
                        if (to_next < to_end) {
                                *to_next++ = '?';
                        }
                        break;
                case partial:
                        from_next = from;
                        return res;
                case ok:
                        break;
                }
        }

        return ok;
}

//--------------------------------------

WRUTIL_API auto
codecvt_utf8_narrow::do_out(
        state_type         &state,
        const intern_type  *from,
        const intern_type  *from_end,
        const intern_type *&from_next,
        extern_type        *to,
        extern_type        *to_end,
        extern_type       *&to_next
) const -> result
{
        if (!body_) {
                return noconv;
        }

        return body_->do_inout(state, body_->utf8_, from, from_end, from_next,
                               body_->narrow_, to, to_end, to_next);
}

//--------------------------------------

WRUTIL_API auto
codecvt_utf8_narrow::do_in(
        state_type         &state,
        const extern_type  *from,
        const extern_type  *from_end,
        const extern_type *&from_next,
        intern_type        *to,
        intern_type        *to_end,
        intern_type       *&to_next
) const -> result
{
        if (!body_) {
                return noconv;
        }

        return body_->do_inout(state, body_->narrow_, from, from_end, from_next,
                               body_->utf8_, to, to_end, to_next);
}

//--------------------------------------

WRUTIL_API int
codecvt_utf8_narrow::do_encoding()
        const noexcept
{
        return 0;
}

//--------------------------------------

WRUTIL_API int
codecvt_utf8_narrow::do_max_length()
        const noexcept
{
        return 4;
}

//--------------------------------------

WRUTIL_API auto
codecvt_utf8_narrow::do_unshift(
        state_type   &state,
        extern_type  *to,
        extern_type  *to_end,
        extern_type *&to_next
) const -> result
{
        if (body_) {
                body_->narrow_.unshift(state, to, to_end, to_next);
        }
        to_next = to;
        return noconv;
}

//--------------------------------------

WRUTIL_API bool
codecvt_utf8_narrow::do_always_noconv() const noexcept
{
        return !body_;
}

//--------------------------------------

WRUTIL_API int
codecvt_utf8_narrow::do_length(
        state_type        &state,
        const extern_type *from,
        const extern_type *from_end,
        std::size_t        max
) const
{
        if (do_always_noconv()) {
                return static_cast<int>(
                      std::min(static_cast<std::size_t>(from_end - from), max));
        }

        return -1;
}

//--------------------------------------
// FIXME: these really belong elsewhere...

WRUTIL_API u8string_convert<> &
utf8_narrow_cvt()
{
        static thread_local u8string_convert<> cvt;
        return cvt;
}

//--------------------------------------

WRUTIL_API wstring_convert<codecvt_wide_narrow> &
wide_narrow_cvt()
{
        static thread_local wstring_convert<codecvt_wide_narrow> cvt;
        return cvt;
}


} // namespace wr
