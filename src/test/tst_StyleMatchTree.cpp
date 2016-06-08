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

#include "StyleMatchTree.hpp"

#include "Convert.hpp"
#include "CssParser.hpp"
#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <boost/variant/get.hpp>
#include <catch/catch.hpp>
#include <QtCore/QString>
#include <QtGui/QColor>
RESTORE_WARNINGS


//========================================================================================

using namespace aqt::stylesheets;

namespace
{
std::string propertyAsString(PropertyMap pm, const char* pPropertyName)
{
  if (const std::string* str =
        boost::get<std::string>(&pm[QString(pPropertyName)].mValues[0])) {
    return *str;
  }
  return std::string();
}

QColor propertyAsColor(PropertyMap pm, const char* pPropertyName)
{
  auto result = convertProperty<QColor>(pm[QString(pPropertyName)].mValues[0]);
  if (result) {
    return *result;
  }
  return QColor();
}
} // anon namespace

TEST_CASE("Match by type name", "[match]")
{
  const std::string src =
    "A { \n"
    "  background: red;\n"
    "}\n";

  auto mt = createMatchTree(parseStdString(src));

  UiItemPath p = {PathElement("A")};
  PropertyMap pm = matchPath(mt.get(), p);

  REQUIRE(1 == pm.size());
  REQUIRE("red" == propertyAsString(pm, "background"));
}

TEST_CASE("Match by type and object name", "[match]")
{
  const std::string src =
    ".boo { background: red; }\n"
    ".foo.boo { background: blue; }\n"
    "Foo.boo { background: yellow; }\n"
    "Foo > .boo { background: green; }\n";

  auto mt = createMatchTree(parseStdString(src));

  UiItemPath p = {PathElement("Foo")};
  PropertyMap pm = matchPath(mt.get(), p);
  REQUIRE(0 == pm.size());

  p = {PathElement("", {"boo"})};
  pm = matchPath(mt.get(), p);
  REQUIRE(1 == pm.size());
  REQUIRE("red" == propertyAsString(pm, "background"));

  p = {PathElement("", {"boo", "foo"})};
  pm = matchPath(mt.get(), p);
  REQUIRE(1 == pm.size());
  REQUIRE("blue" == propertyAsString(pm, "background"));

  p = {PathElement("Foo", {"boo"})};
  pm = matchPath(mt.get(), p);
  REQUIRE(1 == pm.size());
  REQUIRE("yellow" == propertyAsString(pm, "background"));

  p = {PathElement("Foo"), PathElement("Bar", {"boo"})};
  pm = matchPath(mt.get(), p);
  REQUIRE(1 == pm.size());
  REQUIRE("green" == propertyAsString(pm, "background"));
}

TEST_CASE("Match descendants", "[match]")
{
  const std::string src =
    "Foo       { background: red; }\n"
    "Foo > Bar { background: yellow; }\n"
    "Foo Bar   { background: green; }\n";

  auto mt = createMatchTree(parseStdString(src));

  UiItemPath p = {PathElement("Foo"), PathElement("Bar")};
  PropertyMap pm = matchPath(mt.get(), p);
  REQUIRE(1 == pm.size());
  // "Foo > Bar" and "Foo Bar" have same specificity, therefore the later
  // wins.
  REQUIRE("green" == propertyAsString(pm, "background"));

  p = {PathElement("Foo"), PathElement("Gaz"), PathElement("Bar")};
  pm = matchPath(mt.get(), p);
  REQUIRE(1 == pm.size());
  REQUIRE("green" == propertyAsString(pm, "background"));

  p = {PathElement("Gaz"), PathElement("Bar")};
  pm = matchPath(mt.get(), p);
  REQUIRE(0 == pm.size());
}

TEST_CASE("Later matches win", "[match]")
{
  const std::string src =
    "Foo       { background: red; }\n"
    "Foo Bar   { background: green; }\n"
    "Foo > Bar { background: yellow; }\n";

  auto mt = createMatchTree(parseStdString(src));

  UiItemPath p = {PathElement("Foo"), PathElement("Bar")};
  PropertyMap pm = matchPath(mt.get(), p);
  REQUIRE(1 == pm.size());
  REQUIRE("yellow" == propertyAsString(pm, "background"));
}

