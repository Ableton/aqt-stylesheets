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

#include "StyleMatchTree.hpp"
#include "CssParser.hpp"

#include "estd/memory.hpp"
#include "Warnings.hpp"
#include "Log.hpp"

SUPPRESS_WARNINGS
#include <QtCore/QtCore>
#include <boost/assert.hpp>
#include <boost/functional/hash.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
RESTORE_WARNINGS

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace aqt
{
namespace stylesheets
{
namespace
{

SUPPRESS_WARNINGS
const std::string kDescendantAxisId = "::desc::";
const std::string kConjunctionIndicator = "&";
const std::string kChildIndicator = ">";
const std::string kDot = ".";
RESTORE_WARNINGS

using PropertyDefMap = std::unordered_map<std::string, Property>;

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

  PropertyDefMap properties;

  using Matches = std::unordered_map<std::string, std::unique_ptr<MatchNode>>;
  Matches matches;
};

class StyleMatchTree : public IStyleMatchTree
{
public:
  StyleMatchTree()
    : rootMatches(estd::make_unique<MatchNode>())
  {
  }

  std::unique_ptr<MatchNode> rootMatches;
};

PropertyDefMap makeProperties(const std::vector<PropertySpec>& props,
                              const int sourceLayer)
{
  PropertyDefMap properties;

  for (const auto& prop : props) {
    SourceLocation propSrcLoc(prop.mSourceLoc);
    propSrcLoc.mSourceLayer = sourceLayer;

    auto propDef = Property(propSrcLoc, prop.values);
    properties.insert(std::make_pair(prop.name, propDef));
  }

  return properties;
}

// copies all properties from @p src into @p dest if
// - the property is not yet contained in @p dest, or
// - @p src's property source loc has a higher weight than the one in @p dest
void mergePropertyDefs(PropertyDefMap& dest, const PropertyDefMap& src)
{
  for (const auto& propdef : src) {
    auto foundIt = dest.find(propdef.first);
    if (foundIt == dest.end()) {
      dest.insert(propdef);
    } else if (foundIt->second.mSourceLoc < propdef.second.mSourceLoc) {
      foundIt->second = propdef.second;
    }
  }
}

MatchNode* matchAndInsertSel(MatchNode* node,
                             const std::string& sel,
                             const PropertyDefMap* pProperties)
{
  auto it = node->matches.find(sel);
  if (it != node->matches.end()) {
    if (pProperties) {
      mergePropertyDefs(it->second.get()->properties, *pProperties);
    }
    return it->second.get();
  }

  auto newNode = estd::make_unique<MatchNode>(pProperties);
  return (node->matches[sel] = std::move(newNode)).get();
}

std::vector<std::string> transformSelector(
  const std::vector<std::vector<std::string>>& selector)
{
  std::vector<std::string> result;
  auto last_was_symbol = false;
  auto is_conjunction = false;

  for (auto sel : selector) {
    is_conjunction = false;
    for (auto selPart : sel) {
      if (selPart == kChildIndicator) {
        // skip
        last_was_symbol = false;
      } else {
        if (!is_conjunction) {
          if (last_was_symbol) {
            result.push_back(kDescendantAxisId);
            result.push_back(selPart);
          } else {
            result.push_back(selPart);
            last_was_symbol = true;
          }
        } else {
          result.push_back(kConjunctionIndicator);
          result.push_back(selPart);
          last_was_symbol = true;
        }
      }
      is_conjunction = true;
    }
  }

  return result;
}

void mergePropSet(MatchNode* parent, int sourceLayer, const PropertySpecSet& ps)
{
  auto properties = makeProperties(ps.properties, sourceLayer);

  for (const auto& rawSelector : ps.selectors) {
    auto* node = parent;
    auto selector = transformSelector(rawSelector);

    for (auto sel = selector.rbegin(), end = std::prev(selector.rend()); sel != end;
         ++sel) {
      node = matchAndInsertSel(node, *sel, nullptr);
    }

    node = matchAndInsertSel(node, selector.front(), &properties);
  }
}

} // anon namespace

#define DEFAULT_STYLESHEET_LAYER 0
#define USER_STYLESHEET_LAYER 1

std::unique_ptr<IStyleMatchTree> createMatchTree(const StyleSheet& stylesheet,
                                                 const StyleSheet& defaultStylesheet)
{
  auto result = estd::make_unique<StyleMatchTree>();

  for (auto ps : defaultStylesheet.propsets) {
    mergePropSet(result->rootMatches.get(), DEFAULT_STYLESHEET_LAYER, ps);
  }

  for (auto ps : stylesheet.propsets) {
    mergePropSet(result->rootMatches.get(), USER_STYLESHEET_LAYER, ps);
  }

  return std::move(result);
}

namespace
{

/*! Specificity for matching selectors
 *
 * This bascially works like CSS specificity computation, but since we
 * don't support style arguments and IDs (CSS's most specific values) our
 * specificity encodes two values only: class (incl. pseudo class and
 * attribute) and elements. */
class Specificity
{
public:
  Specificity()
    : mClass(0)
    , mElements(0)
  {
  }

  Specificity(const Specificity& other, int incClass, int incElements)
    : mClass(other.mClass + incClass)
    , mElements(other.mElements + incElements)
  {
  }

  bool operator<(const Specificity& other) const
  {
    return std::tie(mClass, mElements) < std::tie(other.mClass, other.mElements);
  }

  bool operator==(const Specificity& other) const
  {
    return std::tie(mClass, mElements) == std::tie(other.mClass, other.mElements);
  }

  bool operator!=(const Specificity& other) const
  {
    return !(*this == other);
  }

  // int mStyle -- no style attribute!
  // int mId -- no id!
  int mClass;
  int mElements;
};

std::ostream& operator<<(std::ostream& os, const Specificity& spec)
{
  os << "[" << spec.mClass << "," << spec.mElements << "]";
  return os;
}

class MatchRec
{
public:
  using Nodes = std::vector<std::tuple<Specificity, const MatchNode*>>;

  MatchRec()
  {
  }

  MatchRec(const Nodes& pNodes_)
    : pNodes(pNodes_)
  {
  }

  MatchRec& operator+=(const MatchRec& rhs)
  {
    pNodes.insert(pNodes.end(), rhs.pNodes.begin(), rhs.pNodes.end());
    return *this;
  }

  Nodes pNodes;
};

using MatchTuple = std::tuple<Specificity, PropertyDefMap>;
using MatchResult = std::vector<MatchTuple>;

void findDescendantMatchOnNode(MatchResult& result,
                               Specificity specificity,
                               const MatchNode* node,
                               const PathElement& pathElt,
                               UiItemPath::const_reverse_iterator nextEltIter,
                               UiItemPath::const_reverse_iterator pathEltEnd);

void iterateOverMatches(MatchResult& result,
                        const MatchRec m,
                        const PathElement& pathElt,
                        UiItemPath::const_reverse_iterator nextEltIter,
                        UiItemPath::const_reverse_iterator pathEltEnd);

Specificity getMatchRecSpecificity(const std::tuple<Specificity, const MatchNode*>& tuple)
{
  return std::get<0>(tuple);
}

const MatchNode* getMatchRecNode(const std::tuple<Specificity, const MatchNode*>& tuple)
{
  return std::get<1>(tuple);
}

Specificity getMatchSpecificity(const MatchTuple& tuple)
{
  return std::get<0>(tuple);
}

const PropertyDefMap& getMatchProperties(const MatchTuple& tuple)
{
  return std::get<1>(tuple);
}

MatchRec findPattern(MatchResult& result,
                     Specificity specificity,
                     const MatchNode* node,
                     const std::string& name)
{
  auto found = node->matches.find(name);
  if (found != node->matches.end()) {
    auto const* nd = found->second.get();
    if (!nd->properties.empty()) {
      result.emplace_back(std::make_tuple(specificity, nd->properties));
    }

    return MatchRec({std::make_tuple(specificity, nd)});
  }

  return MatchRec();
}

MatchRec findPathElement(MatchResult& result,
                         Specificity specificity,
                         const MatchNode* node,
                         const PathElement& pathElt)
{
  auto matchRec =
    findPattern(result, Specificity(specificity, 0, 1), node, pathElt.mTypeName);

  for (const auto& className : pathElt.mClassNames) {
    std::string dotName(kDot + className);
    auto m2 = findPattern(result, Specificity(specificity, 1, 0), node, dotName);
    matchRec += m2;
  }

  return matchRec;
}

void findMatchOnNode(MatchResult& result,
                     Specificity specificity,
                     const MatchNode* node,
                     const PathElement& pathElt,
                     UiItemPath::const_reverse_iterator nextEltIter,
                     UiItemPath::const_reverse_iterator pathEltEnd)
{
  auto m = findPathElement(result, specificity, node, pathElt);
  iterateOverMatches(result, m, pathElt, nextEltIter, pathEltEnd);
}

void iterateOverMatches(MatchResult& result,
                        const MatchRec matchRec,
                        const PathElement& pathElt,
                        UiItemPath::const_reverse_iterator nextEltIter,
                        UiItemPath::const_reverse_iterator pathEltEnd)
{
  auto tryToMatchConjunction = [&](Specificity specificity, const MatchNode* node) {
    auto m = findPattern(result, specificity, node, kConjunctionIndicator);
    for (const auto& tup : m.pNodes) {
      findMatchOnNode(result, getMatchRecSpecificity(tup), getMatchRecNode(tup), pathElt,
                      nextEltIter, pathEltEnd);
    }
  };

  auto tryToMatchChild = [&](Specificity specificity, const MatchNode* node) {
    if (nextEltIter != pathEltEnd) {
      findMatchOnNode(
        result, specificity, node, *nextEltIter, std::next(nextEltIter), pathEltEnd);
    }
  };

  auto tryToMatchDescendant = [&](Specificity specificity, const MatchNode* node) {
    if (nextEltIter != pathEltEnd) {
      auto m = findPattern(result, specificity, node, kDescendantAxisId);
      for (const auto& tup : m.pNodes) {
        findDescendantMatchOnNode(result, getMatchRecSpecificity(tup),
                                  getMatchRecNode(tup), *nextEltIter,
                                  std::next(nextEltIter), pathEltEnd);
      }
    }
  };

  for (const auto& tup : matchRec.pNodes) {
    auto specificity = getMatchRecSpecificity(tup);
    auto node = getMatchRecNode(tup);
    tryToMatchConjunction(specificity, node);
    tryToMatchChild(specificity, node);
    tryToMatchDescendant(specificity, node);
  }
}

void findDescendantMatchOnNode(MatchResult& result,
                               Specificity specificity,
                               const MatchNode* node,
                               const PathElement& pathElt,
                               UiItemPath::const_reverse_iterator nextEltIter,
                               UiItemPath::const_reverse_iterator pathEltEnd)
{
  auto m = findPathElement(result, specificity, node, pathElt);
  if (m.pNodes.empty()) {
    if (nextEltIter != pathEltEnd) {
      findDescendantMatchOnNode(
        result, specificity, node, *nextEltIter, std::next(nextEltIter), pathEltEnd);
    }
  } else {
    iterateOverMatches(result, m, pathElt, nextEltIter, pathEltEnd);
  }
}

MatchResult findMatchingRules(const StyleMatchTree& tree, const UiItemPath& path)
{
  MatchResult result;

  UiItemPath::const_reverse_iterator pathEltIter = path.rbegin();
  if (pathEltIter != path.rend()) {
    findMatchOnNode(result, Specificity(), tree.rootMatches.get(), *pathEltIter,
                    std::next(pathEltIter), path.rend());
  }

  return result;
}

void sortMatchResults(MatchResult& result)
{
  std::sort(
    result.begin(), result.end(), [](const MatchTuple& lhs, const MatchTuple& rhs) {
      return getMatchSpecificity(lhs) < getMatchSpecificity(rhs);
    });
}

using SourceLocationMap = std::unordered_map<std::string, SourceLocation>;

template <typename Pred>
void mergePropertiesIntoPropertyMap(PropertyMap& dest,
                                    const PropertyDefMap& defs,
                                    SourceLocationMap& locationMap,
                                    Pred isPropLessSpecificPred)
{
  for (auto const& propdef : defs) {
    auto foundIt = locationMap.find(propdef.first);
    if (foundIt == locationMap.end()
        || isPropLessSpecificPred(foundIt->second, propdef.second.mSourceLoc)) {
      dest[QString::fromStdString(propdef.first)] = propdef.second;
      locationMap[propdef.first] = propdef.second.mSourceLoc;
    }
  }
}

PropertyMap mergeMatchResults(const MatchResult& result)
{
  PropertyMap props;
  SourceLocationMap locationMap;
  Specificity lastSpec;

  for (const auto& tup : result) {
    BOOST_ASSERT(lastSpec < getMatchSpecificity(tup)
                 || lastSpec == getMatchSpecificity(tup));

    mergePropertiesIntoPropertyMap(
      props, getMatchProperties(tup), locationMap,
      [&lastSpec, &tup](const SourceLocation& one, const SourceLocation& two) {
        return lastSpec != getMatchSpecificity(tup) || one < two;
      });
    lastSpec = getMatchSpecificity(tup);
  }

  return props;
}

std::ostream& operator<<(std::ostream& os, const SourceLocation& srcloc)
{
  std::string sourceLayerName =
    srcloc.mSourceLayer == 0 ? "default stylesheet" : "user stylesheet";
  os << sourceLayerName << " at line " << srcloc.mLine << " column " << srcloc.mColumn;
  return os;
}

std::ostream& operator<<(std::ostream& os, const PropertyValues& values)
{
  class StreamVisitor : public boost::static_visitor<>
  {
    std::ostream& mStream;

  public:
    StreamVisitor(std::ostream& stream)
      : mStream(stream)
    {
    }

    void operator()(const std::string& value)
    {
      mStream << value;
    }

    void operator()(const Expression& expr)
    {
      mStream << expr.name << "(";
      for (const auto& arg : expr.args) {
        mStream << arg << ", ";
      }
      mStream << ")";
    }
  };

  StreamVisitor visitor(os);
  for (const auto& value : values) {
    boost::apply_visitor(visitor, value);
    os << ", ";
  }

  return os;
}

void dumpPropertyDefMap(const PropertyDefMap& properties,
                        std::ostream& stream = std::cout)
{
  stream << "{" << std::endl;
  for (const auto& it : properties) {
    stream << "  " << it.first << ": " << it.second.mValues << " //"
           << it.second.mSourceLoc << std::endl;
  }
  stream << "}" << std::endl;
}

void dumpMatchResults(const MatchResult& result, std::ostream& stream = std::cout)
{
  for (const auto& tup : result) {
    stream << "// specificity: " << getMatchSpecificity(tup) << std::endl;
    dumpPropertyDefMap(getMatchProperties(tup), stream);
  }
}

} // anon namespace

std::string describeMatchedPath(const IStyleMatchTree* itree, const UiItemPath& path)
{
  if (itree) {
    const StyleMatchTree& tree = *static_cast<const StyleMatchTree*>(itree);

    MatchResult result = findMatchingRules(tree, path);
    sortMatchResults(result);
    std::reverse(result.begin(), result.end());

    std::ostringstream stream;
    stream << "Style info for path " << path << std::endl;
    dumpMatchResults(result, stream);

    return stream.str();
  }

  return {};
}

PropertyMap matchPath(const IStyleMatchTree* itree, const UiItemPath& path)
{
  if (itree) {
    const StyleMatchTree& tree = *static_cast<const StyleMatchTree*>(itree);

    MatchResult result = findMatchingRules(tree, path);
    sortMatchResults(result);
    return mergeMatchResults(result);
  }

  return PropertyMap{};
}

std::ostream& operator<<(std::ostream& os, const UiItemPath& path)
{
  return os << pathToString(path);
}

std::string pathToString(const UiItemPath& path)
{
  std::ostringstream ss;
  bool isFirst = true;
  for (auto p : path) {
    if (!isFirst) {
      ss << "/";
    } else {
      isFirst = false;
    }

    ss << p.mTypeName;

    if (!p.mClassNames.empty()) {
      ss << ".";
    }

    if (p.mClassNames.size() > 1) {
      ss << "{";
    }

    bool isFirstClass = true;
    for (const auto& cn : p.mClassNames) {
      if (!isFirstClass) {
        ss << ",";
      } else {
        isFirstClass = false;
      }
      ss << cn;
    }

    if (p.mClassNames.size() > 1) {
      ss << "}";
    }
  }

  return ss.str();
}

std::size_t hash_value(const PathElement& pathElement)
{
  std::size_t seed = boost::hash<std::string>{}(pathElement.mTypeName);
  boost::hash_combine(
    seed, boost::hash<std::vector<std::string>>{}(pathElement.mClassNames));
  return seed;
}

std::size_t UiItemPathHasher::operator()(const UiItemPath& path) const
{
  return boost::hash<UiItemPath>{}(path);
}

} // namespace stylesheets
} // namespace aqt
