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

add_library(gtest_internal STATIC
  ${GTEST_SOURCE}/src/gtest-all.cc
  ${GTEST_SOURCE}/src/gtest_main.cc
)
target_include_directories(gtest_internal PUBLIC
  ${GTEST_SOURCE} ${GTEST_SOURCE}/include)

if(CMAKE_CXX_COMPILER_ID STREQUAL Clang
    OR CMAKE_CXX_COMPILER_ID STREQUAL AppleClang)
  target_compile_options(gtest_internal PUBLIC
    ${cxx11_options} -Wno-global-constructors)
endif()

add_executable(StyleSheetParserTest
  tst_Convert.cpp
  tst_CssParser.cpp
  tst_StyleMatchTree.cpp
)

target_link_libraries(StyleSheetParserTest
  StyleSheetParser gtest_internal)

add_test(StyleSheetParserTestCase StyleSheetParserTest)
