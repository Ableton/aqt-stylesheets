// Copyright (c) 2015 Ableton AG, Berlin

import QtQuick 2.3
import QtTest 1.0
import QtQuick.Layouts 1.1

import Aqt.StyleSheets 1.1
import Aqt.Testing 1.0 as AqtTests

Item {
    id: scene

    /*! ensure minimum width to be larger than the minimum allowed width on
     * Windows */
    implicitWidth: 124
    /*! there are no constraints on the height, but it is convenient to have a
     *  default size */
    implicitHeight: 116


    AqtTests.MsgTracker {
        id: msgTracker
    }

    SignalSpy {
        id: spy
        target: styleEngine
        signalName: "exception"
    }

    StyleEngine {
        id: styleEngine
    }

    TestCase {
        name: "stylesheets with missing fonts"
        when: windowShown

        function test_setStyleSheetLoadsFonts() {
            msgTracker.expectMessage(AqtTests.MsgTracker.Debug,
                                     /^INFO:.*Load font face .*Aqt.otf.*/);
            compare(spy.count, 0);
            // load the css from a subfolder.  The font referenced from the
            // css file must be resolved relative to the css file
            styleEngine.styleSheetSource = "css/aqt-font.css"
            compare(spy.count, 0);

            spy.clear();
        }

        function test_missingFontsGivesAWarning() {
            msgTracker.expectMessage(AqtTests.MsgTracker.Warning,
                                     /^WARN:.*Could not find font file.*a-missing-font.ttf.*/);
            compare(spy.count, 0);

            styleEngine.styleSheetSource = "missing-font.css"

            compare(spy.count, 1);
            compare(spy.signalArguments[0][0], "fontWasNotLoaded");

            spy.clear();
        }
    }
}
