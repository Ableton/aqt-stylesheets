# Copyright (c) 2014-15 Ableton AG, Berlin
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

if (CMAKE_BUILD_TYPE MATCHES Debug)
  add_definitions(-DDEBUG=1)
else()
  add_definitions(-DNDEBUG=1)
endif()

add_definitions(-DPEGLIB_NO_UNICODE_CHARS)

add_library(StyleSheetParser
  Convert.hpp
  Convert.cpp
  CssParser.cpp
  CssParser.hpp
  Log.hpp
  Property.hpp
  StyleMatchTree.cpp
  StyleMatchTree.hpp
  UrlUtils.cpp
  UrlUtils.hpp
  Warnings.hpp
)
target_include_directories(StyleSheetParser PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include;${PROJECT_SOURCE_DIR}/third-party;${Boost_INCLUDE_DIRS}>")
target_compile_options(StyleSheetParser
  PUBLIC ${cxx11_options} ${warning_options})

target_link_libraries(StyleSheetParser Qt5::Quick)

add_library(StylePlugin MODULE
  StyleChecker.cpp
  StyleChecker.hpp
  StyleEngine.cpp
  StyleEngine.hpp
  StyleEngineSetup.cpp
  StyleEngineSetup.hpp
  StylePlugin.cpp
  StylePlugin.hpp
  StyleSet.cpp
  StyleSet.hpp
  StyleSetProps.cpp
  StyleSetProps.hpp
  StyleSetProps.ipp
  StylesDirWatcher.cpp
  StylesDirWatcher.hpp
)
target_link_libraries(StylePlugin StyleSheetParser Qt5::Quick)

set(plugin_output "${PROJECT_BINARY_DIR}/output")
set_target_properties(StylePlugin
  PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${plugin_output})
set_target_properties(StylePlugin
  PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${plugin_output})
set_target_properties(StylePlugin
  PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${plugin_output})

install(TARGETS StylePlugin
  LIBRARY DESTINATION "${PLUGIN_INSTALL_DIR}/Aqt/StyleSheets")
install(DIRECTORY "${PROJECT_SOURCE_DIR}/qml/Aqt"
  DESTINATION "${PLUGIN_INSTALL_DIR}"
  FILES_MATCHING REGEX "(qmldir|(.*\\.(qml|js)))$")

add_subdirectory(test)
