#
# Copyright 2016 James S. Waller
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

project(wrutil CXX C)
cmake_minimum_required(VERSION 2.6)
enable_testing()

include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)
include(CheckTypeSize)
include(FindPkgConfig OPTIONAL)

set(WRUTIL_VERSION_MAJOR 0)
set(WRUTIL_VERSION_MINOR 2)
set(WRUTIL_VERSION_PATCH 0)

if (NOT TOP_BUILD_DIR)
        set(TOP_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
endif()

########################################
#
# C++ Compiler Configuration
#
include(cmake/compiler-config.cmake)

math(EXPR WR_WORDSIZE "${CMAKE_SIZEOF_VOID_P} * 8")

check_type_size(wchar_t WR_SIZEOF_WCHAR_T)

########################################
#
# Third Party Components
#

set(CMAKE_REQUIRED_FLAGS ${CXX_CHOSEN_STD_FLAG})

#
# Check for availability of std::codecvt<char16_t, char, std::mbstate_t>
# and std::codecvt<char32_t, char, std::mbstate_t> in C++ standard library
# (optional; in particular, missing from libstdc++ version 4.x)
#
set(CHECK_CXX_CODE "#include <locale>\nstruct foo : std::codecvt<char16_t, char, std::mbstate_t> { ~foo(){} }\;int main() { foo f\; (void) f\; return 0\; }")
check_cxx_source_runs(${CHECK_CXX_CODE} WR_HAVE_STD_CODECVT_CHAR16)
        # use check_cxx_source_runs() to verify no link errors

set(CHECK_CXX_CODE "#include <locale>\nstruct foo : std::codecvt<char32_t, char, std::mbstate_t> { ~foo(){} }\;int main() { foo f\; (void) f\; return 0\; }")
check_cxx_source_runs(${CHECK_CXX_CODE} WR_HAVE_STD_CODECVT_CHAR32)
        # use check_cxx_source_runs() to verify no link errors

#
# Check for availability of std::codecvt_utf8 in C++ standard library (optional;
# in particular, missing from libstdc++ version 4.x)
#
set(CHECK_CXX_CODE "#include <codecvt>\nint main() { std::codecvt_utf8<wchar_t> cvt\; (void) cvt\; return 0\; }\n")
check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_STD_CODECVT_UTF8)

if (WR_HAVE_STD_CODECVT_UTF8)
        # Check for availability of wbuffer_convert in C++ standard library
        set(CHECK_CXX_CODE "#include <codecvt>\n#include <locale>\nint main() { std::wbuffer_convert<std::codecvt_utf8<wchar_t>> cvt\; (void) cvt\; return 0\; }\n")
        check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_STD_WBUFFER_CONVERT)

        # Check for availability of wstring_convert in C++ standard library
        set(CHECK_CXX_CODE "#include <codecvt>\n#include <locale>\nint main() { std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt\; (void) cvt\; return 0\; }\n")
        check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_STD_WSTRING_CONVERT)
endif()

#
# Check for availability of filesystem module in C++ standard library (optional)
#
set(CHECK_CXX_CODE "#include <filesystem>\nint main(int, char **argv) { std::filesystem::path p(argv[0])\; (void) p\; return 0\; }\n")
check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_STD_FILESYSTEM)

if (WR_HAVE_STD_FILESYSTEM)
        set(FILESYSTEM_HEADER <filesystem>)
        set(FILESYSTEM_NS std::filesystem)
