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
#include <catch/catch.hpp>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtGui/QColor>
#include <QtGui/QFont>
RESTORE_WARNINGS

//========================================================================================

using namespace aqt::stylesheets;

TEST_CASE("Convert to QString", "[convert]")
{
  REQUIRE(QLatin1String("hello world!")
          == *convertProperty<QString>(PropertyValue(std::string("hello world!"))));
  REQUIRE(QString() == *convertProperty<QString>(PropertyValue(std::string())));
  REQUIRE(QLatin1String("3.14")
          == *convertProperty<QString>(PropertyValue(std::string("3.14"))));

  REQUIRE(!convertProperty<QString>(
            PropertyValue(Expression({"ix", std::vector<std::string>{"250"}}))));
}

TEST_CASE("Convert to QDouble", "[convert]")
{
  REQUIRE(*convertProperty<double>(PropertyValue(std::string("3.14"))) == Approx(3.14));
  REQUIRE(*convertProperty<double>(PropertyValue(std::string("0"))) == Approx(0.0));
  REQUIRE(!convertProperty<double>(PropertyValue(std::string("hello world"))));
}

TEST_CASE("Convert to boolean", "[convert]")
{
  REQUIRE(convertProperty<bool>(PropertyValue(std::string("true"))).get());
  REQUIRE(convertProperty<bool>(PropertyValue(std::string("True"))).get());
  REQUIRE(convertProperty<bool>(PropertyValue(std::string("YES"))).get());
  REQUIRE(convertProperty<bool>(PropertyValue(std::string("yEs"))).get());
  REQUIRE(!convertProperty<bool>(PropertyValue(std::string("no"))).get());
  REQUIRE(!convertProperty<bool>(PropertyValue(std::string("false"))).get());
  REQUIRE(!convertProperty<bool>(PropertyValue(std::string("1"))));
  REQUIRE(!convertProperty<bool>(PropertyValue(std::string())));

  REQUIRE(!convertProperty<bool>(
            PropertyValue(Expression({"ix", std::vector<std::string>{"250"}}))));
}

//----------------------------------------------------------------------------------------

TEST_CASE("Convert to font", "[convert]")
{
  auto f = convertProperty<QFont>(
    PropertyValue(std::string("italic mixedcase light nohinting 12pt Times New Roman")));

  REQUIRE(f);
  REQUIRE(QLatin1String("Times New Roman") == f->family());
  REQUIRE(12 == f->pointSize());
  REQUIRE(f->italic());
  REQUIRE(!f->bold());
  REQUIRE(QFont::Light == f->weight());
  REQUIRE(QFont::MixedCase == f->capitalization());
  REQUIRE(QFont::PreferNoHinting == f->hintingPreference());
}

TEST_CASE("Convert to font with partial specification", "[convert]")
{
  auto f = convertProperty<QFont>(PropertyValue(std::string("12pt Arial")));

  REQUIRE(f);
  REQUIRE(QLatin1String("Arial") == f->family());
  REQUIRE(12 == f->pointSize());
  REQUIRE(!f->italic());
  REQUIRE(!f->bold());
  REQUIRE(QFont::StyleNormal == f->style());
  REQUIRE(QFont::Normal == f->weight());
  REQUIRE(QFont::MixedCase == f->capitalization());
  REQUIRE(QFont::PreferDefaultHinting == f->hintingPreference());
}

TEST_CASE("Convert to font with float size", "[convert]")
{
  auto f = convertProperty<QFont>(PropertyValue(std::string("12.7pt Arial")));

  REQUIRE(f);
  REQUIRE(QLatin1String("Arial") == f->family());
  REQUIRE(f->pointSizeF() == Approx(12.7));
}

TEST_CASE("Convert to font with pixelsize", "[convert]")
{
  auto f = convertProperty<QFont>(PropertyValue(std::string("18px Arial")));

  REQUIRE(f);
  REQUIRE(QLatin1String("Arial") == f->family());
  REQUIRE(18 == f->pixelSize());
}

