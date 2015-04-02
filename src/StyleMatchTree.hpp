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

#include "CssParser.hpp"
#include "Property.hpp"

#include "estd/memory.hpp"
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

class SourceLocation
{
public:
  SourceLocation()
    : mSourceLayer(0)
    , mLocInfo()
  {
  }

  SourceLocation(int sourceLayer, const LocInfo& locInfo)
    : mSourceLayer(sourceLayer)
    , mLocInfo(locInfo)
  {
  }

  bool operator<(const SourceLocation& other) const
  {
    return std::tie(mSourceLayer, mLocInfo.byteofs)
           < std::tie(other.mSourceLayer, other.mLocInfo.byteofs);
  }

  int mSourceLayer;
  LocInfo mLocInfo;
};

class PropertyDef
{
public:
  PropertyDef(const SourceLocation& loc, const PropValues& values)
    : mSourceLoc(loc)
    , mValues(values)
  {
  }

  SourceLocation mSourceLoc;
  PropValues mValues;
};

using PropertyDefMap = std::unordered_map<std::string, PropertyDef>;

/*! The basic building block for a "match tree"
 *
 * A StyleMatchTree is constructed as a tree of @c MatchNode instances.  It
 * is built over the selectors defined in a style sheet.  Each node is a
 * dictionary of type/class name mapping to further match nodes.
 * Ultimately, a match node carries a set of property definitions (in
 * member propmap).
 *
 * The tree is built upside down: A selector "A B C" leads to a tree
 * starting at the root "C".
 *
 * All selectors from all stylesheets are merged in one matchnode tree,
 * i.e. the two selectors "Gaz > Bar" and "Foo > Gaz > Bar" will result in
 * a match node tree like this (where "{}" denote the empty property
 * definition set):
 *
 * @code
 * Bar -> {}
 *   Gaz -> { properties }
 *     Foo -> { properties }
 * @endcode
 *
 * Note the properties both at node "Gaz" and "Bar".
 *
 * Descendants selectors are constructed using the special axis denotator
 * "::desc::".  A selector "Foo > Gaz Bar" will be constructed like this:
 *
 * @code
 * Bar -> {}
 *   ::desc:: -> {}
 *     Gaz -> {}
 *       Foo -> { properties }
 * @endcode
 */
class MatchNode
{
public:
  MatchNode()
  {
  }

  MatchNode(const PropertyDefMap* pProperties)
  {
    if (pProperties) {
      properties = *pProperties;
    }
  }

  MatchNode(const MatchNode&) = delete;
  MatchNode& operator=(const MatchNode&) = delete;

#if defined(DEBUG)
  void dump(const std::string& path) const;
#endif

  PropertyDefMap properties;

  using Matches = std::unordered_map<std::string, std::unique_ptr<MatchNode> >;
  Matches matches;
};

class StyleMatchTree
{
public:
  StyleMatchTree()
    : rootMatches(estd::make_unique<MatchNode>())
  {
  }

  // For some reason visual studio 2013 wasn't capable of default-creating the
  // move ctors here, so we provide them explicitly here.
  StyleMatchTree(StyleMatchTree&& other)
    : rootMatches(std::move(other.rootMatches))
  {
  }

  StyleMatchTree& operator=(StyleMatchTree&& other)
  {
    if (this != &other) {
      rootMatches = std::move(other.rootMatches);
    }
    return *this;
  }

#if defined(DEBUG)
  void dump() const;
#endif

  std::unique_ptr<MatchNode> rootMatches;
};

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

using PropertyMap = std::map<QString, PropValues>;

#if defined(DEBUG)
void dumpPropertyMap(const PropertyMap& properties);
#endif

void mergeInheritableProperties(PropertyMap& dest, const PropertyMap& b);

StyleMatchTree createMatchTree(const StyleSheet& stylesheet,
                               const StyleSheet& defaultStylesheet = StyleSheet());

PropertyMap matchPath(const StyleMatchTree& tree, const UiItemPath& path);
std::string describeMatchedPath(const StyleMatchTree& tree, const UiItemPath& path);

} // namespace stylesheets
} // namespace aqt

/*! @endcond */
