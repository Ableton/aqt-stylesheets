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

#include "StyleMatchTree.hpp"

#include "CssParser.hpp"
#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtCore/QString>
#include <QtGui/QColor>
#include <gtest/gtest.h>
RESTORE_WARNINGS

#include <iostream>

//========================================================================================

// This must live outside of any namespace, otherwise C++ won't match QString
static void PrintTo(const QString& str, ::std::ostream* os)
{
  *os << "\"" << str.toStdString() << "\"";
}

using namespace aqt::stylesheets;

namespace
{
QString propertyAsString(PropertyMap pm, const char* pPropertyName)
{
  return pm[QString(pPropertyName)].value<QString>();
}

QColor propertyAsColor(PropertyMap pm, const char* pPropertyName)
{
  return pm[QString(pPropertyName)].value<QColor>();
}
} // anon namespace

TEST(StyleMatchTreeTest, matchByTypeName)
{
  const std::string src =
    "A { \n"
    "  background: red;\n"
    "}\n";

  StyleMatchTree mt = createMatchTree(parseStdString(src));

  UiItemPath p = {PathElement("A")};
  PropertyMap pm = matchPath(mt, p);

  EXPECT_EQ(1, pm.size());
  EXPECT_EQ("red", propertyAsString(pm, "background"));
}

TEST(StyleMatchTreeTest, matchByTypeAndObjectName)
{
  const std::string src =
    ".boo { background: red; }\n"
    ".foo.boo { background: blue; }\n"
    "Foo.boo { background: yellow; }\n"
    "Foo > .boo { background: green; }\n";

  StyleMatchTree mt = createMatchTree(parseStdString(src));

  UiItemPath p = {PathElement("Foo")};
  PropertyMap pm = matchPath(mt, p);
  EXPECT_EQ(0, pm.size());

  p = {PathElement("", {"boo"})};
  pm = matchPath(mt, p);
  EXPECT_EQ(1, pm.size());
  EXPECT_EQ("red", propertyAsString(pm, "background"));

  p = {PathElement("", {"boo", "foo"})};
  pm = matchPath(mt, p);
  EXPECT_EQ(1, pm.size());
  EXPECT_EQ("blue", propertyAsString(pm, "background"));

  p = {PathElement("Foo", {"boo"})};
  pm = matchPath(mt, p);
  EXPECT_EQ(1, pm.size());
  EXPECT_EQ("yellow", propertyAsString(pm, "background"));

  p = {PathElement("Foo"), PathElement("Bar", {"boo"})};
  pm = matchPath(mt, p);
  EXPECT_EQ(1, pm.size());
  EXPECT_EQ("green", propertyAsString(pm, "background"));
}

TEST(StyleMatchTreeTest, matchDescendants)
{
  const std::string src =
    "Foo       { background: red; }\n"
    "Foo > Bar { background: yellow; }\n"
    "Foo Bar   { background: green; }\n";

  StyleMatchTree mt = createMatchTree(parseStdString(src));

  UiItemPath p = {PathElement("Foo"), PathElement("Bar")};
  PropertyMap pm = matchPath(mt, p);
  EXPECT_EQ(1, pm.size());
  // "Foo > Bar" and "Foo Bar" have same specificity, therefore the later
  // wins.
  EXPECT_EQ("green", propertyAsString(pm, "background"));

  p = {PathElement("Foo"), PathElement("Gaz"), PathElement("Bar")};
  pm = matchPath(mt, p);
  EXPECT_EQ(1, pm.size());
  EXPECT_EQ("green", propertyAsString(pm, "background"));

  p = {PathElement("Gaz"), PathElement("Bar")};
  pm = matchPath(mt, p);
  EXPECT_EQ(0, pm.size());
}

TEST(StyleMatchTreeTest, laterMatchesWin)
{
  const std::string src =
    "Foo       { background: red; }\n"
    "Foo Bar   { background: green; }\n"
    "Foo > Bar { background: yellow; }\n";

  StyleMatchTree mt = createMatchTree(parseStdString(src));

  UiItemPath p = {PathElement("Foo"), PathElement("Bar")};
  PropertyMap pm = matchPath(mt, p);
  EXPECT_EQ(1, pm.size());
  EXPECT_EQ("yellow", propertyAsString(pm, "background"));
}

