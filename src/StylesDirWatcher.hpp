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

/*! The StylesDirWatcher class provides an interface to list style sheets in a
 *  folder.
 *
 * A watcher can be used to monitor a specific folder and signal any new or
 * removed file matching a set of file extensions.  It is mostly useful for
 * building choosers or menus implementing a style sheet selector (see
 * e.g. @ref StyleSheetMenu).
 *
 * @par Example
 * @code
 * StylesDirWatcher {
 *   id: watcher
 *   stylePath: 'src/css'
 *   fileExtensions: [ '*.css', '*.styles' ]
 *   onAvailableStylesChanged: console.log(watcher.availableStyles)
 * }
 * @endcode
 *
 * @par Import in QML:
 * ```import Aqt.StyleSheets 1.1```
 * @since 1.1
 */
class StylesDirWatcher : public QObject
{
  Q_OBJECT

  /*! @public Contains the URL of folder containing style sheets
   *
   * The URL is resolved relative to the location of the QML file in which the
   * StyleEngine is instantiated.  It must resolve to a local file path.  The
   * StyleEngine is actively watching this folder.  Appearing or disappearing
   * style sheet file will fire availableStylesChanged signals.
   */
  Q_PROPERTY(QUrl stylePath READ stylePath WRITE setStylePath NOTIFY stylePathChanged)

  /*! @public Defines the list of support file extensions
   *
   * Only files with these extensions will be found as style sheets.  Default
   * is *.css only.
   */
  Q_PROPERTY(QVariantList fileExtensions READ fileExtensions WRITE setFileExtensions
               NOTIFY fileExtensionsChanged)

  /*! @public Contains the list of all style sheet files found in the stylePath folder
   *
   * Contains all file names in the folder given by the stylePath property.
   * Only files ending in the extensions set with setFileExtensions are listed.
   *
   * Whenever the list of style sheet files in the watch folder (given by the
   * stylePath property) is changed the availableStylesChanged() signal will
   * be fired.
   */
  Q_PROPERTY(QVariantList availableStyles READ availableStyles NOTIFY
               availableStylesChanged)

public:
  /*! @cond DOXYGEN_IGNORE */
  StylesDirWatcher(QObject* pParent = nullptr);
  /*! @endcond */

  /*! Returns the set folder to watch. */
  QUrl stylePath() const;
  /*! Set the folder to watchas URL. */
  void setStylePath(const QUrl& path);
  /*! Returns the list of file extensions. */
  QVariantList fileExtensions() const;
  /*! Set the list of file extensions as a list of global formated strings:
   *
   * @code
   * QVariantList exts;
   * exts.push_back(QVariant("*.css"));
   * exts.push_back(QVariant("*.style"));
   * watcher.setFileExtensions(exts);
   * @endcode
   */
  void setFileExtensions(const QVariantList& fileExtensions);

  /*! Returns the list of available style sheets as a list of QUrls */
  QVariantList availableStyles() const;

  /*! Returns the list of available style sheets as a list of file names */
  Q_INVOKABLE QVariantList availableStyleSheetNames() const;

Q_SIGNALS:
  /*! This signal is emitted when a new file matching the given file
   * extensions appears or a previously seen files disappeared.  When the root
   * watch path is changed by setting the stylePath property or the list of
   * file extensions changes this signal might also be emitted.
   *
   * @see fileExtensionsChanged, stylePathChanged
   */
  void availableStylesChanged();

  /*! This signal is emitted when a new list of file extensions is set. */
  void fileExtensionsChanged();

  /*! This signal is emitted when a new stylePath is set. */
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
