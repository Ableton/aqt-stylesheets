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

#include "StylesDirWatcher.hpp"

#include "Warnings.hpp"
#include "Log.hpp"

SUPPRESS_WARNINGS
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QUrl>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlFile>
#include <QtQml/qqml.h>
RESTORE_WARNINGS

namespace aqt
{
namespace stylesheets
{

StylesDirWatcher::StylesDirWatcher(QObject* pParent)
  : QObject(pParent)
{
  mStyleFilters.push_back(QLatin1String("*.css"));

  connect(&mFsWatcher, &QFileSystemWatcher::directoryChanged, this,
          &StylesDirWatcher::onDirectoryChanged);
}

QUrl StylesDirWatcher::stylePath() const
{
  return mStylePathUrl;
}

void StylesDirWatcher::setStylePath(const QUrl& url)
{
  if (mStylePathUrl != url) {
    if (!mStylePath.isEmpty()) {
      mFsWatcher.removePath(mStylePath);
    }

    mStylePathUrl = url;

    QQmlEngine* pEngine = qmlEngine(this);
    if (!pEngine && parent()) {
      pEngine = qmlEngine(parent());
    }

    if (pEngine) {
      mStylePath = pEngine->baseUrl().resolved(mStylePathUrl).toLocalFile();

      if (mFsWatcher.addPath(mStylePath)) {
        updateStyleFiles();
      }
    }

    Q_EMIT stylePathChanged(url);
  }
}

QVariantList StylesDirWatcher::fileExtensions() const
{
  return mFileExtensions;
}

void StylesDirWatcher::setFileExtensions(const QVariantList& fileExtensions)
{
  QStringList styleFilters;
  for (auto ext : fileExtensions) {
    styleFilters.push_back(ext.toString());
  }

  if (mStyleFilters != styleFilters) {
    mFileExtensions = fileExtensions;
    mStyleFilters.swap(styleFilters);

    updateStyleFiles();

    Q_EMIT fileExtensionsChanged();
  }
}

void StylesDirWatcher::onDirectoryChanged(const QString&)
{
  updateStyleFiles();
}

QVariantList StylesDirWatcher::availableStyleSheetNames() const
{
  QVariantList result;

  for (auto str : mStyleSheetFiles) {
    result.append(str);
  }

  return result;
}

QVariantList StylesDirWatcher::availableStyles() const
{
  QVariantList result;

  QDir styleDir(mStylePath);
  for (auto str : mStyleSheetFiles) {
    result.append(QUrl::fromLocalFile(styleDir.absoluteFilePath(str)));
  }

  return result;
}

void StylesDirWatcher::updateStyleFiles()
{
  if (!mStylePath.isEmpty() && !mStyleFilters.isEmpty()) {
    QDir styleDir(mStylePath);
    styleDir.setNameFilters(mStyleFilters);

    QStringList styleFiles =
      styleDir.entryList(QDir::NoDotAndDotDot | QDir::Files, QDir::Name);
    if (styleFiles != mStyleSheetFiles) {
      mStyleSheetFiles.swap(styleFiles);
      Q_EMIT availableStylesChanged();
    }
  }
}

} // namespace stylesheets
} // namespace aqt