//----------------------------------------------------------------------------------------

TEST_CASE("Convert RGB colors with hash notation", "[convert]")
{
  REQUIRE(QColor(0xde, 0xad, 0x01, 0xff)
          == *convertProperty<QColor>(PropertyValue(std::string("#dead01"))));
  REQUIRE(QColor(0x00, 0x00, 0x00, 0xff)
          == *convertProperty<QColor>(PropertyValue(std::string("#000000"))));
}

TEST_CASE("Convert ARGB colors with hash notation", "[convert]")
{
  REQUIRE(QColor(0xde, 0xad, 0x01, 0xe3)
          == *convertProperty<QColor>(PropertyValue(std::string("#e3dead01"))));
  REQUIRE(QColor(0xad, 0xfa, 0xce, 0xde)
          == *convertProperty<QColor>(PropertyValue(std::string("#deadface"))));
}

TEST_CASE("Converting bad color string fails", "[convert]")
{
  // Questionable API
  REQUIRE(QColor()
          == *convertProperty<QColor>(PropertyValue(std::string("hello world"))));
}

TEST_CASE("Convert RGB colors expression", "[convert][expressions]")
{
  REQUIRE(QColor(254, 112, 1, 255)
          == *convertProperty<QColor>(
               Expression{"rgb", std::vector<std::string>{"254", "112", "1"}}));

  REQUIRE(QColor(254, 128, 255, 255)
          == *convertProperty<QColor>(
               Expression{"rgb", std::vector<std::string>{"254", "50%", "100%"}}));
}

TEST_CASE("Converting bad RGB color expressions fails", "[convert][expressions]")
{
  // rgb() requires exactly 3 parameters
  REQUIRE_THROWS_AS(convertProperty<QColor>(Expression{
                      "rgb", std::vector<std::string>{"254", "112", "1", "0.5"}}),
                    ConvertException);

  REQUIRE_THROWS_AS(
    convertProperty<QColor>(Expression{"rgb", std::vector<std::string>{"254"}}),
    ConvertException);

  // rgb() requires integers or percentage, not floats for the color channel
  REQUIRE_THROWS_AS(convertProperty<QColor>(
                      Expression{"rgb", std::vector<std::string>{"0.1", "1.0", "0.3"}}),
                    ConvertException);
}

TEST_CASE("Convert RGBA color expression", "[convert][expressions]")
{
  REQUIRE(QColor(254, 112, 1, 128)
          == *convertProperty<QColor>(
               Expression{"rgba", std::vector<std::string>{"254", "112", "1", "0.5"}}));

  REQUIRE(QColor(64, 128, 255, 0)
          == *convertProperty<QColor>(Expression{
               "rgba", std::vector<std::string>{"25%", "50%", "100%", "0.0"}}));
}

TEST_CASE("Converting bad RGBA color expressions fails", "[convert][expressions]")
{
  // rgba() requires exactly 4 parameters
  REQUIRE_THROWS_AS(convertProperty<QColor>(
                      Expression{"rgba", std::vector<std::string>{"254", "112", "1"}}),
                    ConvertException);

  REQUIRE_THROWS_AS(
    convertProperty<QColor>(
      Expression{"rgba", std::vector<std::string>{"254", "112", "23", "0.5", "712"}}),
    ConvertException);

  // rgba() requires integers or percentage for the color channels, but a float
  // for the alpha channel
  REQUIRE_THROWS_AS(convertProperty<QColor>(Expression{
                      "rgba", std::vector<std::string>{"0.1", "1.0", "0.3", "0.4"}}),
                    ConvertException);
  REQUIRE_THROWS_AS(convertProperty<QColor>(Expression{
                      "rgba", std::vector<std::string>{"100%", "100%", "100%", "100%"}}),
                    ConvertException);

  // BUT: the following expression works, because "12" is a rather large float
  // which is clamped to 1.0f (=255).
  REQUIRE(QColor(128, 64, 0, 255)
          == *convertProperty<QColor>(
               Expression{"rgba", std::vector<std::string>{"128", "64", "0", "12"}}));
}

