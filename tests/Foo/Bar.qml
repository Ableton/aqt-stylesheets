// Copyright (c) 2015 Ableton AG, Berlin

pragma Singleton

import QtQuick 2.3
import Aqt.StyleSheets 1.1

QtObject {
    property var textValue: StyleSet.props.get("text")
}
