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

#pragma once

#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QVariantList>
RESTORE_WARNINGS

#include <set>


namespace aqt
{
namespace stylesheets
{

class StylesDirWatcher : public QObject
{
  Q_OBJECT

  /*! Contains the URL of folder containing style sheets
   *
   * The URL is resolved relative to the location of the QML file in which the
   * StyleEngine is instantiated.  It must resolve to a local file path.  The
   * StyleEngine is actively watching this folder.  Appearing or disappearing
   * style sheet file will fire availableStylesChanged() signals.
   */
  Q_PROPERTY(QUrl stylePath READ stylePath WRITE setStylePath NOTIFY stylePathChanged)

  /*! Defines the list of support file extensions
   *
   * Only files with these extensions will be found as style sheets.  Default
   * is *.css only.
   */
  Q_PROPERTY(QVariantList fileExtensions READ fileExtensions WRITE setFileExtensions
               NOTIFY fileExtensionsChanged)

  /*! Contains the list of all style sheet files found in the stylePath folder
   *
   * Contains all file names in the folder given by the stylePath property.
   * Only files ending in the extensions set with setFileExtensions are listed.
   *
   * Whenever the list of style sheet files in the watch folder (given by the
   * stylePath property) is changed the availableStylesChanged() signal will
   * be fired.
   *
   * The most like usage of this property is to build a style sheet chooser
   * (@ref StyleSheetMenu).
   */
  Q_PROPERTY(QVariantList availableStyles READ availableStyles NOTIFY
               availableStylesChanged)

public:
  StylesDirWatcher(QObject* pParent = nullptr);

  QUrl stylePath() const;
  void setStylePath(const QUrl& path);
  QVariantList fileExtensions() const;
  void setFileExtensions(const QVariantList& fileExtensions);

  /*! Returns the list of available style sheets as a list of QUrls */
  QVariantList availableStyles() const;

  /*! Returns the list of available style sheets as a list of file names */
  Q_INVOKABLE QVariantList availableStyleSheetNames() const;

Q_SIGNALS:
  void availableStylesChanged();
  void fileExtensionsChanged();
  void stylePathChanged(const QUrl& url);

private Q_SLOTS:
  void onDirectoryChanged(const QString& path);

private:
  void updateStyleFiles();

  QUrl mStylePathUrl;
  QString mStylePath;
  QStringList mStyleSheetFiles;
  QVariantList mFileExtensions;
  QStringList mStyleFilters;
  QFileSystemWatcher mFsWatcher;
};

} // namespace stylesheets
} // namespace aqt