TEST_CASE("Convert HSL color expression", "[convert][expressions]")
{
  QColor c1;
  c1.setHslF(0.25, 0.25, 0.75, 1.0);

  REQUIRE(c1
          == *convertProperty<QColor>(
               Expression{"hsl", std::vector<std::string>{"90", "25%", "75%"}}));

  QColor c2;
  c2.setHslF(1.0, 0.0, 1.0, 1.0);
  REQUIRE(c2
          == *convertProperty<QColor>(
               Expression{"hsl", std::vector<std::string>{"375", "0%", "100%"}}));
}

TEST_CASE("Converting bad HSL color expressions fails", "[convert][expressions]")
{
  // hsl() needs exactly 3 arguments
  REQUIRE_THROWS_AS(convertProperty<QColor>(Expression{
                      "hsl", std::vector<std::string>{"90", "25%", "75%", "0.1"}}),
                    ConvertException);

  // arguments are degrees, percentage, percantage
  REQUIRE_THROWS_AS(convertProperty<QColor>(
                      Expression{"hsl", std::vector<std::string>{"90%", "0%", "100%"}}),
                    ConvertException);
  REQUIRE_THROWS_AS(convertProperty<QColor>(
                      Expression{"hsl", std::vector<std::string>{"255", "50", "100"}}),
                    ConvertException);
  REQUIRE_THROWS_AS(convertProperty<QColor>(
                      Expression{"hsl", std::vector<std::string>{"0.5", "0.75", "1.0"}}),
                    ConvertException);
}

TEST_CASE("Convert HSLA color expression", "[convert][expressions]")
{
  QColor c1;
  c1.setHslF(0.25, 0.25, 0.75, 0.6);

  REQUIRE(c1
          == *convertProperty<QColor>(
               Expression{"hsla", std::vector<std::string>{"90", "25%", "75%", "0.6"}}));

  QColor c2;
  c2.setHslF(1.0, 0.0, 1.0, 1.0);
  REQUIRE(c2
          == *convertProperty<QColor>(
               Expression{"hsla", std::vector<std::string>{"375", "0%", "100%", "1.0"}}));
}

TEST_CASE("Converting bad HSLA color expressions fails", "[convert][expressions]")
{
  // hsla() needs exactly 4 arguments
  REQUIRE_THROWS_AS(convertProperty<QColor>(
                      Expression{"hsla", std::vector<std::string>{"90", "25%", "75%"}}),
                    ConvertException);

  // arguments are degrees, percentage, percantage
  REQUIRE_THROWS_AS(convertProperty<QColor>(Expression{
                      "hsla", std::vector<std::string>{"90%", "0%", "100%", "20%"}}),
                    ConvertException);
  REQUIRE_THROWS_AS(convertProperty<QColor>(Expression{
                      "hsla", std::vector<std::string>{"255", "50", "100", "75"}}),
                    ConvertException);
  REQUIRE_THROWS_AS(convertProperty<QColor>(Expression{
                      "hsla", std::vector<std::string>{"0.5", "0.75", "1.0", "0.3"}}),
                    ConvertException);
}

TEST_CASE("Convert HSB color expression", "[convert][expressions]")
{
  QColor c1;
  c1.setHsvF(0.25, 0.25, 0.75, 1.0);

  REQUIRE(c1
          == *convertProperty<QColor>(
               Expression{"hsb", std::vector<std::string>{"90", "25%", "75%"}}));

  QColor c2;
  c2.setHsvF(1.0, 0.0, 1.0, 1.0);
  REQUIRE(c2
          == *convertProperty<QColor>(
               Expression{"hsb", std::vector<std::string>{"375", "0%", "100%"}}));
}

