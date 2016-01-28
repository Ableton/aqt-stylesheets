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
#include "StyleSetProps.hpp"
#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
RESTORE_WARNINGS

#include <memory>
#include <unordered_map>
#include <vector>

class QQmlEngine;

namespace aqt
{
namespace stylesheets
{

/*! The singleton StyleEngine
 *
 * Provides css properties that it loads from style sheet source urls.
 *
 * @see StyleEngineSetup for configuration from QML
 */
class StyleEngine : public QObject
{
  Q_OBJECT
  Q_DISABLE_COPY(StyleEngine)

public:
  /*! @cond DOXYGEN_IGNORE */
  static StyleEngine& instance();

  void bindToQmlEngine(QQmlEngine& qmlEngine);

  QUrl styleSheetSource() const;
  void setStyleSheetSource(const QUrl& url);

  QUrl defaultStyleSheetSource() const;
  void setDefaultStyleSheetSource(const QUrl& url);

  std::string describeMatchedPath(const UiItemPath& path) const;

  /*! @endcond */

  /*! Resolve @p url against @p baseUrl or search for it in a search path.
   *
   * See aqt::stylesheets::searchForResourceSearchPath() for details.  This
   * method takes QQmlEngine::importPathList() as searchPath for url resolution.
   */
  QUrl resolveResourceUrl(const QUrl& baseUrl, const QUrl& url) const;

  /*! Returns a StyleSetPropsRef to StyleSetProps corresponding to @p path
   *
   * Subsequent calls with identical @p path will return StyleSetPropsRefs with pointers
   * to the same StyleSetProps instance.
   *
   * StyleSetPropsRef.get() will never return nullptr, but pointers will be invalidated if
   * and only if this StyleEngine instance is destroyed.
   */
  StyleSetPropsRef styleSetProps(const UiItemPath& path);

  /*! Returns a pointer to the PropertyMap corresponding to @p path
   *
   * The element path @p path is matched against the rules loaded from the
   * current style sheet.  The resulting set of properties is returned.  If
   * the path is not matching any rule the result is an empty property map.
   *
   * Subsequent calls with identical @p path will return pointers to the same
   * PropertyMap instance.
   *
   * Will never return nullptr, but pointers will be invalidated if and only
   * if the style changes or this StyleEngine instance is destroyed.
   */
  PropertyMap* properties(const UiItemPath& path);

  /*! Loads the styles from the previously set style sheet sources
   *
   * It is safe to call if the sources have not been set yet or have been only partly set.
   */
  void loadStyles();

  bool hasStylesLoaded() const;
  void unloadStyles();

  void setMissingPropertiesFound();

  void checkProperties();

Q_SIGNALS:
  /*! Fires when the style sheet is replaced or changed on the disk */
  void styleChanged();

  /*! Emitted when any part of the style sheet subsystem has to report some
   *  exceptional situation
   *
   * @param type Indicates a type of exception, like "font-not-found", etc.
   * @param message Some descriptive message
   *
   * @since 1.1
   */
  Q_REVISION(1) void exception(const QString& type, const QString& message);

  void propertiesPotentiallyMissing();

private:
  StyleEngine() = default;

  StyleSheet loadStyleSheet(const QUrl& srcurl);
  void resolveFontFaceDecl(const StyleSheet& styleSheet);
  void reloadAllProperties();

  PropertyMap* effectivePropertyMap(const UiItemPath& path);

  void notifyMissingProperties();

private:
  using StyleSetPropsInstances = std::vector<std::shared_ptr<UsageCountedStyleSetProps>>;
  using StyleSetPropsRefs =
    std::unordered_map<UiItemPath, StyleSetPropsRef, UiItemPathHasher>;

  using PropertyMapInstances = std::vector<std::unique_ptr<PropertyMap>>;
  using PropertyMaps = std::unordered_map<UiItemPath, PropertyMap*, UiItemPathHasher>;

  QUrl mStyleSheetSourceUrl;
  QUrl mDefaultStyleSheetSourceUrl;

  QUrl mBaseUrl;
  QStringList mImportPaths;

  std::unique_ptr<IStyleMatchTree> mpStyleTree;

  StyleSetPropsInstances mStyleSetPropsInstances;
  StyleSetPropsRefs mStyleSetPropsRefs;

  PropertyMapInstances mPropertyMapInstances;
  PropertyMaps mPropertyMaps;

  bool mHasStylesLoaded = false;
  bool mMissingPropertiesFound = false;
  bool mMissingPropertiesNotified = false;
};

} // namespace stylesheets
} // namespace aqt
