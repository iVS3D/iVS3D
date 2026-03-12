import QtQuick 2.11
import QtPositioning 5.11
import QtLocation 5.11
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.11


Item {
    id: base
    visible: true
    anchors.fill: parent
    property bool partialSelectionEnabled: true
    property var polygonVertexItems: []
    property var polygonSegmentItems: []
    property var traceSegmentItems: []
    signal gpsClicked(string msg)
    signal qmlClosed()
    signal mapClicked(string geo)
    signal mapItems(var items)
    signal deleteSelection()
    signal selectionBack()
    signal selectionForward()

    function clearPolygonVertexItems() {
        for (var i = 0; i < polygonVertexItems.length; ++i) {
            mapToken.removeMapItem(polygonVertexItems[i])
            polygonVertexItems[i].destroy()
        }
        polygonVertexItems = []
    }

    function clearPolygonSegmentItems() {
        for (var i = 0; i < polygonSegmentItems.length; ++i) {
            mapToken.removeMapItem(polygonSegmentItems[i])
            polygonSegmentItems[i].destroy()
        }
        polygonSegmentItems = []
    }

    function clearTraceSegmentItems() {
        for (var i = 0; i < traceSegmentItems.length; ++i) {
            mapToken.removeMapItem(traceSegmentItems[i])
            traceSegmentItems[i].destroy()
        }
        traceSegmentItems = []
    }


    Plugin {
        id: mapPlugin
        name: "osm"
        PluginParameter
        {
            name: "osm.mapping.custom.host"
            value: "http://a.basemaps.cartocdn.com/rastertiles/voyager_nolabels/"
        }

    }
    Map {
        id: mapToken
        anchors.fill: parent
        plugin: mapPlugin
        zoomLevel: 17
        activeMapType: supportedMapTypes[supportedMapTypes.length - 1] // 1 = Satellite, 0 = Street Map


        Connections{
            target: handler
            onCircleSignal : {
                var component = Qt.createComponent("mapmarker.qml");
                if (component.status === Component.Ready){
                    var o = component.createObject(mapToken);
                    o.coordinate = coordinate
                    o.objectName = name
                    o.isUsed = used
                    o.selectionRatio = ratio
                    o.partialSelectionEnabled = base.partialSelectionEnabled
                    mapToken.addMapItem(o)
                }
            }
            onAdjustMap : {
                mapToken.center = coordinate
            }

            onSetTracePath : {
                var traceCoords = coordinates ? coordinates : []
                clearTraceSegmentItems()
                for (var i = 1; i < traceCoords.length; ++i) {
                    var seg = traceSegmentComponent.createObject(mapToken)
                    seg.addCoordinate(traceCoords[i - 1])
                    seg.addCoordinate(traceCoords[i])
                    mapToken.addMapItem(seg)
                    traceSegmentItems.push(seg)
                }
            }

            onSetMapSelect : {
                // Legacy path signal (kept for compatibility)
            }

            onSetMapSelectCoordinates : {
                var polyPath = coordinates ? coordinates : []
                // Rebuild path explicitly to avoid QVariantList conversion quirks.
                selectGPS.path = []
                clearPolygonVertexItems()
                clearPolygonSegmentItems()
                for (var i = 0; i < polyPath.length; ++i) {
                    selectGPS.addCoordinate(polyPath[i])
                    var marker = polygonVertexComponent.createObject(mapToken)
                    marker.coordinate = polyPath[i]
                    mapToken.addMapItem(marker)
                    polygonVertexItems.push(marker)

                    if (i > 0) {
                        var segment = polygonSegmentComponent.createObject(mapToken)
                        segment.addCoordinate(polyPath[i - 1])
                        segment.addCoordinate(polyPath[i])
                        mapToken.addMapItem(segment)
                        polygonSegmentItems.push(segment)
                    }
                }
                selectGPS.visible = selectGPS.path.length > 1
            }

            onSetPoint : {
                // mapItems is a list, so access by index is in O(N)!
                // finding the item is very expensive if there are many items!
                var item = mapToken.mapItems[index]
                if (!item)
                    return
                item.isUsed = used
                item.selectionRatio = used ? 1.0 : 0.0
            }

            onSetPointState : {
                var item = mapToken.mapItems[index]
                if (!item)
                    return
                item.isUsed = used
                item.selectionRatio = ratio
            }

            onSetPartialSelectionEnabled : {
                base.partialSelectionEnabled = enabled
                for (var i = 0; i < mapToken.mapItems.length; ++i) {
                    var item = mapToken.mapItems[i]
                    if (!item)
                        continue
                    if (item.partialSelectionEnabled === undefined)
                        continue
                    item.partialSelectionEnabled = enabled
                }
            }

            onSetPointHighlight : {
                var item = mapToken.mapItems[index]
                if (!item)
                    return
                item.isCurrent = highlighted
            }

            onGetMapItems : {
                handler.onQmlMapItems(mapToken.mapItems)
            }

            onClearMap : {
                clearPolygonVertexItems()
                clearPolygonSegmentItems()
                clearTraceSegmentItems()
                while (mapToken.mapItems.length > 0) {
                    mapToken.removeMapItem(mapToken.mapItems[0])
                }
                selectGPS.path = []
            }
        }

        MapPolyline {
            id: selectGPS
            visible: true
            z: 1000
            line.width: 8
            line.color: '#ff00ff'
        }

        Component {
            id: polygonVertexComponent
            MapQuickItem {
                z: 1100
                anchorPoint.x: 5
                anchorPoint.y: 5
                sourceItem: Rectangle {
                    width: 10
                    height: 10
                    radius: 5
                    color: "#00ffff"
                    border.width: 2
                    border.color: "black"
                }
            }
        }

        Component {
            id: polygonSegmentComponent
            MapPolyline {
                z: 1090
                line.width: 6
                line.color: "#00ffff"
            }
        }

        Component {
            id: traceSegmentComponent
            MapPolyline {
                z: 50
                line.width: 2
                line.color: "black"
            }
        }


        MouseArea {
            enabled: true
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            onClicked: {
                if (mouse.button !== Qt.RightButton)
                    return
                var cord = mapToken.toCoordinate(Qt.point(mouse.x,mouse.y))
                var string = cord.latitude + 'x' + cord.longitude
                handler.onQmlMapClicked(string)
            }
        }


    }
}
