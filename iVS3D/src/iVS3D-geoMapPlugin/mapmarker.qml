import QtQuick 2.10
import QtPositioning 5.11
import QtLocation 5.11

MapQuickItem {
    id: mapMarker
    sourceItem: Rectangle {id: rect; width: 15; height: 15; color: "#e41e25"; border.width: 1; border.color: "white"; smooth: true; radius: 10 }
    opacity: 0.1
    anchorPoint.x: rect.width/2
    anchorPoint.y: rect.height/2

    MouseArea {
        enabled: true
        anchors.fill: parent
        onClicked: {
            mapMarker.opacity = (mapMarker.opacity == 1.0) ? 0.1 : 1.0
            base.gpsClicked(mapMarker.objectName)
        }
    }
}

