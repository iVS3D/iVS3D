#include "maphandler.h"
#include <QDebug>

// Define < to use QPointF in QMap
inline bool operator<(const QPointF& lhs, const QPointF& rhs)
{
    return lhs.x() < rhs.x() || (lhs.x() == rhs.x() && lhs.y() < rhs.y());
}

// Define qHash to use QPointF in QSet
inline uint qHash(const QPointF& key)
{
    return qHash(QPair<int, int>(key.x(), key.y()));
}

//==================================================================================================
MapHandler::MapHandler(QObject* parent)
    : QObject(parent)
{
    mPolygon = QGeoPolygon();
}

//==================================================================================================
void MapHandler::addPoints(const GpsDataList& gpsData)
{
    //--- loop over gps pps data and insert into map if not already inserted
    for (QPair<QPointF, bool> point : gpsData)
    {
        // Skip if point already in the map and used
        if (mGpsMap.contains(point.first) && mGpsMap[point.first])
            continue;

        mGpsMap.insert(point.first, point.second);
        mOrderedGpsList.push_back(point.first);
    }

    drawGpsDataOnMap();
}

void MapHandler::updatePoints(const GpsDataList& m_changedPoints)
{
    // update all points that changed
    for (auto gpsPoint : m_changedPoints) {
        if (mGpsMap.contains(gpsPoint.first)) {
            // update whether the point is selected or not
            mGpsMap[gpsPoint.first] = gpsPoint.second;
            // get the index of the point in the qml map
            int idx = mMapItems[gpsPoint.first];
            // emit signal to update the corresponding circle
            // IMPORTANT: This signal is handled in QML and is
            // very expensive to compute! see map.qml
            emitSetPoint(idx, gpsPoint.second);
        }
    }

    QGeoPath path = QGeoPath(mPolygon.path());
    emitSetMapSelect(path);
    newMapItems();
}

void MapHandler::setPolygon(const QPolygonF &poly)
{
    mPolygon = QGeoPolygon();
    for (int i = 0; i < poly.size(); i++)
    {
        QPointF p = poly.at(i);
        QGeoCoordinate c = QGeoCoordinate(p.x(), p.y());
        mPolygon.addCoordinate(c);
    }
    newMapItems();
}

void MapHandler::replaceData(const GpsDataList& gpsData,
                             const QPolygonF& polygon) {
    emit clearMap();

    mGpsMap.clear();
    mOrderedGpsList.clear();
    mChangedPoints.clear();
    mMapItems.clear();
    mPolyStack.clear();
    mCurrentStackPos = -1;
    mPolygon = QGeoPolygon();
    mCurrentMapItemIndex = -1;

    addPoints(gpsData);
    setPolygon(polygon);
    mPolyStack.append(mPolygon);
    mCurrentStackPos = mPolyStack.size() - 1;
}

void MapHandler::updatePointsAndPolygon(const GpsDataList& changedPoints,
                                        const QPolygonF& polygon) {
    mPolygon = QGeoPolygon();
    for (int i = 0; i < polygon.size(); i++) {
        const QPointF p = polygon.at(i);
        mPolygon.addCoordinate(QGeoCoordinate(p.x(), p.y()));
    }
    updatePoints(changedPoints);
}

//==================================================================================================
void MapHandler::drawGpsDataOnMap()
{
    if (mGpsMap.empty())
        return;

    //--- draw gps points on map
    QMapIterator<QPointF, bool> iter(mGpsMap);
    while (iter.hasNext())
    {
        iter.next();
        double latitude  = iter.key().x();
        double longitude = iter.key().y();

        // Point on map is identified by latitude and longitude (as a string) -> save double in qlonglong pointer to prevent double values from getting changed when cast into a QString
        qlonglong* pointLatLong  = (qlonglong*)&latitude;
        qlonglong* pointLongLong = (qlonglong*)&longitude;

        QString posId = QString::number(*pointLatLong) + "x" + QString::number(*pointLongLong);
        this->emitCircleSignal(QGeoCoordinate(latitude, longitude), posId, iter.value());
    }
    this->emitGetMapItems();

    //--- draw trace
    QPointF avgGpsPnt(0, 0);
    for (QPointF point : mOrderedGpsList)
    {
        avgGpsPnt += point;
        this->emitCreatePolyline(QGeoCoordinate(point.x(), point.y()));
    }

    //--- center map around average gps point
    avgGpsPnt /= mOrderedGpsList.size();
    this->emitAdjustMapCenter(QGeoCoordinate(avgGpsPnt.x(), avgGpsPnt.y()));
}

