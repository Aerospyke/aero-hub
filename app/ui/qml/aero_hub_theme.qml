pragma Singleton

import QtQuick

QtObject {
    readonly property QtObject fontSize: QtObject {
        readonly property int mainTitle: 26
        readonly property int subTitle: 24
        readonly property int columnHeader: 22
        readonly property int tableEntry: 20
    }
}