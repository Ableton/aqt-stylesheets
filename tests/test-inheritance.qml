// Copyright (c) 2015 Ableton AG, Berlin

import QtQuick 2.3
import QtTest 1.0

import Aqt.StyleSheets 1.1
import Aqt.Testing 1.0 as AqtTests

import Foo 1.0

Item {
    id: scene

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
                {  tag: "FAIL: A/B.custom, onlyA",
                   expectFail: "Bug: A/B.custom does currently not inherit properties from A",
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

                // The following two lines are a work-around to force
                // initialization of StyleSet objects that are not accessed
                // otherwise in some test rows. This is a bug in the style
                // engine which will be fixed later in this branch.
                a.StyleSet.name;
                a.b.StyleSet.name;

                var obj = eval(data.objectPath);

                if (data.expectFail) {
                    expectFail("", data.expectFail);
                }

                compare(obj.StyleSet.props.string(data.prop), data.expectedValue);
            });
        }
    }
}
