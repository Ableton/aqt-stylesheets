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
#include <catch/catch.hpp>
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

Expression getExpr(const PropertyValues& val,
                   size_t idx,
                   const Expression& def = Expression{})
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

TEST_CASE("Parsing CSS from string", "[css][parse]")
{
  const std::string src =
    "A { \n"
    "  background: red;\n"
    "}\n";

  StyleSheet ss = parseStdString(src);
  REQUIRE(ss.propsets.size() == 1);

  REQUIRE(ss.propsets[0].properties.size() == 1);
  REQUIRE(ss.propsets[0].properties[0].name == "background");
  REQUIRE(getFirstValue(ss.propsets[0].properties[0].values) == std::string("red"));
}

TEST_CASE("Selectors with dashes and numbers", "[css][parse]")
{
  const std::string src =
    "A-z { \n"
    "  background1: red;\n"
    "  base-2:  green;\n"
    "  baSe_2:  yellow;\n"
    "}\n";

  StyleSheet ss = parseStdString(src);
  REQUIRE(ss.propsets.size() == 1);

  REQUIRE(ss.propsets[0].properties.size() == 3);
  REQUIRE(ss.propsets[0].properties[0].name == "background1");
  REQUIRE(getFirstValue(ss.propsets[0].properties[0].values) == std::string("red"));

  REQUIRE(ss.propsets[0].properties[1].name == "base-2");
  REQUIRE(getFirstValue(ss.propsets[0].properties[1].values) == std::string("green"));

  REQUIRE(ss.propsets[0].properties[2].name == "baSe_2");
  REQUIRE(getFirstValue(ss.propsets[0].properties[2].values) == std::string("yellow"));
}

TEST_CASE("Parsing CSS from string - different selector styles", "[css][parse]")
{
  const std::string src =
    "A.b { color: #123456; }\n"
    ".b { text: 'green'; }\n"
    "A B.b { background: yellow; }\n"
    "A B .b { foreground: black; }\n"
    "A .b .c { foreground: black; }\n"
    ".b.a { text: 'a and b'; }\n";

  StyleSheet ss = parseStdString(src);
  REQUIRE(ss.propsets.size() == 6);
  REQUIRE(selectorName(ss, 0, 0, 0) == "A");
  REQUIRE(selectorName(ss, 0, 0, 1) == ".b");

  REQUIRE(selectorName(ss, 1, 0, 0) == ".b");

  REQUIRE(selectorName(ss, 2, 0, 0) == "A");
  REQUIRE(selectorName(ss, 2, 1, 0) == "B");
  REQUIRE(selectorName(ss, 2, 1, 1) == ".b");

  REQUIRE(selectorName(ss, 3, 0, 0) == "A");
  REQUIRE(selectorName(ss, 3, 1, 0) == "B");
  REQUIRE(selectorName(ss, 3, 2, 0) == ".b");

  REQUIRE(ss.propsets[3].properties.size() == 1);
  REQUIRE(ss.propsets[3].properties[0].name == "foreground");
  REQUIRE(getFirstValue(ss.propsets[3].properties[0].values) == std::string("black"));

  REQUIRE(selectorName(ss, 4, 0, 0) == "A");
  REQUIRE(selectorName(ss, 4, 1, 0) == ".b");
  REQUIRE(selectorName(ss, 4, 2, 0) == ".c");

  REQUIRE(selectorName(ss, 5, 0, 0) == ".b");
  REQUIRE(selectorName(ss, 5, 0, 1) == ".a");
}

TEST_CASE("Parsing CSS from string - separated selectors", "[css][parse]")
{
  const std::string src =
    "A, B, C { foreground: black; }\n"
    "A.a B.b, A.a C.c { foreground: black; }\n";

  StyleSheet ss = parseStdString(src);
  REQUIRE(ss.propsets.size() == 2);
  REQUIRE(selectorNames(ss, 0, 0, 0, 0) == "A");
  REQUIRE(selectorNames(ss, 0, 1, 0, 0) == "B");
  REQUIRE(selectorNames(ss, 0, 2, 0, 0) == "C");

  REQUIRE(selectorNames(ss, 1, 0, 0, 0) == "A");
  REQUIRE(selectorNames(ss, 1, 0, 0, 1) == ".a");
  REQUIRE(selectorNames(ss, 1, 0, 1, 0) == "B");
  REQUIRE(selectorNames(ss, 1, 0, 1, 1) == ".b");
  REQUIRE(selectorNames(ss, 1, 1, 0, 0) == "A");
  REQUIRE(selectorNames(ss, 1, 1, 0, 1) == ".a");
  REQUIRE(selectorNames(ss, 1, 1, 1, 0) == "C");
  REQUIRE(selectorNames(ss, 1, 1, 1, 1) == ".c");
}

