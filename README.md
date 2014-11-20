# Aqt StyleSheets

[![Build Status](https://travis-ci.org/AbletonAG/aqt-stylesheets.svg?branch=master)](https://travis-ci.org/AbletonAG/aqt-stylesheets)

StylesSheets is a library for Qt/QML that allows you to apply CSS stylesheets to
QML applications.

A singleton StyleEngine reads and parses a CSS file. QML components in your
application can use the StyleSet attached object to request style properties. In
doing so, the classname of the requesting object, its position in the component
hierarchy, and its StyleSet.name info are used to build a selector path that is
searched for in the CSS.

Decoupling application visual style from QML code allows designers and
developers freedom to change the visual appearance of an application without
breaking functionality, and allows for changing many similar QML components with
fewer lines of code through selectors that apply across many components.


## Requirements

Dependencies:

  - Qt (>= 5.3)
  - Boost (>= 1.54)
  - CMake (>= 2.8.12)

Test dependencies:

  - GoogleTest (https://chromium.googlesource.com/external/googletest.git)

Mac:

  - Xcode v5.1 or higher

Windows:

  - Visual Studio 2013


## Build and Test

```
  mkdir build
  cd build
  cmake ..
  cmake --build .
```

The unit tests can be executed with ctest:

```
  ctest -V
```

You might set the following variables:

- Boost_INCLUDE_DIR   to the folder, where Boost headers are found
- GTEST_SOURCE        to the folder of GoogleTest sources.

In case the CMake files shipped with Qt are not found, set the CMAKE_PREFIX_PATH
to the Qt installation prefix. See the
[Qt5 CMake manual](http://qt-project.org/doc/qt-5/cmake-manual.html) for more.

Example:

```
  cmake .. -DCMAKE_PREFIX_PATH=~/Qt/Qt5.3.1/clang_64 \
  -DGTEST_SOURCE=~/Dev/googletest -DBoost_INCLUDE_DIR=/opt/local/include/
```


## Examples

In the `examples` folder there's an example app, showing how to use some of the
feature of the StylePlugin. You can run the app with:

```
  qmlscene -I qml/ examples/TestApp.qml
```

While qmlscene is running you can change `examples/style.css` and watch the
application update.


## License

Aqt StyleSheets is distributed under the MIT license (see LICENSE).
