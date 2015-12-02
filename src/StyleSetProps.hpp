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

#include "StyleMatchTree.hpp"
#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtGui/QColor>
#include <QtGui/QFont>
RESTORE_WARNINGS

namespace aqt
{
namespace stylesheets
{

class StyleEngine;
class StyleSet;

/*! Provides style properties to QML via StyleSet */
class StyleSetProps : public QObject
{
  Q_OBJECT

public:
  /*! @cond DOXYGEN_IGNORE */
  StyleSetProps(const UiItemPath& path, StyleEngine* pEngine);

  static StyleSetProps* nullStyleSetProps();
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
  void loadProperties();

Q_SIGNALS:
  void propsChanged();
  void invalidated();

public Q_SLOTS:
  void onStyleChanged();

private:
  bool getImpl(Property& def, const QString& key) const;

  template <typename T>
  T lookupProperty(const QString& key) const;
  template <typename T>
  T lookupProperty(Property& def, const QString& key) const;

private:
  StyleEngine* const mpEngine;
  UiItemPath mPath;
  PropertyMap* mpProperties;
  /*! @endcond */
};

} // namespace stylesheets
} // namespace aqt

// include the header implementations
#include "StyleSetProps.ipp"
