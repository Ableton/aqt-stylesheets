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

#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <boost/variant/variant.hpp>
RESTORE_WARNINGS

#include <string>
#include <tuple>
#include <vector>

/*! @cond DOXYGEN_IGNORE */

namespace aqt
{
namespace stylesheets
{

class Expression
{
public:
  std::string name;
  std::vector<std::string> args;
};

using PropertyValue = boost::variant<std::string, Expression>;
using PropertyValues = std::vector<PropertyValue>;

class SourceLocation
{
public:
  SourceLocation()
    : mSourceLayer(0)
    , mByteOfs(0)
    , mLine(0)
    , mColumn(0)
  {
  }

  SourceLocation(int sourceLayer, int byteOfs, int line, int column)
    : mSourceLayer(sourceLayer)
    , mByteOfs(byteOfs)
    , mLine(line)
    , mColumn(column)
  {
  }

  bool operator<(const SourceLocation& other) const
  {
    return std::tie(mSourceLayer, mByteOfs)
           < std::tie(other.mSourceLayer, other.mByteOfs);
  }

  int mSourceLayer;
  int mByteOfs;
  int mLine;
  int mColumn;
};

class Property
{
public:
  Property() = default;
  Property(const SourceLocation& loc, const PropertyValues& values)
    : mSourceLoc(loc)
    , mValues(values)
  {
  }
  Property(const Property& other) = default;
  Property& operator=(const Property& other) = default;

  SourceLocation mSourceLoc;
  PropertyValues mValues;
};

} // namespace stylesheets
} // namespace aqt

/*! @endcond */
