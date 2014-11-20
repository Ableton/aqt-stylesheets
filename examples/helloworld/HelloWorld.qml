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
import QtQuick.Layouts 1.1

import Aqt.StyleSheets 1.0


Rectangle {
    width: 200
    height: 50

    StyleEngine {
        stylePath: "."
        styleName: "HelloWorld.css"
    }

    StyleSet.name: "root"
    color: StyleSet.props.color("background-color")

    RowLayout {
        Rectangle {
            StyleSet.name: "box"
            width: 100
            Text {
                text: "Hello"
                color: StyleSet.props.color("text-color")
                font: StyleSet.props.font("font")
            }
        }

        Item {
            StyleSet.name: "box"
            width: 100
            Text {
                text: "World"
                color: StyleSet.props.color("text-color")
                font: StyleSet.props.font("font")
            }
        }
    }
}
