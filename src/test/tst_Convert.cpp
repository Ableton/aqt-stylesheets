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

SUPPRESS_WARNINGS
#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <gtest/gtest.h>
RESTORE_WARNINGS

//========================================================================================

using namespace aqt::stylesheets;

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
    PropertyValue(std::string("italic mixedcase light 12pt Times New Roman")));

  EXPECT_TRUE(f);
  EXPECT_EQ(QLatin1String("Times New Roman"), f->family());
  EXPECT_EQ(12, f->pointSize());
  EXPECT_TRUE(f->italic());
  EXPECT_FALSE(f->bold());
  EXPECT_EQ(QFont::Light, f->weight());
  EXPECT_EQ(QFont::MixedCase, f->capitalization());
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
