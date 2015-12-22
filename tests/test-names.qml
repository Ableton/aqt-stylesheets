// Copyright (c) 2015 Ableton AG, Berlin

import QtQuick 2.3
import QtTest 1.0

import Aqt.StyleSheets 1.1
import Aqt.Testing 1.0 as AqtTests

Item {
    StyleEngine {
        styleSheetSource: "names.css"
    }

    Component {
        id: itemComponent
        Item {}
    }

    TestCase {
        name: "class names change path"

        function test_singleClassName() {
            AqtTests.Utils.withComponent(itemComponent, null, {}, function(item) {
                item.StyleSet.name = "foo";
                compare(item.StyleSet.path, "QQuickItem.foo");
            });
        }

        function test_multipleClassNames() {
            AqtTests.Utils.withComponent(itemComponent, null, {}, function(item) {
                item.StyleSet.name = "foo bar baz";
                compare(item.StyleSet.path, "QQuickItem.{foo,bar,baz}");
            });
        }

        function test_redundantWhitespaceIsIgnored() {
            AqtTests.Utils.withComponent(itemComponent, null, {}, function(item) {
                item.StyleSet.name = "bar  foo ";
                compare(item.StyleSet.path, "QQuickItem.{bar,foo}");
            });
        }
    }
}
