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
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtQml/qqml.h>
RESTORE_WARNINGS

#include <string>

class QQuickItem;

namespace aqt
{
namespace stylesheets
{

class StyleEngine;
class StyleSetAttached;

/*! @cond DOXYGEN_IGNORE */
namespace detail
{
template <typename T>
struct PropertyConvertTraits;
}

/*! @endcond */

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
 * <pre>
 * import Aqt.StyleSheets 1.0
 * </pre>
 * @since 1.0
 */
class StyleSet : public QObject
{
  Q_OBJECT

  /*! Contains the path for the element this StyleSet is attached to.
   *
   * @property QString path
   *
   * The path has the following syntax:
   *
   * <pre>
   * path      ::= element ( '/' element )*
   * element   ::= typename [ '.' ( classnames | classname ) ]
   * typename  ::= SYMBOL
   * classnames ::= '{' classname ( ',' classname )* '}'
   * classname ::= SYMBOL
   * </pre>
   *
   * E.g.: @c QQuickRectangle.root/QQuickRowLayout/QQuickItem.box/QQuickText
   *
   * The path is mostly useful for debugging purposes.
   */
  Q_PROPERTY(QString path READ path)

  /*! Contains the style properties for the element this StyleSet is attached to
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
  /*! Contains the style class name
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
   * Looks up the style property named @p key and returns it as is.  The
   * output time is either a @c string (@c QString) or a list of strings (@c
   * QVariantList).  If there's no such property returns and invalid @c
   * QVariant (which maps as @c undefined into Javascript/QML).
   *
   * @par Example:
   * @code
   * Rectangle {
   *   color: StyleSet.props.get("clip-colors")[clipColorId]
   * }
   * @endcode
   */
  Q_INVOKABLE QVariant get(const QString& key) const;

  /*! Returns the style property @p key as a @c QColor.
   *
   * Looks up the style property named @p key and interprets its first value as
   * a color specification.  All common color specs QML supports can be used
   * here:
   *
   * - by name, e.g. @c red, @c black, @c yellow
   * - by RGB string, e.g. @c \#3f4f5f
   * - by ARGB string, e.g. @c \#a0ff0022.
   * - then special @c transparent color
   *
   * Style property names can be any string, but typically end with @c
   * "-color".
   *
   * If there's no such property @p key or its first values can not be
   * converted to a @c QColor prints a warning.
   *
   * @par Example:
   * @code
   * Rectangle {
   *   color: StyleSet.props.color("background-color")
   * }
   * @endcode
   */
  Q_INVOKABLE QColor color(const QString& key) const;

  /*! Returns the style property @p key as a boolean value
   *
   * Looks up the style property named @p key and interprets its first value
   * as a boolean.  If there's no such property @p key or its first value can
   * not be converted to a boolean value prints a warning.
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
   * Looks up the style property named @p key and interprets its first value
   * as a number (a double).  If there's no such property @p key or its first
   * value can not be converted to a double prints a warning.
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
   * Looks up the style property named @p key and interprets its first value as
   * a CSS like font specification.  The font string has the format:
   *
   * <pre>
   * style capMode weight size familyName
   * </pre>
   *
   * Where the following values are defined:
   *
   * - __style__: italic, upright, oblique
   * - __weight__: light, regular, demibold, bold
   * - __capMode__: mixedcase, alluppercase, alllowercase, smallcaps, capitalize
   *
   * The __size__ can either be given in pixels or points (e.g. @c 12px or @c 11pt).
   * The __familyName__ is any valid family name currently installed on the
   * system, loaded via @c FontLoader or directive or via the @c font-face
   * declaration from a style sheet.
   *
   * @par Example for font specs:
   * @code
   *   italic 12px Calibre
   *   oblique smallcaps bold 24px Arial
   * @endcode
   *
   * If there's no such property @p key or its first value can not be
   * converted to a font spec prints a warning.
   *
   * @par Example:
   * @code
   * Text {
   *   font: StyleSet.props.font("font")
   * }
   * @endcode
   */
  Q_INVOKABLE QFont font(const QString& key) const;

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

public Q_SLOTS:
  /*! @cond DOXYGEN_IGNORE */
  void onStyleChanged(int changeCount);

private:
  QObject* grandParent();
  bool getImpl(QVariant& value, const QString& key) const;

  template <typename T, typename Traits = detail::PropertyConvertTraits<T>>
  T lookupProperty(const QString& key, Traits traits = Traits()) const;

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
  Q_PROPERTY(QString path READ path)
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