TEST_CASE("Parsing CSS from string - child selectors", "[css][parse]")
{
  const std::string src = "A.b > B.c { color: #123456; }\n";

  StyleSheet ss = parseStdString(src);
  REQUIRE(ss.propsets.size() == 1);
  REQUIRE(selectorName(ss, 0, 0, 0) == "A");
  REQUIRE(selectorName(ss, 0, 0, 1) == ".b");
  REQUIRE(selectorName(ss, 0, 1, 0) == ">");
  REQUIRE(selectorName(ss, 0, 2, 0) == "B");
  REQUIRE(selectorName(ss, 0, 2, 1) == ".c");
}

TEST_CASE("Parsing CSS from string - properties", "[css][parse]")
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
  REQUIRE(ss.propsets.size() == 1);
  REQUIRE(ss.propsets[0].properties.size() == 6);

  REQUIRE(getFirstValue(ss.propsets[0].properties[0].values) == std::string("#123456"));
  REQUIRE(getFirstValue(ss.propsets[0].properties[1].values) == std::string("string"));
  REQUIRE(getFirstValue(ss.propsets[0].properties[2].values) == std::string("string"));
  REQUIRE(getFirstValue(ss.propsets[0].properties[3].values) == std::string("1234"));
  REQUIRE(getFirstValue(ss.propsets[0].properties[4].values) == std::string("123.45"));
  REQUIRE(getFirstValue(ss.propsets[0].properties[5].values) == std::string("symbol"));
}

TEST_CASE("Parsing CSS from string - string properties", "[css][parse]")
{
  const std::string src =
    "X {\n"
    "  def: 'str\"ing'; \n"
    "  ghi: \"str'ing\"; \n"
    "}\n";

  StyleSheet ss = parseStdString(src);
  REQUIRE(ss.propsets.size() == 1);
  REQUIRE(ss.propsets[0].properties.size() == 2);

  REQUIRE(getFirstValue(ss.propsets[0].properties[0].values) == std::string("str\"ing"));
  REQUIRE(getFirstValue(ss.propsets[0].properties[1].values) == std::string("str'ing"));
}

TEST_CASE("Parsing CSS from string - empty strings", "[css][parse]")
{
  const std::string src = "";
  StyleSheet ss = parseStdString(src);
  REQUIRE(ss.propsets.size() == 0);
}

TEST_CASE("Parsing CSS from string - only whitespace", "[css][parse]")
{
  const std::string src =
    "\n\n\n"
    "\t\t       \n\r"
    "\n";
  StyleSheet ss = parseStdString(src);
  REQUIRE(ss.propsets.size() == 0);
}

TEST_CASE("Parsing CSS from string - only cpp comments", "[css][parse]")
{
  const std::string src = "// Copyright 2014 by Yoyodyne Inc.\n";
  StyleSheet ss = parseStdString(src);
  REQUIRE(ss.propsets.size() == 0);
}

TEST_CASE("Parsing CSS from string - only comments", "[css][parse]")
{
  const std::string src = "/* Copyright 2014 by Yoyodyne Inc. */\n";
  StyleSheet ss = parseStdString(src);
  REQUIRE(ss.propsets.size() == 0);
}

TEST_CASE("Parsing CSS from string - CRLF newlines", "[css][parse]")
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
  REQUIRE(ss.propsets.size() == 2);
  REQUIRE(ss.propsets[0].properties.size() == 2);
  REQUIRE(ss.propsets[1].properties.size() == 1);
}

