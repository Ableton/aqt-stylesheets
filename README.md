# Aqt StyleSheets

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

Mac:

  - Xcode v5.1 or higher

Windows:

  - Visual Studio 2013


## Build and Test

```
  mkdir build
  cd build
  cmake ..
  cmake --build . --config Release
  cmake --build . --config Release --target install
```

The resulting plugin is then found inside `build/lib/qml`

The unit tests can be executed with ctest:

```
  ctest -V -C Release
```

You might set the following variables:

- Boost_INCLUDE_DIR   to the folder, where Boost headers are found

In case the CMake files shipped with Qt are not found, set the CMAKE_PREFIX_PATH
to the Qt installation prefix. See the
[Qt5 CMake manual](http://qt-project.org/doc/qt-5/cmake-manual.html) for more.

Example:

```
  cmake .. -DCMAKE_PREFIX_PATH=~/Qt/Qt5.3.1/clang_64 \
           -DBoost_INCLUDE_DIR=/opt/local/include/
```


## Examples

In the `examples` folder there's an example app, showing how to use some of the
feature of the StylePlugin. You can run the app with:

```
  qmlscene -I build/lib/qml examples/TestApp.qml
```

While qmlscene is running you can change `examples/style.css` and watch the
application update.


## Benchmarks

In the `benchmarks` folder there are benchmarks that can be run manually with:

```
qmltestrunner -import build/lib/qml -input benchmarks/benchmark_*.qml
```

## Maintainers

* [@gck-ableton](https://github.com/gck-ableton)


## License

Aqt StyleSheets is distributed under the MIT license (see LICENSE).


## CI Status

Master: [![Build status](https://ci.appveyor.com/api/projects/status/vvgdowxuay94x7e3/branch/master?svg=true)](https://ci.appveyor.com/project/gck-ableton/aqt-stylesheets/branch/master)
[![Build Status](https://travis-ci.org/Ableton/aqt-stylesheets.svg?branch=master)](https://travis-ci.org/Ableton/aqt-stylesheets)