TEST_CASE("Match separated queries", "[match]")
{
  const std::string src =
    "Foo, Bar { background: red; }\n"
    "Foo.a, Bar.b { background: blue; }\n"
    "Foo.c, Foo.a > Bar.b { background: black; }\n";

  auto mt = createMatchTree(parseStdString(src));

  UiItemPath p = {PathElement("Foo")};
  PropertyMap pm = matchPath(mt.get(), p);
  REQUIRE(1 == pm.size());
  REQUIRE("red" == propertyAsString(pm, "background"));

  p = {PathElement("Bar")};
  pm = matchPath(mt.get(), p);
  REQUIRE(1 == pm.size());
  REQUIRE("red" == propertyAsString(pm, "background"));

  p = {PathElement("Foo", {"a"})};
  pm = matchPath(mt.get(), p);
  REQUIRE(1 == pm.size());
  REQUIRE("blue" == propertyAsString(pm, "background"));

  p = {PathElement("Bar", {"b"})};
  pm = matchPath(mt.get(), p);
  REQUIRE(1 == pm.size());
  REQUIRE("blue" == propertyAsString(pm, "background"));

  p = {PathElement("Foo", {"a"}), PathElement("Bar", {"b"})};
  pm = matchPath(mt.get(), p);
  REQUIRE(1 == pm.size());
  REQUIRE("black" == propertyAsString(pm, "background"));
}

TEST_CASE("Match children", "[match]")
{
  const std::string src = "Foo > .boo { background: green; }\n";

  auto mt = createMatchTree(parseStdString(src));

  UiItemPath p = {PathElement("Foo"), PathElement("Bar", {"boo"})};
  PropertyMap pm = matchPath(mt.get(), p);
  REQUIRE(1 == pm.size());
  REQUIRE("green" == propertyAsString(pm, "background"));

  p = {PathElement("Foo"), PathElement("Gaz"), PathElement("Bar", {"boo"})};
  pm = matchPath(mt.get(), p);
  REQUIRE(0 == pm.size());
}

TEST_CASE("Match descendants always", "[match]")
{
  const std::string src = "Foo .boo { background: green; }\n";

  auto mt = createMatchTree(parseStdString(src));
  auto p =
    UiItemPath{PathElement("Mam"), PathElement("Foo"), PathElement("Bar", {"boo"})};
  auto pm = matchPath(mt.get(), p);
  REQUIRE(1 == pm.size());
  REQUIRE("green" == propertyAsString(pm, "background"));

  p = {PathElement("Foo"), PathElement("Gaz"), PathElement("Bar", {"boo"})};
  pm = matchPath(mt.get(), p);
  REQUIRE(1 == pm.size());
  REQUIRE("green" == propertyAsString(pm, "background"));

  p = {PathElement("Foo"),
       PathElement("mam"),
       PathElement("Gaz"),
       PathElement("Bar", {"boo"})};
  pm = matchPath(mt.get(), p);
  REQUIRE(1 == pm.size());
  REQUIRE("green" == propertyAsString(pm, "background"));
}

TEST_CASE("Inherit properties from all matching selectors", "[match]")
{
  const std::string src =
    "          Foo { propA: 1; }\n"
    "      Foo.bar { propB: 2; }\n"
    "         .bar { propC: 3; }\n";

  auto mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Foo", {"bar"})};
  PropertyMap pm = matchPath(mt.get(), p);

  REQUIRE(3 == pm.size());
  REQUIRE("1" == propertyAsString(pm, "propA"));
  REQUIRE("2" == propertyAsString(pm, "propB"));
  REQUIRE("3" == propertyAsString(pm, "propC"));
}

TEST_CASE("Inherited properties are less specific", "[match]")
{
  const std::string src =
    "          Foo { color: red; }\n"
    "      Foo.bar { color: green; }\n";

  auto mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Foo", {"bar"})};
  PropertyMap pm = matchPath(mt.get(), p);

  REQUIRE(1 == pm.size());
  REQUIRE("green" == propertyAsString(pm, "color"));
}

TEST_CASE("Inherit properties from all matching selectors with many parents", "[match]")
{
  const std::string src =
    "          Bar     { propA: 1; }\n"
    "      Foo Bar     { propB: 2; }\n"
    "    Foo.a Bar     { propC: 3; }\n"
    "  Foo.a > Bar     { propD: 4; }\n"
    "  Foo.a   Bar.b   { propE: 5; }\n"
    "  Foo.a.x Bar.b.y { propF: 6; }\n";

  auto mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Foo", {"a", "x"}), PathElement("Bar", {"b", "y"})};
  PropertyMap pm = matchPath(mt.get(), p);

  REQUIRE(6 == pm.size());
  REQUIRE("1" == propertyAsString(pm, "propA"));
  REQUIRE("2" == propertyAsString(pm, "propB"));
  REQUIRE("3" == propertyAsString(pm, "propC"));
  REQUIRE("4" == propertyAsString(pm, "propD"));
  REQUIRE("5" == propertyAsString(pm, "propE"));
  REQUIRE("6" == propertyAsString(pm, "propF"));
}