//==================================================================================================
void MapHandler::onQmlGpsClicked(const QString& text)
{
    QStringList position = text.split("x");
    // Revert qlonglong values to double (See map::addPoints)
    qlonglong latitude      = position[0].toLongLong();
    qlonglong longitude     = position[1].toLongLong();
    double* pointLatDouble  = (double*)&latitude;
    double* pointLongDouble = (double*)&longitude;

    QPointF gpsPoint(*pointLatDouble, *pointLongDouble);
    mGpsMap[gpsPoint] = !mGpsMap[gpsPoint];
    emit gpsClicked(gpsPoint, mGpsMap[gpsPoint]);
}

//==================================================================================================
void MapHandler::onQmlMapItems(const QVariant& variant)
{
    // Save qml map points
    mMapItems.clear();
    int index         = 0;
    QList<QVariant> t = variant.toList();
    for (QVariant i : t)
    {
        QObject* obj     = qvariant_cast<QObject*>(i);
        QQuickItem* item = qobject_cast<QQuickItem*>(obj);
        if (item == nullptr)
        {
            continue;
        }
        QString t = item->objectName();
        if (t != "")
        {
            QStringList position = t.split("x");
            // Revert qlonglong values to double (See map::addPoints)
            qlonglong latitude      = position[0].toLongLong();
            qlonglong longitude     = position[1].toLongLong();
            double* pointLatDouble  = (double*)&latitude;
            double* pointLongDouble = (double*)&longitude;
            QPointF point(*pointLatDouble, *pointLongDouble);
            mMapItems.insert(point, index);
        }
        index++;
    }
    applyCurrentIndexHighlight();
}

//==================================================================================================
void MapHandler::onQmlDeleteSelection()
{
    mPolygon = QGeoPolygon();
    mPolyStack.append(mPolygon);
    mCurrentStackPos++;
    newMapItems();
}

//==================================================================================================
void MapHandler::onQmlSelectionBack()
{
    if (mCurrentStackPos < 1)
    {
        return;
    }
    mCurrentStackPos--;
    mPolygon = mPolyStack[mCurrentStackPos];
    newMapItems();
}

//==================================================================================================
void MapHandler::onQmlSelectionForward()
{
    if (mCurrentStackPos > mPolyStack.size() - 2)
    {
        return;
    }
    mCurrentStackPos++;
    mPolygon = mPolyStack[mCurrentStackPos];
    newMapItems();
}

//==================================================================================================
void MapHandler::newMapItems()
{
    qDebug() << "[GeoMap][MapHandler] newMapItems polygon size:" << mPolygon.size();
    QGeoPath path = QGeoPath(mPolygon.path());
    emitSetMapSelect(path);
    QVariantList coordinates;
    coordinates.reserve(mPolygon.path().size());
    for (const auto& coord : mPolygon.path()) {
        coordinates.append(QVariant::fromValue(coord));
    }
    emitSetMapSelectCoordinates(coordinates);
    QPolygonF polF = geoPolyToPolyF();
    emit gpsSelected(polF);

}

//==================================================================================================
QPolygonF MapHandler::geoPolyToPolyF()
{
    QPolygonF polF;
    for (int i = 0; i < mPolygon.size(); i++)
    {
        QGeoCoordinate c = mPolygon.coordinateAt(i);
        polF << QPointF(c.latitude(), c.longitude());
    }
    return polF;
}

//==================================================================================================
QList<QLineF> MapHandler::geoPolyToLineList()
{
    QList<QLineF> lineList;
    // Get every subline from the polygon except the last one (last one is auto completed to the starting point)
    for (int i = 0; i < mPolygon.size() - 1; i++)
    {
        QGeoCoordinate first = mPolygon.coordinateAt(i);
        QGeoCoordinate next  = mPolygon.coordinateAt(i + 1);
        QPointF startPoint   = QPointF(first.latitude(), first.longitude());
        QPointF nextPoint    = QPointF(next.latitude(), next.longitude());
        QLineF line(startPoint, nextPoint);
        lineList.append(line);
    }
    return lineList;
}

//==================================================================================================
QPointF MapHandler::pointAtGeo(int index)
{
    QGeoCoordinate pointGeo = mPolygon.coordinateAt(index);
    return QPointF(pointGeo.latitude(), pointGeo.longitude());
}

