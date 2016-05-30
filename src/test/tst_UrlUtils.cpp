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
#include <catch/catch.hpp>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>
#include <QtCore/QFile>
#include <QtCore/QUrl>
RESTORE_WARNINGS

using namespace aqt::stylesheets;

namespace
{
void createFile(QString path)
{
  QFile f(path);
  CHECK(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
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

TEST_CASE("Resolving URLs - local to http", "[url]")
{
  REQUIRE(QUrl("http://abc.org/foo.png")
          == searchForResourceSearchPath(QUrl("http://abc.org"), QUrl("./foo.png"), {}));
  REQUIRE(QUrl("http://abc.org/foo.png")
          == searchForResourceSearchPath(QUrl("http://abc.org"), QUrl("foo.png"), {}));

  REQUIRE(
    QUrl("http://abc.org/x/y/foo.png")
    == searchForResourceSearchPath(QUrl("http://abc.org/x/y/z"), QUrl("foo.png"), {}));
  REQUIRE(
    QUrl("http://abc.org/foo.png")
    == searchForResourceSearchPath(QUrl("http://abc.org/x/y/z"), QUrl("/foo.png"), {}));
}

TEST_CASE("Resolving URLs - http to http", "[url]")
{
  REQUIRE(QUrl("http://xyz.com/foo.png")
          == searchForResourceSearchPath(
               QUrl("http://abc.org"), QUrl("http://xyz.com/foo.png"), {}));
}

TEST_CASE("Resolving URLs - qrc to http", "[url]")
{
  REQUIRE(QUrl("qrc:assets/icons/foo.png")
          == searchForResourceSearchPath(
               QUrl("http://abc.org"), QUrl("qrc:assets/icons/foo.png"), {}));
}

TEST_CASE("Resolving URLs - local relative to qrc", "[url]")
{
  REQUIRE(QUrl("qrc:/qml/assets/icons/foo.png")
          == searchForResourceSearchPath(
               QUrl("qrc:/qml/myfile.qml"), QUrl("assets/icons/foo.png"), {}));
  REQUIRE(QUrl("qrc:/assets/icons/foo.png")
          == searchForResourceSearchPath(
               QUrl("qrc:/qml/myfile.qml"), QUrl("/assets/icons/foo.png"), {}));
}

TEST_CASE("Resolving URLs - relative local to local dir", "[url]")
{
  testWithSandbox([](QTemporaryDir& sandbox) {
    auto temppath = sandbox.path() + "/";

    QDir tempDir(temppath);
    tempDir.mkpath(QLatin1String("assets"));

    auto absPath = tempDir.absoluteFilePath("assets/a.css");
    createFile(absPath);

    REQUIRE(QUrl::fromLocalFile(absPath)
            == searchForResourceSearchPath(
                 QUrl::fromLocalFile(temppath), QUrl("./assets/a.css"), {}));
    REQUIRE(QUrl::fromLocalFile(absPath)
            == searchForResourceSearchPath(
                 QUrl::fromLocalFile(temppath), QUrl("assets/a.css"), {}));
  });
}

TEST_CASE("Resolving URLs - relative local to local file", "[url]")
{
  testWithSandbox([](QTemporaryDir& sandbox) {
    auto temppath = sandbox.path() + "/";

    QDir tempDir(temppath);
    tempDir.mkpath(QLatin1String("assets"));

    auto absPath = tempDir.absoluteFilePath("assets/a.css");
    createFile(absPath);

    auto localFilePath = QDir(temppath).absoluteFilePath("some.qml");

    REQUIRE(QUrl::fromLocalFile(absPath)
            == searchForResourceSearchPath(
                 QUrl::fromLocalFile(localFilePath), QUrl("./assets/a.css"), {}));
    REQUIRE(QUrl::fromLocalFile(absPath)
            == searchForResourceSearchPath(
                 QUrl::fromLocalFile(localFilePath), QUrl("assets/a.css"), {}));
  });
}

TEST_CASE("Resolving URLs - relative local with search path")
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
    REQUIRE(QUrl("assets/a.css")
            == searchForResourceSearchPath(
                 QUrl::fromLocalFile("."), QUrl("assets/a.css"), searchPath));

    // relative path not starting with "/" is not searched in path
    auto localFilePath = QDir(temppath).absoluteFilePath("some.qml");
    REQUIRE(QUrl::fromLocalFile(absPath)
            == searchForResourceSearchPath(
                 QUrl::fromLocalFile(localFilePath), QUrl("assets/a.css"), searchPath));

    // relative path starting with "/" is looked up in search path, not resolved
    // against baseurl
    REQUIRE(QUrl::fromLocalFile(fooAbsPath)
            == searchForResourceSearchPath(
                 QUrl::fromLocalFile(localFilePath), QUrl("/assets/a.css"), searchPath));
  });
}

TEST_CASE("Resolving URLs - relative .. local with search path", "[url]")
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
    REQUIRE(QUrl("../assets/a.css")
            == searchForResourceSearchPath(
                 QUrl::fromLocalFile("."), QUrl("../assets/a.css"), searchPath));

    auto localFilePath = QDir(temppath).absoluteFilePath("xyz/some.qml");
    REQUIRE(QUrl::fromLocalFile(absPath)
            == searchForResourceSearchPath(QUrl::fromLocalFile(localFilePath),
                                           QUrl("../assets/a.css"), searchPath));

    // relative paths starting with '/' must not contain '/../'.  They are returned as-is.
    REQUIRE(QUrl() == searchForResourceSearchPath(QUrl::fromLocalFile(localFilePath),
                                                  QUrl("/../assets/a.css"), searchPath));

    // path "/" fails
    REQUIRE(QUrl() == searchForResourceSearchPath(
                        QUrl::fromLocalFile(localFilePath), QUrl("/"), searchPath));
  });
}

TEST_CASE("Resolving URLs - a non resolvable URL is returned as is", "[url]")
{
  testWithSandbox([](QTemporaryDir& sandbox) {
    auto temppath = sandbox.path() + "/";

    QDir tempDir(temppath);
    auto searchPath = QStringList{tempDir.absoluteFilePath("foo/"),
                                  tempDir.absoluteFilePath("bar/"),
                                  tempDir.absolutePath()};

    REQUIRE(QUrl("../assets/a.css")
            == searchForResourceSearchPath(
                 QUrl::fromLocalFile("."), QUrl("../assets/a.css"), searchPath));
  });
}