TEST_CASE("Inherit properties from all matching selectors with parent", "[match]")
{
  const std::string src =
    "          Bar { propA: 1; }\n"
    "      Foo Bar { propB: 2; }\n"
    "    Foo Bar.b { propC: 3; }\n";

  auto mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Foo"), PathElement("Bar", {"b"})};
  PropertyMap pm = matchPath(mt.get(), p);

  REQUIRE(3 == pm.size());
  REQUIRE("1" == propertyAsString(pm, "propA"));
  REQUIRE("2" == propertyAsString(pm, "propB"));
  REQUIRE("3" == propertyAsString(pm, "propC"));
}

TEST_CASE("Inherit properties from all matching selectors with classed parents",
          "[match]")
{
  const std::string src =
    "          Bar { propA: 1; }\n"
    "      Foo Bar { propB: 2; }\n"
    "    Foo.b Bar { propC: 3; }\n";

  auto mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Foo", {"b"}), PathElement("Bar")};
  PropertyMap pm = matchPath(mt.get(), p);

  REQUIRE(3 == pm.size());
  REQUIRE("1" == propertyAsString(pm, "propA"));
  REQUIRE("2" == propertyAsString(pm, "propB"));
  REQUIRE("3" == propertyAsString(pm, "propC"));
}

TEST_CASE("Inherit properties from duplicate matching selectors", "[match]")
{
  const std::string src =
    " Foo { propA: 1; }\n"
    " Foo { propB: 2; }\n"
    " Foo.bar { propC: 3; }\n"
    " Foo.bar { propD: 4; }\n"
    " Bar, Foo { propE: 5; }\n";

  auto mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Foo", {"bar"})};
  PropertyMap pm = matchPath(mt.get(), p);

  REQUIRE(pm.size() == 5);
  REQUIRE("1" == propertyAsString(pm, "propA"));
  REQUIRE("2" == propertyAsString(pm, "propB"));
  REQUIRE("3" == propertyAsString(pm, "propC"));
  REQUIRE("4" == propertyAsString(pm, "propD"));
  REQUIRE("5" == propertyAsString(pm, "propE"));
}

TEST_CASE("Duplicated properties in identical selectors match last occurrence", "[match]")
{
  const std::string src =
    " Foo { propA: first; }\n"
    " Foo { propA: last; }\n"
    " Foo.bar { propB: begin; }\n"
    " Foo.bar { propB: end; }\n";

  auto mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Foo", {"bar"})};
  PropertyMap pm = matchPath(mt.get(), p);

  REQUIRE(pm.size() == 2);
  REQUIRE("last" == propertyAsString(pm, "propA"));
  REQUIRE("end" == propertyAsString(pm, "propB"));
}

TEST_CASE("Default stylesheet", "[match]")
{
  const std::string defaultSrc =
    "Bar { propA: 100;\n"
    "      propB: 5; }\n"
    "Foo > Bar { propD: 7;\n"
    "            propE: 11 }\n";

  const std::string src =
    "Bar     { propA: 1;\n"
    "          propC: 3 }\n"
    "Foo Bar { propD: 2 }\n";

  auto mt = createMatchTree(parseStdString(src), parseStdString(defaultSrc));
  UiItemPath p = {PathElement("Bar")};
  PropertyMap pm = matchPath(mt.get(), p);

  REQUIRE(3 == pm.size());
  REQUIRE("1" == propertyAsString(pm, "propA"));
  REQUIRE("5" == propertyAsString(pm, "propB"));
  REQUIRE("3" == propertyAsString(pm, "propC"));

  p = {PathElement("Foo"), PathElement("Bar")};
  pm = matchPath(mt.get(), p);

  REQUIRE(5 == pm.size());
  REQUIRE("1" == propertyAsString(pm, "propA"));
  REQUIRE("5" == propertyAsString(pm, "propB"));
  REQUIRE("3" == propertyAsString(pm, "propC"));
  REQUIRE("2" == propertyAsString(pm, "propD"));
  REQUIRE("11" == propertyAsString(pm, "propE"));
}

