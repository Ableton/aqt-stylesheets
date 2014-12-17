import QtQuick 2.3
import Aqt.StyleSheets 1.0 as Styles

Item {
    id: root
    height: 400
    width: 400

    Styles.StyleEngine {
        id: styleEngine
        stylePath: "."
        styleName: "test.css"
    }

    property int numSelections: root.Styles.StyleSet.props.number("numSelections")

    Repeater{
        model: numSelections
        delegate: Text {
            text: Styles.StyleSet.props.number("number")
        }
    }
}
