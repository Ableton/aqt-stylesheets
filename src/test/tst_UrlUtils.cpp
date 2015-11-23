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
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>
#include <QtCore/QFile>
#include <QtCore/QUrl>
#include <gtest/gtest.h>
RESTORE_WARNINGS

using namespace aqt::stylesheets;

namespace
{
void createFile(QString path)
{
  QFile f(path);
  ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
  f.close();
}

template <typename TestFn>
void testWithSandbox(TestFn&& testFn)
{
  QTemporaryDir sandbox;
  if (sandbox.isValid()) {
    testFn(sandbox);
  } else {
    styleSheetsLogError() << "Could not create a temporary folder for test";
  }
}
} // anon namespace

TEST(UrlUtils, resolveResourceUrl_local_to_http)
{
  EXPECT_EQ(QUrl("http://abc.org/foo.png"),
            searchForResourceSearchPath(QUrl("http://abc.org"), QUrl("./foo.png"), {}));
  EXPECT_EQ(QUrl("http://abc.org/foo.png"),
            searchForResourceSearchPath(QUrl("http://abc.org"), QUrl("foo.png"), {}));

  EXPECT_EQ(
    QUrl("http://abc.org/x/y/foo.png"),
    searchForResourceSearchPath(QUrl("http://abc.org/x/y/z"), QUrl("foo.png"), {}));
  EXPECT_EQ(
    QUrl("http://abc.org/foo.png"),
    searchForResourceSearchPath(QUrl("http://abc.org/x/y/z"), QUrl("/foo.png"), {}));
}

TEST(UrlUtils, resolveResourceUrl_http_to_http)
{
  EXPECT_EQ(QUrl("http://xyz.com/foo.png"),
            searchForResourceSearchPath(
              QUrl("http://abc.org"), QUrl("http://xyz.com/foo.png"), {}));
}

TEST(UrlUtils, resolveResourceUrl_qrc_to_http)
{
  EXPECT_EQ(QUrl("qrc:assets/icons/foo.png"),
            searchForResourceSearchPath(
              QUrl("http://abc.org"), QUrl("qrc:assets/icons/foo.png"), {}));
}

TEST(UrlUtils, resolveResourceUrl_local_relative_to_qrc)
{
  EXPECT_EQ(QUrl("qrc:/qml/assets/icons/foo.png"),
            searchForResourceSearchPath(
              QUrl("qrc:/qml/myfile.qml"), QUrl("assets/icons/foo.png"), {}));
  EXPECT_EQ(QUrl("qrc:/assets/icons/foo.png"),
            searchForResourceSearchPath(
              QUrl("qrc:/qml/myfile.qml"), QUrl("/assets/icons/foo.png"), {}));
}

TEST(UrlUtils, resolveResourceUrl_relative_local_to_local_dir)
{
  testWithSandbox([](QTemporaryDir& sandbox) {
    auto temppath = sandbox.path() + "/";

    QDir tempDir(temppath);
    tempDir.mkpath(QLatin1String("assets"));

    auto absPath = tempDir.absoluteFilePath("assets/a.css");
    createFile(absPath);

    EXPECT_EQ(QUrl::fromLocalFile(absPath),
              searchForResourceSearchPath(
                QUrl::fromLocalFile(temppath), QUrl("./assets/a.css"), {}));
    EXPECT_EQ(QUrl::fromLocalFile(absPath),
              searchForResourceSearchPath(
                QUrl::fromLocalFile(temppath), QUrl("assets/a.css"), {}));
  });
}

TEST(UrlUtils, resolveResourceUrl_relative_local_to_local_file)
{
  testWithSandbox([](QTemporaryDir& sandbox) {
    auto temppath = sandbox.path() + "/";

    QDir tempDir(temppath);
    tempDir.mkpath(QLatin1String("assets"));

    auto absPath = tempDir.absoluteFilePath("assets/a.css");
    createFile(absPath);

    auto localFilePath = QDir(temppath).absoluteFilePath("some.qml");

    EXPECT_EQ(QUrl::fromLocalFile(absPath),
              searchForResourceSearchPath(
                QUrl::fromLocalFile(localFilePath), QUrl("./assets/a.css"), {}));
    EXPECT_EQ(QUrl::fromLocalFile(absPath),
              searchForResourceSearchPath(
                QUrl::fromLocalFile(localFilePath), QUrl("assets/a.css"), {}));
  });
}

