// Copyright (c) 2015 Ableton AG, Berlin

import QtQuick 2.3
import QtTest 1.0

import Aqt.StyleSheets 1.1

Item {
    id: scene

    TestCase {
        id: creation
        name: "creation"

        StyleEngine {
            styleSheetSource: "benchmark_StyleSet.css"
        }

        QtObject {
            id: globalProperties
            property QtObject props: QtObject {
                property color color: "red"
            }
        }

        Component {
            id: siblings

            A {
                id: rootItem

                property string mode

                B {
                    C {
                        D {
                            Repeater {
                                model: 10000

                                property Component simpleBindingRect: Rectangle {
                                    anchors.fill: parent
                                    color: globalProperties.props.color
                                }

                                property Component styleSetBindingRect: Rectangle {
                                    anchors.fill: parent
                                    color: StyleSet.props.color("background")
                                }

                                delegate: rootItem.mode == "styleSet"
                                    ? styleSetBindingRect
                                    : simpleBindingRect
                            }
                        }
                    }
                }
            }
        }

        Component {
            id: sharedTreeRoot

            A {
                property string mode
                property int level: 0

                // The following line is a work-around to force
                // initialization of StyleSet objects that are not accessed
                // otherwise in in the tree. This is a bug in the style
                // engine which will be fixed later in this branch.
                property string workaround: StyleSet.name

                Loader {
                    property string parentMode: parent.mode
                    property int parentLevel: parent.level
                    sourceComponent: sharedTree
                }
            }
        }

        Component {
            id: sharedTree

            B {
                property string mode: parentMode
                property int level: parentLevel + 1

                Loader {
                    property string parentMode: parent.mode
                    property int parentLevel: parent.level
                    sourceComponent: level < 2000 ? sharedTree : leaves
                }

                Component {
                    id: leaves
                    C {
                        Repeater {
                            model: 4

                            property Component simpleBindingRect: Rectangle {
                                anchors.fill: parent
                                color: globalProperties.props.color
                            }

                            property Component styleSetBindingRect: Rectangle {
                                anchors.fill: parent
                                color: StyleSet.props.color("background")
                            }

                            delegate: mode == "styleSet"
                                ? styleSetBindingRect
                                : simpleBindingRect
                        }
                    }
                }
            }
        }

        function benchmark_once_creationOfSiblingsWithSimpleBinding() {
            var obj = siblings.createObject(creation, { mode: "simple" });
            obj.destroy();
        }

        function benchmark_once_creationOfSiblingsWithStyleSetBinding() {
            var obj = siblings.createObject(creation, { mode: "styleSet" });
            obj.destroy();
        }

        function benchmark_once_creationOfSharedTreeWithSimpleBinding() {
            var obj = sharedTreeRoot.createObject(creation, { mode: "simple" });
            obj.destroy();
        }

        function benchmark_once_creationOfSharedTreeWithStyleSetBinding() {
            var obj = sharedTreeRoot.createObject(creation, { mode: "styleSet" });
            obj.destroy();
        }

        function cleanup() {
            gc();
        }
    }
}