else()
        set(CHECK_CXX_CODE "#include <experimental/filesystem>\nint main(int, char **argv) { std::experimental::filesystem::path p(argv[0])\; (void) p\; return 0\; }\n")

        unset(CMAKE_REQUIRED_LIBRARIES)
        check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_STD_EXP_FILESYSTEM)

        if (NOT WR_HAVE_STD_EXP_FILESYSTEM)
                unset(WR_HAVE_STD_EXP_FILESYSTEM)
                set(CMAKE_REQUIRED_LIBRARIES stdc++fs)
                check_cxx_source_compiles(${CHECK_CXX_CODE}
                                          WR_HAVE_STD_EXP_FILESYSTEM_LIBSTDCXX)
                if (WR_HAVE_STD_EXP_FILESYSTEM_LIBSTDCXX)
                        set(WR_HAVE_STD_EXP_FILESYSTEM 1)
                        list(INSERT WRUTIL_SYS_LIBS 0 stdc++fs)
                endif()
        endif()

        if (NOT WR_HAVE_STD_EXP_FILESYSTEM)
                unset(WR_HAVE_STD_EXP_FILESYSTEM)
                set(CMAKE_REQUIRED_LIBRARIES c++experimental)
                check_cxx_source_compiles(${CHECK_CXX_CODE}
                                          WR_HAVE_STD_EXP_FILESYSTEM_LIBCXX)
                if (WR_HAVE_STD_EXP_FILESYSTEM_LIBCXX)
                        set(WR_HAVE_STD_EXP_FILESYSTEM 1)
                        list(INSERT WRUTIL_SYS_LIBS 0 c++experimental)
                endif()
        endif()

        if (WR_HAVE_STD_EXP_FILESYSTEM)
                set(FILESYSTEM_HEADER <experimental/filesystem>)
                set(FILESYSTEM_NS std::experimental::filesystem)
                set(WR_HAVE_STD_FILESYSTEM 1)
        endif()

        unset(CMAKE_REQUIRED_LIBRARIES)
endif()

if (NOT WR_HAVE_STD_FILESYSTEM)
        message("Using Boost Filesystem library to provide filesystem module")
        set(FILESYSTEM_HEADER <boost/filesystem.hpp>)
        set(FILESYSTEM_NS boost::filesystem)
        set(NEED_BOOST_LIBS ${NEED_BOOST_LIBS} filesystem system)
        list(INSERT WRUTIL_SYS_LIBS 0 boost_filesystem boost_system)
endif()

#
# Check for availability of required libraries (mandatory)
#
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.44.0 REQUIRED ${NEED_BOOST_LIBS})

if (Boost_FOUND)
        include_directories(${Boost_INCLUDE_DIRS})
        link_directories(${Boost_LIBRARIES})
else()
        message(SEND_ERROR "Could not find all required Boost libraries (${Boost_LIBRARIES})")
endif()

#
# Check for filesystem features with inconsistent availability
#
if (NOT WR_HAVE_STD_FILESYSTEM)
        set(CMAKE_REQUIRED_INCLUDES ${Boost_INCLUDE_DIRS})
        set(CMAKE_REQUIRED_LIBRARIES ${Boost_LIBRARIES})
endif()

set(CHECK_CXX_CODE "#include ${FILESYSTEM_HEADER}\nint main(int, char **argv) { using namespace ${FILESYSTEM_NS}\; path p(proximate(path(argv[0])))\; (void) p\; return 0\; }\n")
check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_FSIMPL_PROXIMATE)

set(CHECK_CXX_CODE "#include ${FILESYSTEM_HEADER}\nint main(int, char **argv) { using namespace ${FILESYSTEM_NS}\; path p(relative(path(argv[0]), current_path()))\; (void) p\; return 0\; }\n")
check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_FSIMPL_RELATIVE)

set(CHECK_CXX_CODE "#include ${FILESYSTEM_HEADER}\nint main(int, char **argv) { using namespace ${FILESYSTEM_NS}\; path p(weakly_canonical(path(argv[0])))\; (void) p\; return 0\; }\n")
check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_FSIMPL_WEAKLY_CANONICAL)

set(CHECK_CXX_CODE "#include ${FILESYSTEM_HEADER}\nint main(int, char **argv) { using namespace ${FILESYSTEM_NS}\; path p(unique_path())\; (void) p\; return 0\; }\n")
check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_FSIMPL_UNIQUE_PATH)

set(CHECK_CXX_CODE "#include ${FILESYSTEM_HEADER}\nint main(int, char **argv) { using namespace ${FILESYSTEM_NS}\; path p(u8path(\"\"))\; (void) p\; return 0\; }\n")
check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_FSIMPL_U8PATH)

