/*
Copyright (c) 2015 Ableton AG, Berlin

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
#include "LogUtils.hpp"

SUPPRESS_WARNINGS
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <gtest/gtest.h>
RESTORE_WARNINGS

//========================================================================================

using namespace aqt::stylesheets;
using namespace aqt::log_utils;

TEST(Convert, qString)
{
  EXPECT_EQ(QLatin1String("hello world!"),
            *convertProperty<QString>(PropertyValue(std::string("hello world!"))));
  EXPECT_EQ(QString(), *convertProperty<QString>(PropertyValue(std::string())));
  EXPECT_EQ(
    QLatin1String("3.14"), *convertProperty<QString>(PropertyValue(std::string("3.14"))));

  EXPECT_FALSE(convertProperty<QString>(
    PropertyValue(Expression({"ix", std::vector<std::string>{"250"}}))));
}

TEST(Convert, qDouble)
{
  EXPECT_EQ(3.14, *convertProperty<double>(PropertyValue(std::string("3.14"))));
  EXPECT_EQ(0.0, *convertProperty<double>(PropertyValue(std::string("0"))));
  EXPECT_FALSE(convertProperty<double>(PropertyValue(std::string("hello world"))));
}

TEST(Convert, boolean)
{
  EXPECT_TRUE(convertProperty<bool>(PropertyValue(std::string("true"))).get());
  EXPECT_TRUE(convertProperty<bool>(PropertyValue(std::string("True"))).get());
  EXPECT_TRUE(convertProperty<bool>(PropertyValue(std::string("YES"))).get());
  EXPECT_TRUE(convertProperty<bool>(PropertyValue(std::string("yEs"))).get());
  EXPECT_FALSE(convertProperty<bool>(PropertyValue(std::string("no"))).get());
  EXPECT_FALSE(convertProperty<bool>(PropertyValue(std::string("false"))).get());
  EXPECT_FALSE(convertProperty<bool>(PropertyValue(std::string("1"))));
  EXPECT_FALSE(convertProperty<bool>(PropertyValue(std::string())));

  EXPECT_FALSE(convertProperty<bool>(
    PropertyValue(Expression({"ix", std::vector<std::string>{"250"}}))));
}

//----------------------------------------------------------------------------------------

TEST(Convert, fonts)
{
  auto f = convertProperty<QFont>(
    PropertyValue(std::string("italic mixedcase light nohinting 12pt Times New Roman")));

  EXPECT_TRUE(f);
  EXPECT_EQ(QLatin1String("Times New Roman"), f->family());
  EXPECT_EQ(12, f->pointSize());
  EXPECT_TRUE(f->italic());
  EXPECT_FALSE(f->bold());
  EXPECT_EQ(QFont::Light, f->weight());
  EXPECT_EQ(QFont::MixedCase, f->capitalization());
  EXPECT_EQ(QFont::PreferNoHinting, f->hintingPreference());
}

TEST(Convert, fonts_with_partial_specification)
{
  auto f = convertProperty<QFont>(PropertyValue(std::string("12pt Arial")));

  EXPECT_TRUE(f);
  EXPECT_EQ(QLatin1String("Arial"), f->family());
  EXPECT_EQ(12, f->pointSize());
  EXPECT_FALSE(f->italic());
  EXPECT_FALSE(f->bold());
  EXPECT_EQ(QFont::StyleNormal, f->style());
  EXPECT_EQ(QFont::Normal, f->weight());
  EXPECT_EQ(QFont::MixedCase, f->capitalization());
  EXPECT_EQ(QFont::PreferDefaultHinting, f->hintingPreference());
}

TEST(Convert, fonts_with_float_size)
{
  auto f = convertProperty<QFont>(PropertyValue(std::string("12.7pt Arial")));

  EXPECT_TRUE(f);
  EXPECT_EQ(QLatin1String("Arial"), f->family());
  EXPECT_EQ(12.7, f->pointSizeF());
}

TEST(Convert, fonts_with_pixelsize)
{
  auto f = convertProperty<QFont>(PropertyValue(std::string("18px Arial")));

  EXPECT_TRUE(f);
  EXPECT_EQ(QLatin1String("Arial"), f->family());
  EXPECT_EQ(18, f->pixelSize());
}

//----------------------------------------------------------------------------------------

TEST(Convert, colors_rgb_hash)
{
  EXPECT_EQ(QColor(0xde, 0xad, 0x01, 0xff),
            *convertProperty<QColor>(PropertyValue(std::string("#dead01"))));
  EXPECT_EQ(QColor(0x00, 0x00, 0x00, 0xff),
            *convertProperty<QColor>(PropertyValue(std::string("#000000"))));
}

TEST(Convert, colors_argb_hash)
{
  EXPECT_EQ(QColor(0xde, 0xad, 0x01, 0xe3),
            *convertProperty<QColor>(PropertyValue(std::string("#e3dead01"))));
  EXPECT_EQ(QColor(0xad, 0xfa, 0xce, 0xde),
            *convertProperty<QColor>(PropertyValue(std::string("#deadface"))));
}

TEST(Convert, colors_failing)
{
  // Questionable API
  EXPECT_EQ(
    QColor(), *convertProperty<QColor>(PropertyValue(std::string("hello world"))));
}

TEST(Convert, colors_rgb_expression)
{
  EXPECT_EQ(
    QColor(254, 112, 1, 255), *convertProperty<QColor>(Expression{
                                "rgb", std::vector<std::string>{"254", "112", "1"}}));

  EXPECT_EQ(QColor(254, 128, 255, 255),
            *convertProperty<QColor>(
              Expression{"rgb", std::vector<std::string>{"254", "50%", "100%"}}));
}

TEST(Convert, colors_rgb_expression_fails)
{
  LogTracker tracker;

  // rgb() requires exactly 3 parameters
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"rgb", std::vector<std::string>{"254", "112", "1", "0.5"}}));

  EXPECT_FALSE(
    convertProperty<QColor>(Expression{"rgb", std::vector<std::string>{"254"}}));

  // rgb() requires integers or percentage, not floats for the color channel
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"rgb", std::vector<std::string>{"0.1", "1.0", "0.3"}}));

  EXPECT_EQ(6, tracker.messageCount(LogTracker::kWarn));
}

TEST(Convert, colors_rgba_expression)
{
  EXPECT_EQ(QColor(254, 112, 1, 128),
            *convertProperty<QColor>(
              Expression{"rgba", std::vector<std::string>{"254", "112", "1", "0.5"}}));

  EXPECT_EQ(QColor(64, 128, 255, 0),
            *convertProperty<QColor>(
              Expression{"rgba", std::vector<std::string>{"25%", "50%", "100%", "0.0"}}));
}

TEST(Convert, colors_rgba_expression_fails)
{
  LogTracker tracker;

  // rgba() requires exactly 4 parameters
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"rgba", std::vector<std::string>{"254", "112", "1"}}));

  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"rgba", std::vector<std::string>{"254", "112", "23", "0.5", "712"}}));

  // rgba() requires integers or percentage for the color channels, but a float
  // for the alpha channel
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"rgba", std::vector<std::string>{"0.1", "1.0", "0.3", "0.4"}}));
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"rgba", std::vector<std::string>{"100%", "100%", "100%", "100%"}}));

  // BUT: the following expression works, because "12" is a rather large float
  // which is clamped to 1.0f (=255).
  EXPECT_EQ(QColor(128, 64, 0, 255),
            *convertProperty<QColor>(
              Expression{"rgba", std::vector<std::string>{"128", "64", "0", "12"}}));

  EXPECT_EQ(8, tracker.messageCount(LogTracker::kWarn));
}

TEST(Convert, colors_hsl_expression)
{
  QColor c1;
  c1.setHslF(0.25, 0.25, 0.75, 1.0);

  EXPECT_EQ(c1, *convertProperty<QColor>(
                  Expression{"hsl", std::vector<std::string>{"90", "25%", "75%"}}));

  QColor c2;
  c2.setHslF(1.0, 0.0, 1.0, 1.0);
  EXPECT_EQ(c2, *convertProperty<QColor>(
                  Expression{"hsl", std::vector<std::string>{"375", "0%", "100%"}}));
}

TEST(Convert, colors_hsl_expression_fails)
{
  LogTracker tracker;

  // hsl() needs exactly 3 arguments
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"hsl", std::vector<std::string>{"90", "25%", "75%", "0.1"}}));

  // arguments are degrees, percentage, percantage
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"hsl", std::vector<std::string>{"90%", "0%", "100%"}}));
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"hsl", std::vector<std::string>{"255", "50", "100"}}));
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"hsl", std::vector<std::string>{"0.5", "0.75", "1.0"}}));

  EXPECT_EQ(8, tracker.messageCount(LogTracker::kWarn));
}

TEST(Convert, colors_hsla_expression)
{
  QColor c1;
  c1.setHslF(0.25, 0.25, 0.75, 0.6);

  EXPECT_EQ(c1, *convertProperty<QColor>(Expression{
                  "hsla", std::vector<std::string>{"90", "25%", "75%", "0.6"}}));

  QColor c2;
  c2.setHslF(1.0, 0.0, 1.0, 1.0);
  EXPECT_EQ(c2, *convertProperty<QColor>(Expression{
                  "hsla", std::vector<std::string>{"375", "0%", "100%", "1.0"}}));
}

TEST(Convert, colors_hsla_expression_fails)
{
  LogTracker tracker;

  // hsla() needs exactly 4 arguments
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"hsla", std::vector<std::string>{"90", "25%", "75%"}}));

  // arguments are degrees, percentage, percantage
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"hsla", std::vector<std::string>{"90%", "0%", "100%", "20%"}}));
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"hsla", std::vector<std::string>{"255", "50", "100", "75"}}));
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"hsla", std::vector<std::string>{"0.5", "0.75", "1.0", "0.3"}}));

  EXPECT_EQ(8, tracker.messageCount(LogTracker::kWarn));
}

TEST(Convert, colors_hsb_expression)
{
  QColor c1;
  c1.setHsvF(0.25, 0.25, 0.75, 1.0);

  EXPECT_EQ(c1, *convertProperty<QColor>(
                  Expression{"hsb", std::vector<std::string>{"90", "25%", "75%"}}));

  QColor c2;
  c2.setHsvF(1.0, 0.0, 1.0, 1.0);
  EXPECT_EQ(c2, *convertProperty<QColor>(
                  Expression{"hsb", std::vector<std::string>{"375", "0%", "100%"}}));
}

TEST(Convert, colors_hsb_expression_fails)
{
  LogTracker tracker;

  // hsb() needs exactly 3 arguments
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"hsb", std::vector<std::string>{"90", "25%", "75%", "0.1"}}));

  // arguments are degrees, percentage, percantage
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"hsb", std::vector<std::string>{"90%", "0%", "100%"}}));
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"hsb", std::vector<std::string>{"255", "50", "100"}}));
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"hsb", std::vector<std::string>{"0.5", "0.75", "1.0"}}));

  EXPECT_EQ(8, tracker.messageCount(LogTracker::kWarn));
}

TEST(Convert, colors_hsba_expression)
{
  QColor c1;
  c1.setHsvF(0.25, 0.25, 0.75, 0.6);

  EXPECT_EQ(c1, *convertProperty<QColor>(Expression{
                  "hsba", std::vector<std::string>{"90", "25%", "75%", "0.6"}}));

  QColor c2;
  c2.setHsvF(1.0, 0.0, 1.0, 1.0);
  EXPECT_EQ(c2, *convertProperty<QColor>(Expression{
                  "hsba", std::vector<std::string>{"375", "0%", "100%", "1.0"}}));
}

TEST(Convert, colors_hsba_expression_fails)
{
  LogTracker tracker;

  // hsba() needs exactly 4 arguments
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"hsba", std::vector<std::string>{"90", "25%", "75%"}}));

  // arguments are degrees, percentage, percantage
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"hsba", std::vector<std::string>{"90%", "0%", "100%", "20%"}}));
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"hsba", std::vector<std::string>{"255", "50", "100", "75"}}));
  EXPECT_FALSE(convertProperty<QColor>(
    Expression{"hsba", std::vector<std::string>{"0.5", "0.75", "1.0", "0.3"}}));

  EXPECT_EQ(8, tracker.messageCount(LogTracker::kWarn));
}

//------------------------------------------------------------------------------

TEST(Convert, url_from_strings)
{
  EXPECT_EQ(QUrl("http://abc.org"),
            *convertProperty<QUrl>(PropertyValue(std::string("http://abc.org"))));
  EXPECT_EQ(QUrl("assets/icon/foo.png"),
            *convertProperty<QUrl>(PropertyValue(std::string("assets/icon/foo.png"))));
}

TEST(Convert, url_from_expressions)
{
  EXPECT_EQ(
    QUrl("http://abc.org"), *convertProperty<QUrl>(Expression{
                              "url", std::vector<std::string>{"http://abc.org"}}));
  EXPECT_EQ(QUrl("assets/icon/foo.png"),
            *convertProperty<QUrl>(
              Expression{"url", std::vector<std::string>{"assets/icon/foo.png"}}));
}

TEST(Convert, url_conversion_fails)
{
  LogTracker tracker;

  // url() expects 1 argument exactly
  EXPECT_FALSE(convertProperty<QUrl>(
    Expression{"url", std::vector<std::string>{"abc.png", "foo/bar"}}));

  EXPECT_FALSE(
    convertProperty<QUrl>(Expression{"rgb", std::vector<std::string>{"1", "2", "3"}}));

  EXPECT_EQ(3, tracker.messageCount(LogTracker::kWarn));
}
