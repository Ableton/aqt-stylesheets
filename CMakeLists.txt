# Copyright (c) 2014 Ableton AG, Berlin
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

cmake_minimum_required(VERSION 2.8.12)

project(StyleSheets CXX)
set(StyleSheets_VERSION_MAJOR 0)
set(StyleSheets_VERSION_MINOR 1)

# Enable C++11
include(CheckCXXCompilerFlag)
if(WIN32)
  # Check if we are using Visual Studio 2013 or later
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 12)
    message(FATAL_ERROR "You are using an unsupported Windows compiler! (Visual C++ 2013 or later required)")
  endif()
else()
  set(cxx11_options -std=c++11)
  CHECK_CXX_COMPILER_FLAG(${cxx11_options} COMPILER_SUPPORTS_CXX11)

  if(NOT COMPILER_SUPPORTS_CXX11)
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has insufficient C++11 support. Please use a different C++ compiler.")
  endif()
endif()

# Build debug by default
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Enable warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL Clang
    OR CMAKE_CXX_COMPILER_ID STREQUAL AppleClang)
  set(warning_options
    -Werror
    -Weverything
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-disabled-macro-expansion
    -Wno-documentation-unknown-command
    -Wno-exit-time-destructors
    -Wno-padded
    -Wno-undefined-reinterpret-cast
    -Wno-documentation-deprecated-sync
    -Wno-weak-vtables
    )
elseif(MSVC)
  set(warning_options /WX /W4 /wd4127 /wd4275 /wd4503 /wd4512 /wd4714)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)
set(CMAKE_MACOSX_RPATH ON)

# We install aqt-stylesheets within the build dir. It's up to you to
# move it elsewhere.
#
# You can customize PLUGIN_INSTALL_DIR to be inside the Qt
# distribution like all the other qml plugins, if you want to install
# it system-wide.

set(PLUGIN_INSTALL_DIR "${PROJECT_BINARY_DIR}/lib/qml")

enable_testing()

find_package(Qt5Quick 5.3.0 REQUIRED)
find_package(Qt5Qml 5.3.0 REQUIRED)
find_package(Qt5Test 5.3.0 REQUIRED)
find_package(Qt5QuickTest 5.3.0 REQUIRED)

if(DEFINED Boost_INCLUDE_DIR)
  get_filename_component(Boost_INCLUDE_DIR ${Boost_INCLUDE_DIR} ABSOLUTE)
endif()
find_package(Boost 1.54 REQUIRED)

include(FeatureSummary)
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

add_subdirectory(src)
add_subdirectory(tests)
