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

include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)


########################################
#
# C++ Compiler Configuration
#
check_cxx_compiler_flag(-std=c++11 CXX11_STD_FLAG)
if (CXX11_STD_FLAG)
        set(CXX11_STD_FLAG -std=c++11)
else()
        check_cxx_compiler_flag(-std=c++0x CXX0X_STD_FLAG)
        if (CXX0X_STD_FLAG)
                set(CXX11_STD_FLAG -std=c++0x)
        endif()
endif()

check_cxx_compiler_flag(-std=c++14 CXX14_STD_FLAG)
if (CXX14_STD_FLAG)
        set(CXX14_STD_FLAG -std=c++14)
else()
        check_cxx_compiler_flag(-std=c++1y CXX1Y_STD_FLAG)
        if (CXX1Y_STD_FLAG)
                set(CXX14_STD_FLAG -std=c++1y)
        endif()
endif()

check_cxx_compiler_flag(-std=c++17 CXX17_STD_FLAG)
if (CXX17_STD_FLAG)
        set(CXX17_STD_FLAG -std=c++17)
else()
        check_cxx_compiler_flag(-std=c++1z CXX1Z_STD_FLAG)
        if (CXX1Z_STD_FLAG)
                set(CXX17_STD_FLAG -std=c++1z)
        endif()
endif()

if (CXX17_STD_FLAG AND USE_CXX17)
        set(CXX_CHOSEN_STD_FLAG ${CXX17_STD_FLAG})
elseif (CXX14_STD_FLAG AND USE_CXX14)
        set(CXX_CHOSEN_STD_FLAG ${CXX14_STD_FLAG})
elseif (CXX11_STD_FLAG)
        set(CXX_CHOSEN_STD_FLAG ${CXX11_STD_FLAG})
endif()

if (CXX_CHOSEN_STD_FLAG)
        message("Compiling with ${CXX_CHOSEN_STD_FLAG}")
        add_definitions(${CXX_CHOSEN_STD_FLAG})
        list(APPEND CMAKE_REQUIRED_DEFINITIONS ${CXX_CHOSEN_STD_FLAG})
endif()

if (NOT WIN32)
        set(CHECK_CXX_CODE "int __attribute__((visibility(\"hidden\"))) foo() { return 0\; }\nint main() { return foo()\; }")
        check_cxx_source_compiles(${CHECK_CXX_CODE} WR_HAVE_ELF_VISIBILITY_ATTR)
endif()

if (WR_HAVE_ELF_VISIBILITY_ATTR)
        check_cxx_compiler_flag(-fvisibility=hidden HAVE_ELF_VISIBILITY_FLAG)
        if (HAVE_ELF_VISIBILITY_FLAG)
                set(WR_SOFLAGS "${WR_SOFLAGS} -fvisibility=hidden")
        endif()
        check_cxx_compiler_flag(-fvisibility-inlines-hidden
                                HAVE_ELF_INLINE_VISIBILITY_FLAG)
        if (HAVE_ELF_INLINE_VISIBILITY_FLAG)
                set(WR_SOFLAGS "${WR_SOFLAGS} -fvisibility-inlines-hidden")
        endif()
endif()
