// Copyright (c) 2014 Ableton AG, Berlin
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts 1.1

import Aqt.StyleSheets 1.0

ApplicationWindow {
    width: 500
    height: 410

    StyleEngine {
       id: styleEngine
       stylePath: "."
       styleName: "style.css"
       defaultStyleName: "default.css"
       fileExtensions: [ "*.css" ]
    }

    function stateColor(colors, isPressed, def) {
        if (colors !== undefined && colors.length >= 2) {
            return isPressed ? colors[0] : colors[1];
        }
        return def;
    }

    Rectangle {
        id: root

        StyleSet.name: "root"
        anchors.fill: parent

        color: StyleSet.props.color("background")

        StyleDebugMouseArea {
            debug: true
        }

        GridLayout {
            id: grid
            columns: 2
            rowSpacing: 10
            columnSpacing: 10

            Rectangle {
                StyleSet.name: "one color-mixin"
                width: 245
                height: 200
                color: stateColor(StyleSet.props.get("colors"), area1.pressed, "gray")
                radius: StyleSet.props.number("radius")
                Text {
                    text: "Hello"
                    font: StyleSet.props.font("font")
                }

                MouseArea {
                    id: area1
                    anchors.fill: parent
                }
            }

            Rectangle {
                StyleSet.name: "two"
                width: 245
                height: 200
                color: StyleSet.props.color("background")
                radius: StyleSet.props.number("radius")
                Text {
                    text: "world"
                    font: StyleSet.props.font("font")
                }
                visible: StyleSet.props.boolean("visible")
            }

            Rectangle {
                StyleSet.name: "three"
                width: 245
                height: 200
                color: StyleSet.props.color("background")
                radius: StyleSet.props.number("radius")
                Text {
                    text: "dlrow"
                    font: StyleSet.props.font("font")
                }
            }

            Rectangle {
                width: 245
                StyleSet.name: "four"
                height: 200
                color: StyleSet.props.color("background")
                radius: StyleSet.props.number("radius")

                Item {
                    anchors.fill: parent
                    Rectangle {
                        StyleSet.name: "color-mixin"
                        width: parent.width
                        height: parent.height / 2
                        color: stateColor(StyleSet.props.get("colors"), area1.pressed, "red")
                        radius: StyleSet.props.number("radius")
                    }

                    Item {
                        width: parent.width
                        height: parent.height / 2
                        y: parent.height / 2

                        Text {
                            text: "elloH"
                            font: StyleSet.props.font("font")
                        }
                    }
                }
            }
        }
    }

    function displayStyleName(styleName) {
        var baseName = styleName.split(".style")[0];
        return baseName.charAt(0).toUpperCase() + baseName.slice(1);
    }

    menuBar: MenuBar {
        Menu {
            title: qsTr("Appearance")
            Menu {
                id: changeStyleMenu
                title: qsTr("Change Theme")

                Instantiator {
                    model: styleEngine.availableStyles

                    MenuItem {
                        id: styleMenuItem

                        property var styleName: modelData

                        text: displayStyleName(modelData)

                        checkable: true
                        checked: styleMenuItem.styleName === styleEngine.styleName
                        onTriggered: styleEngine.styleName = modelData
                    }

                    onObjectAdded: changeStyleMenu.insertItem(index, object)
                    onObjectRemoved: changeStyleMenu.removeItem(object)
                }
            }
        }
    }
}
