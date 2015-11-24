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

#include "Convert.hpp"
#include "Log.hpp"
#include "StyleEngine.hpp"
#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
RESTORE_WARNINGS

#include <string>

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
  StyleSetAttached* pStyleSetAttached =
    qobject_cast<StyleSetAttached*>(qmlAttachedPropertiesObject<StyleSet>(pObj, false));

  std::vector<std::string> classNames;

  if (pStyleSetAttached) {
    for (auto className : pStyleSetAttached->name().split(" ")) {
      classNames.emplace_back(className.toStdString());
    }
  }

  return classNames;
}

template <typename T, typename ObjVisitor>
T traverseParentChain(QObject* pObj, ObjVisitor visitor)
{
  QObject* p = pObj;
  while (p) {
    if (visitor(p)) {
      QObject* nextp = p->parent();
      if (!nextp) {
        if (QQuickItem* pItem = qobject_cast<QQuickItem*>(p)) {
          if (QQuickItem* pParentItem = pItem->parentItem()) {
            nextp = pParentItem;
          }
        }
      }
      p = nextp;
    } else {
      p = nullptr;
    }
  }

  return visitor.result();
}

class CollectPathVisitor
{
public:
  UiItemPath mResult;

  bool operator()(QObject* pObj)
  {
    mResult.insert(mResult.begin(), PathElement(typeName(pObj), styleClassName(pObj)));
    return true;
  }

  UiItemPath result() const
  {
    return mResult;
  }
};

UiItemPath traversePathUp(QObject* pObj)
{
  CollectPathVisitor collectPathVisitor;
  return traverseParentChain<UiItemPath>(pObj, collectPathVisitor);
}

class PropertyMapMergeVisitor
{
public:
  PropertyMap mPropertyMap;
  int mCurrentChangeCount;

  PropertyMapMergeVisitor(int currentChangeCount)
    : mCurrentChangeCount(currentChangeCount)
  {
  }

  bool operator()(QObject* pObj)
  {
    StyleSetAttached* pStyleSetAttached =
      qobject_cast<StyleSetAttached*>(qmlAttachedPropertiesObject<StyleSet>(pObj, false));

    if (pStyleSetAttached) {
      pStyleSetAttached->updateStyle();

      if (StyleSet* pStyle = pStyleSetAttached->props()) {
        mergeInheritableProperties(mPropertyMap, pStyle->properties(mCurrentChangeCount));

        // since our ancestors style should have been compiled already, stop
        // at it
        // return false;
      }
    }

    return true;
  }

  PropertyMap result() const
  {
    return mPropertyMap;
  }
};

PropertyMap effectivePropertyMap(QObject* pObj, int currentChangeCount)
{
  PropertyMapMergeVisitor visitor(currentChangeCount);
  return traverseParentChain<PropertyMap>(pObj, visitor);
}

} // anon namespace

StyleSet::StyleSet(QObject* pParent)
  : QObject(pParent)
  , mChangeCount(0)
{
}

void StyleSet::initStyleSet(const UiItemPath& path, StyleEngine* pEngine)
{
  const bool isDiffEngine = mpEngine != pEngine;

  if (isDiffEngine || mPath != path) {
    if (mpEngine && isDiffEngine) {
      disconnect(mpEngine, &StyleEngine::styleChanged, this, &StyleSet::onStyleChanged);
      mChangeCount = 0;
    }

    mpEngine = pEngine;
    mPath = path;

    if (mpEngine && isDiffEngine) {
      connect(mpEngine, &StyleEngine::styleChanged, this, &StyleSet::onStyleChanged);
    }

    loadProperties(parent());
  }
}

StyleSetAttached* StyleSet::qmlAttachedProperties(QObject* pObject)
{
  return new StyleSetAttached(pObject);
}

QString StyleSet::path() const
{
  return QString::fromStdString(pathToString(mPath));
}

StyleSet* StyleSet::props()
{
  return this;
}

bool StyleSet::isValid() const
{
  return !mProperties.empty();
}

bool StyleSet::isSet(const QString& key) const
{
  return mProperties.find(key) != mProperties.end();
}

bool StyleSet::getImpl(Property& prop, const QString& key) const
{
  PropertyMap::const_iterator it = mProperties.find(key);
  if (it != mProperties.end()) {
    prop = it->second;
    return true;
  }

  if (!mpEngine.isNull() && mChangeCount == mpEngine->changeCount()) {
    styleSheetsLogWarning() << "Property " << key.toStdString() << " not found ("
                            << pathToString(mPath) << ")";
    Q_EMIT mpEngine->exception(QString::fromLatin1("propertyNotFound"),
                               QString::fromLatin1("Property '%1' not found (%2)")
                                 .arg(key, QString::fromStdString(pathToString(mPath))));
  }

  return false;
}

QVariant StyleSet::get(const QString& key) const
{
  Property prop;
  getImpl(prop, key);

  if (prop.mValues.size() == 1) {
    auto conv = convertProperty<QString>(prop.mValues[0]);
    if (conv) {
      return QVariant::fromValue(*conv);
    }
  } else if (prop.mValues.size() > 1) {
    QVariantList result;
    for (const auto& propValue : prop.mValues) {
      auto conv = convertProperty<QString>(propValue);
      if (conv) {
        result.push_back(conv.get());
      }
    }

    return result;
  }

  return QVariant();
}

QVariant StyleSet::values(const QString& key) const
{
  Property prop;
  getImpl(prop, key);

  if (prop.mValues.size() == 1) {
    return convertValueToVariant(prop.mValues[0]);
  }

  return convertValueToVariantList(prop.mValues);
}