TEST(StyleMatchTreeTest, matchSeparatedQueries)
{
  const std::string src =
    "Foo, Bar { background: red; }\n"
    "Foo.a, Bar.b { background: blue; }\n"
    "Foo.c, Foo.a > Bar.b { background: black; }\n";

  StyleMatchTree mt = createMatchTree(parseStdString(src));

  UiItemPath p = {PathElement("Foo")};
  PropertyMap pm = matchPath(mt, p);
  EXPECT_EQ(1, pm.size());
  EXPECT_EQ("red", propertyAsString(pm, "background"));

  p = {PathElement("Bar")};
  pm = matchPath(mt, p);
  EXPECT_EQ(1, pm.size());
  EXPECT_EQ("red", propertyAsString(pm, "background"));

  p = {PathElement("Foo", {"a"})};
  pm = matchPath(mt, p);
  EXPECT_EQ(1, pm.size());
  EXPECT_EQ("blue", propertyAsString(pm, "background"));

  p = {PathElement("Bar", {"b"})};
  pm = matchPath(mt, p);
  EXPECT_EQ(1, pm.size());
  EXPECT_EQ("blue", propertyAsString(pm, "background"));

  p = {PathElement("Foo", {"a"}), PathElement("Bar", {"b"})};
  pm = matchPath(mt, p);
  EXPECT_EQ(1, pm.size());
  EXPECT_EQ("black", propertyAsString(pm, "background"));
}

TEST(StyleMatchTreeTest, matchChildren)
{
  const std::string src = "Foo > .boo { background: green; }\n";

  StyleMatchTree mt = createMatchTree(parseStdString(src));

  UiItemPath p = {PathElement("Foo"), PathElement("Bar", {"boo"})};
  PropertyMap pm = matchPath(mt, p);
  EXPECT_EQ(1, pm.size());
  EXPECT_EQ("green", propertyAsString(pm, "background"));

  p = {PathElement("Foo"), PathElement("Gaz"), PathElement("Bar", {"boo"})};
  pm = matchPath(mt, p);
  EXPECT_EQ(0, pm.size());
}

TEST(StyleMatchTreeTest, matchDescendantsAlways)
{
  const std::string src = "Foo .boo { background: green; }\n";

  auto mt = createMatchTree(parseStdString(src));
  auto p =
    UiItemPath{PathElement("Mam"), PathElement("Foo"), PathElement("Bar", {"boo"})};
  auto pm = matchPath(mt, p);
  EXPECT_EQ(1, pm.size());
  EXPECT_EQ("green", propertyAsString(pm, "background"));

  p = {PathElement("Foo"), PathElement("Gaz"), PathElement("Bar", {"boo"})};
  pm = matchPath(mt, p);
  EXPECT_EQ(1, pm.size());
  EXPECT_EQ("green", propertyAsString(pm, "background"));

  p = {PathElement("Foo"),
       PathElement("mam"),
       PathElement("Gaz"),
       PathElement("Bar", {"boo"})};
  pm = matchPath(mt, p);
  EXPECT_EQ(1, pm.size());
  EXPECT_EQ("green", propertyAsString(pm, "background"));
}

TEST(StyleMatchTreeTest, inheritPropertiesFromAllMatchingSelectors)
{
  const std::string src =
    "          Foo { propA: 1; }\n"
    "      Foo.bar { propB: 2; }\n"
    "         .bar { propC: 3; }\n";

  StyleMatchTree mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Foo", {"bar"})};
  PropertyMap pm = matchPath(mt, p);

  EXPECT_EQ(3, pm.size());
  EXPECT_EQ("1", propertyAsString(pm, "propA"));
  EXPECT_EQ("2", propertyAsString(pm, "propB"));
  EXPECT_EQ("3", propertyAsString(pm, "propC"));
}

TEST(StyleMatchTreeTest, inheritedPropertiesAreLessSpecific)
{
  const std::string src =
    "          Foo { color: red; }\n"
    "      Foo.bar { color: green; }\n";

  StyleMatchTree mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Foo", {"bar"})};
  PropertyMap pm = matchPath(mt, p);

  EXPECT_EQ(1, pm.size());
  EXPECT_EQ("green", propertyAsString(pm, "color"));
}

TEST(StyleMatchTreeTest, inheritPropertiesFromAllMatchingSelectorsWithManyParents)
{
  const std::string src =
    "          Bar     { propA: 1; }\n"
    "      Foo Bar     { propB: 2; }\n"
    "    Foo.a Bar     { propC: 3; }\n"
    "  Foo.a > Bar     { propD: 4; }\n"
    "  Foo.a   Bar.b   { propE: 5; }\n"
    "  Foo.a.x Bar.b.y { propF: 6; }\n";

  StyleMatchTree mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Foo", {"a", "x"}), PathElement("Bar", {"b", "y"})};
  PropertyMap pm = matchPath(mt, p);

  EXPECT_EQ(6, pm.size());
  EXPECT_EQ("1", propertyAsString(pm, "propA"));
  EXPECT_EQ("2", propertyAsString(pm, "propB"));
  EXPECT_EQ("3", propertyAsString(pm, "propC"));
  EXPECT_EQ("4", propertyAsString(pm, "propD"));
  EXPECT_EQ("5", propertyAsString(pm, "propE"));
  EXPECT_EQ("6", propertyAsString(pm, "propF"));
}

