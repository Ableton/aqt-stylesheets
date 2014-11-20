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

#include "StyleEngine.hpp"

#include "CssParser.hpp"
#include "Log.hpp"
#include "StyleMatchTree.hpp"
#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtCore/QPointer>
#include <QtGui/QFontDatabase>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlFile>
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
  , mStyleName("style.rc")
  , mChangeCount(0)
{
  mStyleFilters.push_back("*.css");

  connect(&mFsWatcher, SIGNAL(directoryChanged(const QString&)), this,
          SLOT(onDirectoryChanged(const QString&)));
}

int StyleEngine::changeCount() const
{
  return mChangeCount;
}

QUrl StyleEngine::stylePath() const
{
  return mStylePathUrl;
}

void StyleEngine::setStylePath(const QUrl& url)
{
  if (mStylePathUrl != url) {
    QString oldPath = mStylePath;

    mStylePathUrl = url;

    QQmlEngine pEngine(this);
    mStylePath = pEngine.baseUrl().resolved(mStylePathUrl).toLocalFile();

    if (oldPath != mStylePath) {
      mFsWatcher.addPath(mStylePath);
    }

    loadStyle();
  }
}

QString StyleEngine::styleName() const
{
  return mStyleName;
}

void StyleEngine::setStyleName(const QString& styleName)
{
  if (mStyleName != styleName) {
    mStyleName = styleName;

    loadStyle();

    Q_EMIT styleNameChanged();
  }
}

QString StyleEngine::defaultStyleName() const
{
  return mDefaultStyleName;
}

void StyleEngine::setDefaultStyleName(const QString& styleName)
{
  if (mDefaultStyleName != styleName) {
    mDefaultStyleName = styleName;

    loadStyle();

    Q_EMIT defaultStyleNameChanged();
  }
}

QVariantList StyleEngine::fileExtensions() const
{
  return mFileExtensions;
}

void StyleEngine::setFileExtensions(const QVariantList& exts)
{
  mFileExtensions = exts;
  mStyleFilters.clear();

  for (auto ext : mFileExtensions) {
    mStyleFilters.push_back(ext.toString());
  }

  Q_EMIT fileExtensionsChanged();
}

QVariantList StyleEngine::availableStyles()
{
  QVariantList result;

  QQmlEngine pEngine(this);
  QDir styleDir(pEngine.baseUrl().resolved(mStylePathUrl).toLocalFile());

  styleDir.setNameFilters(mStyleFilters);

  QStringList styleFiles =
    styleDir.entryList(QDir::NoDotAndDotDot | QDir::Files, QDir::Name);
  for (auto str : styleFiles) {
    result.append(str);
  }

  return result;
}

PropertyMap StyleEngine::matchPath(const UiItemPath& path)
{
  return aqt::stylesheets::matchPath(mStyleTree, path);
}

std::string StyleEngine::describeMatchedPath(const UiItemPath& path)
{
  return aqt::stylesheets::describeMatchedPath(mStyleTree, path);
}

void StyleEngine::onDirectoryChanged(const QString& path)
{
  loadStyle();

  bool found = false;
  for (auto s : mFsWatcher.directories()) {
    if (s == mStylePath) {
      found = true;
      break;
    }
  }

  if (!found) {
    mFsWatcher.addPath(path);
  }

  Q_EMIT availableStylesChanged();
}

void StyleEngine::resolveFontFaceDecl(const StyleSheet& styleSheet)
{
  QQmlEngine pEngine(this);

  for (auto ffd : styleSheet.fontfaces) {

    QUrl styleUrl = pEngine.baseUrl().resolved(mStylePathUrl);
    QUrl fontFaceUrl = styleUrl.resolved(QUrl(QString::fromStdString(ffd.url)));

    QString fontFaceFile = QQmlFile::urlToLocalFileOrQrc(fontFaceUrl);

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
      }
    } else {
      styleSheetsLogDebug() << " [" << fontCacheIt->second << "]";
    }
  }
}

StyleSheet StyleEngine::loadStyleSheet(const QString& stylePath, const QString& styleName)
{
  QDir path(stylePath);
  QString styleFilePath = path.absoluteFilePath(styleName);

  if (!QFile::exists(styleFilePath)) {
    styleSheetsLogError() << "Style '" << styleFilePath.toStdString() << "' not found";
  } else {
    styleSheetsLogInfo() << "Load style from '" << styleFilePath.toStdString() << "' ...";

    try {
      StyleSheet styleSheet = parseStyleFile(styleFilePath);

      resolveFontFaceDecl(styleSheet);

      return styleSheet;
    } catch (const ParseException& e) {
      styleSheetsLogError() << e.message() << ": " << e.errorContext();
    } catch (const std::ios_base::failure& fail) {
      styleSheetsLogError() << "loading style sheet failed: " << fail.what();
    }
  }

  return StyleSheet();
}

void StyleEngine::loadStyle()
{
  StyleSheet styleSheet;
  StyleSheet defaultStyleSheet;

  if (!mStylePath.isEmpty()) {
    if (!mStyleName.isEmpty()) {
      styleSheet = loadStyleSheet(mStylePath, mStyleName);
    }

    if (!mDefaultStyleName.isEmpty()) {
      defaultStyleSheet = loadStyleSheet(mStylePath, mDefaultStyleName);
    }
  }

  mStyleTree = std::move(createMatchTree(styleSheet, defaultStyleSheet));

  mChangeCount++;
  Q_EMIT styleChanged(mChangeCount);
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

} // namespace stylesheets
} // namespace aqt