set(CHECK_CXX_CODE "#include ${FILESYSTEM_HEADER}\nint main(int, char **argv) { using namespace ${FILESYSTEM_NS}\; path p(argv[0])\; p = p.lexically_normal()\; return 0\; }\n")
check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_FS_PATH_LEXICALLY_NORMAL)

set(CHECK_CXX_CODE "#include ${FILESYSTEM_HEADER}\nint main(int, char **argv) { using namespace ${FILESYSTEM_NS}\; path p(argv[0])\; p = p.lexically_relative(current_path())\; return 0\; }\n")
check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_FS_PATH_LEXICALLY_RELATIVE)

set(CHECK_CXX_CODE "#include ${FILESYSTEM_HEADER}\nint main(int, char **argv) { using namespace ${FILESYSTEM_NS}\; path p(argv[0])\; p = p.lexically_proximate(current_path())\; return 0\; }\n")
check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_FS_PATH_LEXICALLY_PROXIMATE)

set(CHECK_CXX_CODE "#include ${FILESYSTEM_HEADER}\nint main(int, char **argv) { using namespace ${FILESYSTEM_NS}\; auto s = current_path().to_u8string()\; (void) s\; return 0\; }\n")
check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_FS_PATH_TO_U8STRING)

unset(CMAKE_REQUIRED_INCLUDES)
unset(CMAKE_REQUIRED_LIBRARIES)

#
# Check for availability of std::string_view in C++ standard library
# (optional)
#
set(CHECK_CXX_CODE "#include <string_view>\nint main(int, char **argv) { std::string_view s(argv[0])\; (void) s\; return 0\; }\n")
check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_STD_STRING_VIEW)

if (NOT WR_HAVE_STD_STRING_VIEW)
        set(CHECK_CXX_CODE "#include <experimental/string_view>\nint main(int, char **argv) { std::experimental::string_view s(argv[0])\; (void) s\; return 0\; }\n")
        check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_STD_EXP_STRING_VIEW)
        if (WR_HAVE_STD_EXP_STRING_VIEW)
                set(WR_HAVE_STD_STRING_VIEW 1)
        endif()
endif()

#
# Check for availability of std::optional in C++ standard library (optional)
#
set(CHECK_CXX_CODE "#include <optional>\nint main() { std::optional<int> o(1234)\; (void) o\; return 0\; }\n")
check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_STD_OPTIONAL)

if (NOT WR_HAVE_STD_OPTIONAL)
        set(CHECK_CXX_CODE "#include <experimental/optional>\nint main() { std::experimental::optional<int> o(1234)\; (void) o\; return 0\; }\n")
        check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_STD_EXP_OPTIONAL)
        if (WR_HAVE_STD_EXP_OPTIONAL)
                set(WR_HAVE_STD_OPTIONAL 1)
        endif()
endif()

#
# Check for presence of std::ctype_base::blank (older versions of libstdc++
# with partial C++11 support don't have it)
#
set(CHECK_CXX_CODE "#include <locale>\nint main() { auto m = std::ctype_base::blank\; (void) m\; return 0\; }\n")
check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_STD_CTYPE_BLANK)

#
# Check for std::is_trivially_destructible (otherwise fall back to
# std::has_trivial_destructor for older versions of libstdc++)
#
set(CHECK_CXX_CODE "#include <type_traits>\nint main() { struct Foo {}\; bool b = std::is_trivially_destructible<Foo>::value\; (void) b\; return 0\; }\n")
check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_STD_IS_TRIVIALLY_DESTRUCTIBLE)
set(CHECK_CXX_CODE "#include <type_traits>\nint main() { struct Foo {}\; bool b = std::has_trivial_destructor<Foo>::value\; (void) b\; return 0\; }\n")
check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_STD_HAS_TRIVIAL_DESTRUCTOR)

#
# Check for std::is_nothrow_destructible
#
set(CHECK_CXX_CODE "#include <type_traits>\nint main() { struct Foo {}\; bool b = std::is_nothrow_destructible<Foo>::value\; (void) b\; return 0\; }\n")
check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_STD_IS_NOTHROW_DESTRUCTIBLE)

