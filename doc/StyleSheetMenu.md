Implementing a style sheet menu                {#StyleSheetMenu}
===============================

Create a StyleEngine in your main application:

    import QtQuick 2.3
    import QtQuick.Controls 1.2

    import Aqt.StyleSheets 1.1

    ApplicationWindow {
        StyleEngine {
            id: styleEngine
            styleSheetSource: "style.css"
        }

        StylesDirWatcher {
           id: stylesDirWatcher
           stylePath: "."
           fileExtensions: ["*.css"]
        }

        function displayStyleName(styleUrl) {
            var tokens = styleUrl.toString().split(/\/|\\/),
                baseName = tokens[tokens.length - 1].split(/.css$/)[0]
            return baseName.charAt(0).toUpperCase() + baseName.slice(1);
        }

        menuBar: MenuBar {
            Menu {
                title: qsTr("Appearance")
                Menu {
                    id: changeStyleMenu
                    title: qsTr("Change Theme")

                    Instantiator {
                        model: stylesDirWatcher.availableStyles

                        MenuItem {
                            id: styleMenuItem

                            property var styleSource: modelData

                            text: displayStyleName(modelData)

                            checkable: true
                            checked: styleMenuItem.styleSource === styleEngine.styleSheetSource
                            onTriggered: styleEngine.styleSheetSource = styleSource
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
