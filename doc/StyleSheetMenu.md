Implementing a style sheet menu                {#StyleSheetMenu}
===============================

Create a StyleEngine in your main application:

    import QtQuick 2.3
    import QtQuick.Controls 1.2

    import Aqt.StyleSheets 1.0

    ApplicationWindow {
        StyleEngine {
            id: styleEngine
            stylePath: "."
            styleName: "style.css"
        }

        function displayStyleName(styleName) {
            var baseName = styleName.split(".css")[0];
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


This code uses an `Instantiator` to construct `MenuItem`s from the
styleEngines availableStyles property.  The `displayStyleName()` function
cuts off the file name extension.
