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
#include <boost/variant/get.hpp>
#include <cpp-peglib/peglib.h>
RESTORE_WARNINGS

#include <cassert>
#include <fstream>
#include <ios>
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

  using namespace peg;

  int line = 0;

  Definition STYLESHEET, FONTFACE_DECL, PROPSET, SELECTORS, SELECTOR, CHILD_SEL, SEL_ID,
    VALUE_PAIRS, VALUE_PAIR, VALUES, VALUE, EXPRESSION, ARGS, ATOM_VALUE, STRING_VAL,
    COLOR_VAL, NUMBER_VAL, SYMBOL_VAL, DOT_IDENTIFIER, IDENTIFIER, STRING, COLOR, NUMBER,
    IDENT_INIT_CHAR, IDENT_CHAR, WS, END, COMMENT, BLOCK_COMMENT, LINE_COMMENT,
    END_OF_LINE, END_OF_FILE, SPACE;

  // clang-format off
  STYLESHEET      <= seq(ign(WS), zom(cho(PROPSET, FONTFACE_DECL)), END_OF_FILE);

  FONTFACE_DECL   <= seq(lit("@font-face"), ign(WS),
                         chr('{'), ign(WS), lit("src"), ign(WS), chr(':'), ign(WS),
                           lit("url"), ign(WS), chr('('), ign(WS), cho(IDENTIFIER, STRING), ign(WS), chr(')'), ign(WS),
                           opt(chr(';')), ign(WS),
                         chr('}'), ign(WS)),
                          [&stylesheet](const SemanticValues& sv) {
                            stylesheet.fontfaces.push_back({sv[0].get<std::string>()});
                          };

  PROPSET         <= seq(SELECTORS, ign(WS), chr('{'), ign(WS), VALUE_PAIRS, ign(WS), chr('}'), ign(WS)),
                          [&stylesheet](const SemanticValues& sv) {
                            PropertySpecSet set;

                            set.selectors = sv[0].get<std::vector<Selector> >();
                            set.properties = sv[1].get<std::vector<PropertySpec> >();
                            stylesheet.propsets.emplace_back(std::move(set));
                          };
  SELECTORS       <= seq(SELECTOR, zom(seq(ign(WS), chr(','), ign(WS), SELECTOR))),
                          [](const SemanticValues& sv) {
                            std::vector<Selector> sels;
                            for (const auto& val : sv) {
                              sels.emplace_back(val.get<Selector>());
                            }
                            return sels;
                          };
  SELECTOR        <= seq(SEL_ID, zom(seq(ign(WS), cho(CHILD_SEL, SEL_ID)))),
                          [](const SemanticValues& sv) {
                            Selector sel;
                            for (const auto& val : sv) {
                              sel.emplace_back(val.get<SelectorParts>());
                            }
                            return sel;
                          };
  CHILD_SEL       <= tok(chr('>')),
                          [](const SemanticValues& sv) {
                            return SelectorParts({sv.token()});
                          };
  SEL_ID          <= oom(cho(DOT_IDENTIFIER, IDENTIFIER)),
                          [](const SemanticValues& sv) {
                            SelectorParts selp;
                            for (const auto& val : sv) {
                              selp.emplace_back(val.get<std::string>());
                            }
                            return selp;
                          };

  VALUE_PAIRS     <= zom(VALUE_PAIR),
                          [](const SemanticValues& sv) {
                            std::vector<PropertySpec> specs;
                            for (const auto& val : sv) {
                              specs.emplace_back(val.get<PropertySpec>());
                            }
                            return specs;
                          };
  VALUE_PAIR      <= seq(IDENTIFIER, ign(WS), chr(':'), ign(WS), VALUES, ign(WS), opt(chr(';')), ign(WS)),
                          [&data, &line](const SemanticValues& sv) {
                            auto sl = SourceLocation(0, static_cast<int>(sv.c_str() - data.data()), line,
                                                     0); // no column info
                            return PropertySpec{
                              sv[0].get<std::string>(), sv[1].get<PropertyValues>(), std::move(sl)};
                          };
  VALUES          <= seq(VALUE, zom(seq(chr(','), ign(WS), VALUE))),
                          [](const SemanticValues& sv) {
                            PropertyValues values;
                            for (const auto& val : sv) {
                              values.emplace_back(val.get<PropertyValue>());
                            }
                            return values;
                          };

  VALUE           <= seq(cho(STRING_VAL, NUMBER_VAL, COLOR_VAL, EXPRESSION, SYMBOL_VAL), ign(WS));
  STRING_VAL      <= STRING,
                          [](const SemanticValues& sv) { return PropertyValue(sv[0].get<std::string>()); };
  NUMBER_VAL      <= NUMBER,
                          [](const SemanticValues& sv) { return PropertyValue(sv[0].get<std::string>()); };
  COLOR_VAL       <= COLOR,
                          [](const SemanticValues& sv) { return PropertyValue(sv[0].get<std::string>()); };
  SYMBOL_VAL      <= IDENTIFIER,
                          [](const SemanticValues& sv) { return PropertyValue(sv[0].get<std::string>()); };

  EXPRESSION      <= seq(IDENTIFIER, ign(WS), chr('('), ign(WS), opt(ARGS), ign(WS), chr(')')),
                          [](const SemanticValues& sv) {
                            if (sv.size() == 2) {
                              return PropertyValue(
                                Expression{sv[0].get<std::string>(), sv[1].get<std::vector<std::string> >()});
                            } else {
                              assert(sv.size() == 1);
                              return PropertyValue(Expression{sv[0].get<std::string>(), {}});
                            }
                          };
  ARGS            <= seq(ATOM_VALUE, zom(seq(chr(','), ign(WS), ATOM_VALUE))),
                          [](const SemanticValues& sv) {
                            std::vector<std::string> args;
                            for (const auto& val : sv) {
                              args.emplace_back(val.get<std::string>());
                            }
                            return args;
                          };
  ATOM_VALUE      <= seq(cho(STRING, NUMBER, COLOR, IDENTIFIER), ign(WS));

  DOT_IDENTIFIER  <= tok(seq(chr('.'), IDENT_INIT_CHAR, zom(IDENT_CHAR))),
                          [](const SemanticValues& sv) { return sv.token(); };
  IDENTIFIER      <= tok(seq(IDENT_INIT_CHAR, zom(IDENT_CHAR))),
                          [](const SemanticValues& sv) { return sv.token(); };

  COLOR           <= tok(seq(chr('#'), oom(cls("0-9a-fA-F")))),
                          [](const SemanticValues& sv) { return sv.token(); };
  NUMBER          <= tok(seq(opt(chr('-')), oom(cls("0-9")), opt(seq(chr('.'), oom(cls("0-9")))), opt(chr('%')))),
                          [](const SemanticValues& sv) { return sv.token(); };
  STRING          <= cho(seq(chr('\''), tok(zom(seq(npd(chr('\'')), dot()))), chr('\'')),
                         seq(chr('"'), tok(zom(seq(npd(chr('"')), dot()))), chr('"'))),
                          [](const SemanticValues& sv) { return sv.token(); };

  IDENT_INIT_CHAR <= cls("a-zA-Z_");
  IDENT_CHAR      <= cls("-a-zA-Z0-9_");

  WS              <= zom(cho(cls(" \t"), END_OF_LINE, COMMENT));
  END             <= cho(END_OF_LINE, END_OF_FILE);

  COMMENT         <= cho(BLOCK_COMMENT, LINE_COMMENT);
  BLOCK_COMMENT   <= seq(lit("/*"), zom(seq(npd(lit("*/")), dot())), lit("*/"));
  LINE_COMMENT    <= seq(lit("//"), zom(seq(npd(END), dot())), END);

  END_OF_LINE     <= cho(lit("\r\n"), chr('\n'), chr('\r')),
                          [&line](const SemanticValues&) { line++; };
  END_OF_FILE     <= npd(dot());
  // clang-format on

  auto retv = STYLESHEET.parse(data.data(), data.size());
  if (!retv.ret) {
    std::stringstream ss;
    if (retv.message_pos) {
      auto ln_info = peg::line_info(data.data(), retv.message_pos);
      ss << ln_info.first << ":" << ln_info.second << ": " << retv.message;
    } else {
      auto ln_info = peg::line_info(data.data(), retv.error_pos);
      ss << ln_info.first << ":" << ln_info.second << ": syntax error";
    }

    throw ParseException(ss.str());
  }

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
