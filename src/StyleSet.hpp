/*
Copyright (c) 2014-15 Ableton AG, Berlin

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

#include "Property.hpp"
#include "StyleMatchTree.hpp"
#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtQml/qqml.h>
RESTORE_WARNINGS

class QQuickItem;

namespace aqt
{
namespace stylesheets
{

class StyleEngine;
class StyleSetAttached;

/*! An attached type used for accessing CSS like style settings
 *
 * StyleSet is the type to access the style properties and information for a
 * QML item.  When a StyleSet is instantiated and attached to
 * a QML item it
 *
 *   - determines the path of the attached-to-item up to its very root (the root
 *     of the QQuickItem object hierarchy).
 *   - connects itself to the singleton styleEngine or its styleChanged signal resp.
 *   - determines its style properties by matching the attached-to item's element
 *     path against the selector rules loaded from the style sheet.
 *
 * A style set can be queried for properties using the props property's
 * various functions, like color(), font(), etc.
 *
 * @par Example:
 * @code
 * Rectangle {
 *   StyleSet.name: "gridline"
 *   color: StyleSet.props.color("background-color")
 * }
 * @endcode
 *
 * @note Style properties should always be accessed through the props
 * property only.  Only by doing this it is possible to get notifications
 * about style changes.  (The reason is that QML does not create bindings
 * for attached types directly).
 *
 * @par The Element Path (UiItemPath)
 *
 * The element path is constructed from the typenames and attached "style
 * class" names of the QML and QQuickItem object tree.  In the following
 * example the path for each item is added as a comment:
 *
 * @code
 * Rectangle {               // QQuickRectangle
 *   Text {                  // QQuickRectangle QQuickText
 *   }
 *   Item {                  // QQuickRectangle QQuickItem
 *     ListView {            // QQuickRectangle QQuickItem QQuickListView
 *       delegate: MyView {  // QQuickRectangle QQuickItem QQuickListView MyView
 *       }
 *     }
 *   }
 * }
 * @endcode
 *
 * MyView is a user type defined in the module's qmldir file as MyView.
 * Built-in types like Rectangle or Text begin with @c QQuick.  For
 * debugging purposes the path of an item can be printed to stdout with the
 * StyleSet::path property.
 *
 * @note Matching, determining element path and properties may be delayed in case
 * the global style engine has not been initialized yet (e.g. because it is
 * created lazily from a loader).  In this case using style property lookup
 * functions like get(const QString&) const or
 * color(const QString&, const QColor&) const may not
 * return expected values until the StyleEngine is actually initialized.
 *
 * @par Import in QML:
 * ```import Aqt.StyleSheets 1.0```
 * @since 1.0
 */
class StyleSet : public QObject
{
  Q_OBJECT

  /*! @public Contains the path for the element this StyleSet is attached to.
   *
   * The path will look similar to this:
   *
   * ```QQuickRectangle.root/QQuickRowLayout/QQuickItem.box/QQuickText```
   *
   * and reads from root to leaf, i.e. the left most token is the element at
   * the root of the QML object hierarchy.  The path is mostly useful for
   * debugging purposes.
   */
  Q_PROPERTY(QString path READ path NOTIFY pathChanged)

  /*! @public Contains the style properties for the element this StyleSet is
   * attached to
   *
   * @par Example:
   * @code
   * Text {
   *     color: StyleSet.props.color("color")
   * }
   * @endcode
   *
   * @note Due to the nature of attached properties in QML only if you
   * access the style properties by going through the props() method you
   * get a property binding, which will notify in case of style sheet
   * changes.  If you access the property settings with color() or font()
   * directly, the QML code won't listen to style changes.
   */
  Q_PROPERTY(aqt::stylesheets::StyleSet* props READ props NOTIFY propsChanged)

// Fake this additional property definition.  The property is actually
// defined in StyleSetAttached, which should not appear in the documentation
// at all.  doxygen's @property tag somehow ends up in the function section,
// which is not very helpful.
#ifdef DOXYGEN_GENERATED
  /*! @public Contains the style class name
   *
   * The style class name is part of the element path used for matching selector
   * rules.  This name forms the "dot" part of an elements selector name.
   *
   * @par Example:
   * @code
   * Item {
   *   StyleSet.name: "root"
   *   Component.onCompleted: console.log(StyleSet.path);
   * }
   *
   * => QQuickItem.root
   * @endcode
   *
   * It is possible to define multiple class names for the same item by
   * separating them with whitespace.
   *
   * @par Example:
   * @code
   * Item {
   *   StyleSet.name: "example colors"
   *   Component.onCompleted: console.log(StyleSet.path);
   * }
   *
   * => QQuickItem.{example,colors}
   * @endcode
   *
   * The properties for all matching selectors are merged.
   */
  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
#endif

public:
  /*! @cond DOXYGEN_IGNORE */
  StyleSet(QObject* pParent = nullptr);

  void initStyleSet(const UiItemPath& path, StyleEngine* pEngine);

  QString path() const;
  aqt::stylesheets::StyleSet* props();
  /*! @endcond */

