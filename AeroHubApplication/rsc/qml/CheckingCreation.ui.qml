

/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/
import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    width: 1920
    height: 1080
    color: "#9e9e9e"

    Button {
        id: button
        x: 539
        y: 319
        text: qsTr("Button")
        font.pointSize: 32
        icon.color: "#d8d31a1a"
    }

    Button {
        id: button1
        x: 970
        y: 539
        width: 218
        height: 122
        text: qsTr("ButtonTwo")
        icon.color: "#d8bf1717"
        highlighted: false
        flat: false
        checked: true
        checkable: true
    }

    Switch {
        id: switch1
        x: 1068
        y: 331
        text: qsTr("Switch")
    }
}
