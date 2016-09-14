/**
 * \file filesystem.h
 *
 * \brief Types and functions for handling filesystem paths
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
#ifndef WRUTIL_FILESYSTEM_H
#define WRUTIL_FILESYSTEM_H

#include <wrutil/Config.h>
#include <wrutil/string_view.h>
#include <wrutil/u8string_view.h>


#if WR_HAVE_STD_FILESYSTEM


#if WR_HAVE_STD_EXP_FILESYSTEM
#       include <experimental/filesystem>
#else
#       include <filesystem>
#endif


namespace wr {


#if WR_HAVE_STD_EXP_FILESYSTEM
        namespace fs_impl = std::experimental::filesystem;
#else
        namespace fs_impl = std::filesystem;
#endif

using fs_impl::file_type;
using fs_impl::perms;
using fs_impl::copy_options;
using fs_impl::file_time_type;
using fs_error_code = std::error_code;
using fs_impl::is_block_file;
using fs_impl::is_character_file;
using fs_impl::is_fifo;
using fs_impl::is_socket;


} // namespace wr


#else // WR_HAVE_STD_FILESYSTEM

#include <algorithm>
#include <locale>
#include <system_error>
#include <type_traits>
#include <boost/filesystem.hpp>
#include <boost/version.hpp>


namespace wr {


namespace fs_impl = boost::filesystem;

using file_time_type = std::time_t;
using fs_error_code = boost::system::error_code;

//--------------------------------------

class file_type
{
public:
        constexpr file_type() noexcept : type_(fs_impl::status_error) {}
        constexpr file_type(fs_impl::file_type t) noexcept : type_(t) {}

        constexpr operator fs_impl::file_type() const noexcept { return type_; }

        constexpr explicit operator int() const noexcept
                { return static_cast<int>(type_); }

        static constexpr fs_impl::file_type none      = fs_impl::status_error,
                                            not_found = fs_impl::file_not_found,
                                            regular   = fs_impl::regular_file,
                                            directory = fs_impl::directory_file,
                                            symlink   = fs_impl::symlink_file,
                                            block     = fs_impl::block_file,
                                            character = fs_impl::character_file,
                                            fifo      = fs_impl::fifo_file,
                                            socket    = fs_impl::socket_file,
                                            unknown   = fs_impl::type_unknown;
private:
        fs_impl::file_type type_;
};

//--------------------------------------

class perms
{
public:
        constexpr perms() noexcept : perms_(fs_impl::no_perms) {}
        constexpr perms(fs_impl::perms p) noexcept : perms_(p) {}

        constexpr operator fs_impl::perms() const noexcept { return perms_; }

        constexpr explicit operator int() const noexcept
                { return static_cast<int>(perms_); }

        perms operator&=(perms p) noexcept
                { return perms_ &= p.perms_; }

        perms operator|=(perms p) noexcept
                { return perms_ |= p.perms_; }

        perms operator^=(perms p) noexcept
                { return perms_ ^= p.perms_; }

        friend perms operator&(perms p1, perms p2) noexcept
                { return p1.perms_ & p2.perms_; }

        friend perms operator|(perms p1, perms p2) noexcept
                { return p1.perms_ | p2.perms_; }

        friend perms operator^(perms p1, perms p2) noexcept
                { return p1.perms_ ^ p2.perms_; }

        friend perms operator~(perms p) noexcept
                { return ~p.perms_; }

        static constexpr fs_impl::perms
                none             = fs_impl::no_perms,
                owner_read       = fs_impl::owner_read,
                owner_write      = fs_impl::owner_write,
                owner_exec       = fs_impl::owner_exe,
                owner_all        = fs_impl::owner_all,
                group_read       = fs_impl::group_read,
                group_write      = fs_impl::group_write,
                group_exec       = fs_impl::group_exe,
                group_all        = fs_impl::group_all,
                others_read      = fs_impl::others_read,
                others_write     = fs_impl::others_write,
                others_exec      = fs_impl::others_exe,
                others_all       = fs_impl::others_all,
                all_all          = fs_impl::all_all,
                set_uid          = fs_impl::set_uid_on_exe,
                set_gid          = fs_impl::set_gid_on_exe,
                sticky_bit       = fs_impl::sticky_bit,
                mask             = fs_impl::perms_mask,
                unknown          = fs_impl::perms_not_known,
                add_perms        = fs_impl::add_perms,
                remove_perms     = fs_impl::remove_perms,
                resolve_symlinks = fs_impl::symlink_perms;

private:
        fs_impl::perms perms_;
};

//--------------------------------------

inline bool is_block_file(const fs_impl::file_status s) noexcept
        { return s.type() == file_type::block; }

inline bool is_block_file(const fs_impl::path &p)
        { return is_block_file(status(p)); }

inline bool is_block_file(const fs_impl::path &p, fs_error_code &ec) noexcept
        { return is_block_file(status(p, ec)); }

inline bool is_character_file(const fs_impl::file_status s) noexcept
        { return s.type() == file_type::character; }

inline bool is_character_file(const fs_impl::path &p)
        { return is_character_file(status(p)); }

inline bool is_character_file(const fs_impl::path &p,
                              fs_error_code &ec) noexcept
        { return is_character_file(status(p, ec)); }

inline bool is_fifo(const fs_impl::file_status s) noexcept
        { return s.type() == file_type::fifo; }

inline bool is_fifo(const fs_impl::path &p)
        { return is_fifo(status(p)); }

inline bool is_fifo(const fs_impl::path &p, fs_error_code &ec) noexcept
        { return is_fifo(status(p, ec)); }

inline bool is_socket(const fs_impl::file_status s) noexcept
        { return s.type() == file_type::socket; }

inline bool is_socket(const fs_impl::path &p)
        { return is_socket(status(p)); }

inline bool is_socket(const fs_impl::path &p, fs_error_code &ec) noexcept
        { return is_socket(status(p, ec)); }


} // namespace wr


namespace std {


template <> inline void swap(wr::fs_impl::path &a, wr::fs_impl::path &b)
        { a.swap(b); }

template <>
struct is_error_condition_enum<boost::system::errc::errc_t> :
        true_type {};


} // namespace std


#endif // WR_HAVE_STD_FILESYSTEM


namespace wr {


using fs_impl::path;
using fs_impl::filesystem_error;
using fs_impl::file_status;
using fs_impl::space_info;
using fs_impl::directory_entry;
using fs_impl::directory_iterator;
using fs_impl::recursive_directory_iterator;

using fs_impl::absolute;
using fs_impl::canonical;
using fs_impl::copy;
using fs_impl::copy_file;
using fs_impl::copy_symlink;
using fs_impl::create_directory;
using fs_impl::create_directories;
using fs_impl::create_hard_link;
using fs_impl::create_symlink;
using fs_impl::current_path;
using fs_impl::equivalent;
using fs_impl::exists;
using fs_impl::file_size;
using fs_impl::hard_link_count;
using fs_impl::is_directory;
using fs_impl::is_empty;
using fs_impl::is_other;
using fs_impl::is_regular_file;
using fs_impl::is_symlink;
using fs_impl::last_write_time;
using fs_impl::permissions;
using fs_impl::read_symlink;
using fs_impl::remove;
using fs_impl::remove_all;
using fs_impl::rename;
using fs_impl::resize_file;
using fs_impl::space;
using fs_impl::status;
using fs_impl::status_known;
using fs_impl::symlink_status;
using fs_impl::system_complete;
using fs_impl::temp_directory_path;


WRUTIL_API bool has_prefix_path(const path &p, const path &prefix);
WRUTIL_API bool is_separator(path::value_type c);


#if WR_HAVE_FSIMPL_PROXIMATE
using fs_impl::proximate;
#else
WRUTIL_API path proximate(const path &p, fs_error_code &ec);

WRUTIL_API path proximate(const path &p,
                          const path &base = fs_impl::current_path());

WRUTIL_API path proximate(const path &p, const path &base,
                          fs_error_code &ec);
#endif

#if WR_HAVE_FSIMPL_RELATIVE
using fs_impl::relative;
#else
WRUTIL_API path relative(const path &p, fs_error_code &ec);

WRUTIL_API path relative(const path &p,
                         const path &base = fs_impl::current_path());

WRUTIL_API path relative(const path &p, const path &base,
                         fs_error_code &ec);
#endif

#if WR_HAVE_FSIMPL_WEAKLY_CANONICAL
using fs_impl::weakly_canonical;
#else
WRUTIL_API path weakly_canonical(const path &p);
WRUTIL_API path weakly_canonical(const path &p, fs_error_code &ec);
#endif

#if WR_HAVE_FSIMPL_UNIQUE_PATH
using fs_impl::unique_path;
#else
WRUTIL_API path unique_path(const path &pattern = "%%%%-%%%%-%%%%-%%%%");
WRUTIL_API path unique_path(const path &pattern, fs_error_code &ec);
#endif

#if WR_HAVE_FS_PATH_LEXICALLY_NORMAL
inline path lexically_normal(const path &p)
        { return p.lexically_normal(); }
#else
WRUTIL_API path lexically_normal(const path &p);
#endif

#if WR_HAVE_FS_PATH_LEXICALLY_RELATIVE
inline path lexically_relative(const path &p,
                                        const path &base)
        { return p.lexically_relative(base); }
#else
WRUTIL_API path lexically_relative(const path &p, const path &base);
#endif

#if WR_HAVE_FS_PATH_LEXICALLY_PROXIMATE
inline path lexically_proximate(const path &p,
                                         const path &base)
        { return p.lexically_proximate(base); }
#else
WRUTIL_API path lexically_proximate(const path &p, const path &base);
#endif

#if WR_HAVE_FS_PATH_TO_U8STRING
inline std::string to_u8string(const path &p) { return p.u8string(); }

inline std::string to_generic_u8string(const path &p)
        { return p.generic_u8string(); }
#else
WRUTIL_API std::string to_u8string(const path &p);
WRUTIL_API std::string to_generic_u8string(const path &p);
#endif

#if WR_HAVE_FSIMPL_U8PATH
using fs_impl::u8path;

#if !WR_DINKUM
template <>
#endif
inline path u8path(const wr::u8string_view &s)
        { return u8path(s.char_data(), s.char_data() + s.bytes()); }

#if !WR_DINKUM
template <>
#endif
inline path u8path(const wr::string_view &s)
        { return u8path(s.data(), s.data() + s.size()); }
#else
template <typename Source> path u8path(const Source &source);
template <typename InputIt> path u8path(InputIt first, InputIt last);

template <size_t N> inline path u8path(const char (&source)[N])
        { return u8path(source, std::find(source, source + N, '\0')); }

template <size_t N> inline path u8path(char (&source)[N])
        { return u8path<const char (&)[N]>(source); }
#endif

//--------------------------------------

class explicit_path
{
public:
        explicit_path(const path &p) : p_(p) {}

        operator const path &() const { return p_; }

private:
        const path &p_;
};

//--------------------------------------

constexpr char path_list_delimiter =
#if WR_WINDOWS && !WR_CYGWIN
        ';'
#else
        ':'
#endif
        ;

//--------------------------------------

extern WRUTIL_API const path DOT, DOTDOT;

//--------------------------------------
/*
 * wr::print*() support
 */
namespace fmt {


struct Arg;
struct Params;
template <typename> struct TypeHandler;

template <> struct WRUTIL_API TypeHandler<path>
{
        static void set(Arg &arg, const path &val);
        static bool format(const Params &parms);
};

//--------------------------------------

template <> struct WRUTIL_API TypeHandler<file_type>
{
        static void set(Arg &arg, file_type val);
        static bool format(const Params &parms);
};

//--------------------------------------

template <> struct WRUTIL_API TypeHandler<perms>
{
        static void set(Arg &arg, perms val);
        static bool format(const Params &parms);
};

//--------------------------------------

template <> struct WRUTIL_API TypeHandler<file_status>
{
        static void set(Arg &arg, const file_status &val);
        static bool format(const Params &parms);
};

//--------------------------------------

template <> struct WRUTIL_API TypeHandler<directory_entry>
{
        static void set(Arg &arg, const directory_entry &val);
};


} // namespace fmt


} // namespace wr


#endif // !WRUTIL_FILESYSTEM_H
