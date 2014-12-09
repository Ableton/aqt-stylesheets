/**
 * @namespace Aqt.StyleSheets
 * Utilities for working with stylesheets.
 *
 * @version 1.0
 */

.pragma library

"use strict";

/*!
 * @fn string conditionalName(array pairs);
 *
 * Maps a list of (boolean, string) pairs to a string that is
 * the concatenation of all the strings for which the boolean
 * value is true.
 *
 * @param pairs Pairs of (boolean, string)
 *
 */
function conditionalName(pairs) {
    return pairs.filter(function (item) {
        return item[0];
    }).map(function(item) {
        return item[1];
    }).join(' ');
}
