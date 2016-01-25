// Copyright (c) 2015 Ableton AG, Berlin

import QtQuick 2.3
import QtTest 1.0

import Aqt.StyleSheets 1.1
import Aqt.Testing 1.0 as AqtTests

Item {
    /*! ensure minimum width to be larger than the minimum allowed width on
     * Windows */
    implicitWidth: 124
    /*! there are no constraints on the height, but it is convenient to have a
     *  default size */
    implicitHeight: 116


    StyleEngine {
        styleSheetSource: "names.css"
    }

    Component {
        id: itemComponent

        Item {
            id: a
            property alias b: b

            Item {
                id: b
                property alias c: c

                Item {
                    id: c
                    property alias d: d

                    Item {
                        id: d
                        property alias e: e
                        Item { id: e }
                    }
                }
            }
        }
    }

    TestCase {
        name: "class names change path"
        when: windowShown

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

        function test_nameChangeIsPropagatedToDescendants(a) {
            AqtTests.Utils.withComponent(itemComponent, null, {}, function(a) {
                compare(a.b.c.StyleSet.path, "QQuickItem/QQuickItem/QQuickItem");
                compare(a.b.c.d.e.StyleSet.path,
                        "QQuickItem/QQuickItem/QQuickItem/QQuickItem/QQuickItem");

                a.b.StyleSet.name = "bar";
                compare(a.b.c.StyleSet.path, "QQuickItem/QQuickItem.bar/QQuickItem");
                compare(a.b.c.d.e.StyleSet.path,
                        "QQuickItem/QQuickItem.bar/QQuickItem/QQuickItem/QQuickItem");
            });
        }
    }
}
