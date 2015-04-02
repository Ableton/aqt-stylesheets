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

#include "Convert.hpp"

namespace aqt
{
namespace stylesheets
{

namespace detail {
template<typename T>
struct TypeName;

template<>
struct TypeName<double>
{
  const char* operator()() const { return "dobule"; }
};

template<>
struct TypeName<bool>
{
  const char* operator()() const { return "bool"; }
};

template<>
struct TypeName<QFont>
{
  const char* operator()() const { return "font"; }
};

template<>
struct TypeName<QString>
{
  const char* operator()() const { return "string"; }
};

template<>
struct TypeName<QColor>
{
  const char* operator()() const { return "color"; }
};

} // detail namespace


template <typename T>
T StyleSet::lookupProperty(const QString& key) const
{
  PropValues values;

  if (getImpl(values, key)) {
    if (values.size() == 1) {
      auto result = convertProperty<T>(values[0]);
      if (result) {
        return result.get();
      }
    }

    qWarning("Property %s is not convertible to a %s (%s)", key.toStdString().c_str(),
             detail::TypeName<T>()(),
             pathToString(mPath).c_str());
  }

  return T();
}

} // namespace stylesheets
} // namespace aqt