TEST_CASE("Converting bad HSB color expressions fails", "[convert][expressions]")
{
  // hsb() needs exactly 3 arguments
  REQUIRE_THROWS_AS(convertProperty<QColor>(Expression{
                      "hsb", std::vector<std::string>{"90", "25%", "75%", "0.1"}}),
                    ConvertException);

  // arguments are degrees, percentage, percantage
  REQUIRE_THROWS_AS(convertProperty<QColor>(
                      Expression{"hsb", std::vector<std::string>{"90%", "0%", "100%"}}),
                    ConvertException);
  REQUIRE_THROWS_AS(convertProperty<QColor>(
                      Expression{"hsb", std::vector<std::string>{"255", "50", "100"}}),
                    ConvertException);
  REQUIRE_THROWS_AS(convertProperty<QColor>(
                      Expression{"hsb", std::vector<std::string>{"0.5", "0.75", "1.0"}}),
                    ConvertException);
}

TEST_CASE("Convert HSBA color expression", "[convert][expressions]")
{
  QColor c1;
  c1.setHsvF(0.25, 0.25, 0.75, 0.6);

  REQUIRE(c1
          == *convertProperty<QColor>(
               Expression{"hsba", std::vector<std::string>{"90", "25%", "75%", "0.6"}}));

  QColor c2;
  c2.setHsvF(1.0, 0.0, 1.0, 1.0);
  REQUIRE(c2
          == *convertProperty<QColor>(
               Expression{"hsba", std::vector<std::string>{"375", "0%", "100%", "1.0"}}));
}

TEST_CASE("Converting bad HSBA color expressions fails", "[convert][expressions]")
{
  // hsba() needs exactly 4 arguments
  REQUIRE_THROWS_AS(convertProperty<QColor>(
                      Expression{"hsba", std::vector<std::string>{"90", "25%", "75%"}}),
                    ConvertException);

  // arguments are degrees, percentage, percantage
  REQUIRE_THROWS_AS(convertProperty<QColor>(Expression{
                      "hsba", std::vector<std::string>{"90%", "0%", "100%", "20%"}}),
                    ConvertException);
  REQUIRE_THROWS_AS(convertProperty<QColor>(Expression{
                      "hsba", std::vector<std::string>{"255", "50", "100", "75"}}),
                    ConvertException);
  REQUIRE_THROWS_AS(convertProperty<QColor>(Expression{
                      "hsba", std::vector<std::string>{"0.5", "0.75", "1.0", "0.3"}}),
                    ConvertException);
}

//------------------------------------------------------------------------------

TEST_CASE("Convert url from strings", "[convert]")
{
  REQUIRE(QUrl("http://abc.org")
          == *convertProperty<QUrl>(PropertyValue(std::string("http://abc.org"))));
  REQUIRE(QUrl("assets/icon/foo.png")
          == *convertProperty<QUrl>(PropertyValue(std::string("assets/icon/foo.png"))));
}

TEST_CASE("Convert url from expressions", "[convert][expressions]")
{
  REQUIRE(QUrl("http://abc.org")
          == *convertProperty<QUrl>(
               Expression{"url", std::vector<std::string>{"http://abc.org"}}));
  REQUIRE(QUrl("assets/icon/foo.png")
          == *convertProperty<QUrl>(
               Expression{"url", std::vector<std::string>{"assets/icon/foo.png"}}));
}

TEST_CASE("Converting bad url expressions fails", "[convert][expressions]")
{
  // url() expects 1 argument exactly
  REQUIRE_THROWS_AS(convertProperty<QUrl>(
                      Expression{"url", std::vector<std::string>{"abc.png", "foo/bar"}}),
                    ConvertException);

  REQUIRE_THROWS_AS(
    convertProperty<QUrl>(Expression{"rgb", std::vector<std::string>{"1", "2", "3"}}),
    ConvertException);
}
