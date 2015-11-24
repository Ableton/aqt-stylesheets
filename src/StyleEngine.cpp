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

#include "CssParser.hpp"
#include "Log.hpp"
#include "StyleMatchTree.hpp"
#include "UrlUtils.hpp"
#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtCore/QPointer>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QUrl>
#include <QtGui/QFontDatabase>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlFile>
#include <QtQml/qqml.h>
RESTORE_WARNINGS

#include <iostream>

namespace aqt
{
namespace stylesheets
{

namespace
{

QPointer<StyleEngine>& globalStyleEngineImpl()
{
  static QPointer<StyleEngine> sGlobalStyleEngine;
  return sGlobalStyleEngine;
}

void setGlobalStyleEngine(StyleEngine* pEngine)
{
  if (globalStyleEngineImpl() != pEngine) {
    globalStyleEngineImpl() = pEngine;
    Q_EMIT StyleEngineHost::globalStyleEngineHost()->styleEngineLoaded(
      globalStyleEngineImpl());
  }
}

} // anon namespace

StyleEngine::StyleEngine(QObject* pParent)
  : QObject(pParent)
  , mFontIdCache(StyleEngineHost::globalStyleEngineHost()->fontIdCache())
  , mStylesDir(this)
{
  connect(
    &mFsWatcher, &QFileSystemWatcher::fileChanged, this, &StyleEngine::onFileChanged);

  connect(&mStylesDir, &StylesDirWatcher::availableStylesChanged, this,
          &StyleEngine::availableStylesChanged);
  connect(&mStylesDir, &StylesDirWatcher::fileExtensionsChanged, this,
          &StyleEngine::fileExtensionsChanged);
}

QUrl StyleEngine::styleSheetSource() const
{
  return mStyleSheetSourceUrl.url();
}

void StyleEngine::setStyleSheetSource(const QUrl& url)
{
  if (mStyleSheetSourceUrl.url() != url) {
    mStyleSheetSourceUrl.set(url, this, mFsWatcher);

    loadStyle();

    Q_EMIT styleSheetSourceChanged(url);
  }
}

QUrl StyleEngine::defaultStyleSheetSource() const
{
  return mDefaultStyleSheetSourceUrl.url();
}

void StyleEngine::setDefaultStyleSheetSource(const QUrl& url)
{
  if (mDefaultStyleSheetSourceUrl.url() != url) {
    mDefaultStyleSheetSourceUrl.set(url, this, mFsWatcher);

    loadStyle();

    Q_EMIT defaultStyleSheetSourceChanged(url);
  }
}

QUrl StyleEngine::stylePath() const
{
  return mStylePathUrl;
}

void StyleEngine::setStylePath(const QUrl& url)
{
  mStylesDir.setStylePath(url);

  if (mStylePathUrl != url) {
    mStylePathUrl = url;

    mStylePath = qmlEngine(this)->baseUrl().resolved(mStylePathUrl).toLocalFile();

    updateSourceUrls();
    loadStyle();
  }
}

QString StyleEngine::styleName() const
{
  return mStyleSheetSourceUrl.url().fileName();
}

void StyleEngine::setStyleName(const QString& styleName)
{
  if (mStyleName != styleName) {
    mStyleName = styleName;

    updateSourceUrls();

    Q_EMIT styleNameChanged();
  }
}

QString StyleEngine::defaultStyleName() const
{
  return mDefaultStyleSheetSourceUrl.url().fileName();
}

void StyleEngine::setDefaultStyleName(const QString& styleName)
{
  if (mDefaultStyleName != styleName) {
    mDefaultStyleName = styleName;

    updateSourceUrls();

    Q_EMIT defaultStyleNameChanged();
  }
}

void StyleEngine::updateSourceUrls()
{
  if (!mStylePath.isEmpty()) {
    QDir styleDir(mStylePath);

    if (!mStyleName.isEmpty() && styleDir.exists(mStyleName)) {
      setStyleSheetSource(QUrl::fromLocalFile(styleDir.absoluteFilePath(mStyleName)));
    }

    if (!mDefaultStyleName.isEmpty() && styleDir.exists(mDefaultStyleName)) {
      setDefaultStyleSheetSource(
        QUrl::fromLocalFile(styleDir.absoluteFilePath(mDefaultStyleName)));
    }
  }
}

QVariantList StyleEngine::fileExtensions() const
{
  return mStylesDir.fileExtensions();
}

void StyleEngine::setFileExtensions(const QVariantList& exts)
{
  mStylesDir.setFileExtensions(exts);
}

QVariantList StyleEngine::availableStyles()
{
  return mStylesDir.availableStyleSheetNames();
}

PropertyMap StyleEngine::matchPath(const UiItemPath& path)
{
  return aqt::stylesheets::matchPath(mpStyleTree.get(), path);
}

std::string StyleEngine::describeMatchedPath(const UiItemPath& path)
{
  return aqt::stylesheets::describeMatchedPath(mpStyleTree.get(), path);
}

void StyleEngine::onFileChanged(const QString&)
{
  loadStyle();
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
      std::map<QString, int>::iterator fontCacheIt = mFontIdCache.find(fontFaceFile);
      if (fontCacheIt == mFontIdCache.end()) {
        int fontId = QFontDatabase::addApplicationFont(fontFaceFile);
        styleSheetsLogDebug() << " [" << fontId << "]";

        if (fontId != -1) {
          QString fontFamily = QFontDatabase::applicationFontFamilies(fontId).at(0);
          styleSheetsLogDebug() << " -> family: " << fontFamily.toStdString();
          mFontIdCache[fontFaceFile] = fontId;
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

StyleSheet StyleEngine::loadStyleSheet(const SourceUrl& srcurl)
{
  if (srcurl.url().isLocalFile() || srcurl.url().isRelative()) {
    QString styleFilePath = srcurl.toLocalFile(this);

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

void StyleEngine::loadStyle()
{
  StyleSheet styleSheet;
  StyleSheet defaultStyleSheet;

  if (!mStyleSheetSourceUrl.isEmpty()) {
    styleSheet = loadStyleSheet(mStyleSheetSourceUrl);
  }

  if (!mDefaultStyleSheetSourceUrl.isEmpty()) {
    defaultStyleSheet = loadStyleSheet(mDefaultStyleSheetSourceUrl);
  }

  mpStyleTree = createMatchTree(styleSheet, defaultStyleSheet);

  Q_EMIT styleChanged();
}

void StyleEngine::classBegin()
{
}

void StyleEngine::componentComplete()
{
  if (globalStyleEngineImpl()) {
    styleSheetsLogWarning() << "There's a StyleEngine already";
    return;
  }

  setGlobalStyleEngine(this);
}

StyleEngineHost* StyleEngineHost::globalStyleEngineHost()
{
  static StyleEngineHost gGlobalStyleEngineHost;

  return &gGlobalStyleEngineHost;
}

StyleEngine* StyleEngineHost::globalStyleEngine()
{
  return globalStyleEngineImpl();
}

StyleEngineHost::FontIdCache& StyleEngineHost::fontIdCache()
{
  return mFontIdCache;
}

QUrl StyleEngine::resolveResourceUrl(const QUrl& baseUrl, const QUrl& url) const
{
  return searchForResourceSearchPath(baseUrl, url, qmlEngine(this)->importPathList());
}

//----------------------------------------------------------------------------------------

void StyleEngine::SourceUrl::set(const QUrl& url,
                                 StyleEngine* pParent,
                                 QFileSystemWatcher& watcher)
{
  if (mSourceUrl.isLocalFile()) {
    auto stylePath = qmlEngine(pParent)->baseUrl().resolved(mSourceUrl).toLocalFile();
    if (!stylePath.isEmpty() && QFile(stylePath).exists()) {
      watcher.removePath(stylePath);
    }
  }

  mSourceUrl = url;

  if (mSourceUrl.isLocalFile()) {
    auto stylePath = qmlEngine(pParent)->baseUrl().resolved(mSourceUrl).toLocalFile();
    if (!stylePath.isEmpty() && QFile(stylePath).exists()) {
      watcher.addPath(stylePath);
    }
  }
}

QString StyleEngine::SourceUrl::toLocalFile(StyleEngine* pParent) const
{
  return qmlEngine(pParent)->baseUrl().resolved(mSourceUrl).toLocalFile();
}

} // namespace stylesheets
} // namespace aqt
