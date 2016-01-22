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

#include "StyleSet.hpp"

#include "Log.hpp"
#include "StyleEngine.hpp"
#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtQuick/QQuickItem>
RESTORE_WARNINGS

#include <string>
#include <unordered_set>

namespace aqt
{
namespace stylesheets
{

namespace
{

const std::string normalizeTypename(const std::string& tynm)
{
  size_t pos = tynm.find("_QMLTYPE_");
  if (pos != std::string::npos) {
    return tynm.substr(0, pos);
  }

  pos = tynm.find("_QML_");
  if (pos != std::string::npos) {
    return tynm.substr(0, pos);
  }
  return tynm;
}

std::string typeName(QObject* obj)
{
  if (obj) {
    const QMetaObject* meta = obj->metaObject();
    return normalizeTypename(meta->className());
  }
  return "(null)";
}

std::vector<std::string> styleClassName(QObject* pObj)
{
  StyleSet* pStyleSet =
    qobject_cast<StyleSet*>(qmlAttachedPropertiesObject<StyleSet>(pObj, false));

  std::vector<std::string> classNames;

  if (pStyleSet) {
    for (auto className : pStyleSet->name().split(" ", QString::SkipEmptyParts)) {
      classNames.emplace_back(className.toStdString());
    }
  }

  return classNames;
}

QObject* uiPathParent(QObject* pObj)
{
  QObject* pParent = pObj->parent();
  if (!pParent || !pParent->isWindowType()) {
    if (QQuickItem* pItem = qobject_cast<QQuickItem*>(pObj)) {
      if (QQuickItem* pParentItem = pItem->parentItem()) {
        return pParentItem;
      }
    }
  }
  return pParent;
}

class CollectPath
{
public:
  CollectPath(StyleSet* pStyleSet)
    : mpStyleSet(pStyleSet)
  {
    QObject* pParent = pStyleSet->parent();
    Q_ASSERT(pParent);
    traverseParentChain(pParent);
  }

  const UiItemPath& result() const
  {
    return mResult;
  }

private:
  void traverseParentChain(QObject* pObj)
  {
    if (pObj) {
      if (StyleSet* pOtherStyleSet = otherStyleSet(pObj)) {
        mResult = pOtherStyleSet->path();
      } else {
        traverseParentChain(uiPathParent(pObj));
        mResult.emplace_back(typeName(pObj), styleClassName(pObj));
      }
    }
  }

  StyleSet* otherStyleSet(QObject* pObj)
  {
    auto* pStyleSet =
      qobject_cast<StyleSet*>(qmlAttachedPropertiesObject<StyleSet>(pObj, false));
    return pStyleSet != mpStyleSet ? pStyleSet : nullptr;
  }

  StyleSet* mpStyleSet;
  UiItemPath mResult;
};

UiItemPath traversePathUp(StyleSet* pStyleSet)
{
  return CollectPath(pStyleSet).result();
}

std::unordered_set<QObject*> allUniqueChildren(QObject* pParent)
{
  using std::begin;
  using std::end;

  const auto& children = pParent->children();

  auto result = std::unordered_set<QObject*>{begin(children), end(children)};

  if (auto pItem = qobject_cast<QQuickItem*>(pParent)) {
    const auto& childItems = pItem->childItems();
    result.insert(begin(childItems), end(childItems));
  }

  return result;
}

void propagatePathDown(QObject* pRoot)
{
  const auto children = allUniqueChildren(pRoot);

  for (auto pChild : children) {
    if (uiPathParent(pChild) == pRoot) {
      if (auto pStyleSet = qobject_cast<StyleSet*>(
            qmlAttachedPropertiesObject<StyleSet>(pChild, false))) {
        pStyleSet->refreshPath();
      }
      propagatePathDown(pChild);
    }
  }
}

} // anon namespace

StyleSet::StyleSet(QObject* pParent)
  : QObject(pParent)
{
  QObject* p = parent();
  Q_ASSERT(p);

  QQuickItem* pItem = qobject_cast<QQuickItem*>(p);
  if (pItem != nullptr) {
    connect(pItem, &QQuickItem::parentChanged, this, &StyleSet::onParentChanged);
  } else if (p->parent() != nullptr) {
    styleSheetsLogInfo() << "Parent to StyleSet is not a QQuickItem but '"
                         << p->metaObject()->className() << "'. "
                         << "Hierarchy changes for this component won't be detected.";

    Q_EMIT StyleEngine::instance().exception(
      QString::fromLatin1("noParentChangeReports"),
      QString::fromLatin1("Hierarchy changes for this component won't be detected"));
  }

  refreshPath();
  propagatePathDown(p);
}

StyleSet* StyleSet::qmlAttachedProperties(QObject* pObject)
{
  return new StyleSet(pObject);
}

void StyleSet::setupStyle()
{
  auto pStyleSetProps = mStyleSetPropsRef.get();

  if (pStyleSetProps) {
    disconnect(
      pStyleSetProps, &StyleSetProps::propsChanged, this, &StyleSet::propsChanged);
  }

  mStyleSetPropsRef = StyleEngine::instance().styleSetProps(mPath);

  pStyleSetProps = mStyleSetPropsRef.get();
  connect(pStyleSetProps, &StyleSetProps::propsChanged, this, &StyleSet::propsChanged);

  Q_EMIT propsChanged();
}

QString StyleSet::name() const
{
  return mName;
}

void StyleSet::setName(const QString& val)
{
  if (mName != val) {
    mName = val;
    refreshPath();
    propagatePathDown(parent());
    Q_EMIT nameChanged(mName);
  }
}

const UiItemPath& StyleSet::path() const
{
  return mPath;
}

QString StyleSet::pathString() const
{
  return QString::fromStdString(pathToString(mPath));
}

void StyleSet::refreshPath()
{
  setPath(traversePathUp(this));
}

void StyleSet::setPath(const UiItemPath& path)
{
  if (mPath != path) {
    mPath = path;
    setupStyle();

    Q_EMIT pathChanged();
  }
}

QString StyleSet::styleInfo() const
{
  return QString::fromStdString(StyleEngine::instance().describeMatchedPath(mPath));
}

StyleSetProps* StyleSet::props()
{
  return mStyleSetPropsRef.get();
}

void StyleSet::onParentChanged(QQuickItem* pNewParent)
{
  if (pNewParent != nullptr) {
    const auto newPath = traversePathUp(this);
    if (mPath != newPath) {
      setPath(newPath);
      propagatePathDown(parent());
    }
  }
}

} // namespace stylesheets
} // namespace aqt
