/*
Copyright (c) 2014 Ableton AG, Berlin

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

#include "Warnings.hpp"

SUPPRESS_WARNINGS
#if defined(USE_BOOST_LOG)
#include <boost/log/trivial.hpp>
#elif defined(USE_CUSTOM_LOG)
#include CUSTOM_LOG_INCLUDE
#else
#include <QtCore/QtDebug>
#endif
RESTORE_WARNINGS

#if defined(USE_BOOST_LOG)
#define styleSheetsLogError() BOOST_LOG_TRIVIAL(error)
#define styleSheetsLogWarning() BOOST_LOG_TRIVIAL(warning)
#define styleSheetsLogInfo() BOOST_LOG_TRIVIAL(info)
#define styleSheetsLogDebug() BOOST_LOG_TRIVIAL(debug)

#elif !defined(USE_CUSTOM_LOG)

#define styleSheetsLogError() qCritical() << "CRITICAL:"
#define styleSheetsLogWarning() qWarning() << "WARN:"
#define styleSheetsLogInfo() qDebug() << "INFO:"
#define styleSheetsLogDebug() qDebug() << "DEBUG:"

inline QDebug& operator<<(QDebug& debug, const std::string& str)
{
  debug << QString::fromStdString(str);
  return debug;
}

#endif
