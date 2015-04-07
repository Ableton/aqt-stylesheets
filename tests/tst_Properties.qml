// Copyright (c) 2015 Ableton AG, Berlin

import QtQuick 2.3
import QtTest 1.0
import QtQuick.Layouts 1.1

import Aqt.StyleSheets 1.2
import Aqt.StyleSheets.Tests 1.0 as AqtTests

import "testUtils.js" as TestUtils

Item {
    id: scene

    /*! ensure minimum width to be larger than the minimum allowed width on
     * Windows */
    implicitWidth: 124
    /*! there are no constraints on the height, but it is convenient to have a
     *  default size */
    implicitHeight: 116


    AqtTests.TestUtils {
        id: msgTracker
    }

    StyleEngine {
        id: styleEngine
        styleSheetSource: "tst_Props.css"
    }

    Component {
        id: singleColorScene

        Item {
            property alias foo: rect.foo

            Rectangle {
                id: rect
                StyleSet.name: "root"
                anchors.fill: parent

                property var foo: StyleSet.props.color("bar")
            }
        }
    }

    TestCase {
        name: "lookup and convert a single color"
        when: windowShown

        function test_lookupAndConvertASingleColor() {
            TestUtils.withComponent(singleColorScene, scene, {}, function(comp) {
                verify(typeof(comp.foo) != "string");
                verify(Qt.colorEqual(comp.foo, "#123456"));
            });
        }
    }


    //--------------------------------------------------------------------------

    Component {
        id: multipleColorsScene

        Item {
            property alias foo: rect2.foo
            property alias foo1: rect2.foo1
            property alias exprs: rect2.exprs

            Rectangle {
                id: rect2
                StyleSet.name: "root"
                anchors.fill: parent

                property var foo: StyleSet.props.get("foo")
                property var foo1: StyleSet.props.values("foo")
                property var exprs: StyleSet.props.values("exprs")
            }
        }
    }

    TestCase {
        name: "lookup lists of colors."
        when: windowShown

        // The get() function on StyleSet returns the lookedup value as is (i.e. a list of strings)
        function test_lookupStringLists() {
            TestUtils.withComponent(multipleColorsScene, scene, {}, function(comp) {
                compare(comp.foo.length, 3);
                verify(Qt.colorEqual(comp.foo[0], "#ff0000"));
                verify(Qt.colorEqual(comp.foo[1], "#00aa00"));
                verify(Qt.colorEqual(comp.foo[2], "#000033"));
                compare(typeof(comp.foo[0]), "string");
                compare(typeof(comp.foo[1]), "string");
                compare(typeof(comp.foo[2]), "string");

                compare(comp.foo1.length, 3);
                verify(Qt.colorEqual(comp.foo1[0], "#ff0000"));
                verify(Qt.colorEqual(comp.foo1[1], "#00aa00"));
                verify(Qt.colorEqual(comp.foo1[2], "#000033"));

                compare(typeof(comp.foo1[0]), "string");
                compare(typeof(comp.foo1[1]), "string");
                compare(typeof(comp.foo1[2]), "string");
            });
        }

        function test_lookupListsOfColors() {
            TestUtils.withComponent(multipleColorsScene, scene, {}, function(comp) {
                compare(comp.foo1.length, 3);

                verify(Qt.colorEqual(comp.exprs[0], "#ff0000"));
                verify(Qt.colorEqual(comp.exprs[1], "#008000"));
                verify(Qt.colorEqual(comp.exprs[2], "#80000040"));

                verify(typeof(comp.exprs[0]) != "string");
                verify(typeof(comp.exprs[1]) != "string");
                verify(typeof(comp.exprs[2]) != "string");
            });
        }
    }
}
