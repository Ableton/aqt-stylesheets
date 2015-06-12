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
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_line_pos_iterator.hpp>
RESTORE_WARNINGS

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

// this must be outside of the anon namespace
// clang-format off
BOOST_FUSION_ADAPT_STRUCT(
  aqt::stylesheets::PropertySpec,
  (std::string, name)
  (aqt::stylesheets::PropertyValues, values)
  )

BOOST_FUSION_ADAPT_STRUCT(
  aqt::stylesheets::PropertySpecSet,
  (std::vector<aqt::stylesheets::Selector>, selectors)
  (std::vector<aqt::stylesheets::PropertySpec>, properties)
  )

BOOST_FUSION_ADAPT_STRUCT(
  aqt::stylesheets::FontFaceDecl,
  (std::string, url)
  )

BOOST_FUSION_ADAPT_STRUCT(
  aqt::stylesheets::StyleSheet,
  (std::vector<aqt::stylesheets::PropertySpecSet>, propsets)
  (std::vector<aqt::stylesheets::FontFaceDecl>, fontfaces)
  )

BOOST_FUSION_ADAPT_STRUCT(
  aqt::stylesheets::Expression,
  (std::string, name)
  (std::vector<std::string>, args)
  )
// clang-format on

namespace
{

namespace phoenix = boost::phoenix;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

template <typename It>
class LocationAnnotator
{
  const It mFirst;

public:
  // to make phoenix::function happy
  template <typename>
  struct result {
    using type = void;
  };

  LocationAnnotator(It first)
    : mFirst(first)
  {
  }

