/*
Copyright (c) 2014-2015 Ableton AG, Berlin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "StylePlugin.hpp"

#include "StyleChecker.hpp"
#include "StyleEngineSetup.hpp"
#include "StylesDirWatcher.hpp"
#include "StyleSet.hpp"
#include "StyleSetProps.hpp"
#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
RESTORE_WARNINGS

void aqt::stylesheets::StylePlugin::registerTypes(const char* pUri)
{
  qmlRegisterUncreatableType<aqt::stylesheets::StyleSet>(
    pUri, 1, 0, "StyleSet", "StyleSet is exposed as an attached property");
  qmlRegisterUncreatableType<aqt::stylesheets::StyleSetProps>(
    pUri, 1, 0, "StyleSetProps", "Exposed as StyleSet.props");
  qmlRegisterUncreatableType<aqt::stylesheets::StyleSetProps, 2>(
    pUri, 1, 2, "StyleSetProps", "Exposed as StyleSet.props");
  qmlRegisterType<aqt::stylesheets::StyleEngineSetup>(pUri, 1, 0, "StyleEngine");
  qmlRegisterType<aqt::stylesheets::StyleEngineSetup, 1>(pUri, 1, 1, "StyleEngine");
  qmlRegisterType<aqt::stylesheets::StylesDirWatcher>(pUri, 1, 1, "StylesDirWatcher");
  qmlRegisterType<aqt::stylesheets::StyleChecker>(pUri, 1, 3, "StyleChecker");
}

#if !defined(NOT_INCLUDE_MOC)
SUPPRESS_WARNINGS
#include "moc_StylePlugin.cpp"
RESTORE_WARNINGS
#endif
