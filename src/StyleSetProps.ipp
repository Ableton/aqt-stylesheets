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

#pragma once

#include "Convert.hpp"
#include "Log.hpp"

namespace aqt
{
namespace stylesheets
{

namespace detail
{
template <typename T>
struct TypeName;

#define AQT_DEFINE_TYPENAME(_T_)                                                         \
  template <>                                                                            \
  struct TypeName<_T_> {                                                                 \
    const char* operator()() const                                                       \
    {                                                                                    \
      return #_T_;                                                                       \
    }                                                                                    \
  }

AQT_DEFINE_TYPENAME(double);
AQT_DEFINE_TYPENAME(bool);
AQT_DEFINE_TYPENAME(QFont);
AQT_DEFINE_TYPENAME(QString);
AQT_DEFINE_TYPENAME(QColor);
AQT_DEFINE_TYPENAME(QUrl);

#undef AQT_DEFINE_TYPENAME
} // namespace detail

template <typename T>
T StyleSetProps::lookupProperty(Property& def, const QString& key) const
{
  if (getImpl(def, key)) {
    if (def.mValues.size() == 1) {
      auto result = convertProperty<T>(def.mValues[0]);
      if (result) {
        return result.get();
      }
    }

    styleSheetsLogWarning() << "Property " << key.toStdString()
                            << " is not convertible to a '" << detail::TypeName<T>()()
                            << "' (" << pathToString(mPath) << ")";
  }

  return T();
}

template <typename T>
T StyleSetProps::lookupProperty(const QString& key) const
{
  Property def;
  return lookupProperty<T>(def, key);
}

} // namespace stylesheets
} // namespace aqt