//==================================================================================================
void MapHandler::onQmlMapClicked(const QString& text)
{
    qDebug() << "[GeoMap][MapHandler] onQmlMapClicked:" << text;
    // Get clicked gps point
    QStringList position = text.split("x");
    double latitude      = position[0].toDouble();
    double longitude     = position[1].toDouble();
    QPointF currentPoint(latitude, longitude);
    QGeoCoordinate currentCoord(latitude, longitude);

    if (mPolygon.size() < 2)
    {
        mPolygon.addCoordinate(currentCoord);
        qDebug() << "[GeoMap][MapHandler] add first/second point, size now:"
                 << mPolygon.size();
    }
    // Add line back to the starting point
    else if (mPolygon.size() == 2)
    {
        mPolygon.addCoordinate(currentCoord);
        mPolygon.addCoordinate(mPolygon.coordinateAt(0));
        qDebug() << "[GeoMap][MapHandler] close polygon, size now:"
                 << mPolygon.size();
    }
    else
    {
        QPair<bool, int> nearestOnPoint(false, 0);
        QList<QLineF> currentLines = geoPolyToLineList();
        QLineF nearestLine;
        int nearestLineIndex;
        float minDistanceFloat = FLT_MAX;
        for (int i = 0; i < currentLines.size(); i++)
        {
            QPointF start   = currentLines[i].p1();
            QPointF end     = currentLines[i].p2();
            QPointF dist    = minDistance(start, end, currentPoint);
            double distance = dist.x();

            if (distance < minDistanceFloat)
            {
                minDistanceFloat = distance;
                nearestLine      = currentLines[i];
                nearestLineIndex = i;
                // If dist.y() is 0/1 the nearest point on the current line segment is located on the end/start of it
                // In this case the previous/next line segment has the exact same distance to the new point (but because of floating points they don't caculate the same distance).
                // In this case this point will be removed and replaced by the new point
                if (dist.y() == 1)
                {
                    nearestOnPoint.first  = true;
                    nearestOnPoint.second = 1;
                }
                else if (dist.y() == 0)
                {
                    nearestOnPoint.first  = true;
                    nearestOnPoint.second = 0;
                }
                else
                {
                    nearestOnPoint.first = false;
                }
            }
        }
        // If two line segments have the same distance to the new point their intersection will be replaced by the new point
        if (nearestOnPoint.first)
        {
            if (nearestOnPoint.second == 0)
            {
                mPolygon.replaceCoordinate(nearestLineIndex, currentCoord);
                if (nearestLineIndex == 0)
                {
                    // add line back to start
                    mPolygon.insertCoordinate(0, mPolygon.coordinateAt(mPolygon.size() - 1));
                }
            }
            else
            {
                mPolygon.replaceCoordinate(nearestLineIndex + 1, currentCoord);
                if (nearestLineIndex + 1 == mPolygon.size() - 1)
                {
                    // add line back to start
                    mPolygon.addCoordinate(mPolygon.coordinateAt(0));
                }
            }
        }
        // Otherwise the new point will split the nearest line segment
        else
        {
            mPolygon.insertCoordinate(nearestLineIndex + 1, currentCoord);
        }
        qDebug() << "[GeoMap][MapHandler] insert/replace polygon point, size now:"
                 << mPolygon.size();
    }
    mPolyStack.append(mPolygon);
    mCurrentStackPos++;
    qDebug() << "[GeoMap][MapHandler] emitting polygon update, size:"
             << mPolygon.size();
    newMapItems();
}

void MapHandler::setCurrentIndex(uint index) {
    mCurrentSourceIndex = static_cast<int>(index);
    applyCurrentIndexHighlight();
}

//==================================================================================================
QPointF MapHandler::minDistance(QPointF A, QPointF B, QPointF newPoint)
{
    QPointF diffNew      = newPoint - A;
    QPointF diffExisting = B - A;
    qreal dotProduct     = QPointF::dotProduct(diffNew, diffExisting);
    double norm          = diffExisting.x() * diffExisting.x() + diffExisting.y() * diffExisting.y();
    double t             = dotProduct / norm;
    double bestT         = fmin(fmax(t, 0), 1);

    QPointF pointOnLine     = A + bestT * (B - A);
    QPointF distanceToPoint = pointOnLine - newPoint;

    return QPointF(distanceToPoint.x() * distanceToPoint.x() + distanceToPoint.y() * distanceToPoint.y(), bestT);
}

void MapHandler::applyCurrentIndexHighlight() {
    int newMapIndex = -1;
    if (mCurrentSourceIndex >= 0 &&
        mCurrentSourceIndex < mOrderedGpsList.size()) {
        const QPointF point = mOrderedGpsList[mCurrentSourceIndex];
        if (mMapItems.contains(point)) {
            newMapIndex = mMapItems.value(point);
        }
    }

    if (mCurrentMapItemIndex == newMapIndex) {
        return;
    }

    if (mCurrentMapItemIndex >= 0) {
        emit setPointHighlight(mCurrentMapItemIndex, false);
    }

    mCurrentMapItemIndex = newMapIndex;
    if (mCurrentMapItemIndex >= 0) {
        emit setPointHighlight(mCurrentMapItemIndex, true);
    }
}