TEST(StyleMatchTreeTest, inheritPropertiesFromAllMatchingSelectorsWithParent)
{
  const std::string src =
    "          Bar { propA: 1; }\n"
    "      Foo Bar { propB: 2; }\n"
    "    Foo Bar.b { propC: 3; }\n";

  StyleMatchTree mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Foo"), PathElement("Bar", {"b"})};
  PropertyMap pm = matchPath(mt, p);

  EXPECT_EQ(3, pm.size());
  EXPECT_EQ("1", propertyAsString(pm, "propA"));
  EXPECT_EQ("2", propertyAsString(pm, "propB"));
  EXPECT_EQ("3", propertyAsString(pm, "propC"));
}

TEST(StyleMatchTreeTest, inheritPropertiesFromAllMatchingSelectorsWithClassedParents)
{
  const std::string src =
    "          Bar { propA: 1; }\n"
    "      Foo Bar { propB: 2; }\n"
    "    Foo.b Bar { propC: 3; }\n";

  StyleMatchTree mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Foo", {"b"}), PathElement("Bar")};
  PropertyMap pm = matchPath(mt, p);

  EXPECT_EQ(3, pm.size());
  EXPECT_EQ("1", propertyAsString(pm, "propA"));
  EXPECT_EQ("2", propertyAsString(pm, "propB"));
  EXPECT_EQ("3", propertyAsString(pm, "propC"));
}

TEST(StyleMatchTreeTest, inheritPropertiesFromDuplicateMatchingSelectors)
{
  const std::string src =
    " Foo { propA: 1; }\n"
    " Foo { propB: 2; }\n"
    " Foo.bar { propC: 3; }\n"
    " Foo.bar { propD: 4; }\n"
    " Bar, Foo { propE: 5; }\n";

  StyleMatchTree mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Foo", {"bar"})};
  PropertyMap pm = matchPath(mt, p);

  EXPECT_EQ(pm.size(), 5);
  EXPECT_EQ("1", propertyAsString(pm, "propA"));
  EXPECT_EQ("2", propertyAsString(pm, "propB"));
  EXPECT_EQ("3", propertyAsString(pm, "propC"));
  EXPECT_EQ("4", propertyAsString(pm, "propD"));
  EXPECT_EQ("5", propertyAsString(pm, "propE"));
}

TEST(StyleMatchTreeTest, duplicatedPropertiesInIdenticalSelectorsMatchLastOccurrence)
{
  const std::string src =
    " Foo { propA: first; }\n"
    " Foo { propA: last; }\n"
    " Foo.bar { propB: begin; }\n"
    " Foo.bar { propB: end; }\n";

  StyleMatchTree mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Foo", {"bar"})};
  PropertyMap pm = matchPath(mt, p);

  EXPECT_EQ(pm.size(), 2);
  EXPECT_EQ("last", propertyAsString(pm, "propA"));
  EXPECT_EQ("end", propertyAsString(pm, "propB"));
}

TEST(StyleMatchTreeTest, defaultStyleSheet)
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

  StyleMatchTree mt = createMatchTree(parseStdString(src), parseStdString(defaultSrc));
  UiItemPath p = {PathElement("Bar")};
  PropertyMap pm = matchPath(mt, p);

  EXPECT_EQ(3, pm.size());
  EXPECT_EQ("1", propertyAsString(pm, "propA"));
  EXPECT_EQ("5", propertyAsString(pm, "propB"));
  EXPECT_EQ("3", propertyAsString(pm, "propC"));

  p = {PathElement("Foo"), PathElement("Bar")};
  pm = matchPath(mt, p);

  EXPECT_EQ(5, pm.size());
  EXPECT_EQ("1", propertyAsString(pm, "propA"));
  EXPECT_EQ("5", propertyAsString(pm, "propB"));
  EXPECT_EQ("3", propertyAsString(pm, "propC"));
  EXPECT_EQ("2", propertyAsString(pm, "propD"));
  EXPECT_EQ("11", propertyAsString(pm, "propE"));
}

