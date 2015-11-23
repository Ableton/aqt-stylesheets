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

#include "CssParser.hpp"

#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <boost/variant/get.hpp>
#include <boost/variant/variant.hpp>
#include <gtest/gtest.h>
RESTORE_WARNINGS

//========================================================================================

using namespace aqt::stylesheets;

namespace
{

std::string selectorNames(const StyleSheet& ss,
                          size_t propsetIndex,
                          size_t selNumber,
                          size_t selIndex0,
                          size_t selIndex1)
{
  return ss.propsets[propsetIndex].selectors[selNumber][selIndex0][selIndex1];
}

std::string selectorName(const StyleSheet& ss,
                         size_t propsetIndex,
                         size_t selIndex0,
                         size_t selIndex1)
{
  return ss.propsets[propsetIndex].selectors[0][selIndex0][selIndex1];
}

std::string getFirstValue(const PropertyValues& val, const std::string& def = "")
{
  if (!val.empty()) {
    if (const std::string* str = boost::get<std::string>(&val[0])) {
      return *str;
    }
  }

  return def;
}

Expression getExpr(const PropertyValues& val, size_t idx, const Expression& def = {})
{
  if (!val.empty()) {
    if (const Expression* expr = boost::get<Expression>(&val[idx])) {
      return *expr;
    }
  }

  return def;
}

size_t getNumberOfValues(const PropertyValues& val)
{
  return val.size();
}

} // anonymous namespace

TEST(CssParserTest, ParserFromString_basic)
{
  const std::string src =
    "A { \n"
    "  background: red;\n"
    "}\n";

  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(ss.propsets.size(), 1);

  EXPECT_EQ(ss.propsets[0].properties.size(), 1);
  EXPECT_EQ(ss.propsets[0].properties[0].name, "background");
  EXPECT_EQ(getFirstValue(ss.propsets[0].properties[0].values), std::string("red"));
}

TEST(CssParserTest, ParserFromString_selectors)
{
  const std::string src =
    "A.b { color: #123456; }\n"
    ".b { text: 'green'; }\n"
    "A B.b { background: yellow; }\n"
    "A B .b { foreground: black; }\n"
    "A .b .c { foreground: black; }\n"
    ".b.a { text: 'a and b'; }\n";

  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(ss.propsets.size(), 6);
  EXPECT_EQ(selectorName(ss, 0, 0, 0), "A");
  EXPECT_EQ(selectorName(ss, 0, 0, 1), ".b");

  EXPECT_EQ(selectorName(ss, 1, 0, 0), ".b");

  EXPECT_EQ(selectorName(ss, 2, 0, 0), "A");
  EXPECT_EQ(selectorName(ss, 2, 1, 0), "B");
  EXPECT_EQ(selectorName(ss, 2, 1, 1), ".b");

  EXPECT_EQ(selectorName(ss, 3, 0, 0), "A");
  EXPECT_EQ(selectorName(ss, 3, 1, 0), "B");
  EXPECT_EQ(selectorName(ss, 3, 2, 0), ".b");

  EXPECT_EQ(ss.propsets[3].properties.size(), 1);
  EXPECT_EQ(ss.propsets[3].properties[0].name, "foreground");
  EXPECT_EQ(getFirstValue(ss.propsets[3].properties[0].values), std::string("black"));

  EXPECT_EQ(selectorName(ss, 4, 0, 0), "A");
  EXPECT_EQ(selectorName(ss, 4, 1, 0), ".b");
  EXPECT_EQ(selectorName(ss, 4, 2, 0), ".c");

  EXPECT_EQ(selectorName(ss, 5, 0, 0), ".b");
  EXPECT_EQ(selectorName(ss, 5, 0, 1), ".a");
}

