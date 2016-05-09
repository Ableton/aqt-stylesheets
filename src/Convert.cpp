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

#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <boost/algorithm/clamp.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/lexical_cast.hpp>
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
const std::string kHsbaColorExpr = "hsba";
const std::string kHsbColorExpr = "hsb";
const std::string kUrlExpr = "url";
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

/**
If the first token in the list is a font hinting token,
convert it to a font hinting and remove it from the list.
*/
QFont::HintingPreference takeFontHintingFromTokenList(QStringList& tokens)
{
  static const std::map<QString, QFont::HintingPreference> dictionary = {
    {"defaulthinting", QFont::PreferDefaultHinting},
    {"nohinting", QFont::PreferNoHinting},
    {"verticalhinting", QFont::PreferVerticalHinting},
    {"fullhinting", QFont::PreferFullHinting}};

  return (!tokens.isEmpty() && dictionary.count(tokens.at(0)))
           ? dictionary.at(tokens.takeFirst())
           : QFont::PreferDefaultHinting;
}

struct FontSize {
  int pixelSize = 0;
  qreal pointSize = 0.0;
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
    } else if (sizeStr.contains(QRegExp("^\\d+(\\.\\d+)?pt$"))) {
      fontSize.pointSize = sizeStr.split(QRegExp("pt")).at(0).toDouble();
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
  const QFont::HintingPreference hinting = takeFontHintingFromTokenList(tokens);
  const FontSize size = takeFontSizeFromTokenList(tokens);
  const QString familyName = tokens.join(' ');

  QFont font(familyName, 0, weight);
  if (size.pointSize > 0) {
    font.setPointSizeF(size.pointSize);
  }
  if (size.pixelSize > 0) {
    font.setPixelSize(size.pixelSize);
  }
  font.setCapitalization(capMode);
  font.setStyle(fontStyle);
  font.setHintingPreference(hinting);
  return font;
}

//----------------------------------------------------------------------------------------

struct Undefined {
};
using ExprValue = boost::variant<Undefined, QColor, QUrl>;

int rgbColorOrPercentage(const std::string& arg)
{
  if (!arg.empty() && arg.back() == '%') {
    auto factor = boost::lexical_cast<float>(arg.substr(0, arg.size() - 1));
    return boost::algorithm::clamp(int(std::round(255 * factor / 100.0f)), 0, 255);
  }

  return boost::algorithm::clamp(boost::lexical_cast<int>(arg), 0, 255);
}

int transformAlphaFromFloatRatio(const std::string& arg)
{
  auto factor = boost::lexical_cast<float>(arg);
  return boost::algorithm::clamp(int(std::round(256 * factor)), 0, 255);
}

double hslHue(const std::string& arg)
{
  return boost::algorithm::clamp(boost::lexical_cast<int>(arg) / 360.0, 0.0, 1.0);
}

double percentageToFactor(const std::string& arg)
{
  if (!arg.empty() && arg.back() == '%') {
    return boost::algorithm::clamp(
      boost::lexical_cast<int>(arg.substr(0, arg.size() - 1)) / 100.0, 0.0, 1.0);
  }

  throw boost::bad_lexical_cast();
}

double factorFromFloat(const std::string& arg)
{
  return boost::algorithm::clamp(boost::lexical_cast<double>(arg), 0.0, 1.0);
}

ExprValue makeRgbaColor(const std::vector<std::string>& args)
{
  if (args.size() == 4u) {
    try {
      return QColor(rgbColorOrPercentage(args[0]), rgbColorOrPercentage(args[1]),
                    rgbColorOrPercentage(args[2]), transformAlphaFromFloatRatio(args[3]));
    } catch (const boost::bad_lexical_cast&) {
      throw ConvertException(
        std::string().append(kRgbaColorExpr).append("() expression with bad value"));
    }
  }

  throw ConvertException(
    std::string().append(kRgbaColorExpr).append("() expression expects 4 arguments"));
}

ExprValue makeRgbColor(const std::vector<std::string>& args)
{
  if (args.size() == 3u) {
    try {
      return QColor(rgbColorOrPercentage(args[0]), rgbColorOrPercentage(args[1]),
                    rgbColorOrPercentage(args[2]), 0xff);
    } catch (const boost::bad_lexical_cast&) {
      throw ConvertException(
        std::string().append(kRgbColorExpr).append("() expression with bad value"));
    }
  }

  throw ConvertException(std::string().append(kRgbColorExpr).append("() expression expects 3 arguments"));
}

ExprValue makeHslaColor(const std::vector<std::string>& args)
{
  if (args.size() == 4u) {
    try {
      QColor color;
      color.setHslF(hslHue(args[0]), percentageToFactor(args[1]),
                    percentageToFactor(args[2]), factorFromFloat(args[3]));
      return color;
    } catch (const boost::bad_lexical_cast&) {
      throw ConvertException(
        std::string().append(kHslaColorExpr).append("() expression with bad values"));
    }
  }

  throw ConvertException(
    std::string().append(kHslaColorExpr).append("() expression expects 4 arguments"));
}

