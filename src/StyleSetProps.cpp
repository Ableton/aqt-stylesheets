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

#include "StyleSetProps.hpp"

#include "Convert.hpp"
#include "Log.hpp"
#include "Property.hpp"
#include "StyleEngine.hpp"

#include <algorithm>

namespace aqt
{
namespace stylesheets
{

namespace
{

PropertyMap* nullProperties()
{
  static PropertyMap sNullPropertyMap;
  return &sNullPropertyMap;
}

} // anon namespace

StyleSetProps::StyleSetProps(const UiItemPath& path)
  : mPath(path)
  , mpProperties(nullProperties())
{
  loadProperties();
}

bool StyleSetProps::isValid() const
{
  return !mpProperties->empty();
}

bool StyleSetProps::isSet(const QString& key) const
{
  return mpProperties->find(key) != mpProperties->end();
}

bool StyleSetProps::getImpl(Property& prop, const QString& key) const
{
  PropertyMap::const_iterator it = mpProperties->find(key);
  if (it != mpProperties->end()) {
    prop = it->second;
    return true;
  }

  mMissingProps.insert(key);
  StyleEngine::instance().setMissingPropertiesFound();

  return false;
}

QVariant StyleSetProps::get(const QString& key) const
{
  Property prop;
  getImpl(prop, key);

  if (prop.mValues.size() == 1) {
    try {
      auto conv = convertProperty<QString>(prop.mValues[0]);
      if (conv) {
        return QVariant::fromValue(*conv);
      }
    } catch (ConvertException& e) {
      styleSheetsLogWarning() << e.what();
    }
  } else if (prop.mValues.size() > 1) {
    QVariantList result;
    for (const auto& propValue : prop.mValues) {
      try {
        auto conv = convertProperty<QString>(propValue);
        if (conv) {
          result.push_back(conv.get());
        }
      } catch (ConvertException& e) {
        styleSheetsLogWarning() << e.what();
      }
    }

    return result;
  }

  return QVariant();
}

QVariant StyleSetProps::values(const QString& key) const
{
  Property prop;
  getImpl(prop, key);

  try {
    if (prop.mValues.size() == 1) {
      return convertValueToVariant(prop.mValues[0]);
    }

    return convertValueToVariantList(prop.mValues);
  } catch (ConvertException& e) {
    styleSheetsLogWarning() << e.what();
  }

  return QVariant();
}

QColor StyleSetProps::color(const QString& key) const
{
  return lookupProperty<QColor>(key);
}

QFont StyleSetProps::font(const QString& key) const
{
  return lookupProperty<QFont>(key);
}

double StyleSetProps::number(const QString& key) const
{
  return lookupProperty<double>(key);
}

bool StyleSetProps::boolean(const QString& key) const
{
  return lookupProperty<bool>(key);
}

QString StyleSetProps::string(const QString& key) const
{
  return lookupProperty<QString>(key);
}

QUrl StyleSetProps::url(const QString& key) const
{
  Property prop;
  auto url = lookupProperty<QUrl>(prop, key);

  auto& engine = StyleEngine::instance();

  auto baseUrl = prop.mSourceLoc.mSourceLayer == 0 ? engine.defaultStyleSheetSource()
                                                   : engine.styleSheetSource();
  return engine.resolveResourceUrl(baseUrl, url);
}

void StyleSetProps::loadProperties()
{
  mMissingProps.clear();
  mpProperties = StyleEngine::instance().properties(mPath);
  Q_EMIT propsChanged();
}

void StyleSetProps::invalidate()
{
  mMissingProps.clear();
  mpProperties = nullProperties();
}

void StyleSetProps::checkProperties() const
{
  for (const auto& key : mMissingProps) {
    styleSheetsLogWarning() << "Property " << key.toStdString() << " not found ("
                            << pathToString(mPath) << ")";
    Q_EMIT StyleEngine::instance().exception(
      QString::fromLatin1("propertyNotFound"),
      QString::fromLatin1("Property '%1' not found (%2)")
        .arg(key, QString::fromStdString(pathToString(mPath))));
  }

  mMissingProps.clear();
}

StyleSetPropsRef::StyleSetPropsRef()
  : StyleSetPropsRef{nullptr}
{
}

StyleSetPropsRef::StyleSetPropsRef(const std::shared_ptr<UsageCountedStyleSetProps>& pUsageCountedStyleSetProps)
  : mpUsageCountedStyleSetProps{pUsageCountedStyleSetProps}
{
  if (auto pStyleSetProps = mpUsageCountedStyleSetProps.lock()) {
    ++pStyleSetProps->usageCount;
  }
}

StyleSetPropsRef::~StyleSetPropsRef()
{
  if (auto pStyleSetProps = mpUsageCountedStyleSetProps.lock()) {
    --pStyleSetProps->usageCount;
  }
}

StyleSetPropsRef::StyleSetPropsRef(const StyleSetPropsRef& other)
  : mpUsageCountedStyleSetProps{other.mpUsageCountedStyleSetProps}
{
  if (auto pStyleSetProps = mpUsageCountedStyleSetProps.lock()) {
    ++pStyleSetProps->usageCount;
  }
}

StyleSetPropsRef& StyleSetPropsRef::operator=(StyleSetPropsRef other)
{
  swap(*this, other);
  return *this;
}

size_t StyleSetPropsRef::usageCount() const
{
  if (auto pStyleSetProps = mpUsageCountedStyleSetProps.lock()) {
    return pStyleSetProps->usageCount;
  }
  return 0;
}

StyleSetProps* StyleSetPropsRef::get()
{
  if (auto pStyleSetProps = mpUsageCountedStyleSetProps.lock()) {
    return &pStyleSetProps->styleSetProps;
  }
  return nullptr;
}

void swap(StyleSetPropsRef& a, StyleSetPropsRef& b)
{
  using std::swap;
  swap(a.mpUsageCountedStyleSetProps, b.mpUsageCountedStyleSetProps);
}

} // namespace stylesheets
} // namespace aqt
