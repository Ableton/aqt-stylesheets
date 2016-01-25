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

#include "StyleChecker.hpp"

#include "StyleEngine.hpp"

namespace aqt
{
namespace stylesheets
{

StyleChecker::StyleChecker(QQuickItem* pParent)
  : QQuickItem(pParent)
{
  setIsActive(true);
}

bool StyleChecker::isActive() const
{
  return mIsActive;
}

void StyleChecker::setIsActive(bool isActive)
{
  if (mIsActive != isActive) {
    if (mIsActive) {
      disconnect(&StyleEngine::instance(), &StyleEngine::propertiesPotentiallyMissing,
                 this, &QQuickItem::polish);
    }

    mIsActive = isActive;

    if (mIsActive) {
      connect(&StyleEngine::instance(), &StyleEngine::propertiesPotentiallyMissing, this,
              &QQuickItem::polish);
      polish();
    }
  }
}

void StyleChecker::updatePolish()
{
  if (mIsActive) {
    StyleEngine::instance().checkProperties();
  }
}

} // namespace stylesheets
} // namespace aqt