TEST(StyleMatchTreeTest, multipleClassNames)
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

  StyleMatchTree mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Bar", {"boo", "def"})};
  PropertyMap pm = matchPath(mt, p);

  EXPECT_EQ(4, pm.size());
  EXPECT_EQ("1", propertyAsString(pm, "propA"));
  EXPECT_EQ("2", propertyAsString(pm, "propB"));
  EXPECT_EQ("3", propertyAsString(pm, "propC"));
  EXPECT_EQ("7", propertyAsString(pm, "propE"));

  p = {PathElement("Foo", {"abc"}), PathElement("Bar", {"boo", "def"})};
  pm = matchPath(mt, p);

  EXPECT_EQ(5, pm.size());
  EXPECT_EQ("1", propertyAsString(pm, "propA"));
  EXPECT_EQ("2", propertyAsString(pm, "propB"));
  EXPECT_EQ("3", propertyAsString(pm, "propC"));
  EXPECT_EQ("4", propertyAsString(pm, "propD"));
  EXPECT_EQ("7", propertyAsString(pm, "propE"));
}

TEST(StyleMatchTreeTest, multipleClassNamesAndDefaultStyleSheet)
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

  StyleMatchTree mt = createMatchTree(parseStdString(src), parseStdString(defaultSrc));
  UiItemPath p = {PathElement("Foo", {"boo", "gaz"}), PathElement("Bar", {"aha", "nam"})};
  PropertyMap pm = matchPath(mt, p);

  EXPECT_EQ(5, pm.size());
  EXPECT_EQ("100", propertyAsString(pm, "propA"));
  EXPECT_EQ("5", propertyAsString(pm, "propB"));
  EXPECT_EQ("3", propertyAsString(pm, "propC"));
  EXPECT_EQ("2", propertyAsString(pm, "propD"));
  EXPECT_EQ("11", propertyAsString(pm, "propE"));
}

TEST(StyleMatchTreeTest, multipleClassNamesUndefinedClassNameDontMatter)
{
  const std::string src =
    "Bar     { propA: 1 }\n"
    "Bar.boo { propB: 2 }\n";

  StyleMatchTree mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Bar", {"boo", "abc", "xyz", "def"})};
  PropertyMap pm = matchPath(mt, p);

  EXPECT_EQ(2, pm.size());
  EXPECT_EQ("1", propertyAsString(pm, "propA"));
  EXPECT_EQ("2", propertyAsString(pm, "propB"));
}

TEST(StyleMatchTreeTest, multipleClassNames_theLastOfAmbiguousDefinitionsWins)
{
  const std::string src =
    ".typeA { color: red; }\n"
    ".typeB { color: blue; }\n"
    ".typeC { color: yellow; }\n";

  StyleMatchTree mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Bar", {"typeA", "typeB"})};
  PropertyMap pm = matchPath(mt, p);

  EXPECT_EQ(1, pm.size());
  EXPECT_EQ("blue", propertyAsString(pm, "color"));

  p = {PathElement("Bar", {"typeC", "typeB"})};
  pm = matchPath(mt, p);

  EXPECT_EQ(1, pm.size());
  EXPECT_EQ("yellow", propertyAsString(pm, "color"));

  p = {PathElement("Bar", {"typeB", "typeC", "typeA"})};
  pm = matchPath(mt, p);

  EXPECT_EQ(1, pm.size());
  EXPECT_EQ("yellow", propertyAsString(pm, "color"));
}

TEST(StyleMatchTreeTest, multipleClassNamesMatchChildren)
{
  const std::string src =
    "Abc.mno                   { propA: 1 }\n"
    "Abc.ixw                   { propB: 2 }\n"
    "Bar.boo.def > Abc.mno.ixw { propC: 3 }\n";

  StyleMatchTree mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Bar", {"boo", "def"}), PathElement("Abc", {"mno", "ixw"})};
  PropertyMap pm = matchPath(mt, p);

  EXPECT_EQ(3, pm.size());
  EXPECT_EQ("1", propertyAsString(pm, "propA"));
  EXPECT_EQ("2", propertyAsString(pm, "propB"));
  EXPECT_EQ("3", propertyAsString(pm, "propC"));
}

TEST(StyleMatchTreeTest, multipleClassNames_even_without_whitespace)
{
  const std::string src =
    "Abc.mno                 { propA: 1 }\n"
    "Abc.ixw                 { propB: 2 }\n"
    "Bar.boo.def>Abc.mno.ixw { propC: 3 }\n";

  StyleMatchTree mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Bar", {"boo", "def"}), PathElement("Abc", {"mno", "ixw"})};
  PropertyMap pm = matchPath(mt, p);

  EXPECT_EQ(3, pm.size());
  EXPECT_EQ("1", propertyAsString(pm, "propA"));
  EXPECT_EQ("2", propertyAsString(pm, "propB"));
  EXPECT_EQ("3", propertyAsString(pm, "propC"));
}

