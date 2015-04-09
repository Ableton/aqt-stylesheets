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

#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <QtCore/QStringList>
#include <QtCore/QUrl>
RESTORE_WARNINGS

namespace aqt
{
namespace stylesheets
{
/*! Resolve @p url against @p baseUrl or search for it in @p searchPath.
 *
 * If @p url is an absolute url @p url is returned as-is.
 *
 * If @p baseUrl is not a local file url resolve @p url relative to it.  In this
 * case @p searchPath is ignored.
 *
 * If @p baseUrl is a local file url @p url is resolved depending on @p urls
 * form.
 * If @p url starts with a "/" (i.e. is an absolute "relative" path) the path is
 * tested as relative path to the local file directories in @p searchPath in the
 * order given.  The first path existing as file on the local file system is
 * returned as local file url.
 *
 * @pre Example
 * @code
 * searchPath: ["foo", "bar"] & url: "x/y/z"
 * tested paths:
 *   foo/x/y/z
 *   bar/x/y/z
 * @endcode
 *
 * If no path tested matches the function returns an invalid url.
 *
 * @p{url}s starting with "/" must not contain ".." (and invalid url will be
 * returned otherwise).
 *
 * If @p url starts with any other character than "/" (e.g. a letter or ".") the
 * path is resolved relatived to @p baseUrl *and* tested whether the file exists
 * on the local file system.  Otherwise an invalid url is returned.
 */
QUrl searchForResourceSearchPath(const QUrl& baseUrl,
                                 const QUrl& url,
                                 const QStringList& searchPath);

} // namespace stylesheets
} // namespace aqt