TEST_CASE("Parsing CSS from string - numbers", "[css][parse]")
{
  const std::string src =
    "X {\n"
    "  def: 42; \n"
    "  ghi: -127; \n"
    "}\n";

  StyleSheet ss = parseStdString(src);
  REQUIRE(ss.propsets.size() == 1);
  REQUIRE(ss.propsets[0].properties.size() == 2);

  REQUIRE(getFirstValue(ss.propsets[0].properties[0].values) == std::string("42"));
  REQUIRE(getFirstValue(ss.propsets[0].properties[1].values) == std::string("-127"));
}

TEST_CASE("Parsing CSS from string - mixed new lines", "[css][parse]")
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
  REQUIRE(ss.propsets.size() == 2);
  REQUIRE(ss.propsets[0].properties.size() == 2);
  REQUIRE(ss.propsets[1].properties.size() == 1);
}

TEST_CASE("Parsing CSS from string - no line breaks", "[css][parse]")
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
  REQUIRE(ss.propsets.size() == 2);
  REQUIRE(ss.propsets[0].properties.size() == 2);
  REQUIRE(ss.propsets[1].properties.size() == 1);
}

TEST_CASE("Parsing CSS from string - no semicolons", "[css][parse]")
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
  REQUIRE(ss.propsets.size() == 2);
  REQUIRE(ss.propsets[0].properties.size() == 2);
  REQUIRE(ss.propsets[1].properties.size() == 1);
}

TEST_CASE("Parsing CSS from string - multiple values", "[css][parse]")
{
  const std::string src =
    "X {"
    "  abc: a, b, c, d;\n"
    "}";

  StyleSheet ss = parseStdString(src);
  REQUIRE(ss.propsets.size() == 1);
  REQUIRE(ss.propsets[0].properties.size() == 1);
  REQUIRE(getNumberOfValues(ss.propsets[0].properties[0].values) == 4);
}

TEST_CASE("Parsing CSS from string - font face declarations", "[css][parse]")
{
  const std::string src =
    "// Copyright\n"
    "@font-face { src: url('../../Assets/times.ttf'); }\n";

  StyleSheet ss = parseStdString(src);
  REQUIRE(ss.propsets.size() == 0);
  REQUIRE(ss.fontfaces.size() == 1);

  REQUIRE(ss.fontfaces[0].url == "../../Assets/times.ttf");
}

//----------------------------------------------------------------------------------------

TEST_CASE("Parsing CSS from string - URL expressions", "[css][parse][expressions]")
{
  const std::string src = "foo { bar: url('hello world'); }\n";

  StyleSheet ss = parseStdString(src);
  REQUIRE(1 == ss.propsets.size());
  REQUIRE(1 == ss.propsets[0].properties.size());
  REQUIRE(1 == getNumberOfValues(ss.propsets[0].properties[0].values));

  REQUIRE(std::string("url") == getExpr(ss.propsets[0].properties[0].values, 0).name);
  REQUIRE((std::vector<std::string>{"hello world"})
          == getExpr(ss.propsets[0].properties[0].values, 0).args);
}

TEST_CASE("Parsing CSS from string - multiple expressions per property",
          "[css][parse][expressions]")
{
  const std::string src =
    "foo { bar: rgba(123, 45, 92, 0.1), "
    "           foo(), "
    "           hsla(320, 100%, 20%, 0.3); }\n";

  StyleSheet ss = parseStdString(src);
  REQUIRE(1 == ss.propsets.size());
  REQUIRE(1 == ss.propsets[0].properties.size());

  REQUIRE(std::string("rgba") == getExpr(ss.propsets[0].properties[0].values, 0).name);
  REQUIRE((std::vector<std::string>{"123", "45", "92", "0.1"})
          == getExpr(ss.propsets[0].properties[0].values, 0).args);
  REQUIRE(std::string("foo") == getExpr(ss.propsets[0].properties[0].values, 1).name);
  REQUIRE(getExpr(ss.propsets[0].properties[0].values, 1).args.empty());

  REQUIRE(std::string("hsla") == getExpr(ss.propsets[0].properties[0].values, 2).name);
  REQUIRE((std::vector<std::string>{"320", "100%", "20%", "0.3"})
          == getExpr(ss.propsets[0].properties[0].values, 2).args);
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
