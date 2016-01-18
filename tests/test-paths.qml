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
        id: parentHierarchyCase

        ApplicationWindow {
            property alias a: a
            property alias c: c

            Foo.A {
                id: a
                property alias b: ab
                property alias listView: listView
                property alias flickable: flickable
                property alias loader: loader
                property alias instantiator: instantiator

                Foo.B {
                    id: ab
                    property alias c: abc

                    Foo.C { id: abc }
                }

                ListView {
                    id: listView
                    model: 1
                    delegate: Foo.B {}
                }

                Flickable {
                    id: flickable
                    property alias a: flickable_a

                    Foo.A { id: flickable_a }
                }

                Loader {
                    id: loader
                    sourceComponent: Foo.C {}
                }

                Instantiator {
                    id: instantiator
                    delegate: Foo.A {}
                }
            }

            Foo.C {
                id: c
                property alias a: ca
                property alias b: cb

                Foo.A { id: ca }

                Foo.B {
                    id: cb
                    property alias c: cbc

                    Foo.C { id: cbc }
                }
            }
        }
    }

    TestCase {
        name: "build path by walking parentItem and parent hierarchy"
        when: windowShown

        function test_simpleHierarchy() {
            AqtTests.Utils.withComponent(parentHierarchyCase, null, {}, function(w) {
                compare(w.a.b.c.StyleSet.path,
                        "ApplicationWindow/A/B/C");
            });
        }

        function test_hierarchyWithListView() {
            AqtTests.Utils.withComponent(parentHierarchyCase, null, {}, function(w) {
                compare(w.a.listView.currentItem.StyleSet.path,
                        "ApplicationWindow/A/QQuickListView/QQuickItem/B");
            });
        }

        function test_hierarchyWithFlickable() {
            AqtTests.Utils.withComponent(parentHierarchyCase, null, {}, function(w) {
                compare(w.a.flickable.a.StyleSet.path,
                        "ApplicationWindow/A/QQuickFlickable/QQuickItem/A");
            });
        }

        function test_hierarchyWithLoader() {
            AqtTests.Utils.withComponent(parentHierarchyCase, null, {}, function(w) {
                compare(w.a.loader.item.StyleSet.path,
                        "ApplicationWindow/A/QQuickLoader/C");
            });
        }

        function test_hierarchyWithInstantiator() {
            AqtTests.Utils.withComponent(parentHierarchyCase, null, {}, function(w) {
                compare(w.a.instantiator.object.StyleSet.path,
                        "ApplicationWindow/A/QQmlInstantiator/A");
            });
        }

        function test_reparentedItem() {
            AqtTests.Utils.withComponent(parentHierarchyCase, null, {}, function(w) {
                // Force side effect of building the path before reparenting.
                compare(w.c.b.StyleSet.path, "ApplicationWindow/C/B");

                w.c.b.parent = w.c.a;
                compare(w.c.b.StyleSet.path, "ApplicationWindow/C/A/B");
            });
        }

        function test_childOfReparentedItem_StyleSetCreatedBeforeReparenting() {
            AqtTests.Utils.withComponent(parentHierarchyCase, null, {}, function(w) {
                // Access StyleSet to create it
                w.c.b.StyleSet.path;

                // Force side effect of building the path before reparenting the parent.
                compare(w.c.b.c.StyleSet.path, "ApplicationWindow/C/B/C");

                w.c.b.parent = w.c.a;
                compare(w.c.b.c.StyleSet.path, "ApplicationWindow/C/A/B/C");
            });
        }

        function test_childOfReparentedItem_StyleSetCreatedAfterReparenting() {
            AqtTests.Utils.withComponent(parentHierarchyCase, null, {}, function(w) {
                // Force side effect of building the path before reparenting the parent.
                compare(w.c.b.c.StyleSet.path, "ApplicationWindow/C/B/C");

                w.c.b.parent = w.c.a;
                compare(w.c.b.c.StyleSet.path, "ApplicationWindow/C/B/C"); // not yet up-to-date

                // Access StyleSet to create it
                w.c.b.StyleSet.path;

                compare(w.c.b.c.StyleSet.path, "ApplicationWindow/C/A/B/C");
            });
        }
    }
}
