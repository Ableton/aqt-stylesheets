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

#include "UrlUtils.hpp"

#include "Log.hpp"
#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QRegularExpression>
RESTORE_WARNINGS

#include <vector>

namespace aqt
{
namespace stylesheets
{

QUrl searchForResourceSearchPath(const QUrl& baseUrl,
                                 const QUrl& url,
                                 const QStringList& searchPath)
{
  if (url.isRelative()) {
    if (!baseUrl.isLocalFile()) {
      return baseUrl.resolved(url);
    }

    auto path = url.path();
    if (!path.startsWith("/")) {
      auto resolvedUrl = baseUrl.resolved(url);
      if (QFile::exists(resolvedUrl.toLocalFile())) {
        return resolvedUrl;
      }
    } else if (!path.contains("/../")) {
      auto pathRelPart = path.split(QRegularExpression("^/+"))[1];
      for (const auto& str : searchPath) {
        auto dir = QDir(str);
        auto absPath = QDir::cleanPath(dir.absoluteFilePath(pathRelPart));

        if (QFile::exists(absPath)) {
          return QUrl::fromLocalFile(absPath);
        }
      }

      return QUrl();
    } else {
      return QUrl();
    }
  }

  return url;
}

} // namespace stylesheets
} // namespace aqt
