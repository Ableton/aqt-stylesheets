// Copyright (c) 2015 Ableton AG, Berlin

import QtQuick 2.3
import QtTest 1.0
import QtQuick.Layouts 1.1

import Aqt.StyleSheets 1.1
import Aqt.Testing 1.0 as AqtTests

Item {
    id: scene

    /*! ensure minimum width to be larger than the minimum allowed width on
     * Windows */
    implicitWidth: 124
    /*! there are no constraints on the height, but it is convenient to have a
     *  default size */
    implicitHeight: 116


    StyleEngine {
        id: styleEngine
        styleSheetSource: "basic.css"
    }

    Component {
        id: minimalScene

        Item {
            property alias foo: textObj.text

            Rectangle {
                id: rect
                StyleSet.name: "root"
                anchors.fill: parent

                Text {
                    id: textObj
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: StyleSet.props.get("text")
                    font.pixelSize: 72
                }
            }
        }
    }

    TestCase {
        name: "path to hierarchy objects"
        when: windowShown

        function test_basePropertyLookup() {
            AqtTests.Utils.withComponent(minimalScene, scene, {}, function(comp) {
                compare(comp.foo, "B");
            });
        }
    }
}