TEST(CssParserTest, ParserFromString_separatedSelectors)
{
  const std::string src =
    "A, B, C { foreground: black; }\n"
    "A.a B.b, A.a C.c { foreground: black; }\n";

  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(ss.propsets.size(), 2);
  EXPECT_EQ(selectorNames(ss, 0, 0, 0, 0), "A");
  EXPECT_EQ(selectorNames(ss, 0, 1, 0, 0), "B");
  EXPECT_EQ(selectorNames(ss, 0, 2, 0, 0), "C");

  EXPECT_EQ(selectorNames(ss, 1, 0, 0, 0), "A");
  EXPECT_EQ(selectorNames(ss, 1, 0, 0, 1), ".a");
  EXPECT_EQ(selectorNames(ss, 1, 0, 1, 0), "B");
  EXPECT_EQ(selectorNames(ss, 1, 0, 1, 1), ".b");
  EXPECT_EQ(selectorNames(ss, 1, 1, 0, 0), "A");
  EXPECT_EQ(selectorNames(ss, 1, 1, 0, 1), ".a");
  EXPECT_EQ(selectorNames(ss, 1, 1, 1, 0), "C");
  EXPECT_EQ(selectorNames(ss, 1, 1, 1, 1), ".c");
}

TEST(CssParserTest, ParserFromString_childrenSelectors)
{
  const std::string src = "A.b > B.c { color: #123456; }\n";

  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(ss.propsets.size(), 1);
  EXPECT_EQ(selectorName(ss, 0, 0, 0), "A");
  EXPECT_EQ(selectorName(ss, 0, 0, 1), ".b");
  EXPECT_EQ(selectorName(ss, 0, 1, 0), ">");
  EXPECT_EQ(selectorName(ss, 0, 2, 0), "B");
  EXPECT_EQ(selectorName(ss, 0, 2, 1), ".c");
}

TEST(CssParserTest, ParserFromString_properties)
{
  const std::string src =
    "X {\n"
    "  abc: #123456; \n"
    "  def: 'string'; \n"
    "  ghi: \"string\"; \n"
    "  jkl: 1234; \n"
    "  mno: 123.45; \n"
    "  pqr: symbol; \n"
    "}\n";

  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(ss.propsets.size(), 1);
  EXPECT_EQ(ss.propsets[0].properties.size(), 6);

  EXPECT_EQ(getFirstValue(ss.propsets[0].properties[0].values), std::string("#123456"));
  EXPECT_EQ(getFirstValue(ss.propsets[0].properties[1].values), std::string("string"));
  EXPECT_EQ(getFirstValue(ss.propsets[0].properties[2].values), std::string("string"));
  EXPECT_EQ(getFirstValue(ss.propsets[0].properties[3].values), std::string("1234"));
  EXPECT_EQ(getFirstValue(ss.propsets[0].properties[4].values), std::string("123.45"));
  EXPECT_EQ(getFirstValue(ss.propsets[0].properties[5].values), std::string("symbol"));
}

TEST(CssParserTest, ParserFromString_stringProperties)
{
  const std::string src =
    "X {\n"
    "  def: 'str\"ing'; \n"
    "  ghi: \"str'ing\"; \n"
    "}\n";

  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(ss.propsets.size(), 1);
  EXPECT_EQ(ss.propsets[0].properties.size(), 2);

  EXPECT_EQ(getFirstValue(ss.propsets[0].properties[0].values), std::string("str\"ing"));
  EXPECT_EQ(getFirstValue(ss.propsets[0].properties[1].values), std::string("str'ing"));
}

TEST(CssParserTest, ParserFromString_emptyString)
{
  const std::string src = "";
  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(ss.propsets.size(), 0);
}

TEST(CssParserTest, ParserFromString_onlyWhitespace)
{
  const std::string src =
    "\n\n\n"
    "\t\t       \n\r"
    "\n";
  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(ss.propsets.size(), 0);
}

TEST(CssParserTest, ParserFromString_onlyCppComment)
{
  const std::string src = "// Copyright 2014 by Yoyodyne Inc.\n";
  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(ss.propsets.size(), 0);
}

TEST(CssParserTest, ParserFromString_onlyComment)
{
  const std::string src = "/* Copyright 2014 by Yoyodyne Inc. */\n";
  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(ss.propsets.size(), 0);
}

TEST(CssParserTest, ParserFromString_crlfNewlines)
{
  const std::string src =
    "X {\r\n"
    "abc: #123456; \r\n"
    "def: 'string'; \r\n"
    "}\r\n"
    "X .a {\r\n"
    "xyz: red;\r\n"
    "}\r\n";

  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(ss.propsets.size(), 2);
  EXPECT_EQ(ss.propsets[0].properties.size(), 2);
  EXPECT_EQ(ss.propsets[1].properties.size(), 1);
}

