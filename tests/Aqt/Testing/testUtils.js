// Copyright (c) 2015 Ableton AG, Berlin

.pragma library

"use strict";


function withComponent(componentClass, parent, args, proc) {
    var obj = componentClass.createObject(parent, args);
    if (obj == null) {
        console.warn("Could not create object of class " + componentClass);
    }

    try {
        return proc(obj);
    }
    finally {
        obj.destroy();
    }
}
