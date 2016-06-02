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

#pragma once

#include "Property.hpp"
#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtGui/QColor>
#include <QtGui/QFont>
RESTORE_WARNINGS

#if defined(ABL_HAVE_STD_OPTIONAL)
#  include <optional>
#else
#  include <estd/optional.hpp>
#endif
#include <string>


namespace aqt
{
namespace stylesheets
{

/*! Exception thrown whenever a conversion fails */
class ConvertException
{
public:
  ConvertException(std::string msg)
    : mMsg(std::move(msg))
  {
  }

  std::string what() const
  {
    return mMsg;
  }

  std::string mMsg;
};


/*! @cond DOXYGEN_IGNORE */
template <typename T>
struct PropertyValueConvertTraits;

template <>
struct PropertyValueConvertTraits<QFont> {
  std::optional<QFont> convert(const PropertyValue& value) const;
};

template <>
struct PropertyValueConvertTraits<QColor> {
  std::optional<QColor> convert(const PropertyValue& value) const;
};

template <>
struct PropertyValueConvertTraits<QString> {
  std::optional<QString> convert(const PropertyValue& value) const;
};

template <>
struct PropertyValueConvertTraits<double> {
  std::optional<double> convert(const PropertyValue& value) const;
};

template <>
struct PropertyValueConvertTraits<bool> {
  std::optional<bool> convert(const PropertyValue& value) const;
};

template <>
struct PropertyValueConvertTraits<QUrl> {
  std::optional<QUrl> convert(const PropertyValue& value) const;
};

template <typename T, typename Traits = PropertyValueConvertTraits<T>>
std::optional<T> convertProperty(const PropertyValue& value, Traits traits = Traits())
{
  return traits.convert(value);
}

QVariant convertValueToVariant(const PropertyValue& value);
QVariantList convertValueToVariantList(const PropertyValues& values);

} // namespace stylesheets
} // namespace aqt
