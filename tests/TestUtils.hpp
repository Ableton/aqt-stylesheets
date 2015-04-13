/*
Copyright (c) 2015 Ableton AG, Berlin

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

#pragma once

#include "../src/Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtCore/QObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtQml/QQmlExtensionPlugin>
#include <QtTest/QTest>
RESTORE_WARNINGS

/*! @cond DOXYGEN_IGNORE */

namespace aqt
{
namespace stylesheets
{
namespace tests
{

class MsgTracker : public QObject
{
  Q_OBJECT
  Q_ENUMS(LogLevel)

public:
  MsgTracker(QObject* pParent = nullptr)
    : QObject(pParent)
  {
  }

  enum LogLevel { Debug = QtDebugMsg, Warning = QtWarningMsg, Critical = QtCriticalMsg };

  Q_INVOKABLE void expectMessage(LogLevel level, QVariant pattern)
  {
    if (pattern.userType() == QMetaType::QRegularExpression) {
      QTest::ignoreMessage(QtMsgType(level), pattern.toRegularExpression());
    } else if (pattern.userType() == QMetaType::QRegExp) {
      QTest::ignoreMessage(
        QtMsgType(level), QRegularExpression(pattern.toRegExp().pattern()));
    } else {
      QTest::ignoreMessage(QtMsgType(level), pattern.toString().toUtf8().data());
    }
  }
};

class TestUtilsPlugin : public QQmlExtensionPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "Aqt.Testing")

public:
  void registerTypes(const char* uri);
};

} // namespace tests
} // namespace stylesheets
} // namespace aqt

/*! @endcond */