########################################
#
# Target Definitions
#

configure_file(Config.h.in include/wrutil/Config.h)

include_directories(include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

set(WRUTIL_SOURCES
        src/ArgVBuilder.cxx
        src/CityHash.cxx
        src/codecvt/codecvt_utf8_narrow.cxx
        src/ctype.cxx
        src/Format.cxx
        src/Option.cxx
        src/SHA256.cxx
        src/string_view.cxx
        src/string_view_format.cxx
        src/TestManager.cxx
        src/u8string_view.cxx
        src/u8string_view_format.cxx
        src/uiostream.cxx
        ${CMAKE_CURRENT_BINARY_DIR}/src/ucd/CharClasses.cxx
        ${CMAKE_CURRENT_BINARY_DIR}/src/ucd/CoreProperties.cxx
        ${CMAKE_CURRENT_BINARY_DIR}/src/ucd/DecimalDigits.cxx
        ${CMAKE_CURRENT_BINARY_DIR}/src/ucd/GeneralCategories.cxx
        ${CMAKE_CURRENT_BINARY_DIR}/src/ucd/HexDigits.cxx
        ${CMAKE_CURRENT_BINARY_DIR}/src/ucd/LowerCasing.cxx
        ${CMAKE_CURRENT_BINARY_DIR}/src/ucd/TitleCasing.cxx
        ${CMAKE_CURRENT_BINARY_DIR}/src/ucd/UpperCasing.cxx
        src/utf8.cxx
        src/utf16.cxx
        src/filesystem/fs.cxx
        src/filesystem/format.cxx
)

set(WRUTIL_HEADERS
        include/wrutil/allocator.h
        include/wrutil/ArgVBuilder.h
        include/wrutil/arraybuf.h
        include/wrutil/arraystream.h
        include/wrutil/circ_fwd_list.h
        include/wrutil/CityHash.h
        include/wrutil/codecvt.h
        ${CMAKE_CURRENT_BINARY_DIR}/include/wrutil/Config.h
        include/wrutil/ctype.h
        include/wrutil/filesystem.h
        include/wrutil/Format.h
        include/wrutil/Option.h
        include/wrutil/optional.h
        include/wrutil/numeric_cast.h
        include/wrutil/SHA256.h
        include/wrutil/StdioFilePtr.h
        include/wrutil/string_view.h
        include/wrutil/TestManager.h
        include/wrutil/u8string_view.h
        include/wrutil/uiostream.h
        include/wrutil/UnicodeData.h
        include/wrutil/utf8.h
        include/wrutil/utf16.h
        include/wrutil/VarGuard.h
        include/wrutil/wbuffer_convert.h
        include/wrutil/wstring_convert.h
        src/filesystem/private.h
)

set(WRDEBUG_SOURCES
        src/ExceptionTrace.cxx
)

set(WRDEBUG_HEADERS
        include/wrutil/debug.h
)

if (NOT WR_HAVE_STD_CODECVT_CHAR16)
        list(APPEND WRUTIL_SOURCES src/codecvt/char16.cxx)
        list(APPEND WRUTIL_HEADERS include/wrutil/codecvt/char16.h)
endif()

if (NOT WR_HAVE_STD_CODECVT_CHAR32)
        list(APPEND WRUTIL_SOURCES src/codecvt/char32.cxx)
        list(APPEND WRUTIL_SOURCES src/codecvt/utf8_ucs4.cxx)
        list(APPEND WRUTIL_HEADERS src/codecvt/utf8_ucs4.h)
        list(APPEND WRUTIL_HEADERS include/wrutil/codecvt/char32.h)
endif()

if (NOT WR_HAVE_STD_CODECVT_UTF8)
        list(APPEND WRUTIL_SOURCES src/codecvt/cvt_utf8.cxx)
        list(APPEND WRUTIL_SOURCES src/codecvt/utf8_ucs4.cxx)
        list(APPEND WRUTIL_HEADERS src/codecvt/utf8_ucs4.h)
        list(APPEND WRUTIL_HEADERS include/wrutil/codecvt/cvt_utf8.h)
endif()

if (NOT WR_HAVE_FSIMPL_PROXIMATE)
        list(APPEND WRUTIL_SOURCES src/filesystem/proximate.cxx)
endif()

if (NOT WR_HAVE_FSIMPL_RELATIVE)
        list(APPEND WRUTIL_SOURCES src/filesystem/relative.cxx)
endif()

if (NOT WR_HAVE_FSIMPL_WEAKLY_CANONICAL)
        list(APPEND WRUTIL_SOURCES src/filesystem/weakcanon.cxx)
        list(APPEND WRUTIL_SOURCES src/filesystem/mklexnorm.cxx)
endif()

if (NOT WR_HAVE_FSIMPL_UNIQUE_PATH)
        list(APPEND WRUTIL_SOURCES src/filesystem/unique.cxx)
endif()

if (NOT WR_HAVE_FSIMPL_U8PATH)
        list(APPEND WRUTIL_SOURCES src/filesystem/u8path.cxx)
endif()

if (NOT WR_HAVE_FS_PATH_LEXICALLY_NORMAL)
        list(APPEND WRUTIL_SOURCES src/filesystem/lexnorm.cxx)
        list(APPEND WRUTIL_SOURCES src/filesystem/mklexnorm.cxx)
endif()

if (NOT WR_HAVE_FS_PATH_LEXICALLY_RELATIVE)
        list(APPEND WRUTIL_SOURCES src/filesystem/lexrel.cxx)
endif()

if (NOT WR_HAVE_FS_PATH_LEXICALLY_PROXIMATE)
        list(APPEND WRUTIL_SOURCES src/filesystem/lexprox.cxx)
endif()

if (NOT WR_HAVE_FS_PATH_TO_U8STRING)
        list(APPEND WRUTIL_SOURCES src/filesystem/u8path.cxx)
endif()

if (NOT WR_HAVE_STD_OPTIONAL)
        list(APPEND WRUTIL_SOURCES src/optional.cxx)
endif()

if (UNIX)
        list(APPEND WRUTIL_SOURCES src/StdioFilePtr_posix.cxx)
        list(APPEND WRUTIL_SOURCES src/TestManager_posix.cxx)
        list(APPEND WRUTIL_SOURCES src/ustreambuf_posix.cxx)
        list(APPEND WRDEBUG_SOURCES src/ExceptionTrace_posix.cxx)
        list(APPEND WRUTIL_SYS_LIBS dl)
        list(APPEND WRDEBUG_SYS_LIBS dl)
elseif (WIN32)
        list(APPEND WRUTIL_SOURCES src/StdioFilePtr_win32.cxx)
        list(APPEND WRUTIL_SOURCES src/TestManager_win32.cxx)
        list(APPEND WRUTIL_SOURCES src/ustreambuf_win32.cxx)
        list(APPEND WRDEBUG_SOURCES src/ExceptionTrace_win32.cxx)
        list(APPEND WRDEBUG_SYS_LIBS dbghelp imagehlp)
endif()


add_executable(unidatagen src/unidatagen.cxx)
target_link_libraries(unidatagen ${WRUTIL_SYS_LIBS})

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src/ucd)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/src/ucd/GeneralCategories.cxx
                          ${CMAKE_CURRENT_BINARY_DIR}/src/ucd/UpperCasing.cxx
                          ${CMAKE_CURRENT_BINARY_DIR}/src/ucd/LowerCasing.cxx
                          ${CMAKE_CURRENT_BINARY_DIR}/src/ucd/TitleCasing.cxx
                          ${CMAKE_CURRENT_BINARY_DIR}/src/ucd/DecimalDigits.cxx
                          ${CMAKE_CURRENT_BINARY_DIR}/src/ucd/HexDigits.cxx
                          ${CMAKE_CURRENT_BINARY_DIR}/src/ucd/CoreProperties.cxx
                          ${CMAKE_CURRENT_BINARY_DIR}/src/ucd/CharClasses.cxx
        COMMAND unidatagen ${CMAKE_CURRENT_SOURCE_DIR}/src/ucd
        DEPENDS unidatagen
                ${CMAKE_CURRENT_SOURCE_DIR}/src/ucd/DerivedCoreProperties.txt
                ${CMAKE_CURRENT_SOURCE_DIR}/src/ucd/PropList.txt
                ${CMAKE_CURRENT_SOURCE_DIR}/src/ucd/UnicodeData.txt
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src/ucd
)