TEST(CssParserTest, ParserFromString_mixedNewlines)
{
  const std::string src =
    "X {\r\n"
    "abc: #123456; \n\r"
    "def: 'string'; \n"
    "}"
    "X .a {"
    "xyz: red;"
    "}\n\n";

  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(ss.propsets.size(), 2);
  EXPECT_EQ(ss.propsets[0].properties.size(), 2);
  EXPECT_EQ(ss.propsets[1].properties.size(), 1);
}

TEST(CssParserTest, ParserFromString_noNewlines)
{
  const std::string src =
    "X {"
    "abc: #123456;"
    "def: 'string';"
    "}"
    "X .a {"
    "xyz: red;"
    "}";

  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(ss.propsets.size(), 2);
  EXPECT_EQ(ss.propsets[0].properties.size(), 2);
  EXPECT_EQ(ss.propsets[1].properties.size(), 1);
}

TEST(CssParserTest, ParserFromString_noSemicolons)
{
  const std::string src =
    "X {"
    "  abc: #123456"
    "  def: 'string'"
    "}"
    "X .a {"
    "  xyz: red"
    "}";

  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(ss.propsets.size(), 2);
  EXPECT_EQ(ss.propsets[0].properties.size(), 2);
  EXPECT_EQ(ss.propsets[1].properties.size(), 1);
}

TEST(CssParserTest, ParserFromString_multipleValues)
{
  const std::string src =
    "X {"
    "  abc: a, b, c, d;\n"
    "}";

  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(ss.propsets.size(), 1);
  EXPECT_EQ(ss.propsets[0].properties.size(), 1);
  EXPECT_EQ(getNumberOfValues(ss.propsets[0].properties[0].values), 4);
}

TEST(CssParserTest, ParserFromString_fontFaceDeclarations)
{
  const std::string src =
    "// Copyright\n"
    "@font-face { src: url('../../Assets/times.ttf'); }\n";

  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(ss.propsets.size(), 0);
  EXPECT_EQ(ss.fontfaces.size(), 1);

  EXPECT_EQ(ss.fontfaces[0].url, "../../Assets/times.ttf");
}

//----------------------------------------------------------------------------------------

TEST(CssParserTest, ParserFromString_Expressions)
{
  const std::string src = "foo { bar: url('hello world'); }\n";

  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(1, ss.propsets.size());
  EXPECT_EQ(1, ss.propsets[0].properties.size());
  EXPECT_EQ(1, getNumberOfValues(ss.propsets[0].properties[0].values));

  EXPECT_EQ(std::string("url"), getExpr(ss.propsets[0].properties[0].values, 0).name);
  EXPECT_EQ((std::vector<std::string>{"hello world"}),
            getExpr(ss.propsets[0].properties[0].values, 0).args);
}

TEST(CssParserTest, ParserFromString_ExpressionsInLists)
{
  const std::string src =
    "foo { bar: rgba(123, 45, 92, 0.1), "
    "           foo(), "
    "           hsla(320, 100%, 20%, 0.3); }\n";

  StyleSheet ss = parseStdString(src);
  EXPECT_EQ(1, ss.propsets.size());
  EXPECT_EQ(1, ss.propsets[0].properties.size());

  EXPECT_EQ(std::string("rgba"), getExpr(ss.propsets[0].properties[0].values, 0).name);
  EXPECT_EQ((std::vector<std::string>{"123", "45", "92", "0.1"}),
            getExpr(ss.propsets[0].properties[0].values, 0).args);
  EXPECT_EQ(std::string("foo"), getExpr(ss.propsets[0].properties[0].values, 1).name);
  EXPECT_TRUE(getExpr(ss.propsets[0].properties[0].values, 1).args.empty());

  EXPECT_EQ(std::string("hsla"), getExpr(ss.propsets[0].properties[0].values, 2).name);
  EXPECT_EQ((std::vector<std::string>{"320", "100%", "20%", "0.3"}),
            getExpr(ss.propsets[0].properties[0].values, 2).args);
}

/* Missing tests:

   pathological cases:
   - non closed }
   - non closed "
   - non closed '
   - ambiguous selectors
   - no selectors
   - invalid chars in selector
   - invalid chars in propertyname
 */
