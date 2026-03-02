import QtQuick 2.10
import QtPositioning 5.11
import QtLocation 5.11

MapQuickItem {
    id: mapMarker
    property bool isCurrent: false
    property bool isUsed: true
    z: mapMarker.isCurrent ? 5000 : 100
    sourceItem: Rectangle {
        id: rect
        width: mapMarker.isCurrent ? 18 : (mapMarker.isUsed ? 15 : 9)
        height: width
        color: mapMarker.isCurrent
               ? "#00c853"
               : (mapMarker.isUsed ? "#e41e25" : "#8f7c7e")
        border.width: mapMarker.isCurrent ? 2 : 1
        border.color: mapMarker.isCurrent
                      ? "black"
                      : (mapMarker.isUsed ? "white" : "#d6c9ca")
        smooth: true
        radius: width / 2
    }
    opacity: mapMarker.isUsed ? 1.0 : 0.75
    anchorPoint.x: rect.width/2
    anchorPoint.y: rect.height/2

    MouseArea {
        enabled: true
        anchors.fill: parent
        onClicked: {
            mapMarker.isUsed = !mapMarker.isUsed
            base.gpsClicked(mapMarker.objectName)
        }
    }
}

