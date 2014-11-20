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

#pragma once

#include <typeinfo>

namespace aqt
{
namespace stylesheets
{

namespace detail
{

template <typename T>
struct LookupPropertyTrait {
};

template <>
struct LookupPropertyTrait<QColor> {
  static const int qTypeId = QMetaType::QColor;
  static const char* typeName()
  {
    return "color";
  }
};

template <>
struct LookupPropertyTrait<double> {
  static const int qTypeId = QMetaType::Double;
  static const char* typeName()
  {
    return "double";
  }
};

template <>
struct LookupPropertyTrait<bool> {
  static const int qTypeId = QMetaType::Bool;
  static const char* typeName()
  {
    return "bool";
  }
};

template <typename T>
struct PropertyConvertTraits {
  const char* typeName() const
  {
    return detail::LookupPropertyTrait<T>::typeName();
  }

  bool convert(T& result, QVariant& value) const
  {
    int qTypeId = detail::LookupPropertyTrait<T>::qTypeId;
    if (value.canConvert(qTypeId) && value.convert(qTypeId)) {
      result = value.value<T>();
      return true;
    }
    return false;
  }
};

} // namespace detail

template <typename T, typename Traits>
T StyleSet::lookupProperty(const QString& key, Traits traits) const
{
  QVariant value;

  if (getImpl(value, key)) {
    T result;
    if (traits.convert(result, value)) {
      return result;
    }

    qWarning("Property %s is not convertible to a %s (%s)", key.toStdString().c_str(),
             traits.typeName(), pathToString(mPath).c_str());
  }

  return T();
}

} // namespace stylesheets
} // namespace aqt
