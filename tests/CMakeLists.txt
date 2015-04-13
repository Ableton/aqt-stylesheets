# Copyright (c) 2015 Ableton AG, Berlin
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

if(WIN32)
  if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(QMLTESTRUNNER_APP qmltestrunnerd)
  else()
    set(QMLTESTRUNNER_APP qmltestrunner)
  endif()
else()
  set(QMLTESTRUNNER_APP qmltestrunner)
endif()


add_library(AqtTestUtilsPlugin MODULE
  TestUtils.hpp
  TestUtils.cpp
)
target_compile_options(AqtTestUtilsPlugin
  PUBLIC ${cxx11_options} ${warning_options})
target_link_libraries(AqtTestUtilsPlugin
  Qt5::Qml Qt5::Test Qt5::QuickTest)

set(plugin_output "${PROJECT_BINARY_DIR}/output")
set_target_properties(AqtTestUtilsPlugin
  PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${plugin_output})
set_target_properties(AqtTestUtilsPlugin
  PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${plugin_output})
set_target_properties(AqtTestUtilsPlugin
  PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${plugin_output})

set(PATH_TO_PLUGIN "${plugin_output}")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Aqt/Testing/qmldir.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/Aqt/Testing/qmldir"
  @ONLY)
unset(PATH_TO_PLUGIN)

file(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/test-*.qml")

set(i 0)
foreach(filename ${files})
  add_test(NAME "IntegrationTest_${i}"
           COMMAND "${QMLTESTRUNNER_APP}" -import "${PROJECT_SOURCE_DIR}/qml"
                                 -import "${CMAKE_CURRENT_SOURCE_DIR}"
                                 -input "${filename}"
           WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/tests")
  math(EXPR i "${i} + 1")
endforeach()