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
#include <QtCore/QString>
RESTORE_WARNINGS

#include <string>
#include <vector>

/*! @cond DOXYGEN_IGNORE */

namespace aqt
{
namespace stylesheets
{

using PropValues = std::vector<std::string>;
using Selector = std::vector<std::string>;

class LocInfo
{
public:
  LocInfo()
    : byteofs(0)
    , line(0)
    , column(0)
  {
  }

  int byteofs;
  int line;
  int column;
};

class Property
{
public:
  std::string name;
  PropValues values;
  LocInfo locInfo;
};

class Propset
{
public:
  std::vector<Selector> selectors;
  std::vector<Property> properties;
  LocInfo locInfo;
};

class FontFaceDecl
{
public:
  std::string url;
};

class StyleSheet
{
public:
  std::vector<Propset> propsets;
  std::vector<FontFaceDecl> fontfaces;
};

class ParseException
{
public:
  ParseException(const std::string& msg, const std::string& errorContext = "")
    : mMsg(msg)
    , mErrorContext(errorContext)
  {
  }

  std::string message() const
  {
    return mMsg;
  }
  std::string errorContext() const
  {
    return mErrorContext;
  }

private:
  std::string mMsg;
  std::string mErrorContext;
};

StyleSheet parseStdString(const std::string& data);
StyleSheet parseString(const QString& path);

/*! Read and parse the style sheet file from @path
 *
 * @return the parsed style sheet
 *
 * @throw std::ios_base::failure exception on IO error or if the file at @path
 *        can not be opened.
 * @throw ParseException when the stylesheet could not be parsed
 */
StyleSheet parseStyleFile(const QString& path);

} // namespace stylesheets
} // namespace aqt

/*! @endcond */
