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

#include "estd/memory.hpp"
#include "Log.hpp"
#include "StyleEngine.hpp"
#include "Warnings.hpp"

SUPPRESS_WARNINGS
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
  StyleSet* pStyleSet =
    qobject_cast<StyleSet*>(qmlAttachedPropertiesObject<StyleSet>(pObj, false));

  std::vector<std::string> classNames;

  if (pStyleSet) {
    for (auto className : pStyleSet->name().split(" ")) {
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

} // anon namespace

StyleSet::StyleSet(QObject* pParent)
  : QObject(pParent)
  , mpEngine(StyleEngineHost::globalStyleEngine())
  , mpStyleSetProps(estd::make_unique<StyleSetProps>())
{
  connect(
    mpStyleSetProps.get(), &StyleSetProps::propsChanged, this, &StyleSet::propsChanged);

  QObject* p = parent();
  if (p) {
    QQuickItem* pItem = qobject_cast<QQuickItem*>(p);
    if (pItem != nullptr) {
      connect(pItem, &QQuickItem::parentChanged, this, &StyleSet::onParentChanged);
    } else if (p->parent() != nullptr) {
      styleSheetsLogInfo() << "Parent to StyleSet is not a QQuickItem but '"
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
            this, &StyleSet::onStyleEngineChanged);

    setupStyle();
  }
}

StyleSet* StyleSet::qmlAttachedProperties(QObject* pObject)
{
  return new StyleSet(pObject);
}

void StyleSet::onStyleEngineChanged(StyleEngine* pEngine)
{
  setEngine(pEngine);
}

void StyleSet::setEngine(StyleEngine* pEngine)
{
  mpEngine = pEngine;
  setupStyle();
}

void StyleSet::setupStyle()
{
  mpStyleSetProps->initStyleSet(mPath, mpEngine);
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

    QObject* p = parent();
    if (p) {
      mPath = traversePathUp(p);
      setupStyle();
    }

    Q_EMIT nameChanged(mName);
    Q_EMIT pathChanged();
  }
}

QString StyleSet::path() const
{
  return QString::fromStdString(pathToString(mPath));
}

QString StyleSet::styleInfo() const
{
  std::string styleInfoStr(mpEngine ? mpEngine->describeMatchedPath(mPath)
                                    : "No style engine installed");
  return QString::fromStdString(styleInfoStr);
}

StyleSetProps* StyleSet::props()
{
  return mpStyleSetProps.get();
}

void StyleSet::onParentChanged(QQuickItem* pNewParent)
{
  QObject* pParent = parent();
  if (pNewParent != nullptr && pParent != nullptr) {
    mPath = traversePathUp(pParent);
    setupStyle();

    Q_EMIT pathChanged();
  }
}

} // namespace stylesheets
} // namespace aqt
