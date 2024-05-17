#ifndef IVS3D_MAPHANDLER_H
#define IVS3D_MAPHANDLER_H

#include <QGeoCoordinate>
#include <QGeoPath>
#include <QGeoPolygon>
#include <QLineF>
#include <QList>
#include <QObject>
#include <QPointF>
#include <QPolygonF>
#include <QQuickItem>
#include <QtGlobal>
#include <cmath>
#include <float.h>

/**
 * @class MapHandler
 *
 * @ingroup GeoMapPlugin
 *
 * @brief The MapHandler is used to handel the qml map
 *
 * @author Daniel Brommer
 * @author Boitumelo Ruf
 */

class MapHandler : public QObject
{
    Q_OBJECT

    //--- METHOD DECLARATION ---//

  public:
    explicit MapHandler();

    /**
     * @brief addPoints Adds all given gps values as points on the map
     */
    void addPoints(const QList<QPair<QPointF, bool>>& m_gpsData);

    void setPolygon(const QPolygonF& poly);

    /**
     * @brief emitCircleSignal Method to emit the corresponding signal
     * @param coordinate Where to place the point
     * @param name ObjectName of the point
     * @param used @a true if the point is used @a false otherwise
     */
    void emitCircleSignal(const QGeoCoordinate& coordinate, QString name, bool used)
    {
        Q_EMIT circleSignal(coordinate, name, used);
    }
    /**
     * @brief emitAdjustMapCenter Method to emit the corresponding signal
     * @param coordinate Center coordinate
     */
    void emitAdjustMapCenter(const QGeoCoordinate& coordinate)
    {
        Q_EMIT adjustMap(coordinate);
    }
    /**
     * @brief emitCreatePolyline method to emit the corresponding signal
     * @param coordinate New coordinate added to the polyline
     */
    void emitCreatePolyline(const QGeoCoordinate& coordinate)
    {
        Q_EMIT createPolyline(coordinate);
    }
    /**
     * @brief emitSetMapSelect Method to emit the corresponding signal
     * @param path QGeoPath with the perimeter of the current polygon
     */
    void emitSetMapSelect(const QGeoPath& path)
    {
        Q_EMIT setMapSelect(path);
    }
    /**
     * @brief emitSetPoint Method to emit the corresponding signal
     * @param index Index of the point based on the list of map items from the qml map
     * @param used @a true if the point is used @a false otherwise
     */
    void emitSetPoint(const int index, bool used)
    {
        Q_EMIT setPoint(index, used);
    }
    /**
     * @brief emitGetMapItems Method to emit the corresponding signal
     */
    void emitGetMapItems()
    {
        Q_EMIT getMapItems();
    }

    /// draw GPS data onto map.
    void drawGpsDataOnMap();

  Q_SIGNALS:
    /**
     * @brief circleSignal Used to create points on the map at the given coordinate
     * @param coordinate Where to place the point
     * @param name ObjectName of the point
     * @param used @a true if the point is used @a false otherwise
     */
    void circleSignal(const QGeoCoordinate& coordinate, QString name, bool used);
    /**
     * @brief adjustMap Used to center the map at the given coordinate
     * @param coordinate Center coordinate
     */
    void adjustMap(const QGeoCoordinate& coordinate);
    /**
     * @brief emitCreatePolyline Used to add points to the static polyline on the map (And will set the
     * polyline visible)
     * @param coordinate New coordinate added to the polyline
     */
    void createPolyline(const QGeoCoordinate& coordinate);
    /**
     * @brief emitSetMapSelect Used to updated the current user selected polygon on the map
     * @param path QGeoPath with the perimeter of the current polygon
     */
    void setMapSelect(const QGeoPath& path);
    /**
     * @brief emitSetPoint Used to update a points oppacity
     * @param index Index of the point based on the list of map items from the qml map
     * @param used @a true if the point is used @a false otherwise
     */
    void setPoint(const int index, bool used);
    /**
     * @brief getMapItems Used to signal that the map needs to return the current map items
     */
    void getMapItems();

  signals:
    /**
     * @brief gpsClicked Signal emitted when a gps points has been clicked
     * @param gpsPoint Position of the point
     * @param used @a true if the point is used AFTER the click @a false otherwise
     */
    void gpsClicked(QPointF gpsPoint, bool used);

    /**
     * @brief gpsSelected Signal emitted when a new polygon has been selected
     * @param polyF The perimeter of the current polygon
     */
    void gpsSelected(QPolygonF polyF);

  public slots:
    // Slot to signal which is emitted when a gps point on the map is clicked
    void onQmlGpsClicked(const QString& text);
    // Slot to signal which is emitted when the map is right clicked
    void onQmlMapClicked(const QString& text);
    // Slot to signal which returns all items on the map
    void onQmlMapItems(const QVariant& variant);

    void onQmlDeleteSelection();

    void onQmlSelectionBack();

    void onQmlSelectionForward();

  private:
    /// Used to update the polygon on the map, signal the gps class the new polygon and change
    /// opacity of changed points on the map
    void newMapItems();

    /// Converts the current QGeoPolygon to QPolygonF
    QPolygonF geoPolyToPolyF();

    /// Disassembles the GeoPolygon to a List of lines (excluding the automatic last line to the
    /// start)
    QList<QLineF> geoPolyToLineList();

    /// Return the QPointF of the current QGeoPolygon at the given index
    QPointF pointAtGeo(int index);

    /// returns QPoint with x = min distance from line segment (a,b) to newPoint; y = t with nearest
    /// point to newPoint at A+t*(B-A)
    QPointF minDistance(QPointF A, QPointF B, QPointF newPoint);

    //--- METHOD DECLARATION ---//

  private:
    /// GPS points used or not
    QMap<QPointF, bool> mGpsMap;

    /// List of GPS data in the order of acquisition
    QList<QPointF> mOrderedGpsList;

    /// The current polygon
    QGeoPolygon mPolygon;

    /// Save points that have been change with a new polygon
    QSet<QPointF> mChangedPoints;

    /// Used to save the item list from the qml map
    QMap<QPointF, int> mMapItems;

    QList<QGeoPolygon> mPolyStack;

    int mCurrentStackPos = -1;
};

#endif // IVS3D_MAPHANDLER_H
