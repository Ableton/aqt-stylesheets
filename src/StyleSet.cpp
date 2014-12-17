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

#include "StyleSet.hpp"

#include "CssParser.hpp"
#include "Log.hpp"
#include "StyleEngine.hpp"
#include "StyleMatchTree.hpp"
#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtGui/QFont>
#include <QtQuick/QQuickItem>
#include <QtQml/QQmlEngine>
RESTORE_WARNINGS

#include <iostream>
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

/**
If the first token in the list is a font style token,
convert it to a font style and remove it from the list.
*/
QFont::Style takeFontStyleFromTokenList(QStringList& tokens)
{
  static const std::map<QString, QFont::Style> dictionary = {
    {"italic", QFont::StyleItalic},
    {"upright", QFont::StyleNormal},
    {"oblique", QFont::StyleOblique}};

  return (!tokens.isEmpty() && dictionary.count(tokens.at(0)))
           ? dictionary.at(tokens.takeFirst())
           : QFont::StyleNormal;
}

/**
If the first token in the list is a capitalization style token,
convert it to a capitalization style and remove it from the list.
*/
QFont::Capitalization takeCapitalizationStyleFromTokenList(QStringList& tokens)
{
  static const std::map<QString, QFont::Capitalization> dictionary = {
    {"mixedcase", QFont::MixedCase},
    {"alluppercase", QFont::AllUppercase},
    {"alllowercase", QFont::AllLowercase},
    {"smallcaps", QFont::SmallCaps},
    {"capitalize", QFont::Capitalize}};

  return (!tokens.isEmpty() && dictionary.count(tokens.at(0)))
           ? dictionary.at(tokens.takeFirst())
           : QFont::MixedCase;
}

/**
If the first token in the list is a font weight token,
convert it to a font weight and remove it from the list.
*/
QFont::Weight takeFontWeightFromTokenList(QStringList& tokens)
{
  static const std::map<QString, QFont::Weight> dictionary = {
    {"light", QFont::Light},
    {"bold", QFont::Bold},
    {"demibold", QFont::DemiBold},
    {"black", QFont::Black},
    {"regular", QFont::Normal}};

  return (!tokens.isEmpty() && dictionary.count(tokens.at(0)))
           ? dictionary.at(tokens.takeFirst())
           : QFont::Normal;
}

struct FontSize {
  int pixelSize = -1;
  int pointSize = -1;
};

/**
If the first token in the list is a font size token,
convert it to a font size and remove it from the list.
*/
FontSize takeFontSizeFromTokenList(QStringList& tokens)
{
  FontSize fontSize;
  if (!tokens.isEmpty()) {
    const QString sizeStr = tokens.takeFirst();

    if (sizeStr.contains(QRegExp("^\\d+px$"))) {
      fontSize.pixelSize = sizeStr.split(QRegExp("px")).at(0).toInt();
    } else if (sizeStr.contains(QRegExp("^\\d+pt$"))) {
      fontSize.pointSize = sizeStr.split(QRegExp("pt")).at(0).toInt();
    } else {
      tokens.prepend(sizeStr);
    }
  }
  return fontSize;
}

/**
Extract the font style from the string.

Font declarations must conform to a limited subset of the W3 font spec
(http://www.w3.org/TR/css3-fonts/#font-prop), see the following:

@code
// <style> <variant> <weight> <size> <family>
// e.g.:
font: "italic smallcaps bold 16px Times New Roman"
@endcode
*/
QFont fontDeclarationToFont(const QString& fontDecl)
{
  QStringList tokens = fontDecl.split(QRegExp("\\s* \\s*"), QString::SkipEmptyParts);

  const QFont::Style fontStyle = takeFontStyleFromTokenList(tokens);
  const QFont::Capitalization capMode = takeCapitalizationStyleFromTokenList(tokens);
  const QFont::Weight weight = takeFontWeightFromTokenList(tokens);
  const FontSize size = takeFontSizeFromTokenList(tokens);
  const QString familyName = tokens.join(' ');

  QFont font(familyName, size.pointSize, weight);
  if (size.pixelSize > 0) {
    font.setPixelSize(size.pixelSize);
  }
  font.setCapitalization(capMode);
  font.setStyle(fontStyle);
  return font;
}

struct FontPropertyConvertTraits {
  const char* typeName() const
  {
    return "font";
  }

  bool convert(QFont& result, QVariant& value) const
  {
    if (value.canConvert(QMetaType::QString)) {
      result = fontDeclarationToFont(value.toString());
      return true;
    }
    return false;
  }
};
}

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
      disconnect(mpEngine, SIGNAL(styleChanged(int)), this, SLOT(onStyleChanged(int)));
      mChangeCount = 0;
    }

    mpEngine = pEngine;
    mPath = path;

    if (mpEngine && isDiffEngine) {
      connect(mpEngine, SIGNAL(styleChanged(int)), this, SLOT(onStyleChanged(int)));
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

bool StyleSet::getImpl(QVariant& value, const QString& key) const
{
  PropertyMap::const_iterator it = mProperties.find(key);
  if (it != mProperties.end()) {
    value = it->second;
    return true;
  }

  if (!mpEngine.isNull() && mChangeCount == mpEngine->changeCount()) {
    styleSheetsLogWarning() << "Property " << key.toStdString() << " not found ("
                            << pathToString(mPath) << ")";
  }

  return false;
}

QVariant StyleSet::get(const QString& key) const
{
  QVariant value;
  getImpl(value, key);
  return value;
}

QColor StyleSet::color(const QString& key) const
{
  return lookupProperty<QColor>(key);
}

QFont StyleSet::font(const QString& key) const
{
  return lookupProperty<QFont>(key, FontPropertyConvertTraits());
}

double StyleSet::number(const QString& key) const
{
  return lookupProperty<double>(key);
}

bool StyleSet::boolean(const QString& key) const
{
  return lookupProperty<bool>(key);
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
      connect(pItem, SIGNAL(parentChanged(QQuickItem*)), this,
              SLOT(onParentChanged(QQuickItem*)));
    } else {
      styleSheetsLogInfo() << "Parent to StyleSetAttached is not a QQuickItem but '"
                           << p->metaObject()->className() << "'. "
                           << "Hierarchy changes for this component won't be detected.";
    }

    mPath = traversePathUp(p);

    connect(StyleEngineHost::globalStyleEngineHost(),
            SIGNAL(styleEngineLoaded(aqt::stylesheets::StyleEngine*)), this,
            SLOT(onStyleEngineChanged(aqt::stylesheets::StyleEngine*)));

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
  }
}

} // namespace stylesheets
} // namespace aqt
