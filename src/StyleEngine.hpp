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

#include "StyleMatchTree.hpp"
#include "StylesDirWatcher.hpp"
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

#include <memory>

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
  using FontIdCache = std::map<QString, int>;

  static StyleEngineHost* globalStyleEngineHost();

  static StyleEngine* globalStyleEngine();

  FontIdCache& fontIdCache();

Q_SIGNALS:
  void styleEngineLoaded(aqt::stylesheets::StyleEngine* pEngine);

private:
  std::map<QString, int> mFontIdCache;
};

/*! @endcond */

/*! Interface to the singleton StyleEngine
 *
 * Create and setup the style engine for the application.  There must be a
 * single instance of this class for a complete application only.  Trying
 * to create more than one instance will print a warning leaving the first
 * instance untouched.
 *
 * The style engine supports up to two stylesheets specified by the
 * styleSheetSource and defaultStyleSheetSource properties.  Rules from the
 * former take precedence of the those from the later.
 *
 * @par Example
 * @code
 * ApplicationWindow {
 *   StyleEngine {
 *     styleSheetSource: "../Assets/bright.css"
 *     defaultStyleSheetSource: "Resources/default.css"
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

  /*! @public Contains the URL of folder containing style sheets
   *
   * The URL is resolved relative to the location of the QML file in which the
   * StyleEngine is instantiated.  It must resolve to a local file path.  The
   * StyleEngine is actively watching this folder.  Appearing or disappearing
   * style sheet file will fire availableStylesChanged() signals.
   *
   * @deprecated Use styleSheetSource and defaultStyleSheetSource properties
   * instead.  For watching folders of style sheets use StylesDirWatcher.
   */
  Q_PROPERTY(QUrl stylePath READ stylePath WRITE setStylePath)

  /*! @public Contains the file name of the current style sheet
   *
   * The style sheet file is actively watched and, if changing, reloaded.  New
   * or changing properties will fire styleChanged(int) signals.  There's no
   * need though to actively monitor this signal since properties requested
   * via the StyleSet attached type will automatically connect to the style
   * engine.
   *
   * The style sheet file name can be set during app runtime.  This will load
   * the new style sheet and update all StyleSet.props in the app accordingly.
   *
   * @see styleName property
   *
   * @deprecated Use styleSheetSource property instead.
   */
  Q_PROPERTY(QString styleName READ styleName WRITE setStyleName NOTIFY styleNameChanged)

  /*! @public Contains the file name of the default style sheet
   *
   * @see styleName property
   *
   * @deprecated Use defaultStyleSheetSource property instead.
   */
  Q_PROPERTY(QString defaultStyleName READ defaultStyleName WRITE setDefaultStyleName
               NOTIFY defaultStyleNameChanged)

  /*! @public Defines the list of support file extensions
   *
   * Only files with these extensions will be found as style sheets.  Default
   * is *.css only.
   *
   * @deprecated Use StylesDirWatcher instead.
   */
  Q_PROPERTY(QVariantList fileExtensions READ fileExtensions WRITE setFileExtensions
               NOTIFY fileExtensionsChanged)

  /*! @public Contains the list of all style sheet files found in the
   * stylePath folder
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
   *
   * @deprecated Use StylesDirWatcher instead.
   */
  Q_PROPERTY(QVariantList availableStyles READ availableStyles NOTIFY
               availableStylesChanged)

  /*! @public Contains the source url of the current style sheet
   *
   * The style sheet file is actively watched and, if changing, reloaded.  New
   * or changing properties will fire styleChanged(int) signals.  There's no
   * need though to actively monitor this signal since properties requested
   * via the StyleSet attached type will automatically connect to the style
   * engine.
   *
   * The style sheet url can be set during app runtime.  This will load the
   * new style sheet and update all StyleSet.props in the app accordingly.
   *
   * The URL is resolved relative to the location of the QML file in which the
   * StyleEngine is instantiated.
   *
   * @since 1.1
   */
  Q_PROPERTY(QUrl styleSheetSource READ styleSheetSource WRITE setStyleSheetSource NOTIFY
               styleSheetSourceChanged REVISION 1)

  /*! @public Contains the source url of the default style sheet
   *
   * @see styleSheetSource property
   *
   * @since 1.1
   */
  Q_PROPERTY(QUrl defaultStyleSheetSource READ defaultStyleSheetSource WRITE
               setDefaultStyleSheetSource NOTIFY defaultStyleSheetSourceChanged
                 REVISION 1)

