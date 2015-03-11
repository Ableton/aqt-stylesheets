No signals for hierarchy changes

A little test showing that styleset are not listening on hierarchy changes
of QtObject.  This normally will raise an info logging on the console, but
not for singletons (because they won't change their hierarchy anyway).

Run this from the checkouts toplevel folder:

```
  $ cd aqt-stylesheets/
  $ qmlscene -I qml -I examples/items examples/hierarchy-changes/Objects.qml
```

You should see an info warning on the console like this:

```
INFO: Parent to StyleSetAttached is not a QQuickItem but ' QObject_QML_16 '.
Hierarchy changes for this component won't be detected.
```

Run:

```
  $ qmlscene -I qml -I examples/hierarchy-changes examples/hierarchy-changes/Singletons.qml
  $ qmlscene -I qml -I examples/hierarchy-changes examples/hierarchy-changes/Windows.qml
```

For the last two cases You should see no info message.