  /*! Indicates whether this style set has any properties set */
  Q_INVOKABLE bool isValid() const;

  /*! Indicates whether a style property @p key is defined */
  Q_INVOKABLE bool isSet(const QString& key) const;

  /*! Returns the style property named @p key
   *
   * Looks up the style property named @p key and returns it as is.  The output
   * is either a @c string (@c QString) or a list of strings (@c QVariantList).
   *
   * @note If there's no such property returns an invalid @c QVariant which
   * maps to @c undefined into Javascript/QML.
   *
   * @par Example:
   * @code
   * Rectangle {
   *   color: StyleSet.props.get("clip-colors")[clipColorId]
   * }
   * @endcode
   */
  Q_INVOKABLE QVariant get(const QString& key) const;

  /*! Returns the style property named @p key
   *
   * Looks up the style property named @p key, evaluates expressions and returns
   * the result.  The output is either a @c string (@c QString), the resulting
   * type of a CSS value expression (e.g. `rgb()`, `url()`, etc.) or a list of
   * these types (@c QVariantList).
   *
   * @note If there's no such property this method will return an default
   *       constructed @c QVariant which maps to `undefined` in Javascript/QML.
   *
   * @par Example:
   * @code
   * Rectangle {
   *   color: StyleSet.props.values("clip-colors")[clipColorId]
   * }
   * @endcode
   *
   * @since 1.2
   */
  Q_REVISION(2) Q_INVOKABLE QVariant values(const QString& key) const;

  /*! Returns the style property @p key as a @c QColor.
   *
   * Looks up the style property named @p key and interprets its value as a
   * color specification.  All common color specs QML supports can be used here:
   *
   * - by name, e.g. @c red, @c black, @c yellow
   * - by RGB string, e.g. @c \#3f4f5f
   * - by ARGB string, e.g. @c \#a0ff0022.
   * - then special @c transparent color
   * - CSS color expressions like `rgb(120, 64, 230)` or `hsla(340, 20%,
   *   34%, 0.5)`.
   *
   * Style property names can be any string, but typically end with @c
   * "-color".
   *
   * If there's no such property @p key, the property has multiple values, or
   * the property's value can not be converted to a `QColor` prints a warning
   * and returns a default `QColor`.
   *
   * @par Example:
   * @code
   * Rectangle {
   *   color: StyleSet.props.color("background-color")
   * }
   * @endcode
   *
   * The following CSS color expressions are supported:
   *
   * - `rgb(r, g, b)` where `r` (red), `g` (green), and `b` (blue) can be
   *   integers from `0` to `255` or percentage values (e.g. `97%`).
   * - `rgba(r, g, b, a)`, like `rgb()`, `a` (alpha) is a float from `0.0` to
   *   `1.0`.
   * - `hsl(h, s, l)` where `h` (hue) is an integer value from `0` to `359` and
   *   `s` (saturation) and `l` (lightness) are percentage values (e.g. `45%`).
   * - `hsla(h, s, l, a)` like `hsl()`, `a` (alpha) is a float from `0.0` to
       `1.0`.
   * - `hsb(h, s, b)`, like `hsl()`, but instead of lightness takes `b`
   *   (brightness) as percentage.
   * - `hsba(h, s, b, a)`, like `hsla()`, but instead of lightness takes `b`
   *   (brightness) as percentage.
   */
  Q_INVOKABLE QColor color(const QString& key) const;

  /*! Returns the style property @p key as a boolean value
   *
   * Looks up the style property named @p key and interprets its value as a
   * boolean.  If there's no such property @p key, its value can not be
   * converted to a boolean, or the property has multiple values prints a
   * warning and returns false.
   *
   * Only the following conversions are supported:
   *
   * - "true", "yes" -> true
   * - "false", "no" -> false
   *
   * @par Example:
   * @code
   * Rectangle {
   *   visible: StyleSet.props.boolean("visible")
   * }
   * @endcode
   */
  Q_INVOKABLE bool boolean(const QString& key) const;

  /*! Returns the style property @p key as a number
   *
   * Looks up the style property named @p key and interprets its value as a
   * number (a double).  If there's no such property @p key, its value can not
   * be converted to a double, or the property has multiple values prints a
   * warning and return 0.0.
   *
   * @par Example:
   * @code
   * Rectangle {
   *   width: StyleSet.props.number("width")
   * }
   * @endcode
   */
  Q_INVOKABLE double number(const QString& key) const;

