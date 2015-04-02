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
const std::string kRgbaColorExpr = "rgba";
const std::string kRgbColorExpr = "rgb";
const std::string kHslaColorExpr = "hsla";
const std::string kHslColorExpr = "hsl";
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

//----------------------------------------------------------------------------------------

float clampFloat(float val)
{
  if (val < 0) {
    return 0.0f;
  } else if (val > 1.0f) {
    return 1.0f;
  }

  return val;
}

int clamp8Bit(int val)
{
  if (val < 0) {
    return 0;
  } else if (val > 255) {
    return 255;
  }

  return val;
}
int rgbColorOrPercentage(const std::string& arg)
{
  if (!arg.empty()) {
    try {
      if (arg.back() == '%') {
        auto factor = std::stof(arg.substr(0, arg.size() - 1));
        return clamp8Bit(int(std::round(255 * factor / 100.0f)));
      } else {
        return clamp8Bit(std::stoi(arg));
      }
    } catch (const std::invalid_argument&) {
    } catch (const std::out_of_range&) {
    }
  }

  return 0;
}

int transformAlphaFromFloatRatio(const std::string& arg)
{
  if (!arg.empty()) {
    try {
      auto factor = std::stof(arg);
      return clamp8Bit(int(std::round(256 * factor)));
    } catch (const std::invalid_argument&) {
    } catch (const std::out_of_range&) {
    }
  }

  return 0;
}

float hslHue(const std::string& arg)
{
  if (!arg.empty()) {
    try {
      return clampFloat(std::stoi(arg) / 360.0f);
    } catch (const std::invalid_argument&) {
    } catch (const std::out_of_range&) {
    }
  }
  return 0.0f;
}

float percentageToFactor(const std::string& arg)
{
  if (!arg.empty()) {
    try {
      if (arg.back() == '%') {
        return clampFloat(std::stoi(arg.substr(0, arg.size() - 1)) / 100.0f);
      }
    } catch (const std::invalid_argument&) {
    } catch (const std::out_of_range&) {
    }
  }
  return 100.0f;
}

float factorFromFloat(const std::string& arg)
{
  if (!arg.empty()) {
    try {
      return clampFloat(std::stof(arg));
    } catch (const std::invalid_argument&) {
    } catch (const std::out_of_range&) {
    }
  }
  return 0.0f;
}

boost::optional<QColor> makeRgbaColor(const std::vector<std::string>& args)
{
  if (args.size() == 4u) {
    return QColor(rgbColorOrPercentage(args[0]), rgbColorOrPercentage(args[1]),
                  rgbColorOrPercentage(args[2]), transformAlphaFromFloatRatio(args[3]));
  } else {
    styleSheetsLogWarning() << kRgbaColorExpr << "() expression expects 4 arguments";
  }
  return boost::none;
}

boost::optional<QColor> makeRgbColor(const std::vector<std::string>& args)
{
  if (args.size() == 3u) {
    return QColor(rgbColorOrPercentage(args[0]), rgbColorOrPercentage(args[1]),
                  rgbColorOrPercentage(args[2]), 0xff);
  } else {
    styleSheetsLogWarning() << kRgbColorExpr << "() expression expects 3 arguments";
  }
  return boost::none;
}

boost::optional<QColor> makeHslaColor(const std::vector<std::string>& args)
{
  if (args.size() == 4u) {
    QColor color;
    color.setHslF(hslHue(args[0]), percentageToFactor(args[1]),
                  percentageToFactor(args[2]), factorFromFloat(args[3]));
    return color;
  } else {
    styleSheetsLogWarning() << kHslaColorExpr << "() expression expects 3 arguments";
  }
  return boost::none;
}

boost::optional<QColor> makeHslColor(const std::vector<std::string>& args)
{
  if (args.size() == 3u) {
    QColor color;
    color.setHslF(
      hslHue(args[0]), percentageToFactor(args[1]), percentageToFactor(args[2]), 1.0f);
    return color;
  } else {
    styleSheetsLogWarning() << kHslColorExpr << "() expression expects 3 arguments";
  }
  return boost::none;
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
    using ExprEvaluator =
      std::function<boost::optional<QColor>(const std::vector<std::string>&)>;
    using FuncMap = std::unordered_map<std::string, ExprEvaluator>;

    static FuncMap funcMap = {
      {kRgbaColorExpr, &makeRgbaColor},
      {kRgbColorExpr, &makeRgbColor},
      {kHslaColorExpr, &makeHslaColor},
      {kHslColorExpr, &makeHslColor},
    };

    auto iFind = funcMap.find(expr.name);
    if (iFind != funcMap.end()) {
      return iFind->second(expr.args);
    } else {
      styleSheetsLogWarning() << "Unsupported expression '" << expr.name << "'";
    }

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
    if (*str == "true" || *str == "yes") {
      return boost::make_optional(true);
    }
  }

  return boost::none;
}

} // namespace stylesheets
} // namespace aqt