QColor StyleSet::color(const QString& key) const
{
  return lookupProperty<QColor>(key);
}

QFont StyleSet::font(const QString& key) const
{
  return lookupProperty<QFont>(key);
}

double StyleSet::number(const QString& key) const
{
  return lookupProperty<double>(key);
}

bool StyleSet::boolean(const QString& key) const
{
  return lookupProperty<bool>(key);
}

QString StyleSet::string(const QString& key) const
{
  return lookupProperty<QString>(key);
}

QUrl StyleSet::url(const QString& key) const
{
  Property prop;
  auto url = lookupProperty<QUrl>(prop, key);

  if (mpEngine) {
    auto baseUrl = prop.mSourceLoc.mSourceLayer == 0 ? mpEngine->defaultStyleSheetSource()
                                                     : mpEngine->styleSheetSource();
    return mpEngine->resolveResourceUrl(baseUrl, url);
  }

  return url;
}

void StyleSet::onStyleChanged(int changeCount)
{
  if (mChangeCount != changeCount) {
    loadProperties(parent());
  }
}

void StyleSet::loadProperties(QObject* pRefObject)
{
  if (mpEngine) {
    mProperties = mpEngine->matchPath(mPath);
    mChangeCount = mpEngine->changeCount();

    if (pRefObject) {
      PropertyMap inheritedProps(
        effectivePropertyMap(pRefObject, mpEngine->changeCount()));
      mergeInheritableProperties(mProperties, inheritedProps);
    }

    QObject* pParent = parent();
    if (qobject_cast<StyleSetAttached*>(pParent) != nullptr) {
      // emit the change notification from the attached property, otherwise
      // the QML world won't see it.
      Q_EMIT qobject_cast<StyleSetAttached*>(parent())->propsChanged();
    }
  }
}

const PropertyMap& StyleSet::properties(int changeCount)
{
  if (changeCount != mChangeCount) {
    loadProperties(grandParent());
  }

  return mProperties;
}

QObject* StyleSet::grandParent()
{
  QObject* pParent = parent();
  if (pParent) {
    if (qobject_cast<StyleSetAttached*>(pParent) != nullptr) {
      pParent = pParent->parent();

      // asking for the attached style of pParent would return the same
      // attached style again.  So when traversing the tree up, skip the
      // current object.
      if (pParent != nullptr) {
        pParent = pParent->parent();
      }
    }
  }

  return pParent;
}

StyleSetAttached::StyleSetAttached(QObject* pParent)
  : QObject(pParent)
  , mpEngine(StyleEngineHost::globalStyleEngine())
  , mStyle(this)
{
  QQmlEngine::setObjectOwnership(&mStyle, QQmlEngine::CppOwnership);

  QObject* p = parent();
  if (p) {
    QQuickItem* pItem = qobject_cast<QQuickItem*>(p);
    if (pItem != nullptr) {
      connect(
        pItem, &QQuickItem::parentChanged, this, &StyleSetAttached::onParentChanged);
    } else if (p->parent() != nullptr) {
      styleSheetsLogInfo() << "Parent to StyleSetAttached is not a QQuickItem but '"
                           << p->metaObject()->className() << "'. "
                           << "Hierarchy changes for this component won't be detected.";

      if (!mpEngine.isNull()) {
        Q_EMIT mpEngine->exception(
          QString::fromLatin1("noParentChangeReports"),
          QString::fromLatin1("Hierarchy changes for this component won't be detected"));
      }
    }

    mPath = traversePathUp(p);

    connect(StyleEngineHost::globalStyleEngineHost(), &StyleEngineHost::styleEngineLoaded,
            this, &StyleSetAttached::onStyleEngineChanged);

    setupStyle();
  }
}

void StyleSetAttached::onStyleEngineChanged(StyleEngine* pEngine)
{
  setEngine(pEngine);
}

void StyleSetAttached::setEngine(StyleEngine* pEngine)
{
  mpEngine = pEngine;
  setupStyle();
}

void StyleSetAttached::updateStyle()
{
  QObject* p = parent();

  StyleEngine* pEngine = StyleEngineHost::globalStyleEngine();

  if (p != nullptr && pEngine != nullptr
      && mStyle.mChangeCount < pEngine->changeCount()) {
    setEngine(pEngine);
  }
}

void StyleSetAttached::setupStyle()
{
  mStyle.initStyleSet(mPath, mpEngine);
  Q_EMIT propsChanged();
}

QString StyleSetAttached::name() const
{
  return mName;
}

void StyleSetAttached::setName(const QString& val)
{
  if (mName != val) {
    mName = val;

    QObject* p = parent();
    if (p) {
      mPath = traversePathUp(p);
      setupStyle();
    }

    Q_EMIT nameChanged(mName);
    Q_EMIT pathChanged(QString::fromStdString(pathToString(mPath)));
  }
}

QString StyleSetAttached::path() const
{
  return QString::fromStdString(pathToString(mPath));
}

QString StyleSetAttached::styleInfo() const
{
  std::string styleInfoStr(mpEngine ? mpEngine->describeMatchedPath(mPath)
                                    : "No style engine installed");
  return QString::fromStdString(styleInfoStr);
}

StyleSet* StyleSetAttached::props()
{
  return &mStyle;
}

void StyleSetAttached::onParentChanged(QQuickItem* pNewParent)
{
  QObject* pParent = parent();
  if (pNewParent != nullptr && pParent != nullptr) {
    mPath = traversePathUp(pParent);
    setupStyle();

    Q_EMIT pathChanged(QString::fromStdString(pathToString(mPath)));
  }
}

} // namespace stylesheets
} // namespace aqt
