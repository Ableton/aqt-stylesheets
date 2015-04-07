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

#include "Convert.hpp"

#include "Log.hpp"
#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/optional.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/get.hpp>
#include <boost/variant/static_visitor.hpp>
RESTORE_WARNINGS

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>

namespace aqt
{
namespace stylesheets
{

namespace
{

SUPPRESS_WARNINGS
const std::string kTrue = "true";
const std::string kYes = "yes";
const std::string kFalse = "false";
const std::string kNo = "no";
RESTORE_WARNINGS

/**
If the first token in the list is a font style token,
convert it to a font style and remove it from the list.
*/
QFont::Style takeFontStyleFromTokenList(QStringList& tokens)
{
  static const std::map<QString, QFont::Style> dictionary = {
    {"italic", QFont::StyleItalic},
    {"upright", QFont::StyleNormal},
    {"oblique", QFont::StyleOblique}};

  return (!tokens.isEmpty() && dictionary.count(tokens.at(0)))
           ? dictionary.at(tokens.takeFirst())
           : QFont::StyleNormal;
}

/**
If the first token in the list is a capitalization style token,
convert it to a capitalization style and remove it from the list.
*/
QFont::Capitalization takeCapitalizationStyleFromTokenList(QStringList& tokens)
{
  static const std::map<QString, QFont::Capitalization> dictionary = {
    {"mixedcase", QFont::MixedCase},
    {"alluppercase", QFont::AllUppercase},
    {"alllowercase", QFont::AllLowercase},
    {"smallcaps", QFont::SmallCaps},
    {"capitalize", QFont::Capitalize}};

  return (!tokens.isEmpty() && dictionary.count(tokens.at(0)))
           ? dictionary.at(tokens.takeFirst())
           : QFont::MixedCase;
}

/**
If the first token in the list is a font weight token,
convert it to a font weight and remove it from the list.
*/
QFont::Weight takeFontWeightFromTokenList(QStringList& tokens)
{
  static const std::map<QString, QFont::Weight> dictionary = {
    {"light", QFont::Light},
    {"bold", QFont::Bold},
    {"demibold", QFont::DemiBold},
    {"black", QFont::Black},
    {"regular", QFont::Normal}};

  return (!tokens.isEmpty() && dictionary.count(tokens.at(0)))
           ? dictionary.at(tokens.takeFirst())
           : QFont::Normal;
}

struct FontSize {
  int pixelSize = -1;
  int pointSize = -1;
};

/**
If the first token in the list is a font size token,
convert it to a font size and remove it from the list.
*/
FontSize takeFontSizeFromTokenList(QStringList& tokens)
{
  FontSize fontSize;
  if (!tokens.isEmpty()) {
    const QString sizeStr = tokens.takeFirst();

    if (sizeStr.contains(QRegExp("^\\d+px$"))) {
      fontSize.pixelSize = sizeStr.split(QRegExp("px")).at(0).toInt();
    } else if (sizeStr.contains(QRegExp("^\\d+pt$"))) {
      fontSize.pointSize = sizeStr.split(QRegExp("pt")).at(0).toInt();
    } else {
      tokens.prepend(sizeStr);
    }
  }
  return fontSize;
}

/**
Extract the font style from the string.

Font declarations must conform to a limited subset of the W3 font spec
(http://www.w3.org/TR/css3-fonts/#font-prop), see the following:

@code
// <style> <variant> <weight> <size> <family>
// e.g.:
font: "italic smallcaps bold 16px Times New Roman"
@endcode
*/
QFont fontDeclarationToFont(const QString& fontDecl)
{
  QStringList tokens = fontDecl.split(QRegExp("\\s* \\s*"), QString::SkipEmptyParts);

  const QFont::Style fontStyle = takeFontStyleFromTokenList(tokens);
  const QFont::Capitalization capMode = takeCapitalizationStyleFromTokenList(tokens);
  const QFont::Weight weight = takeFontWeightFromTokenList(tokens);
  const FontSize size = takeFontSizeFromTokenList(tokens);
  const QString familyName = tokens.join(' ');

  QFont font(familyName, size.pointSize, weight);
  if (size.pixelSize > 0) {
    font.setPixelSize(size.pixelSize);
  }
  font.setCapitalization(capMode);
  font.setStyle(fontStyle);
  return font;
}

struct PropValueVisitor : public boost::static_visitor<boost::optional<QColor>> {
  boost::optional<QColor> operator()(const std::string& value)
  {
    auto qvalue = QVariant(QString::fromStdString(value));
    if (qvalue.canConvert(QMetaType::QColor)) {
      return qvalue.value<QColor>();
    }
    return boost::none;
  }

  boost::optional<QColor> operator()(const Expression& expr)
  {
    styleSheetsLogWarning() << "Unsupported expression '" << expr.name << "'";
    return boost::none;
  }
};

} // anon namespace

//------------------------------------------------------------------------------

boost::optional<QFont> PropertyValueConvertTraits<QFont>::convert(
  const PropertyValue& value) const
{
  if (const std::string* str = boost::get<std::string>(&value)) {
    QVariant qvalue = QVariant::fromValue(QString::fromStdString(*str));

    if (qvalue.canConvert(QMetaType::QString)) {
      return fontDeclarationToFont(qvalue.toString());
    }
  }
  return boost::none;
}

boost::optional<QColor> PropertyValueConvertTraits<QColor>::convert(
  const PropertyValue& value) const
{
  PropValueVisitor visitor;
  return boost::apply_visitor(visitor, value);
}

boost::optional<QString> PropertyValueConvertTraits<QString>::convert(
  const PropertyValue& value) const
{
  if (const std::string* str = boost::get<std::string>(&value)) {
    return QString::fromStdString(*str);
  }

  return boost::none;
}

boost::optional<double> PropertyValueConvertTraits<double>::convert(
  const PropertyValue& value) const
{
  if (const std::string* str = boost::get<std::string>(&value)) {
    try {
      return boost::make_optional(std::stod(*str));
    } catch (const std::invalid_argument&) {
    } catch (const std::out_of_range&) {
    }
  }

  return boost::none;
}

boost::optional<bool> PropertyValueConvertTraits<bool>::convert(
  const PropertyValue& value) const
{
  if (const std::string* str = boost::get<std::string>(&value)) {
    auto lstr = boost::algorithm::to_lower_copy(*str);
    if (lstr == kTrue || lstr == kYes) {
      return boost::make_optional(true);
    } else if (lstr == kFalse || lstr == kNo) {
      return boost::make_optional(false);
    }
  }

  return boost::none;
}

} // namespace stylesheets
} // namespace aqt
