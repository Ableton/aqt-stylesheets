/*
Copyright (c) 2014-15 Ableton AG, Berlin

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

#include "Property.hpp"
#include "CssParser.hpp"

#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtCore/QString>
#include <QtCore/QVariant>
RESTORE_WARNINGS

#include <map>
#include <memory>
#include <string>
#include <unordered_map>

/*! @cond DOXYGEN_IGNORE */

namespace aqt
{
namespace stylesheets
{
class PathElement
{
public:
  PathElement(const std::string& typeName,
              const std::vector<std::string>& classNames = {})
    : mTypeName(typeName)
    , mClassNames(classNames)
  {
  }

  bool operator==(const PathElement& other) const
  {
    return mTypeName == other.mTypeName && mClassNames == other.mClassNames;
  }

  bool operator!=(const PathElement& other) const
  {
    return !(*this == other);
  }

  std::string mTypeName;
  std::vector<std::string> mClassNames;
};

using UiItemPath = std::vector<PathElement>;

std::ostream& operator<<(std::ostream& os, const UiItemPath& path);
std::string pathToString(const UiItemPath& path);


using PropertyMap = std::map<QString, Property>;

void mergeInheritableProperties(PropertyMap& dest, const PropertyMap& b);

class IStyleMatchTree
{
};

std::unique_ptr<IStyleMatchTree> createMatchTree(
  const StyleSheet& stylesheet, const StyleSheet& defaultStylesheet = StyleSheet());

PropertyMap matchPath(const IStyleMatchTree* tree, const UiItemPath& path);
std::string describeMatchedPath(const IStyleMatchTree* tree, const UiItemPath& path);

} // namespace stylesheets
} // namespace aqt

/*! @endcond */
