/*
Copyright (c) 2014-2015 Ableton AG, Berlin

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

#include "StyleEngine.hpp"

#include "estd/memory.hpp"
#include "CssParser.hpp"
#include "Log.hpp"
#include "StyleMatchTree.hpp"
#include "UrlUtils.hpp"
#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtCore/QFile>
#include <QtCore/QUrl>
#include <QtGui/QFontDatabase>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlFile>
RESTORE_WARNINGS

#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <tuple>

namespace aqt
{
namespace stylesheets
{

namespace
{

using FontIdCache = std::map<QString, int>;

FontIdCache& fontIdCache()
{
  static FontIdCache sFontIdCache;
  return sFontIdCache;
}

std::unique_ptr<StyleEngine>& instanceImpl()
{
  static std::unique_ptr<StyleEngine> spInstance;
  return spInstance;
}

} // anon namespace

StyleEngine& StyleEngine::instance()
{
  if (!instanceImpl()) {
    instanceImpl().reset(new StyleEngine);
  }

  return *instanceImpl();
}

void StyleEngine::bindToQmlEngine(QQmlEngine& qmlEngine)
{
  mBaseUrl = qmlEngine.baseUrl();
  mImportPaths = qmlEngine.importPathList();

  QObject::connect(
    &qmlEngine, &QObject::destroyed, [](QObject*) { instanceImpl().reset(); });
}

bool StyleEngine::hasStylesLoaded() const
{
  return mHasStylesLoaded;
}

void StyleEngine::unloadStyles()
{
  mHasStylesLoaded = false;

  for (auto& element : mStyleSetPropsRefs) {
    auto pStyleSetProps = element.second.get();
    pStyleSetProps->invalidate();
  }

  mPropertyMaps.clear();
  mPropertyMapInstances.clear();

  mpStyleTree = createMatchTree({});
}

QUrl StyleEngine::styleSheetSource() const
{
  return mStyleSheetSourceUrl;
}

void StyleEngine::setStyleSheetSource(const QUrl& url)
{
  if (mStyleSheetSourceUrl != url) {
    mStyleSheetSourceUrl = url;
  }
}

QUrl StyleEngine::defaultStyleSheetSource() const
{
  return mDefaultStyleSheetSourceUrl;
}

void StyleEngine::setDefaultStyleSheetSource(const QUrl& url)
{
  if (mDefaultStyleSheetSourceUrl != url) {
    mDefaultStyleSheetSourceUrl = url;
  }
}

std::string StyleEngine::describeMatchedPath(const UiItemPath& path) const
{
  return aqt::stylesheets::describeMatchedPath(mpStyleTree.get(), path);
}

void StyleEngine::resolveFontFaceDecl(const StyleSheet& styleSheet)
{
  for (auto ffd : styleSheet.fontfaces) {
    QUrl fontFaceUrl = resolveResourceUrl(
      mStyleSheetSourceUrl.url(), QUrl(QString::fromStdString(ffd.url)));
    QString fontFaceFile = QQmlFile::urlToLocalFileOrQrc(fontFaceUrl);

    if (!fontFaceFile.isEmpty()) {
      styleSheetsLogInfo() << "Load font face " << ffd.url << " from "
                           << fontFaceFile.toStdString();
      std::map<QString, int>::iterator fontCacheIt = fontIdCache().find(fontFaceFile);
      if (fontCacheIt == fontIdCache().end()) {
        int fontId = QFontDatabase::addApplicationFont(fontFaceFile);
        styleSheetsLogDebug() << " [" << fontId << "]";

        if (fontId != -1) {
          QString fontFamily = QFontDatabase::applicationFontFamilies(fontId).at(0);
          styleSheetsLogDebug() << " -> family: " << fontFamily.toStdString();
          fontIdCache()[fontFaceFile] = fontId;
        } else {
          Q_EMIT exception(
            QString::fromLatin1("fontWasNotLoaded"),
            QString::fromLatin1("Could not find font in font registry after loading."));
        }
      } else {
        styleSheetsLogDebug() << " [" << fontCacheIt->second << "]";
      }
    } else {
      styleSheetsLogWarning() << "Could not find font file "
                              << fontFaceUrl.toString().toStdString();
      Q_EMIT exception(QString::fromLatin1("fontWasNotLoaded"),
                       QString::fromLatin1("Font url could not be resolved."));
    }
  }
}

StyleSheet StyleEngine::loadStyleSheet(const QUrl& srcurl)
{
  if (srcurl.isLocalFile() || srcurl.isRelative()) {
    QString styleFilePath = mBaseUrl.resolved(srcurl).toLocalFile();

    if (styleFilePath.isEmpty() || !QFile::exists(styleFilePath)) {
      styleSheetsLogError() << "Style '" << styleFilePath.toStdString() << "' not found";

      Q_EMIT exception(QString::fromLatin1("styleSheetNotFound"),
                       QString::fromLatin1("Style '%1' not found.").arg(styleFilePath));
    } else {
      styleSheetsLogInfo() << "Load style from '" << styleFilePath.toStdString()
                           << "' ...";

      try {
        StyleSheet styleSheet = parseStyleFile(styleFilePath);

        resolveFontFaceDecl(styleSheet);

        return styleSheet;
      } catch (const ParseException& e) {
        styleSheetsLogError() << e.message() << ": " << e.errorContext();

        Q_EMIT exception(QString::fromLatin1("parsingStyleSheetfailed"),
                         QString::fromLatin1("Parsing style sheet failed '%1'.")
                           .arg(QString::fromStdString(e.message())));
      } catch (const std::ios_base::failure& fail) {
        styleSheetsLogError() << "loading style sheet failed: " << fail.what();

        Q_EMIT exception(QString::fromLatin1("loadingStyleSheetFailed"),
                         QString::fromLatin1("Loading style sheet failed '%1'.")
                           .arg(QString::fromStdString(fail.what())));
      }
    }
  }

  return StyleSheet();
}

void StyleEngine::loadStyles()
{
  StyleSheet styleSheet;
  StyleSheet defaultStyleSheet;

  if (!mStyleSheetSourceUrl.isEmpty()) {
    styleSheet = loadStyleSheet(mStyleSheetSourceUrl.url());
  }

  if (!mDefaultStyleSheetSourceUrl.isEmpty()) {
    defaultStyleSheet = loadStyleSheet(mDefaultStyleSheetSourceUrl.url());
  }

  mpStyleTree = createMatchTree(styleSheet, defaultStyleSheet);

  reloadAllProperties();

  mHasStylesLoaded = true;
  notifyMissingProperties();

  Q_EMIT styleChanged();
}

void StyleEngine::reloadAllProperties()
{
  mPropertyMaps.clear();

  auto oldPropertyMapInstances = PropertyMapInstances{};
  oldPropertyMapInstances.swap(mPropertyMapInstances);

  for (auto& element : mStyleSetPropsRefs) {
    auto pStyleSetProps = element.second.get();
    pStyleSetProps->loadProperties();
  }
}

QUrl StyleEngine::resolveResourceUrl(const QUrl& baseUrl, const QUrl& url) const
{
  return searchForResourceSearchPath(baseUrl, url, mImportPaths);
}

StyleSetPropsRef StyleEngine::styleSetProps(const UiItemPath& path)
{
  auto iElement = mStyleSetPropsRefs.find(path);

  if (iElement == mStyleSetPropsRefs.end()) {
    mStyleSetPropsInstances.emplace_back(
      std::make_shared<UsageCountedStyleSetProps>(path));

    auto pStyleSetProps = mStyleSetPropsInstances.back();
    std::tie(iElement, std::ignore) =
      mStyleSetPropsRefs.emplace(path, StyleSetPropsRef{pStyleSetProps});
  }

  return iElement->second;
}

PropertyMap* StyleEngine::properties(const UiItemPath& path)
{
  return effectivePropertyMap(path);
}

PropertyMap* StyleEngine::effectivePropertyMap(const UiItemPath& path)
{
  using std::begin;
  using std::end;
  using std::prev;

  const auto iElement = mPropertyMaps.find(path);
  if (iElement != mPropertyMaps.end()) {
    return iElement->second;
  }

  auto props = matchPath(mpStyleTree.get(), path);

  if (path.size() > 1) {
    auto* pAncestorProps = effectivePropertyMap({begin(path), prev(end(path))});

    if (props.empty()) {
      // point to our ancestor props and return them immediately
      // without storing our own props instance
      mPropertyMaps.emplace(path, pAncestorProps);
      return pAncestorProps;
    } else {
      props.insert(begin(*pAncestorProps), end(*pAncestorProps));
    }
  }

  mPropertyMapInstances.emplace_back(estd::make_unique<PropertyMap>(std::move(props)));

  auto* pProps = mPropertyMapInstances.back().get();
  mPropertyMaps.emplace(path, pProps);

  return pProps;
}

void StyleEngine::setMissingPropertiesFound()
{
  mMissingPropertiesFound = true;
  notifyMissingProperties();
}

void StyleEngine::notifyMissingProperties()
{
  if (mHasStylesLoaded && mMissingPropertiesFound && !mMissingPropertiesNotified) {
    mMissingPropertiesNotified = true;
    Q_EMIT propertiesPotentiallyMissing();
  }
}

void StyleEngine::checkProperties()
{
  for (auto& element : mStyleSetPropsRefs) {
    if (element.second.usageCount() > 1) {
      element.second.get()->checkProperties();
    }
  }

  mMissingPropertiesFound = false;
  mMissingPropertiesNotified = false;
}

} // namespace stylesheets
} // namespace aqt