if (UNIX)
        if (NOT WIN32)  # not Cygwin
                set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-E")
                set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-E")
                set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-E")
        endif()
endif()

add_library(wrutil SHARED ${WRUTIL_SOURCES} ${WRUTIL_HEADERS})
target_link_libraries(wrutil ${WRUTIL_SYS_LIBS})

add_library(wrutil_static STATIC ${WRUTIL_SOURCES} ${WRUTIL_HEADERS})

add_library(wrdebug SHARED ${WRDEBUG_SOURCES} ${WRDEBUG_HEADERS})
target_link_libraries(wrdebug ${WRDEBUG_SYS_LIBS})

set_target_properties(wrutil wrdebug PROPERTIES
        COMPILE_FLAGS "${WR_SOFLAGS}"
        SOVERSION ${WRUTIL_VERSION_MAJOR}
        VERSION ${WRUTIL_VERSION_MAJOR}.${WRUTIL_VERSION_MINOR}.${WRUTIL_VERSION_PATCH}
)

########################################
#
# Unit Tests
#
add_executable(ArraybufTests test/ArraybufTests.cxx)
add_executable(CircFwdListTests test/CircFwdListTests.cxx)
add_executable(FilesystemTests test/FilesystemTests.cxx)
add_executable(FormatPrintTests test/FormatPrintTests.cxx)
add_executable(OptionTests test/OptionTests.cxx test/OptionTestUtils.cxx)
add_executable(SuboptionTests test/SuboptionTests.cxx test/OptionTestUtils.cxx)
add_executable(StringViewTests test/StringViewTests.cxx)
add_executable(U8StringViewTests test/U8StringViewTests.cxx)

