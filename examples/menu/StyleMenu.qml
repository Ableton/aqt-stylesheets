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

import Aqt.StyleSheets 1.0

ApplicationWindow {
    StyleEngine {
        id: styleEngine
        stylePath: "."
        styleName: "style-1.style"
        fileExtensions: [ "*.style" ]
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
