import QtQuick 2.15
import QtQuick.Controls 2.15
import "../theme" 

Rectangle {
    id: root

    property int spacing: 5
    property alias buttonGroup: group
    property alias column: columnLayout

    default property alias content: columnLayout.data

    implicitWidth: columnLayout.implicitWidth + 16
    implicitHeight: columnLayout.implicitHeight + 16

    radius: 8
    color: "transparent"

    gradient: Gradient {
        GradientStop { position: 0.0; color: Theme.bgPrimary }
        GradientStop { position: 1.0; color: Theme.bgPrimary }
    }

    ButtonGroup {
        id: group
    }

    Column {
        id: columnLayout
        anchors.fill: parent
        anchors.margins: 8
        spacing: root.spacing

        onChildrenChanged: {
            var arr = [];
            for (var i = 0; i < children.length; ++i) {
                var child = children[i];
                if (child.checkable !== undefined) {
                    arr.push(child);
                }
            }
            group.buttons = arr;
        }
    }
}