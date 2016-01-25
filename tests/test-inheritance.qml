// Copyright (c) 2015 Ableton AG, Berlin

import QtQuick 2.3
import QtTest 1.0

import Aqt.StyleSheets 1.3
import Aqt.Testing 1.0 as AqtTests

import Foo 1.0

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
        styleSheetSource: "inheritance.css"
    }

    Component {
        id: styledSceneComponent

        A {
            property alias b: b
            property alias b_custom: b_custom
            property alias c: c

            B {
                id: b
                property alias c: bc
                property alias a: ba

                C { id: bc }

                A {
                    id: ba
                    property alias c: bac
                    C { id: bac }
                }
            }

            B {
                id: b_custom
                StyleSet.name: "custom"

                property alias c: b_custom_c

                C { id: b_custom_c }
            }

            C { id: c }
        }
    }

    TestCase {
        name: "properties are inherited"
        when: windowShown

        function test_propertyInheritance_data() {
            return [
                {  tag: "A, onlyA",
                   objectPath: "a", prop: "onlyA", expectedValue: "A" },
                {  tag: "A, forEveryone",
                   objectPath: "a", prop: "forEveryone", expectedValue: "a" },
                {  tag: "A/B, onlyA",
                   objectPath: "a.b", prop: "onlyA", expectedValue: "A" },
                {  tag: "A/B, onlyB",
                   objectPath: "a.b", prop: "onlyB", expectedValue: "B" },
                {  tag: "A/B, forEveryone",
                   objectPath: "a.b", prop: "forEveryone", expectedValue: "b" },
                {  tag: "A/B/C, onlyA",
                   objectPath: "a.b.c", prop: "onlyA", expectedValue: "A" },
                {  tag: "A/B/C, onlyB",
                   objectPath: "a.b.c", prop: "onlyB", expectedValue: "B" },
                {  tag: "A/B/C, onlyC",
                   objectPath: "a.b.c", prop: "onlyC", expectedValue: "C" },
                {  tag: "A/B/C, onlyBC",
                   objectPath: "a.b.c", prop: "onlyBC", expectedValue: "BC" },
                {  tag: "A/B/C, onlyBdirectC",
                   objectPath: "a.b.c", prop: "onlyBdirectC", expectedValue: "B>C" },
                {  tag: "A/B/C, forEveryone",
                   objectPath: "a.b.c", prop: "forEveryone", expectedValue: "b>c" },
                {  tag: "A/B/A/C, onlyA",
                   objectPath: "a.b.a.c", prop: "onlyA", expectedValue: "A" },
                {  tag: "A/B/A/C, onlyB",
                   objectPath: "a.b.a.c", prop: "onlyB", expectedValue: "B" },
                {  tag: "A/B/A/C, onlyC",
                   objectPath: "a.b.a.c", prop: "onlyC", expectedValue: "C" },
                {  tag: "A/B/A/C, onlyBC",
                   objectPath: "a.b.a.c", prop: "onlyBC", expectedValue: "BC" },
                {  tag: "A/B/A/C, onlyBdirectC",
                   objectPath: "a.b.a.c", prop: "onlyBdirectC", expectedValue: "" },
                {  tag: "A/B/A/C, forEveryone",
                   objectPath: "a.b.a.c", prop: "forEveryone", expectedValue: "bc" },
                {  tag: "A/B.custom, onlyA",
                   objectPath: "a.b_custom", prop: "onlyA", expectedValue: "A" },
                {  tag: "A/B.custom, onlyB",
                   objectPath: "a.b_custom", prop: "onlyB", expectedValue: "B" },
                {  tag: "A/B.custom, onlyCustom",
                   objectPath: "a.b_custom", prop: "onlyCustom", expectedValue: "CUSTOM" },
                {  tag: "A/B.custom, forEveryone",
                   objectPath: "a.b_custom", prop: "forEveryone", expectedValue: "custom" },
                {  tag: "A/B.custom/C, onlyA",
                   objectPath: "a.b_custom.c", prop: "onlyA", expectedValue: "A" },
                {  tag: "A/B.custom/C, onlyB",
                   objectPath: "a.b_custom.c", prop: "onlyB", expectedValue: "B" },
                {  tag: "A/B.custom/C, onlyCustom",
                   objectPath: "a.b_custom.c", prop: "onlyCustom", expectedValue: "CUSTOM" },
                {  tag: "A/B.custom/C, onlyCustomC",
                   objectPath: "a.b_custom.c", prop: "onlyCustomC", expectedValue: "CUSTOMC" },
                {  tag: "A/B.custom/C, forEveryone",
                   objectPath: "a.b_custom.c", prop: "forEveryone", expectedValue: "customC" },
                {  tag: "A/C, onlyA",
                   objectPath: "a.c", prop: "onlyA", expectedValue: "A" },
                {  tag: "A/C, onlyC",
                   objectPath: "a.c", prop: "onlyC", expectedValue: "C" },
                {  tag: "A/C, onlyBC",
                   objectPath: "a.c", prop: "onlyBC", expectedValue: "" },
                {  tag: "A/C, forEveryone",
                   objectPath: "a.c", prop: "forEveryone", expectedValue: "c" },
            ];
        }

        function test_propertyInheritance(data) {
            AqtTests.Utils.withComponent(styledSceneComponent, scene, {}, function(a) {
                var obj = eval(data.objectPath);
                compare(obj.StyleSet.props.string(data.prop), data.expectedValue);
            });
        }
    }

    //--------------------------------------------------------------------------

    Component {
        id: incompletePathScene

        Item {
            property alias a: a

            StyleChecker {
                id: styleChecker
                active: true
            }

            A {
                id: a
                property alias listView: listView

                ListView {
                    id: listView

                    model: 1
                    delegate: B {
                        property alias c: c

                        // c) This property ensures that the path of c's StyleSet is
                        // updated when B gets reparented by the ListView.
                        property string own: StyleSet.props.string("onlyB")

                        C {
                            id: c

                            // a) This binding ensures that the StyleSet is created
                            // before the evaluation of c) will propagate the reparenting.
                            // Therefore the initial path of the StyleSet is incomplete.
                            StyleSet.name: ""

                            // b) This style property won't be found when the binding is
                            // initially evaluated because it uses the incomplete path
                            // of a).
                            property string inherited: StyleSet.props.string("onlyA")
                        }
                    }
                }
            }
        }
    }

    TestCase {
        name: "intermediate missing properties"
        when: windowShown

        SignalSpy {
            id: missingPropertiesSpy
            target: styleEngine
            signalName: "exception"
        }

        function test_noWarningIsIssuedForIntermediateMissingProperties() {
            AqtTests.Utils.withComponent(incompletePathScene, scene, {}, function(comp) {
                compare(comp.a.listView.currentItem.c.inherited, "A");

                waitForRendering(comp);
                compare(missingPropertiesSpy.count, 0);
            });
        }
    }
}
