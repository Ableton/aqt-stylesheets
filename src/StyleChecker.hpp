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

#pragma once

#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtQuick/QQuickItem>
RESTORE_WARNINGS

namespace aqt
{
namespace stylesheets
{

/*! Item issuing warnings if style properties are not being found
 *
 * Instantiate it somewhere in the view hierarchy. Size and location do not matter.
 *
 * @par Example
 * @code
 * ApplicationWindow {
 *   StyleChecker {}
 * }
 * @endcode
 *
 * @par Import in QML:
 * <pre>
 * import Aqt.StyleSheets 1.3
 * </pre>
 * @since 1.3
 */
class StyleChecker : public QQuickItem
{
  Q_OBJECT

  /*! Set this to false to deactivate warnings (defaults to true) */
  Q_PROPERTY(bool active READ isActive WRITE setIsActive NOTIFY activeChanged)

  /*! @cond DOXYGEN_IGNORE */

public:
  explicit StyleChecker(QQuickItem* pParent = nullptr);

  bool isActive() const;
  void setIsActive(bool isActive);

Q_SIGNALS:
  void activeChanged();

protected:
  void updatePolish() override;

private:
  bool mIsActive = false;
  /*! @endcond */
};

} // namespace stylesheets
} // namespace aqt