  template <typename ThingWithLocation>
  void operator()(ThingWithLocation& thing, It iter) const
  {
    thing.mSourceLoc.mByteOfs = static_cast<int>(std::distance(mFirst, iter));
    thing.mSourceLoc.mLine = static_cast<int>(get_line(iter));

    // The column offset is always off by one. Why? It is a mystery.
    thing.mSourceLoc.mColumn = static_cast<int>(get_column(mFirst, iter)) - 1;
  }
};

template <typename Iterator>
struct StyleSheetGrammar
  : qi::grammar<Iterator, aqt::stylesheets::StyleSheet(), ascii::space_type> {
  StyleSheetGrammar(Iterator first)
    : StyleSheetGrammar::base_type(stylesheet, "stylesheet")
    , locationAnnotator(first)
  {
    using qi::lit;
    using qi::lexeme;
    using qi::omit;
    using ascii::char_;
    using ascii::string;
    using namespace qi::labels;
    using phoenix::at_c;
    using phoenix::push_back;
    using boost::spirit::eol;

    // clang-format off
    cpp_comment     = "//" >> omit[*(char_ - eol) >> eol];
    c_comment       = "/*" >> omit[*(char_ - "*/")] >> "*/";
    comment         = cpp_comment | c_comment;

    identifier      = +(qi::alnum | char_('-'));
    dot_identifier  = char_(".") >> identifier;
    child_sel       = char_(">");
    sel_separator   = char_(",");
    sel_part        = +(dot_identifier | identifier)[push_back(_val, _1)];
    child_sel_part  = (child_sel)[push_back(_val, _1)];
    selector        = +(sel_part | child_sel_part)[push_back(_val, _1)];

    quoted_string   = lexeme['"' >> +(char_ - '"') >> '"']
                      | lexeme['\'' >> +(char_ - '\'') >> '\''];
    color           = char_('#') >> +(qi::xdigit);
    number          = +(qi::digit | char_('-') | char_('.')) >> *char_('%');

    atom_value       = quoted_string
                      | number
                      | color
                      | identifier;

    args            = atom_value [push_back(_val, _1)]
                      >> *(lit(',') > atom_value [push_back(_val, _1)]);
    expression      = identifier[at_c<0>(_val) = _1]
                      >> lit('(') >> *(args[at_c<1>(_val) = _1]) >> lit(')');

    value           = quoted_string
                      | number
                      | color
                      | expression
                      | identifier;
    values          = value [push_back(_val, _1)]
                      >> *(lit(',') > value [push_back(_val, _1)]);

    value_pair      = identifier[at_c<0>(_val) = _1]
                      >> lit(':') >> values[at_c<1>(_val) = _1]
                      >> -lit(';');

    propset         = selector [push_back(at_c<0>(_val), _1)]
                      >> *((sel_separator >> selector[push_back(at_c<0>(_val), _1)])
                            | comment)
                      >> '{'
                      >> *(value_pair [push_back(at_c<1>(_val), _1)]
                           | comment)
                      >> '}';

    fontfacedecl    = "@font-face" >> -comment
                      >> lit('{') >> -comment
                      >> lit("src") >> lit(':') >> -comment
                      >> lit("url") >> lit('(')
                                    >> (identifier | quoted_string)[at_c<0>(_val) = _1]
                      >> lit(')') >> -lit(';') >> -comment
                      >> lit('}');

    stylesheet      = *(propset[push_back(at_c<0>(_val), _1)]
                        | fontfacedecl[push_back(at_c<1>(_val), _1)]
                        | comment);
    // clang-format on

    // give proper names to the rules for error printing
    c_comment.name("comment");
    color.name("color");
    comment.name("comment");
    cpp_comment.name("comment");
    fontfacedecl.name("fontface");
    identifier.name("identifier");
    dot_identifier.name("dot_identifier");
    propset.name("propset");
    quoted_string.name("string");
    stylesheet.name("stylesheet");
    sel_separator.name("selector separator");
    sel_part.name("selector part");
    selector.name("selector");
    values.name("value");
    number.name("number");
    atom_value.name("atomic value");
    value.name("value");
    value_pair.name("key-value-pair");
    child_sel.name("child");
    args.name("args");
    expression.name("expression");

    on_success(propset, locationAnnotator(_val, _1));
    on_success(value_pair, locationAnnotator(_val, _1));
  }

  phoenix::function<LocationAnnotator<Iterator>> locationAnnotator;

  qi::rule<Iterator, std::string()> identifier;
  qi::rule<Iterator, std::string()> dot_identifier;
  qi::rule<Iterator, std::string()> sel_separator;
  qi::rule<Iterator, std::string()> child_sel;
  qi::rule<Iterator, aqt::stylesheets::SelectorParts()> child_sel_part;
  qi::rule<Iterator, aqt::stylesheets::SelectorParts()> sel_part;
  qi::rule<Iterator, aqt::stylesheets::Selector(), ascii::space_type> selector;
  qi::rule<Iterator, std::string()> color;
  qi::rule<Iterator, std::string()> number;

  qi::rule<Iterator, std::string(), ascii::space_type> quoted_string;
  qi::rule<Iterator, aqt::stylesheets::PropertySpec(), ascii::space_type> value_pair;
  qi::rule<Iterator, std::string(), ascii::space_type> atom_value;
  qi::rule<Iterator, aqt::stylesheets::PropertyValue(), ascii::space_type> value;
  qi::rule<Iterator, aqt::stylesheets::PropertyValues(), ascii::space_type> values;
  qi::rule<Iterator, std::vector<std::string>(), ascii::space_type> args;
  qi::rule<Iterator, aqt::stylesheets::PropertySpecSet(), ascii::space_type> propset;
  qi::rule<Iterator, aqt::stylesheets::FontFaceDecl(), ascii::space_type> fontfacedecl;
  qi::rule<Iterator, aqt::stylesheets::StyleSheet(), ascii::space_type> stylesheet;
  qi::rule<Iterator, aqt::stylesheets::Expression(), ascii::space_type> expression;

  qi::rule<Iterator, boost::spirit::unused_type> cpp_comment;
  qi::rule<Iterator, boost::spirit::unused_type> c_comment;
  qi::rule<Iterator, boost::spirit::unused_type> comment;
};

} // anon namespace

namespace aqt
{
namespace stylesheets
{

StyleSheet parseStdString(const std::string& data)
{
  StyleSheet stylesheet;

  using source_iterator = boost::spirit::line_pos_iterator<std::string::const_iterator>;
  using StrStyleSheetGrammar = StyleSheetGrammar<source_iterator>;

  auto iter = source_iterator{data.begin()};
  auto end = source_iterator{data.end()};
  StrStyleSheetGrammar styleGrammar(iter);

  bool retval =
    phrase_parse(iter, end, styleGrammar, boost::spirit::ascii::space, stylesheet);

  if (retval && iter == end) {
    return stylesheet;
  }

  if (iter != end) {
    auto errorOfs = std::distance(source_iterator{data.begin()}, iter);
    auto restLen = std::distance(iter, end);

    auto errorText = restLen > 128 ? data.substr(size_t(errorOfs), 128) + "..."
                                   : data.substr(size_t(errorOfs), std::string::npos);

    throw ParseException("Found unexpected tokens", errorText);
  } else {
    throw ParseException("Unknown error");
  }
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
