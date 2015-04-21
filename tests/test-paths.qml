// Copyright (c) 2015 Ableton AG, Berlin

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1
import QtTest 1.0

import Aqt.StyleSheets 1.1
import Aqt.Testing 1.0 as AqtTests

import Foo 1.0 as Foo

Item {
    id: scene

    /*! ensure minimum width to be larger than the minimum allowed width on
     * Windows */
    implicitWidth: 124
    /*! there are no constraints on the height, but it is convenient to have a
     *  default size */
    implicitHeight: 116


    AqtTests.MsgTracker {
        id: msgTracker
    }


    StyleEngine {
        id: styleEngine
        styleSheetSource: "paths.css"
    }

    SignalSpy {
        id: spy
        target: styleEngine
        signalName: "exception"
    }

    Component {
        id: minimalCase

        Item {
            property alias textValue: textObj.text

            property var obj: QtObject {
                StyleSet.name: "foo"
                property var value: StyleSet.props.get("text")
            }

            Rectangle {
                StyleSet.name: "root"
                anchors.fill: parent

                Text {
                    id: textObj
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: obj.value ? obj.value : "N"
                    font.pixelSize: 72
                }
            }
        }
    }

    TestCase {
        name: "styleset props to non-items warn"
        when: windowShown

        function test_basePropertyLookup() {
            msgTracker.expectMessage(AqtTests.MsgTracker.Debug,
                                     /^INFO:.*Hierarchy changes.*detected.*/);
            compare(spy.count, 0);
            AqtTests.Utils.withComponent(minimalCase, scene, {}, function(comp) {
                compare(comp.textValue, "B");
            });
            compare(spy.count, 1);
            compare(spy.signalArguments[0][0], "noParentChangeReports");
        }
    }


    //--------------------------------------------------------------------------

    Component {
        id: singletonCase

        Item {
            property alias textValue: textObj.text
            Rectangle {
                StyleSet.name: "root"
                anchors.fill: parent

                Text {
                    id: textObj
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: Foo.Bar.textValue
                    font.pixelSize: 72
                }
            }
        }
    }

    TestCase {
        name: "styleset props to singleton objects do not warn"
        when: windowShown

        function test_basePropertyLookup() {
            compare(spy.count, 0);
            AqtTests.Utils.withComponent(singletonCase, scene, {}, function(comp) {
                compare(comp.textValue, "B");
            });
            compare(spy.count, 0);
        }
    }


    //--------------------------------------------------------------------------

    Component {
        id: windowsCase

        ApplicationWindow {
            id: root
            StyleSet.name: "root"

            width: 130
            height: 120

            property var textValue: StyleSet.props.get("text")
        }
    }

    TestCase {
        name: "styleset props to windows do not warn"
        when: windowShown

        function test_basePropertyLookup() {
            compare(spy.count, 0);
            AqtTests.Utils.withComponent(windowsCase, scene, {}, function(comp) {
                compare(comp.textValue, "B");
            });
            compare(spy.count, 0);
        }
    }


    //--------------------------------------------------------------------------

    Component {
        id: setNameCase

        ApplicationWindow {
            id: root

            property alias itemHeight: bar.height

            property var foo: Item {
                id: foo
                StyleSet.name: "setnmFoo"

                property var bar: Item {
                    id: bar
                    StyleSet.name: "setnmBar"
                    height: bar.StyleSet.props.number("height")
                }
            }
        }
    }

    TestCase {
        // StyleSet.name is a normal property an is initialized by QML engine
        // some arbitrary order.  Descendants are probably already setup and got
        // their properties.  In case of setName all descendants must be
        // re-evaluated.  This example requires that both styleset names
        // "setnmFoo" and "setnmBar" are matched.
        name: "StyleSet.name will reevaluate of descendant paths"
        when: windowShown

        function test_basePropertyLookup() {
            compare(spy.count, 0);
            AqtTests.Utils.withComponent(setNameCase, scene, {}, function(comp) {
                compare(comp.itemHeight, 100);

                comp.foo.StyleSet.name = "setnmFoo2"
                compare(comp.itemHeight, 200);
            });
            compare(spy.count, 0);
        }
    }
}