TEST_CASE("Multiple classnames", "[match]")
{
  const std::string src =
    "Bar         { propA: 1 }\n"
    "Bar.boo     { propB: 2 }\n"
    "Bar.def     { propC: 3 }\n"
    "Bar.boo.def { propE: 7 }\n"
    "Bar.def     { propC: 3 }\n"
    "Foo Bar     { propD: 4 }\n"
    "Foo.abc Bar { propE: 5 }\n"
    "Foo.qoo Bar { propF: 6 }\n";

  auto mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Bar", {"boo", "def"})};
  PropertyMap pm = matchPath(mt.get(), p);

  REQUIRE(4 == pm.size());
  REQUIRE("1" == propertyAsString(pm, "propA"));
  REQUIRE("2" == propertyAsString(pm, "propB"));
  REQUIRE("3" == propertyAsString(pm, "propC"));
  REQUIRE("7" == propertyAsString(pm, "propE"));

  p = {PathElement("Foo", {"abc"}), PathElement("Bar", {"boo", "def"})};
  pm = matchPath(mt.get(), p);

  REQUIRE(5 == pm.size());
  REQUIRE("1" == propertyAsString(pm, "propA"));
  REQUIRE("2" == propertyAsString(pm, "propB"));
  REQUIRE("3" == propertyAsString(pm, "propC"));
  REQUIRE("4" == propertyAsString(pm, "propD"));
  REQUIRE("7" == propertyAsString(pm, "propE"));
}

TEST_CASE("Multiple classnames and default stylesheet", "[match]")
{
  const std::string defaultSrc =
    "Bar.aha { propA: 100;\n"
    "          propB: 5; }\n"
    "Foo.boo > Bar { propD: 7;\n"
    "                propE: 11 }\n";

  const std::string src =
    "Bar             { propA: 1;\n"
    "                  propC: 3 }\n"
    "Foo.gaz Bar.nam { propD: 2 }\n";

  auto mt = createMatchTree(parseStdString(src), parseStdString(defaultSrc));
  UiItemPath p = {PathElement("Foo", {"boo", "gaz"}), PathElement("Bar", {"aha", "nam"})};
  PropertyMap pm = matchPath(mt.get(), p);

  REQUIRE(5 == pm.size());
  REQUIRE("100" == propertyAsString(pm, "propA"));
  REQUIRE("5" == propertyAsString(pm, "propB"));
  REQUIRE("3" == propertyAsString(pm, "propC"));
  REQUIRE("2" == propertyAsString(pm, "propD"));
  REQUIRE("11" == propertyAsString(pm, "propE"));
}

TEST_CASE("Multiple class names undefined class name doesnt matter", "[match]")
{
  const std::string src =
    "Bar     { propA: 1 }\n"
    "Bar.boo { propB: 2 }\n";

  auto mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Bar", {"boo", "abc", "xyz", "def"})};
  PropertyMap pm = matchPath(mt.get(), p);

  REQUIRE(2 == pm.size());
  REQUIRE("1" == propertyAsString(pm, "propA"));
  REQUIRE("2" == propertyAsString(pm, "propB"));
}

TEST_CASE("Match multiple classnames - the last of ambiguous definitions wins", "[match]")
{
  const std::string src =
    ".typeA { color: red; }\n"
    ".typeB { color: blue; }\n"
    ".typeC { color: yellow; }\n";

  auto mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Bar", {"typeA", "typeB"})};
  PropertyMap pm = matchPath(mt.get(), p);

  REQUIRE(1 == pm.size());
  REQUIRE("blue" == propertyAsString(pm, "color"));

  p = {PathElement("Bar", {"typeC", "typeB"})};
  pm = matchPath(mt.get(), p);

  REQUIRE(1 == pm.size());
  REQUIRE("yellow" == propertyAsString(pm, "color"));

  p = {PathElement("Bar", {"typeB", "typeC", "typeA"})};
  pm = matchPath(mt.get(), p);

  REQUIRE(1 == pm.size());
  REQUIRE("yellow" == propertyAsString(pm, "color"));
}

TEST_CASE("Multiple classnames with children notation", "[match]")
{
  const std::string src =
    "Abc.mno                   { propA: 1 }\n"
    "Abc.ixw                   { propB: 2 }\n"
    "Bar.boo.def > Abc.mno.ixw { propC: 3 }\n";

  auto mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Bar", {"boo", "def"}), PathElement("Abc", {"mno", "ixw"})};
  PropertyMap pm = matchPath(mt.get(), p);

  REQUIRE(3 == pm.size());
  REQUIRE("1" == propertyAsString(pm, "propA"));
  REQUIRE("2" == propertyAsString(pm, "propB"));
  REQUIRE("3" == propertyAsString(pm, "propC"));
}

