import QtQuick 2.10
import QtPositioning 5.11
import QtLocation 5.11

MapQuickItem {
    id: mapMarker
    property bool isCurrent: false
    property bool isUsed: true
    property bool partialSelectionEnabled: true
    property real selectionRatio: mapMarker.isUsed ? 1.0 : 0.0

    function clamp01(v) {
        return Math.max(0.0, Math.min(1.0, v))
    }

    function mixChannel(a, b, t) {
        return a + (b - a) * t
    }

    function mixColor(a, b, t) {
        return Qt.rgba(
            mixChannel(a.r, b.r, t),
            mixChannel(a.g, b.g, t),
            mixChannel(a.b, b.b, t),
            mixChannel(a.a, b.a, t)
        )
    }

    readonly property real clampedRatio: clamp01(selectionRatio)
    readonly property real effectiveRatio: partialSelectionEnabled
                                        ? clampedRatio
                                        : (mapMarker.isUsed ? 1.0 : 0.0)
    readonly property real baseSize: mixChannel(9.0, 15.0, effectiveRatio)
    readonly property color baseColor: mixColor(
                                     Qt.rgba(0x8f / 255, 0x7c / 255, 0x7e / 255, 1.0),
                                     Qt.rgba(0xe4 / 255, 0x1e / 255, 0x25 / 255, 1.0),
                                     effectiveRatio)
    readonly property color baseBorderColor: mixColor(
                                           Qt.rgba(0xd6 / 255, 0xc9 / 255, 0xca / 255, 1.0),
                                           Qt.rgba(1.0, 1.0, 1.0, 1.0),
                                           effectiveRatio)

    z: mapMarker.isCurrent ? 5000 : 100
    sourceItem: Rectangle {
        id: rect
        width: mapMarker.isCurrent ? 18 : mapMarker.baseSize
        height: width
        color: mapMarker.isCurrent
               ? "#00c853"
               : mapMarker.baseColor
        border.width: mapMarker.isCurrent ? 2 : 1
        border.color: mapMarker.isCurrent
                      ? "black"
                      : mapMarker.baseBorderColor
        smooth: true
        radius: width / 2
    }
    opacity: mapMarker.isCurrent ? 1.0 : mixChannel(0.75, 1.0, mapMarker.effectiveRatio)
    anchorPoint.x: rect.width/2
    anchorPoint.y: rect.height/2

    MouseArea {
        enabled: true
        anchors.fill: parent
        onClicked: {
            mapMarker.isUsed = !mapMarker.isUsed
            mapMarker.selectionRatio = mapMarker.isUsed ? 1.0 : 0.0
            base.gpsClicked(mapMarker.objectName)
        }
    }
}