public:
  /*! @cond DOXYGEN_IGNORE */
  explicit StyleEngine(QObject* pParent = nullptr);

  QUrl styleSheetSource() const;
  void setStyleSheetSource(const QUrl& url);

  QUrl defaultStyleSheetSource() const;
  void setDefaultStyleSheetSource(const QUrl& url);

  /*! @deprecated Use StylesDirWatcher instead. */
  QUrl stylePath() const;
  /*! @deprecated Use StylesDirWatcher instead. */
  void setStylePath(const QUrl& path);

  /*! @deprecated Use styleSheetSource instead. */
  QString styleName() const;
  /*! @deprecated Use setStyleSheetSource instead. */
  void setStyleName(const QString& styleName);

  /*! @deprecated Use defaultStyleSheetSource instead. */
  QString defaultStyleName() const;
  /*! @deprecated Use setDefaultStyleSheetSource instead. */
  void setDefaultStyleName(const QString& styleName);

  /*! @deprecated Use StylesDirWatcher instead */
  QVariantList fileExtensions() const;
  /*! @deprecated Use StylesDirWatcher instead */
  void setFileExtensions(const QVariantList& exts);

  /*! returns a list of URL with available style files
   *
   * @deprecated Use StylesDirWatcher instead */
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
  std::string describeMatchedPath(const UiItemPath& path) const;

  /*! Resolve @p url against @p baseUrl or search for it in a search path.
   *
   * See aqt::stylesheets::searchForResourceSearchPath() for details.  This
   * method takes QQmlEngine::importPathList() as searchPath for url resolution.
   */
  QUrl resolveResourceUrl(const QUrl& baseUrl, const QUrl& url) const;

Q_SIGNALS:
  /*! Fires when the style sheet is replaced or changed on the disk */
  void styleChanged();
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

  /*! Emitted when the style sheet source URL changes.
   *
   * @since 1.1
   */
  Q_REVISION(1) void styleSheetSourceChanged(const QUrl& url);
  /*! Emitted when the default style sheet source URL changes.
   *
   * @since 1.1
   */
  Q_REVISION(1) void defaultStyleSheetSourceChanged(const QUrl& url);

  /*! Emitted when any part of the style sheet subsystem has to report some
   *  exceptional situation
   *
   * @param type Indicates a type of exception, like "font-not-found", etc.
   * @param message Some descriptive message
   *
   * @since 1.1
   */
  Q_REVISION(1) void exception(const QString& type, const QString& message);

private Q_SLOTS:
  void onFileChanged(const QString& path);

private:
  class SourceUrl
  {
  public:
    void set(const QUrl& url, StyleEngine* pParent, QFileSystemWatcher& watcher);
    QString toLocalFile(StyleEngine* pParent) const;

    QUrl url() const
    {
      return mSourceUrl;
    }

    bool isEmpty() const
    {
      return mSourceUrl.isEmpty();
    }

    QUrl mSourceUrl;
  };

  void loadStyle();
  StyleSheet loadStyleSheet(const SourceUrl& srcurl);
  void resolveFontFaceDecl(const StyleSheet& styleSheet);

  void updateSourceUrls();

private:
  QUrl mStylePathUrl;        //!< @deprecated
  QString mStylePath;        //!< @deprecated
  QString mStyleName;        //!< @deprecated
  QString mDefaultStyleName; //!< @deprecated

  SourceUrl mStyleSheetSourceUrl;
  SourceUrl mDefaultStyleSheetSourceUrl;

  std::unique_ptr<IStyleMatchTree> mpStyleTree;
  QFileSystemWatcher mFsWatcher;
  StyleEngineHost::FontIdCache& mFontIdCache;

  StylesDirWatcher mStylesDir;
};

} // namespace stylesheets
} // namespace aqt
