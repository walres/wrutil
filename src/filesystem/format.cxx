/**
 * \file format.cxx
 *
 * \brief wr::format() argument support for path, file_type, perms,
 *        file_status and directory_entry types
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
#include <errno.h>
#include <wrutil/Format.h>
#include <wrutil/filesystem.h>


namespace wr {


WRUTIL_API void
fmt::TypeHandler<path>::set(
        Arg        &arg,
        const path &val
)
{
        arg.type = Arg::OTHER_T;
        arg.other = &val;
        arg.fmt_fn = &TypeHandler<path>::format;
}

//--------------------------------------

WRUTIL_API bool
fmt::TypeHandler<path>::format(
        const Params &parms
)
{
        if (parms.conv != 's') {
                errno = EINVAL;
                return false;
        }

        std::string buf = static_cast<const path *>(parms.arg->other)->string();

        Arg arg2;
        arg2.type = Arg::STR_T;
        arg2.s = { buf.data(), buf.length() };

        return parms.target.format(parms, &arg2);
}

//--------------------------------------

static char
toChar(
        fs_impl::file_type type
)
{
        switch (type) {
        case file_type::none:
                return ' ';
        case file_type::not_found:
                return '!';
        case file_type::regular:
                return '-';
        case file_type::directory:
                return 'd';
        case file_type::symlink:
                return 'l';
        case file_type::block:
                return 'b';
        case file_type::character:
                return 'c';
        case file_type::fifo:
                return 'p';
        case file_type::socket:
                return 's';
        case file_type::unknown: default:
                return '?';
        }
}

//--------------------------------------

static wchar_t
toWChar(
        fs_impl::file_type type
)
{
        switch (type) {
        case file_type::none:
                return L' ';
        case file_type::not_found:
                return L'!';
        case file_type::regular:
                return L'-';
        case file_type::directory:
                return L'd';
        case file_type::symlink:
                return L'l';
        case file_type::block:
                return L'b';
        case file_type::character:
                return L'c';
        case file_type::fifo:
                return L'p';
        case file_type::socket:
                return L's';
        case file_type::unknown: default:
                return L'?';
        }
}

//--------------------------------------

static const char *
toString(
        fs_impl::file_type type
)
{
        switch (type) {
        case file_type::none:
                return "none";
        case file_type::not_found:
                return "not found";
        case file_type::regular:
                return "regular";
        case file_type::directory:
                return "directory";
        case file_type::symlink:
                return "symlink";
        case file_type::block:
                return "block device";
        case file_type::character:
                return "character device";
        case file_type::fifo:
                return "fifo";
        case file_type::socket:
                return "socket";
        case file_type::unknown: default:
                return "unknown";
        }
}

//--------------------------------------

WRUTIL_API void
fmt::TypeHandler<file_type>::set(
        Arg       &arg,
        file_type  val
)
{
        arg.type = Arg::INT_T;
        arg.i = static_cast<int>(val);
        arg.fmt_fn = &TypeHandler<file_type>::format;
}

//--------------------------------------

WRUTIL_API bool
fmt::TypeHandler<file_type>::format(
        const Params &parms
)
{
        fmt::Target &target = parms.target;
        Arg          arg2;
        auto         type = static_cast<fs_impl::file_type>(parms.arg->i);

        switch (parms.conv) {
        case 'c':
                arg2.set(toChar(type));
                break;
        case 'C':
                arg2.set(toWChar(type));
                break;
        case 's':
                arg2.set(toString(type));
                break;
        default:  // format as a number
                arg2.set(parms.arg->i);
                break;
        }

        return target.format(parms, &arg2);
}

//--------------------------------------

static void
toChars(
        fs_impl::perms  src,
        char           *dst  // must have at least 9 characters' capacity
)
{
        if ((src & perms::owner_read) != perms::none) {
                dst[0] = 'r';
        } else {
                dst[0] = '-';
        }
        if ((src & perms::owner_write) != perms::none) {
                dst[1] = 'w';
        } else {
                dst[1] = '-';
        }
        if ((src & perms::owner_exec) != perms::none) {
                if ((src & perms::set_uid) != perms::none) {
                        dst[2] = 's';
                } else {
                        dst[2] = 'x';
                }
        } else if ((src & perms::set_uid) != perms::none) {
                dst[2] = 'S';
        } else {
                dst[2] = '-';
        }

        if ((src & perms::group_read) != perms::none) {
                dst[3] = 'r';
        } else {
                dst[3] = '-';
        }
        if ((src & perms::group_write) != perms::none) {
                dst[4] = 'w';
        } else {
                dst[4] = '-';
        }
        if ((src & perms::group_exec) != perms::none) {
                if ((src & perms::set_gid) != perms::none) {
                        dst[5] = 's';
                } else {
                        dst[5] = 'x';
                }
        } else if ((src & perms::set_gid) != perms::none) {
                dst[5] = 'S';
        } else {
                dst[5] = '-';
        }

        if ((src & perms::group_read) != perms::none) {
                dst[6] = 'r';
        } else {
                dst[6] = '-';
        }
        if ((src & perms::group_write) != perms::none) {
                dst[7] = 'w';
        } else {
                dst[7] = '-';
        }
        if ((src & perms::group_exec) != perms::none) {
                if ((src & perms::sticky_bit) != perms::none) {
                        dst[8] = 't';
                } else {
                        dst[8] = 'x';
                }
        } else if ((src & perms::sticky_bit) != perms::none) {
                dst[8] = 'T';
        } else {
                dst[8] = '-';
        }
}

//--------------------------------------

WRUTIL_API void
fmt::TypeHandler<perms>::set(
        Arg   &arg,
        perms  val
)
{
        arg.type = Arg::INT_T;
        arg.i = static_cast<int>(val);
        arg.fmt_fn = &TypeHandler<perms>::format;
}

//--------------------------------------

WRUTIL_API bool
fmt::TypeHandler<perms>::format(
        const Params &parms
)
{
        fmt::Target &target = parms.target;
        Arg          arg2;
        char         buf[9];
        
        if (parms.conv == 's') {
                toChars(static_cast<fs_impl::perms>(parms.arg->i), buf);
                arg2.type = Arg::STR_T;
                arg2.s = { buf, sizeof(buf) };
        } else {
                // format as a number
                arg2.set(parms.arg->i);
        }

        return target.format(parms, &arg2);
}

//--------------------------------------

WRUTIL_API void
fmt::TypeHandler<file_status>::set(
        Arg               &arg,
        const file_status &val
)
{
        arg.type = Arg::OTHER_T;
        arg.other = &val;
        arg.fmt_fn = &TypeHandler<file_status>::format;
}

//--------------------------------------

WRUTIL_API bool
fmt::TypeHandler<file_status>::format(
        const Params &parms
)
{
        fmt::Target &target = parms.target;

        if (parms.conv == 's') {
                auto status = static_cast<const file_status*>(parms.arg->other);
                char buf[10];
                buf[0] = toChar(status->type());
                toChars(status->permissions(), buf + 1);
                Arg arg2;
                arg2.type = Arg::STR_T;
                arg2.s = { buf, sizeof(buf) };
                return target.format(parms, &arg2);
        } else {
                errno = EINVAL;
                return false;
        }
}

//--------------------------------------

WRUTIL_API void
fmt::TypeHandler<directory_entry>::set(
        Arg                   &arg,
        const directory_entry &val
)
{
        arg.type = Arg::OTHER_T;
        arg.other = &val.path();
        arg.fmt_fn = &TypeHandler<path>::format;
}


} // namespace wr