TEST_CASE("Multiple classnames - match even without whitespace", "[match]")
{
  const std::string src =
    "Abc.mno                 { propA: 1 }\n"
    "Abc.ixw                 { propB: 2 }\n"
    "Bar.boo.def>Abc.mno.ixw { propC: 3 }\n";

  auto mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Bar", {"boo", "def"}), PathElement("Abc", {"mno", "ixw"})};
  PropertyMap pm = matchPath(mt.get(), p);

  REQUIRE(3 == pm.size());
  REQUIRE("1" == propertyAsString(pm, "propA"));
  REQUIRE("2" == propertyAsString(pm, "propB"));
  REQUIRE("3" == propertyAsString(pm, "propC"));
}

//----------------------------------------------------------------------------------------

TEST_CASE("Store RGB colors with percentage value", "[expressions]")
{
  const std::string src =
    ".foo { color: rgb(0%, 25%, 100%); }\n"
    ".bar { color: rgb(10%, 75%, 95%); }\n";

  StyleSheet ss = parseStdString(src);

  auto mt = createMatchTree(parseStdString(src));
  {
    UiItemPath p = {PathElement("Bar", {"foo"})};
    PropertyMap pm = matchPath(mt.get(), p);

    REQUIRE(1 == pm.size());
    REQUIRE(0x00 == propertyAsColor(pm, "color").red());
    REQUIRE(0x40 == propertyAsColor(pm, "color").green());
    REQUIRE(0xff == propertyAsColor(pm, "color").blue());
    REQUIRE(0xff == propertyAsColor(pm, "color").alpha());
  }

  {
    UiItemPath p = {PathElement("Bar", {"bar"})};
    PropertyMap pm = matchPath(mt.get(), p);

    REQUIRE(1 == pm.size());
    REQUIRE(0x1a == propertyAsColor(pm, "color").red());
    REQUIRE(0xbf == propertyAsColor(pm, "color").green());
    REQUIRE(0xf2 == propertyAsColor(pm, "color").blue());
    REQUIRE(0xff == propertyAsColor(pm, "color").alpha());
  }
}

TEST_CASE("RGB colors", "[expressions]")
{
  const std::string src = ".foo { color: rgb(0, 16, 32); }\n";

  StyleSheet ss = parseStdString(src);
  REQUIRE(ss.propsets.size() == 1);

  auto mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Bar", {"foo"})};
  PropertyMap pm = matchPath(mt.get(), p);

  REQUIRE(1 == pm.size());
  REQUIRE(0x00 == propertyAsColor(pm, "color").red());
  REQUIRE(0x10 == propertyAsColor(pm, "color").green());
  REQUIRE(0x20 == propertyAsColor(pm, "color").blue());
  REQUIRE(0xff == propertyAsColor(pm, "color").alpha());
}

TEST_CASE("RGBA colors", "[expressions]")
{
  const std::string src = ".foo { color: rgba(0, 16, 32, 0.5); }\n";

  StyleSheet ss = parseStdString(src);
  REQUIRE(ss.propsets.size() == 1);

  auto mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Bar", {"foo"})};
  PropertyMap pm = matchPath(mt.get(), p);

  REQUIRE(1 == pm.size());
  REQUIRE(0x00 == propertyAsColor(pm, "color").red());
  REQUIRE(0x10 == propertyAsColor(pm, "color").green());
  REQUIRE(0x20 == propertyAsColor(pm, "color").blue());
  REQUIRE(0x80 == propertyAsColor(pm, "color").alpha());
}

TEST_CASE("HSL colors", "[expressions]")
{
  const std::string src = ".foo { color: hsl(120, 100%, 50%); }\n";

  StyleSheet ss = parseStdString(src);
  REQUIRE(ss.propsets.size() == 1);

  auto mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Bar", {"foo"})};
  PropertyMap pm = matchPath(mt.get(), p);

  REQUIRE(1 == pm.size());
  REQUIRE(120 == propertyAsColor(pm, "color").hslHue());
  REQUIRE(propertyAsColor(pm, "color").hslSaturationF() == Approx(1.0));
  REQUIRE(propertyAsColor(pm, "color").lightnessF() == Approx(0.5));
}

TEST_CASE("HSLA colors", "[expressions]")
{
  const std::string src = ".foo { color: hsla(359, 97%, 13%, 0.23); }\n";

  StyleSheet ss = parseStdString(src);
  REQUIRE(ss.propsets.size() == 1);

  auto mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Bar", {"foo"})};
  PropertyMap pm = matchPath(mt.get(), p);

  REQUIRE(1 == pm.size());
  REQUIRE(359 == propertyAsColor(pm, "color").hslHue());
  REQUIRE(propertyAsColor(pm, "color").hslSaturationF() == Approx(0.97));
  REQUIRE(propertyAsColor(pm, "color").lightnessF() == Approx(0.13));
  REQUIRE(propertyAsColor(pm, "color").alphaF() == Approx(0.23));
}
