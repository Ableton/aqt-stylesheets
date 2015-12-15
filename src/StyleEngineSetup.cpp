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

#include "StyleEngineSetup.hpp"

#include "StyleEngine.hpp"
#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QUrl>
#include <QtQml/QQmlEngine>
#include <QtQml/qqml.h>
RESTORE_WARNINGS

namespace aqt
{
namespace stylesheets
{

StyleEngineSetup::StyleEngineSetup(QObject* pParent)
  : QObject(pParent)
  , mStylesDir(this)
{
  connect(&mFsWatcher, &QFileSystemWatcher::fileChanged, this,
          &StyleEngineSetup::onFileChanged);

  connect(&mStylesDir, &StylesDirWatcher::availableStylesChanged, this,
          &StyleEngineSetup::availableStylesChanged);
  connect(&mStylesDir, &StylesDirWatcher::fileExtensionsChanged, this,
          &StyleEngineSetup::fileExtensionsChanged);

  auto* pEngine = &StyleEngine::instance();

  connect(pEngine, &StyleEngine::styleChanged, this, &StyleEngineSetup::styleChanged);
  connect(pEngine, &StyleEngine::exception, this, &StyleEngineSetup::exception);
}

StyleEngineSetup::~StyleEngineSetup()
{
  StyleEngine::instance().unloadStyles();
}

QUrl StyleEngineSetup::styleSheetSource() const
{
  return mStyleSheetSourceUrl.url();
}

void StyleEngineSetup::setStyleSheetSource(const QUrl& url)
{
  if (mStyleSheetSourceUrl.url() != url) {
    mStyleSheetSourceUrl.set(url, this, mFsWatcher);

    StyleEngine::instance().setStyleSheetSource(url);
    StyleEngine::instance().loadStyles();

    Q_EMIT styleSheetSourceChanged(url);
  }
}

QUrl StyleEngineSetup::defaultStyleSheetSource() const
{
  return mDefaultStyleSheetSourceUrl.url();
}

void StyleEngineSetup::setDefaultStyleSheetSource(const QUrl& url)
{
  if (mDefaultStyleSheetSourceUrl.url() != url) {
    mDefaultStyleSheetSourceUrl.set(url, this, mFsWatcher);

    StyleEngine::instance().setDefaultStyleSheetSource(url);
    StyleEngine::instance().loadStyles();

    Q_EMIT defaultStyleSheetSourceChanged(url);
  }
}

QUrl StyleEngineSetup::stylePath() const
{
  return mStylePathUrl;
}

void StyleEngineSetup::setStylePath(const QUrl& url)
{
  mStylesDir.setStylePath(url);

  if (mStylePathUrl != url) {
    mStylePathUrl = url;

    mStylePath = qmlEngine(this)->baseUrl().resolved(mStylePathUrl).toLocalFile();

    updateSourceUrls();
  }
}

QString StyleEngineSetup::styleName() const
{
  return mStyleSheetSourceUrl.url().fileName();
}

void StyleEngineSetup::setStyleName(const QString& styleName)
{
  if (mStyleName != styleName) {
    mStyleName = styleName;

    updateSourceUrls();

    Q_EMIT styleNameChanged();
  }
}

QString StyleEngineSetup::defaultStyleName() const
{
  return mDefaultStyleSheetSourceUrl.url().fileName();
}

void StyleEngineSetup::setDefaultStyleName(const QString& styleName)
{
  if (mDefaultStyleName != styleName) {
    mDefaultStyleName = styleName;

    updateSourceUrls();

    Q_EMIT defaultStyleNameChanged();
  }
}

void StyleEngineSetup::updateSourceUrls()
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

QVariantList StyleEngineSetup::fileExtensions() const
{
  return mStylesDir.fileExtensions();
}

void StyleEngineSetup::setFileExtensions(const QVariantList& exts)
{
  mStylesDir.setFileExtensions(exts);
}

QVariantList StyleEngineSetup::availableStyles()
{
  return mStylesDir.availableStyleSheetNames();
}

void StyleEngineSetup::onFileChanged(const QString&)
{
  StyleEngine::instance().loadStyles();
}

void StyleEngineSetup::classBegin()
{
  StyleEngine::instance().bindToQmlEngine(*qmlEngine(parent()));
}

void StyleEngineSetup::componentComplete()
{
}

void StyleEngineSetup::SourceUrl::set(const QUrl& url,
                                      StyleEngineSetup* pParent,
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

} // namespace stylesheets
} // namespace aqt
