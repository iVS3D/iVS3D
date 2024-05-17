import QtQuick 2.11
import QtPositioning 5.11
import QtLocation 5.11
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.11


Item {
    id: base
    visible: true
    anchors.fill: parent
    signal gpsClicked(string msg)
    signal qmlClosed()
    signal mapClicked(string geo)
    signal mapItems(var items)
    signal deleteSelection()
    signal selectionBack()
    signal selectionForward()


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
                    o.opacity = (used) ? 1.0 : 0.1
                    mapToken.addMapItem(o)
                }
            }
            onAdjustMap : {
                mapToken.center = coordinate
            }

            onCreatePolyline : {
                mapPolyline.addCoordinate(coordinate)
                mapPolyline.visible = true
            }

            onSetMapSelect : {
                selectGPS.setPath(path)
            }

            onSetPoint : {
                var item = mapToken.mapItems[index]
                item.opacity = (used) ? 1.0 : 0.1
            }

            onGetMapItems : {
                mapItems(mapToken.mapItems)
            }
        }

        MapPolyline {
            id: mapPolyline
            visible: false
            line.width: 3
            line.color: 'black'
        }

        MapPolyline {
            id: selectGPS
            visible: true
            line.width: 3
            line.color: 'blue'
        }


        MouseArea {
            enabled: true
            anchors.fill: parent
            acceptedButtons: Qt.RightButton
            onClicked: {
                var cord = mapToken.toCoordinate(Qt.point(mouse.x,mouse.y))
                var string = cord.latitude + 'x' + cord.longitude
                mapClicked(string)
            }
        }


    }
}
