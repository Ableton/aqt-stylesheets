// Copyright (c) 2015 Ableton AG, Berlin

import QtQuick 2.3
import QtTest 1.0
import QtQuick.Layouts 1.1

import Aqt.StyleSheets 1.3
import Aqt.Testing 1.0 as AqtTests

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
        styleSheetSource: "props.css"
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
            AqtTests.Utils.withComponent(singleColorScene, scene, {}, function(comp) {
                verify(typeof(comp.foo) != "string");
                verify(Qt.colorEqual(comp.foo, "#123456"));
            });
        }
    }


    //--------------------------------------------------------------------------

    Component {
        id: stringLookupScene

        Item {
            property alias gaz: rect1.gaz

            Rectangle {
                id: rect1
                StyleSet.name: "root"
                anchors.fill: parent

                property var gaz: StyleSet.props.string("gaz")
            }
        }
    }

    Component {
        id: badStringLookupScene

        Item {
            property alias foo: rect1b.foo

            Rectangle {
                id: rect1b
                StyleSet.name: "root"
                anchors.fill: parent

                property var foo: StyleSet.props.string("foo")
            }
        }
    }

    TestCase {
        name: "lookup a string"
        when: windowShown

        function test_lookupASingleString() {
            AqtTests.Utils.withComponent(stringLookupScene, scene, {}, function(comp) {
                // single string
                compare(comp.gaz, "hello world!");
            });
        }

        function test_lookupASingleString_fails() {
            msgTracker.expectMessage(AqtTests.MsgTracker.Warning,
                                     /^.*Property.*is not convertible.*QString.*/);
            AqtTests.Utils.withComponent(badStringLookupScene, scene, {}, function(comp) {
                // multiple string fails, props.string() returns an empty string.
                compare(comp.foo, "");
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
            AqtTests.Utils.withComponent(multipleColorsScene, scene, {}, function(comp) {
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
            AqtTests.Utils.withComponent(multipleColorsScene, scene, {}, function(comp) {
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


    //--------------------------------------------------------------------------

    Component {
        id: missingPropertyScene

        Item {
            property alias rect: rect2
            property alias checkStyles: styleChecker.active

            StyleChecker { id: styleChecker }

            Rectangle {
                id: rect2
                StyleSet.name: "root"
                anchors.fill: parent

                property var notExisting: "someArbitraryValue"
            }
        }
    }

    TestCase {
        name: "lookup missing properties."
        when: windowShown

        SignalSpy {
            id: missingPropertiesSpy
            target: styleEngine
            signalName: "exception"
        }

        function cleanup() {
            missingPropertiesSpy.clear();
        }

        function test_lookupNotExistingPropertyWarnsIfStyleCheckerActive() {
            msgTracker.expectMessage(AqtTests.MsgTracker.Warning,
                                     /^.*Property.*not-existing.*/);
            AqtTests.Utils.withComponent(missingPropertyScene, scene, {}, function(comp) {
                comp.checkStyles = true;
                compare(missingPropertiesSpy.count, 0);

                comp.rect.notExisting = comp.rect.StyleSet.props.get("not-existing");
                compare(comp.rect.notExisting, undefined);

                waitForRendering(comp);
                waitForRendering(comp); // Wait again to make the test pass on Travis CI in Debug
                compare(missingPropertiesSpy.count, 1);
                compare(missingPropertiesSpy.signalArguments[0][0], "propertyNotFound");
            });
        }

        function test_lookupNotExistingPropertyDoesNotWarnIfStyleCheckerNotActive() {
            AqtTests.Utils.withComponent(missingPropertyScene, scene, {}, function(comp) {
                comp.checkStyles = false;
                compare(missingPropertiesSpy.count, 0);

                comp.rect.notExisting = comp.rect.StyleSet.props.get("not-existing");
                compare(comp.rect.notExisting, undefined);

                waitForRendering(comp);
                compare(missingPropertiesSpy.count, 0);
            });
        }
    }


    //--------------------------------------------------------------------------

    Component {
        id: urlLookupScene

        Item {
            property alias absUrl: rect5.absUrl
            property alias absUrl2: rect5.absUrl2
            property alias iconSize: img1.sourceSize

            Rectangle {
                id: rect5
                StyleSet.name: "url-test"
                anchors.fill: parent

                property var absUrl: StyleSet.props.url("icon")
                property var absUrl2: StyleSet.props.url("icon2")
            }

            Image {
                id: img1
                StyleSet.name: "dot"
                source: StyleSet.props.url("dot-icon")
            }
        }
    }

    TestCase {
        name: "lookup an url"
        when: windowShown

        function test_lookupAnUrl() {
            AqtTests.Utils.withComponent(urlLookupScene, scene, {}, function(comp) {
                compare(comp.absUrl, "http://www.eyestep.org/images/icon.srv?nm=loud");
                compare(comp.absUrl2, "ableton:assets/images/dots.png");
                compare(comp.iconSize.width, 2);
                compare(comp.iconSize.height, 2);
            });
        }
    }
}