set(TESTS
        ArraybufTests
        CircFwdListTests
        FilesystemTests
        FormatPrintTests
        OptionTests
        SuboptionTests
        StringViewTests
        U8StringViewTests
)

set_target_properties(${TESTS} PROPERTIES RUNTIME_OUTPUT_DIRECTORY test)

foreach(TEST ${TESTS})
        add_test(${TEST} test/${TEST})
        target_link_libraries(${TEST} wrutil wrdebug)
endforeach(TEST)

########################################
#
# Output Directories
#
set_target_properties(wrutil wrutil_static wrdebug unidatagen
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TOP_BUILD_DIR}/bin
        ARCHIVE_OUTPUT_DIRECTORY ${TOP_BUILD_DIR}/lib
        LIBRARY_OUTPUT_DIRECTORY ${TOP_BUILD_DIR}/lib
)

########################################
#
# Installation
#
install(TARGETS wrutil wrutil_static wrdebug
        EXPORT wrutil-targets
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
)

install(DIRECTORY include/wrutil ${CMAKE_CURRENT_BINARY_DIR}/include/wrutil
        DESTINATION include
        DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                              GROUP_READ GROUP_EXECUTE
                              WORLD_READ WORLD_EXECUTE
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE
                         GROUP_READ
                         WORLD_READ
)

install(EXPORT wrutil-targets DESTINATION share/wrutil)

configure_file(cmake/wrutil-config.cmake.in wrutil-config.cmake @ONLY)
configure_file(cmake/wrutil-config-internal.cmake.in
               wrutil-config-internal.cmake @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/wrutil-config.cmake
              cmake/compiler-config.cmake
        DESTINATION share/wrutil
        PERMISSIONS OWNER_READ OWNER_WRITE
                    GROUP_READ
                    WORLD_READ
)