ExprValue makeHslColor(const std::vector<std::string>& args)
{
  if (args.size() == 3u) {
    try {
      QColor color;
      color.setHslF(
        hslHue(args[0]), percentageToFactor(args[1]), percentageToFactor(args[2]), 1.0);
      return color;
    } catch (const boost::bad_lexical_cast&) {
      throw ConvertException(
        std::string().append(kHslColorExpr).append("() expression with bad values"));
    }
  }

  throw ConvertException(
    std::string().append(kHslColorExpr).append("() expression expects 3 arguments"));
}

ExprValue makeHsbaColor(const std::vector<std::string>& args)
{
  if (args.size() == 4u) {
    try {
      QColor color;
      color.setHsvF(hslHue(args[0]), percentageToFactor(args[1]),
                    percentageToFactor(args[2]), factorFromFloat(args[3]));
      return color;
    } catch (const boost::bad_lexical_cast&) {
      throw ConvertException(
        std::string().append(kHslaColorExpr).append("() expression with bad values"));
    }
  }

  throw ConvertException(
    std::string().append(kHslaColorExpr).append("() expression expects 3 arguments"));
}

ExprValue makeHsbColor(const std::vector<std::string>& args)
{
  if (args.size() == 3u) {
    try {
      QColor color;
      color.setHsvF(
        hslHue(args[0]), percentageToFactor(args[1]), percentageToFactor(args[2]), 1.0);
      return color;
    } catch (const boost::bad_lexical_cast&) {
      throw ConvertException(
        std::string().append(kHslColorExpr).append("() expression with bad values"));
    }
  }

  throw ConvertException(
    std::string().append(kHslColorExpr).append("() expression expects 3 arguments"));
}

//------------------------------------------------------------------------------

ExprValue makeUrl(const std::vector<std::string>& args)
{
  if (args.size() == 1u) {
    return QUrl(QString::fromStdString(args.front()));
  }

  throw ConvertException(
    std::string().append(kUrlExpr).append("() expression expects 1 argument"));
}

//------------------------------------------------------------------------------

ExprValue evaluateExpression(const Expression& expr)
{
  using ExprEvaluator = std::function<ExprValue(const std::vector<std::string>&)>;
  using FuncMap = std::unordered_map<std::string, ExprEvaluator>;

  static FuncMap funcMap = {
    {kRgbaColorExpr, &makeRgbaColor},
    {kRgbColorExpr, &makeRgbColor},
    {kHslaColorExpr, &makeHslaColor},
    {kHslColorExpr, &makeHslColor},
    {kHsbaColorExpr, &makeHsbaColor},
    {kHsbColorExpr, &makeHsbColor},
    {kUrlExpr, &makeUrl},
  };

  auto iFind = funcMap.find(expr.name);
  if (iFind != funcMap.end()) {
    return iFind->second(expr.args);
  }

  throw ConvertException(
    std::string("Unsupported expression '").append(expr.name).append("'"));
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
    auto value = evaluateExpression(expr);
    if (const QColor* color = boost::get<QColor>(&value)) {
      return *color;
    }

    throw ConvertException(
      std::string("Not a color expression '").append(expr.name).append("'"));
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

boost::optional<QUrl> PropertyValueConvertTraits<QUrl>::convert(
  const PropertyValue& value) const
{
  struct PropValueToUrlVisitor : public boost::static_visitor<boost::optional<QUrl>> {
    boost::optional<QUrl> operator()(const std::string& str)
    {
      return QUrl(QString::fromStdString(str));
    }

    boost::optional<QUrl> operator()(const Expression& expr)
    {
      auto exprValue = evaluateExpression(expr);
      if (const QUrl* url = boost::get<QUrl>(&exprValue)) {
        return *url;
      }

      throw ConvertException(
        std::string("Not an url expression '").append(expr.name).append("'"));
    }
  };

  PropValueToUrlVisitor visitor;
  return boost::apply_visitor(visitor, value);
}

//----------------------------------------------------------------------------------------

namespace
{
struct PropValueToVariantVisitor : public boost::static_visitor<QVariant> {
  QVariant operator()(const std::string& value)
  {
    return QVariant(QString::fromStdString(value));
  }

  QVariant operator()(const Expression& expr)
  {
    struct ExprValueToVariantVisitor : public boost::static_visitor<QVariant> {
      QVariant operator()(const Undefined&)
      {
        return QVariant();
      }

      QVariant operator()(const QColor& color)
      {
        return QVariant(color);
      }

      QVariant operator()(const QUrl& url)
      {
        return QVariant(url);
      }
    };

    auto exprValue = evaluateExpression(expr);

    ExprValueToVariantVisitor visitor;
    return boost::apply_visitor(visitor, exprValue);
  }
};
} // anon namespace

QVariant convertValueToVariant(const PropertyValue& value)
{
  PropValueToVariantVisitor visitor;
  return boost::apply_visitor(visitor, value);
}

QVariantList convertValueToVariantList(const PropertyValues& values)
{
  QVariantList result;
  for (const auto& propValue : values) {
    result.push_back(convertValueToVariant(propValue));
  }

  return result;
}

} // namespace stylesheets
} // namespace aqt