TEST(UrlUtils, resolveResourceUrl_relative_local_with_search_path)
{
  testWithSandbox([](QTemporaryDir& sandbox) {
    auto temppath = sandbox.path() + "/";

    QDir tempDir(temppath);
    tempDir.mkpath(QLatin1String("assets"));
    auto absPath = tempDir.absoluteFilePath("assets/a.css");
    createFile(absPath);

    QDir fooTempDir(temppath);
    fooTempDir.mkpath(QLatin1String("foo/assets"));
    auto fooAbsPath = fooTempDir.absoluteFilePath("foo/assets/a.css");
    createFile(fooAbsPath);

    auto searchPath = QStringList{
      tempDir.absoluteFilePath("foo/"), // this could match
      tempDir.absoluteFilePath("bar/"),
    };

    // relative path not starting with "/" is not searched in path; if that is
    // not valid "." return as-is
    EXPECT_EQ(QUrl("assets/a.css"),
              searchForResourceSearchPath(
                QUrl::fromLocalFile("."), QUrl("assets/a.css"), searchPath));

    // relative path not starting with "/" is not searched in path
    auto localFilePath = QDir(temppath).absoluteFilePath("some.qml");
    EXPECT_EQ(QUrl::fromLocalFile(absPath),
              searchForResourceSearchPath(
                QUrl::fromLocalFile(localFilePath), QUrl("assets/a.css"), searchPath));

    // relative path starting with "/" is looked up in search path, not resolved
    // against baseurl
    EXPECT_EQ(QUrl::fromLocalFile(fooAbsPath),
              searchForResourceSearchPath(
                QUrl::fromLocalFile(localFilePath), QUrl("/assets/a.css"), searchPath));
  });
}

TEST(UrlUtils, resolveResourceUrl_relative_dotdot_local_with_search_path)
{
  testWithSandbox([](QTemporaryDir& sandbox) {
    auto temppath = sandbox.path() + "/";

    QDir tempDir(temppath);
    tempDir.mkpath(QLatin1String("assets"));
    auto absPath = tempDir.absoluteFilePath("assets/a.css");
    createFile(absPath);

    QDir fooTempDir(temppath);
    fooTempDir.mkpath(QLatin1String("foo/assets"));
    auto fooAbsPath = fooTempDir.absoluteFilePath("foo/assets/a.css");
    createFile(fooAbsPath);

    auto searchPath =
      QStringList{tempDir.absoluteFilePath("foo/xyz/"), tempDir.absoluteFilePath("bar/")};

    // up-paths (../) are never resolved against search path
    EXPECT_EQ(QUrl("../assets/a.css"),
              searchForResourceSearchPath(
                QUrl::fromLocalFile("."), QUrl("../assets/a.css"), searchPath));

    auto localFilePath = QDir(temppath).absoluteFilePath("xyz/some.qml");
    EXPECT_EQ(QUrl::fromLocalFile(absPath),
              searchForResourceSearchPath(
                QUrl::fromLocalFile(localFilePath), QUrl("../assets/a.css"), searchPath));

    // relative paths starting with '/' must not contain '/../'.  They are returned as-is.
    EXPECT_EQ(QUrl(), searchForResourceSearchPath(QUrl::fromLocalFile(localFilePath),
                                                  QUrl("/../assets/a.css"), searchPath));

    // path "/" fails
    EXPECT_EQ(QUrl(), searchForResourceSearchPath(
                        QUrl::fromLocalFile(localFilePath), QUrl("/"), searchPath));
  });
}

TEST(UrlUtils, resolveResourceUrl_non_resolvable_url_is_returned_as_is)
{
  testWithSandbox([](QTemporaryDir& sandbox) {
    auto temppath = sandbox.path() + "/";

    QDir tempDir(temppath);
    auto searchPath = QStringList{tempDir.absoluteFilePath("foo/"),
                                  tempDir.absoluteFilePath("bar/"),
                                  tempDir.absolutePath()};

    EXPECT_EQ(QUrl("../assets/a.css"),
              searchForResourceSearchPath(
                QUrl::fromLocalFile("."), QUrl("../assets/a.css"), searchPath));
  });
}
