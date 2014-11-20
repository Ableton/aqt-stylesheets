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

#include "StyleMatchTree.hpp"
#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtCore/QDir>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QVariantList>
#include <QtQml/QQmlParserStatus>
RESTORE_WARNINGS

namespace aqt
{
namespace stylesheets
{

class StyleEngine;

/*! @cond DOXYGEN_IGNORE */

class StyleEngineHost : public QObject
{
  Q_OBJECT
public:
  static StyleEngineHost* globalStyleEngineHost();

  static StyleEngine* globalStyleEngine();

Q_SIGNALS:
  void styleEngineLoaded(aqt::stylesheets::StyleEngine* pEngine);
};

/*! @endcond */

/*! Interface to the singleton StyleEngine
 *
 * Create and setup the style engine for the application.  There must be a
 * single instance of this class for a complete application only.  Trying
 * to create more than one instance will print a warning leaving the first
 * instance untouched.
 *
 * @par Example
 * @code
 * ApplicationWindow {
 *   StyleEngine {
 *     stylePath: "../Assets/StyleSheets/"
 *     styleName: "default.css"
 *   }
 * }
 * @endcode
 *
 * @par Import in QML:
 * <pre>
 * import Aqt.StyleSheets 1.0
 * </pre>
 * @since 1.0
 */
class StyleEngine : public QObject, public QQmlParserStatus
{
  Q_OBJECT
  Q_DISABLE_COPY(StyleEngine)
  Q_INTERFACES(QQmlParserStatus)

  /*! Contains the URL of folder containing style sheets
   *
   * The URL is resolved relative to the location of the QML file in which the
   * StyleEngine is instantiated.  It must resolve to a local file path.  The
   * StyleEngine is actively watching this folder.  Appearing or disappearing
   * style sheet file will fire availableStylesChanged() signals.
   */
  Q_PROPERTY(QUrl stylePath READ stylePath WRITE setStylePath)

  /*! Contains the file name of the current style sheet
   *
   * The style sheet file is actively watched and, if changing, reloaded.  New
   * or changing properties will fire styleChanged(int) signals.  There's no
   * need though to actively monitor this signal since properties requested
   * via the StyleSet attached type will automatically connect to the style
   * engine.
   *
   * The style sheet file name can be set during app runtime.  This will load
   * the new style sheet and update all StyleSet.props in the app accordingly.
   */
  Q_PROPERTY(QString styleName READ styleName WRITE setStyleName NOTIFY styleNameChanged)

  /*! Contains the file name of the default style sheet
   *
   * @see styleName property
   */
  Q_PROPERTY(QString defaultStyleName READ defaultStyleName WRITE setDefaultStyleName
               NOTIFY defaultStyleNameChanged)

  /*! Defines the list of support file extensions
   *
   * Only files with these extensions will be found as style sheets.  Default
   * is *.css only.
   */
  Q_PROPERTY(QVariantList fileExtensions READ fileExtensions WRITE setFileExtensions
               NOTIFY fileExtensionsChanged)

  /*! Contains the list of all style sheet files found in the stylePath folder
   *
   * Contains all files in the folder given by the stylePath property.
   * Only files ending in the @c *.css extension are listed.
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
  /*! @cond DOXYGEN_IGNORE */
  explicit StyleEngine(QObject* pParent = nullptr);

  int changeCount() const;

  QUrl stylePath() const;
  void setStylePath(const QUrl& path);

  QString styleName() const;
  void setStyleName(const QString& styleName);

  QString defaultStyleName() const;
  void setDefaultStyleName(const QString& styleName);

  QVariantList fileExtensions() const;
  void setFileExtensions(const QVariantList& exts);

  // returns a list of URL with available style files
  QVariantList availableStyles();

  virtual void classBegin();
  virtual void componentComplete();
  /*! @endcond */

  /*! Matches an element path
   *
   * The element path @p path is matched against the rules loaded from the
   * current style sheet.  The resulting set of properties is returned.  If
   * the path is not matching any rule the result is an empty property map.
   */
  PropertyMap matchPath(const UiItemPath& path);

  /*! @private */
  std::string describeMatchedPath(const UiItemPath& path);

Q_SIGNALS:
  /*! Fires when the style sheet is replaced or changed on the disk */
  void styleChanged(int changeCount);
  /*! Fires when a new style sheet file name is set to the styleName property */
  void styleNameChanged();
  /*! Fires when a new default style sheet file name is set to the styleName
   * property */
  void defaultStyleNameChanged();
  /*! Fires when a new set of file extensions are set to the fileExtensions
   * property */
  void fileExtensionsChanged();
  /*! Fires when the list of style sheets in the stylePath folder changes */
  void availableStylesChanged();

private Q_SLOTS:
  void onDirectoryChanged(const QString& path);

private:
  void loadStyle();
  StyleSheet loadStyleSheet(const QString& stylePath, const QString& styleName);
  void resolveFontFaceDecl(const StyleSheet& styleSheet);

private:
  QUrl mStylePathUrl;
  QString mStylePath;
  QString mStyleName;
  QString mDefaultStyleName;
  QVariantList mFileExtensions;
  QStringList mStyleFilters;

  StyleMatchTree mStyleTree;
  QFileSystemWatcher mFsWatcher;
  int mChangeCount;
  std::map<QString, int> mFontIdCache;
};

} // namespace stylesheets
} // namespace aqt