  /*! Returns the style property @p key as a @c QFont
   *
   * Looks up the style property named @p key and interprets its value as a CSS
   * like font specification.  The font string has the format (with the terms in
   * `[]` being optional; the order is important though):
   *
   * <pre>
   * [style] [capMode] [weight] [hinting] [size] familyName
   * </pre>
   *
   * Where the following values are defined:
   *
   * - __style__: `italic`, `upright`, `oblique`
   * - __weight__: `light`, `regular`, `demibold`, `bold`
   * - __capMode__: `mixedcase`, `alluppercase`, `alllowercase`, `smallcaps`, `capitalize`
   * - __hinting__: `defaulthinting`, `nohinting`, `verticalhinting`, `fullhinting`
   *
   * The __size__ can either be given in pixels or points (e.g. @c 12px or @c
   * 11pt).  The __familyName__ is any valid family name currently installed on
   * the system, loaded via a @c FontLoader or via the @c font-face declaration
   * from a style sheet.
   *
   * @par Example for font specs:
   * @code
   *   italic 14.5pt Calibre
   *   oblique smallcaps bold nohinting 24px Arial
   * @endcode
   *
   * If there's no such property @p key, its value can not be converted to a
   * font spec, or the property has multiple values prints a warning and returns
   * a default constructed QFont.
   *
   * @par Example:
   * @code
   * Text {
   *   font: StyleSet.props.font("font")
   * }
   * @endcode
   */
  Q_INVOKABLE QFont font(const QString& key) const;

  /*! Returns the style property @p key as a string
   *
   * Looks up the style property named @p key and interprets its value as a
   * string.  If there's no such property @p key, first value can not be
   * converted to a string (esp. a CSS expression like `rgb()` can not be
   * converted), or the property has multiple values prints a warning and
   * returns an empty string.
   *
   * @par Example:
   * @code
   * Text {
   *   text: StyleSet.props.string("title")
   * }
   * @endcode
   *
   * @note The main difference to the generic get() function is that it will
   *       always return a single value only, where get() might return a list of
   *       strings.  This accessor should be used therefore when a single string
   *       is expected.
   *
   * @since 1.2
   */
  Q_REVISION(2) Q_INVOKABLE QString string(const QString& key) const;

  /*! Returns the style property @p key as a URL/URI
   *
   * Looks up the style property named @p key and interprets its value as a URL.
   * If the URL is a relative url or path it is resolved with the corresponding
   * stylesheet source the property was loaded from as base URL.
   *
   * This function understands the `url()` CSS notation.
   *
   * If there's no such property @p key, the first value can not be converted to
   * a URL, or the property has multiple values prints a warning and returns an
   * invalid URL.
   *
   * @par Example:
   * @code
   * Image {
   *   source: StyleSet.props.url("icon")
   * }
   * @endcode
   *
   * @since 1.2
   */
  Q_REVISION(2) Q_INVOKABLE QUrl url(const QString& key) const;

  /*! @cond DOXYGEN_IGNORE */
  void loadProperties(QObject* pRefObject);

  static StyleSetAttached* qmlAttachedProperties(QObject* pObject);

  const PropertyMap& properties(int changeCount);
/*! @endcond */

Q_SIGNALS:
  /*! Fires when properties change
   *
   * When ever the StyleEngine reloads its style sheet and property values are
   * changed or new properties appear this signal will be fired.
   */
  void propsChanged();

  /*! Fires when the path of the item changes
   *
   * Whenever the location of the object his StyleSet is attached to changes
   * the StyleSet will fire this signal.
   */
  void pathChanged(const QString& path);

public Q_SLOTS:
  /*! @cond DOXYGEN_IGNORE */
  void onStyleChanged(int changeCount);

private:
  QObject* grandParent();
  bool getImpl(Property& def, const QString& key) const;

  template <typename T>
  T lookupProperty(const QString& key) const;
  template <typename T>
  T lookupProperty(Property& def, const QString& key) const;

private:
  friend class StyleSetAttached;

  QPointer<StyleEngine> mpEngine;
  UiItemPath mPath;
  PropertyMap mProperties;
  int mChangeCount;
  /*! @endcond */
};

/*! @cond DOXYGEN_IGNORE */

class StyleSetAttached : public QObject
{
  Q_OBJECT

  Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
  Q_PROPERTY(QString path READ path NOTIFY pathChanged)
  Q_PROPERTY(aqt::stylesheets::StyleSet* props READ props NOTIFY propsChanged)
  Q_PROPERTY(QString styleInfo READ styleInfo NOTIFY propsChanged)

public:
  explicit StyleSetAttached(QObject* pParent = nullptr);

  QString name() const;
  void setName(const QString& val);

  QString path() const;
  aqt::stylesheets::StyleSet* props();

  QString styleInfo() const;

  void updateStyle();

Q_SIGNALS:
  void propsChanged();
  void nameChanged(const QString& name);
  void pathChanged(const QString& path);

private Q_SLOTS:
  void onStyleEngineChanged(aqt::stylesheets::StyleEngine* pEngine);
  void onParentChanged(QQuickItem* pNewParent);

private:
  void setEngine(StyleEngine* pEngine);
  void setupStyle();

private:
  QPointer<StyleEngine> mpEngine;
  StyleSet mStyle;
  QString mName;
  UiItemPath mPath;
};

/*! @endcond */

} // namespace stylesheets
} // namespace aqt

QML_DECLARE_TYPEINFO(aqt::stylesheets::StyleSet, QML_HAS_ATTACHED_PROPERTIES)

// include the header implementations
#include "StyleSet.ipp"
