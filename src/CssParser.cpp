/*
Copyright (c) 2014-16 Ableton AG, Berlin

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
#include "cpp-peglib/peglib.h"
#include <boost/variant/get.hpp>
RESTORE_WARNINGS

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

namespace
{

/*! @throw std::ios_base::failure for IO errors or if the file at @path
 *         could not be opened. */
std::string loadFileIntoString(const std::string& path)
{
  std::stringstream result;

  std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
  in.exceptions(std::ifstream::failbit);

  result << in.rdbuf();

  return result.str();
}

} // anon namespace

namespace aqt
{
namespace stylesheets
{

StyleSheet parseStdString(const std::string& data)
{
  StyleSheet stylesheet;

  auto syntax = R"(
    # Grammar for CSS...

    STYLESHEET      <- ( _ ( PROPSET / FONTFACEDECL / Comment ) )* _ EndOfFile
    FONTFACEDECL    <- '@font-face' _ '{' _
                           'src' _ ':' _
                           'url' _ '(' _ (IDENTIFIER / STRING) _ ')' ( _ ';')? _
                       '}'
    PROPSET         <- SELECTORS _ '{' _ VALUE_PAIRS _ '}'


    SELECTORS       <- SELECTOR ( _ ',' _ (SELECTOR / Comment) )*
    SELECTOR        <- SEL_ID ( _ (CHILD_SEL / SEL_ID ))*
    SEL_ID          <- (DOT_IDENTIFIER / IDENTIFIER)+
    CHILD_SEL       <- < '>' >

    VALUE_PAIRS     <- ((VALUE_PAIR / Comment) ( _ (VALUE_PAIR / Comment) )* )?

    VALUE_PAIR      <- IDENTIFIER _ ':' _ VALUES ( _ ';')?

    VALUES          <- VALUE ( _ ',' _ VALUE )*
    VALUE           <- STRING_VAL / NUMBER_VAL / COLOR_VAL / EXPRESSION / SYMBOL_VAL
    STRING_VAL      <- STRING
    NUMBER_VAL      <- NUMBER
    COLOR_VAL       <- COLOR
    SYMBOL_VAL      <- IDENTIFIER

    EXPRESSION      <- IDENTIFIER _ '(' (_ ARGS)? _ ')'
    ARGS            <- ATOM_VALUE ( _ ',' _ ATOM_VALUE )*
    ATOM_VALUE      <- STRING / NUMBER / COLOR / IDENTIFIER

    DOT_IDENTIFIER  <- < '.' IdentInitChar IdentChar* >
    IDENTIFIER      <- < IdentInitChar IdentChar* >

    COLOR           <- < '#' [0-9a-fA-F]+ >
    NUMBER          <- < '-'? [0-9]+ ('.' [0-9]+)? '%'? >
    STRING          <- (['] < (!['] .)* > [']) / ([\"] < (![\"] .)* > [\"])

    ~_              <-  (WhiteSpace / End)*

    WhiteSpace      <-  SpaceChar / Comment
    End             <-  EndOfLine / EndOfFile
    Comment         <-  BlockComment / LineComment
    SpaceChar       <-  ' ' / '\t'
    EndOfLine       <-  '\r\n' / '\n' / '\r'
    EndOfFile       <-  !.
    IdentInitChar   <-  [a-zA-Z_]
    IdentChar       <-  [-a-zA-Z0-9_]
    BlockComment    <-  '/*' (!'*/' .)* '*/'
    LineComment     <-  '//' (!End .)* End
  )";

  int line = 0;
  peg::parser parser;
  auto retv = parser.load_grammar(syntax);
  assert(retv);
  (void)retv;

  parser.log = [&](size_t ln, size_t col, const std::string& msg) {
    std::stringstream ss;
    ss << ln << ":" << col << ": " << msg;

    throw ParseException(ss.str());
  };

  parser["EndOfLine"] = [&line](const peg::SemanticValues&) { line++; };

  parser["FONTFACEDECL"] = [&stylesheet](const peg::SemanticValues& sv) {
    stylesheet.fontfaces.push_back({sv[0].get<std::string>()});
  };

  parser["PROPSET"] = [&stylesheet](const peg::SemanticValues& sv) {
    PropertySpecSet set;
    set.selectors = sv[0].get<std::vector<Selector> >();
    set.properties = sv[1].get<std::vector<PropertySpec> >();
    stylesheet.propsets.emplace_back(std::move(set));
  };

  parser["SELECTORS"] = [](const peg::SemanticValues& sv) {
    std::vector<Selector> sels;
    for (const auto& val : sv) {
      sels.emplace_back(val.get<Selector>());
    }
    return sels;
  };

  parser["SELECTOR"] = [](const peg::SemanticValues& sv) {
    Selector sel;
    for (const auto& val : sv) {
      sel.emplace_back(val.get<SelectorParts>());
    }
    return sel;
  };

  parser["SEL_ID"] = [](const peg::SemanticValues& sv) {
    SelectorParts selp;
    for (const auto& val : sv) {
      selp.emplace_back(val.get<std::string>());
    }
    return selp;
  };

  parser["CHILD_SEL"] =
    [](const peg::SemanticValues& sv) { return SelectorParts({sv.token()}); };

  parser["VALUE_PAIRS"] = [](const peg::SemanticValues& sv) {
    std::vector<PropertySpec> specs;
    for (const auto& val : sv) {
      specs.emplace_back(val.get<PropertySpec>());
    }
    return specs;
  };

  parser["VALUE_PAIR"] = [&data, &line](const peg::SemanticValues& sv) {
    auto sl = SourceLocation(0, static_cast<int>(sv.c_str() - data.data()), line,
                             0); // no column info
    return PropertySpec{
      sv[0].get<std::string>(), sv[1].get<PropertyValues>(), std::move(sl)};
  };

  parser["VALUES"] = [](const peg::SemanticValues& sv) {
    PropertyValues values;
    for (const auto& val : sv) {
      values.emplace_back(val.get<PropertyValue>());
    }
    return values;
  };

  parser["EXPRESSION"] = [](const peg::SemanticValues& sv) {
    if (sv.size() == 2) {
      return PropertyValue(
        Expression{sv[0].get<std::string>(), sv[1].get<std::vector<std::string> >()});
    } else {
      assert(sv.size() == 1);
      return PropertyValue(Expression{sv[0].get<std::string>(), {}});
    }
  };

  parser["ARGS"] = [](const peg::SemanticValues& sv) {
    std::vector<std::string> args;
    for (const auto& val : sv) {
      args.emplace_back(val.get<std::string>());
    }
    return args;
  };

  auto str2propVal =
    [](const peg::SemanticValues& sv) { return PropertyValue(sv[0].get<std::string>()); };
  parser["STRING_VAL"] = str2propVal;
  parser["NUMBER_VAL"] = str2propVal;
  parser["COLOR_VAL"] = str2propVal;
  parser["SYMBOL_VAL"] = str2propVal;

  auto svToken = [](const peg::SemanticValues& sv) { return sv.token(); };
  parser["STRING"] = svToken;
  parser["NUMBER"] = svToken;
  parser["COLOR"] = svToken;
  parser["IDENTIFIER"] = svToken;
  parser["DOT_IDENTIFIER"] = svToken;

  // Enable packrat parsing.  NOTE(gck) For the CSS syntax above this doesn't
  // make sense; without it parsing is much faster.
  //parser.enable_packrat_parsing();

  parser.parse_n(data.data(), data.size());

  return stylesheet;
}

StyleSheet parseString(const QString& data)
{
  return parseStdString(data.toStdString());
}

StyleSheet parseStyleFile(const QString& path)
{
  return parseStdString(loadFileIntoString(path.toStdString()));
}

} // namespace stylesheets
} // namespace aqt