//----------------------------------------------------------------------------------------

TEST(StyleMatchTreeTest, rgbColors_with_percentage_value)
{
  const std::string src =
    ".foo { color: rgb(0%, 25%, 100%); }\n"
    ".bar { color: rgb(10%, 75%, 95%); }\n";

  StyleSheet ss = parseStdString(src);

  StyleMatchTree mt = createMatchTree(parseStdString(src));
  {
    UiItemPath p = {PathElement("Bar", {"foo"})};
    PropertyMap pm = matchPath(mt, p);

    EXPECT_EQ(1, pm.size());
    EXPECT_EQ(0x00, propertyAsColor(pm, "color").red());
    EXPECT_EQ(0x40, propertyAsColor(pm, "color").green());
    EXPECT_EQ(0xff, propertyAsColor(pm, "color").blue());
    EXPECT_EQ(0xff, propertyAsColor(pm, "color").alpha());
  }

  {
    UiItemPath p = {PathElement("Bar", {"bar"})};
    PropertyMap pm = matchPath(mt, p);

    EXPECT_EQ(1, pm.size());
    EXPECT_EQ(0x1a, propertyAsColor(pm, "color").red());
    EXPECT_EQ(0xbf, propertyAsColor(pm, "color").green());
    EXPECT_EQ(0xf2, propertyAsColor(pm, "color").blue());
    EXPECT_EQ(0xff, propertyAsColor(pm, "color").alpha());
  }
}

TEST(StyleMatchTreeTest, rgbColors)
{
  const std::string src = ".foo { color: rgb(0, 16, 32); }\n";

  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(ss.propsets.size(), 1);

  StyleMatchTree mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Bar", {"foo"})};
  PropertyMap pm = matchPath(mt, p);

  EXPECT_EQ(1, pm.size());
  EXPECT_EQ(0x00, propertyAsColor(pm, "color").red());
  EXPECT_EQ(0x10, propertyAsColor(pm, "color").green());
  EXPECT_EQ(0x20, propertyAsColor(pm, "color").blue());
  EXPECT_EQ(0xff, propertyAsColor(pm, "color").alpha());
}


TEST(StyleMatchTreeTest, rgbaColors)
{
  const std::string src = ".foo { color: rgba(0, 16, 32, 0.5); }\n";

  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(ss.propsets.size(), 1);

  StyleMatchTree mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Bar", {"foo"})};
  PropertyMap pm = matchPath(mt, p);

  EXPECT_EQ(1, pm.size());
  EXPECT_EQ(0x00, propertyAsColor(pm, "color").red());
  EXPECT_EQ(0x10, propertyAsColor(pm, "color").green());
  EXPECT_EQ(0x20, propertyAsColor(pm, "color").blue());
  EXPECT_EQ(0x80, propertyAsColor(pm, "color").alpha());
}


TEST(StyleMatchTreeTest, hslColors)
{
  const std::string src = ".foo { color: hsl(120, 100%, 50%); }\n";

  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(ss.propsets.size(), 1);

  StyleMatchTree mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Bar", {"foo"})};
  PropertyMap pm = matchPath(mt, p);

  EXPECT_EQ(1, pm.size());
  EXPECT_EQ(120, propertyAsColor(pm, "color").hslHue());
  EXPECT_NEAR(1.0f, propertyAsColor(pm, "color").hslSaturationF(), 0.00001f);
  EXPECT_NEAR(0.5f, propertyAsColor(pm, "color").lightnessF(), 0.00001f);
}


TEST(StyleMatchTreeTest, hslaColors)
{
  const std::string src = ".foo { color: hsla(359, 97%, 13%, 0.23); }\n";

  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(ss.propsets.size(), 1);

  StyleMatchTree mt = createMatchTree(parseStdString(src));
  UiItemPath p = {PathElement("Bar", {"foo"})};
  PropertyMap pm = matchPath(mt, p);

  EXPECT_EQ(1, pm.size());
  EXPECT_EQ(359, propertyAsColor(pm, "color").hslHue());
  EXPECT_NEAR(0.97f, propertyAsColor(pm, "color").hslSaturationF(), 0.00001f);
  EXPECT_NEAR(0.13f, propertyAsColor(pm, "color").lightnessF(), 0.00001f);
  EXPECT_NEAR(0.23f, propertyAsColor(pm, "color").alphaF(), 0.00001f);
}
