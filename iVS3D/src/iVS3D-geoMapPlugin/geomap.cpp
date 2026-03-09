#include "geomap.h"

#include <QLocale>
#include <QCheckBox>
#include <QDebug>
#include <QMessageBox>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QSettings>
#include <QSpacerItem>

#include "../iVS3D-core/model/metaData/metadata.h"

using PLUG::ApplySettingsResult;
using PLUG::Error;
using PLUG::ErrorCode;
using PLUG::IBase;
using PLUG::InputData;
using PLUG::InputLoadedResult;
using PLUG::InputMetaData;
using PLUG::MetaDataLoadedResult;
using PLUG::SelectionData;
using PLUG::SelectionResult;
using PLUG::SettingsWidgetResult;

namespace {
constexpr int MAP_POINT_UPDATE_THRESHOLD = 100000;

const QString ORG_NAME = "Fraunhofer-IOSB";
const QString APP_NAME = "iVS3D";
const QString SETTINGS_GROUP = "geoMapPlugin";
const QString SETTINGS_PARTIAL_SELECTION_KEY = "partialSelectionEnabled";
}

GeoMap::GeoMap() : IBase() {
    QLocale locale = qApp->property("translation").toLocale();
    QTranslator* translator = new QTranslator();
    translator->load(locale, "geomap", "_", ":/translations", ".qm");
    qApp->installTranslator(translator);

    qRegisterMetaType<GpsDataList>("GpsDataList");
    qRegisterMetaType<GpsPointStateList>("GpsPointStateList");
    qRegisterMetaType<QPolygonF>("QPolygonF");

    loadPersistentSettings();
    qDebug() << "[GeoMap] Plugin constructed";
}

SettingsWidgetResult GeoMap::getSettingsWidget() {
    qDebug() << "[GeoMap] getSettingsWidget()";
    auto widget = createSettingsWidget();
    if (!widget) {
        return tl::make_unexpected(Error(
            ErrorCode::ResourceUnavailable,
            tr("Failed to create GeoMap settings widget.")));
    }
    return widget;
}

QString GeoMap::getName() const { return tr("GeoMap"); }

QMap<QString, QVariant> GeoMap::getSettings() const {
    QMap<QString, QVariant> settings;
    settings[NAME_Polygon] = QVariant::fromValue(mPolygon);
    return settings;
}

ApplySettingsResult GeoMap::applySettings(
    const QMap<QString, QVariant>& settings) {
    qDebug() << "[GeoMap] applySettings keys:" << settings.keys();
    if (!settings.contains(NAME_Polygon)) {
        return {};
    }

    const QVariant polygonVariant = settings.value(NAME_Polygon);
    if (!polygonVariant.canConvert<QPolygonF>()) {
        return tl::make_unexpected(Error(
            ErrorCode::InvalidInput,
            tr("Invalid polygon setting for GeoMap plugin.")));
    }

    mPolygon = polygonVariant.value<QPolygonF>();
    qDebug() << "[GeoMap] Restored polygon with points:" << mPolygon.size();
    emit syncMapPolygon(mPolygon);
    return {};
}

InputLoadedResult GeoMap::onInputLoaded(const InputData& input) {
    qDebug() << "[GeoMap] onInputLoaded reader valid:" << (input.reader != nullptr);
    mReader = input.reader;
    mMetaData.clear();
    mGpsData.clear();
    mPolygon = QPolygonF();
    mIsGpsAvailable = false;

    emit syncMapData(mGpsData, mPolygon);
    return {};
}

MetaDataLoadedResult GeoMap::onMetaDataLoaded(
    const InputMetaData& inputMetaData) {
    MetaData* metaData = inputMetaData.metaData;
    if (!metaData && mReader) {
        metaData = mReader->getMetaData();
    }

    mPolygon = QPolygonF();
    readMetaData(metaData);
    qDebug() << "[GeoMap] onMetaDataLoaded gpsAvailable:" << mIsGpsAvailable
             << "gpsPoints:" << mGpsData.size();
    emit syncMapData(mGpsData, mPolygon);
    return {};
}

void GeoMap::onSelectedImagesChanged(
    const std::vector<uint>& selectedImages) {
    qDebug() << "[GeoMap] onSelectedImagesChanged count:"
             << static_cast<int>(selectedImages.size());
    if (!mIsGpsAvailable || mGpsData.empty()) {
        return;
    }

    const GpsDataList oldGpsData = mGpsData;

    for (auto& gpsEntry : mGpsData) {
        gpsEntry.second = false;
    }

    for (uint index : selectedImages) {
        if (index < static_cast<uint>(mGpsData.size())) {
            mGpsData[int(index)].second = true;
        }
    }

    auto buildPointStateList = [](const GpsDataList& gpsData) {
        QList<QPointF> points;
        QList<int> selectedCounts;
        QList<int> totalCounts;

        for (const auto& entry : gpsData) {
            const int pointIndex = points.indexOf(entry.first);
            if (pointIndex < 0) {
                points.append(entry.first);
                selectedCounts.append(entry.second ? 1 : 0);
                totalCounts.append(1);
                continue;
            }

            totalCounts[pointIndex] += 1;
            if (entry.second) {
                selectedCounts[pointIndex] += 1;
            }
        }

        GpsPointStateList pointStates;
        pointStates.reserve(points.size());
        for (int i = 0; i < points.size(); i++) {
            GpsPointState state;
            state.point = points[i];
            state.used = selectedCounts[i] > 0;
            state.ratio = totalCounts[i] > 0
                              ? qreal(selectedCounts[i]) / qreal(totalCounts[i])
                              : 0.0;
            pointStates.append(state);
        }
        return pointStates;
    };

    const GpsPointStateList oldPointStates = buildPointStateList(oldGpsData);
    const GpsPointStateList newPointStates = buildPointStateList(mGpsData);

    GpsPointStateList changedGpsData;
    for (const GpsPointState& newState : newPointStates) {
        bool foundOldState = false;
        bool oldUsed = false;
        qreal oldRatio = 0.0;

        for (const GpsPointState& oldState : oldPointStates) {
            if (oldState.point == newState.point) {
                foundOldState = true;
                oldUsed = oldState.used;
                oldRatio = oldState.ratio;
                break;
            }
        }

        if (!foundOldState) {
            changedGpsData.append(newState);
            continue;
        }

        if (oldUsed != newState.used) {
            changedGpsData.append(newState);
            continue;
        }

#if GEOMAP_ENABLE_PARTIAL_SELECTION
        if (qAbs(oldRatio - newState.ratio) > 0.000001) {
            changedGpsData.append(newState);
        }
#endif
    }

    if (changedGpsData.isEmpty()) {
        return;
    }

    qDebug() << "[GeoMap] changed GPS points:" << changedGpsData.size();

    if (changedGpsData.length() < MAP_POINT_UPDATE_THRESHOLD) {
        emit syncMapPointUpdates(changedGpsData, mPolygon);
    } else {
        emit syncMapData(mGpsData, mPolygon);
    }
}

SelectionResult GeoMap::selectImages(const SelectionData& data,
                                     volatile bool& cancelFlag) {
    qDebug() << "[GeoMap] selectImages input count:"
             << static_cast<int>(data.selectedIndices.size())
             << "polygon points:" << mPolygon.size();
    if (!mIsGpsAvailable) {
        return data.selectedIndices;
    }

    if (cancelFlag) {
        return data.selectedIndices;
    }

    auto framesInsidePolygon = getFramesInsidePolygon();

    if (cancelFlag) {
        return data.selectedIndices;
    }

    auto selectedIndices = data.selectedIndices;
    if (!std::is_sorted(selectedIndices.begin(), selectedIndices.end())) {
        std::sort(selectedIndices.begin(), selectedIndices.end());
    }
    if (!std::is_sorted(framesInsidePolygon.begin(), framesInsidePolygon.end())) {
        std::sort(framesInsidePolygon.begin(), framesInsidePolygon.end());
    }

    std::vector<uint> selected;
    selected.resize(selectedIndices.size());

    auto end = std::set_intersection(
        selectedIndices.begin(), selectedIndices.end(),
        framesInsidePolygon.begin(), framesInsidePolygon.end(),
        selected.begin());
    selected.resize(size_t(end - selected.begin()));

    mPolygon = QPolygonF();
    emit syncMapPolygon(mPolygon);

    qDebug() << "[GeoMap] selectImages output count:"
             << static_cast<int>(selected.size());
    return selected;
}

void GeoMap::onGpsClicked(QPointF gpsPoint, bool used) {
    qDebug() << "[GeoMap] onGpsClicked point:" << gpsPoint << "used:" << used;
    if (!mIsGpsAvailable) {
        return;
    }

    bool updated = false;
    for (auto& gpsEntry : mGpsData) {
        if (gpsEntry.first == gpsPoint) {
            gpsEntry.second = used;
            updated = true;
        }
    }

    if (!updated) {
        return;
    }

    emit updateSelectedImages(getKeyframesFromGps());
}

void GeoMap::onGpsSelected(QPolygonF polyF) {
    qDebug() << "[GeoMap] onGpsSelected polygon points:" << polyF.size();
    if (!mIsGpsAvailable) {
        return;
    }
    mPolygon = std::move(polyF);
}

void GeoMap::onIndexChanged(uint index) {
    qDebug() << "[GeoMap] onIndexChanged index:" << index;
    emit syncCurrentIndex(index);
}

std::unique_ptr<QWidget> GeoMap::createSettingsWidget() {
    qDebug() << "[GeoMap] createSettingsWidget()";
    auto mapWidget = std::make_unique<QWidget>(nullptr);
    mapWidget->setLayout(new QVBoxLayout());
    mapWidget->layout()->setSpacing(3);
    mapWidget->layout()->setContentsMargins(3, 3, 3, 3);

    QQuickView* quickView = new QQuickView();
    auto* mapHandler = new MapHandler(mapWidget.get());
    QWidget* quickViewContainerWidget =
        QWidget::createWindowContainer(quickView, mapWidget.get());

    quickView->engine()->clearComponentCache();
    quickView->rootContext()->setContextProperty("handler", mapHandler);
    quickView->setSource(QUrl("qrc:/map.qml"));
    dynamic_cast<QVBoxLayout*>(mapWidget->layout())
        ->insertWidget(0, quickViewContainerWidget);

    if (!quickView->errors().isEmpty()) {
        return nullptr;
    }

    QObject* qmlRoot = quickView->rootObject();
    if (!qmlRoot) {
        return nullptr;
    }

    QObject::connect(qmlRoot, SIGNAL(gpsClicked(QString)),
                     mapHandler, SLOT(onQmlGpsClicked(QString)));
    QObject::connect(qmlRoot, SIGNAL(mapClicked(QString)),
                     mapHandler, SLOT(onQmlMapClicked(QString)));
    QObject::connect(qmlRoot, SIGNAL(mapItems(QVariant)),
                     mapHandler, SLOT(onQmlMapItems(QVariant)));
    QObject::connect(qmlRoot, SIGNAL(deleteSelection()),
                     mapHandler, SLOT(onQmlDeleteSelection()));
    QObject::connect(qmlRoot, SIGNAL(selectionBack()),
                     mapHandler, SLOT(onQmlSelectionBack()));
    QObject::connect(qmlRoot, SIGNAL(selectionForward()),
                     mapHandler, SLOT(onQmlSelectionForward()));

    QObject::connect(mapHandler, &MapHandler::gpsClicked,
                     this, &GeoMap::onGpsClicked, Qt::QueuedConnection);
    QObject::connect(mapHandler, &MapHandler::gpsSelected,
                     this, &GeoMap::onGpsSelected, Qt::QueuedConnection);

    QObject::connect(this, &GeoMap::syncMapData, mapHandler,
                     &MapHandler::replaceData, Qt::QueuedConnection);
    QObject::connect(this, &GeoMap::syncMapPointUpdates, mapHandler,
                     &MapHandler::updatePointsAndPolygon,
                     Qt::QueuedConnection);
    QObject::connect(this, &GeoMap::syncMapPolygon, mapHandler,
                     &MapHandler::setPolygon, Qt::QueuedConnection);
    QObject::connect(this, &GeoMap::syncCurrentIndex, mapHandler,
                     &MapHandler::setCurrentIndex, Qt::QueuedConnection);
    QObject::connect(this, &GeoMap::syncPartialSelectionMode, mapHandler,
                     &MapHandler::setPartialSelectionMode,
                     Qt::QueuedConnection);

    emit syncPartialSelectionMode(mPartialSelectionEnabled);

    mapHandler->emitAdjustMapCenter(
        QGeoCoordinate(49.01554184059616, 8.425800420583966));

    emit syncMapData(mGpsData, mPolygon);

    QPushButton* resetButton =
        new QPushButton(QObject::tr("Reset selection"), mapWidget.get());
    QObject::connect(resetButton, &QPushButton::clicked, mapHandler,
                     &MapHandler::onQmlDeleteSelection);

    QPushButton* helpButton =
        new QPushButton(QObject::tr("Help"), mapWidget.get());
    QCheckBox* partialSelectionCheckBox =
        new QCheckBox(QObject::tr("Partial selection visualization"),
                      mapWidget.get());
    partialSelectionCheckBox->setChecked(mPartialSelectionEnabled);
    QObject::connect(partialSelectionCheckBox, &QCheckBox::toggled, this,
                     [this](bool enabled) {
                         if (mPartialSelectionEnabled == enabled) {
                             return;
                         }

                         mPartialSelectionEnabled = enabled;
                         savePersistentSettings();
                         emit syncPartialSelectionMode(enabled);
                     });

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(3);
    buttonLayout->setContentsMargins(3, 3, 3, 3);
    buttonLayout->addWidget(resetButton);
    buttonLayout->addWidget(partialSelectionCheckBox);
    buttonLayout->addSpacerItem(
        new QSpacerItem(20, 20, QSizePolicy::MinimumExpanding,
                        QSizePolicy::Minimum));
    buttonLayout->addWidget(helpButton);
    mapWidget->layout()->addItem(buttonLayout);

    QObject::connect(helpButton, &QPushButton::clicked, [mapWidget = mapWidget.get()]() {
        QMessageBox::about(
            mapWidget, "GeoMap Plugin",
            QObject::tr("Select a group of keyframes by using right "
                        "mouse button to draw an encapsulating polygon."
                        "\n\n"
                        "Select or deselect individual keyframes by "
                        "clicking with the left mouse button "
                        "on the location markings.\n\n"
                        "A combination of both is also allowed."));
    });

    return mapWidget;
}

void GeoMap::loadPersistentSettings() {
    QSettings settings(ORG_NAME, APP_NAME);
    settings.beginGroup(SETTINGS_GROUP);
    mPartialSelectionEnabled =
        settings.value(SETTINGS_PARTIAL_SELECTION_KEY,
                       GEOMAP_ENABLE_PARTIAL_SELECTION != 0)
            .toBool();
    settings.endGroup();
}

void GeoMap::savePersistentSettings() const {
    QSettings settings(ORG_NAME, APP_NAME);
    settings.beginGroup(SETTINGS_GROUP);
    settings.setValue(SETTINGS_PARTIAL_SELECTION_KEY,
                      mPartialSelectionEnabled);
    settings.endGroup();
}

void GeoMap::readMetaData(MetaData* metaData) {
    mMetaData.clear();
    mGpsData.clear();
    mIsGpsAvailable = false;

    if (!metaData) {
        qDebug() << "[GeoMap] readMetaData: metadata pointer is null";
        return;
    }

    const QStringList available = metaData->availableMetaData();
    for (const QString& metaName : available) {
        if (!metaName.startsWith("GPS")) {
            continue;
        }
        qDebug() << "[GeoMap] Found metadata track:" << metaName;

        MetaDataReader* metaReader = metaData->loadMetaData(metaName);
        if (!metaReader) {
            qDebug() << "[GeoMap] Failed to load metadata reader for" << metaName;
            continue;
        }

        mMetaData = metaReader->getAllMetaData();
        for (const QVariant& var : mMetaData) {
            bool ok = false;
            const QPointF point = gpsHashToLatLong(var, &ok);
            if (ok) {
                mGpsData.append(QPair<QPointF, bool>(point, true));
            }
        }

        if (!mGpsData.isEmpty()) {
            mIsGpsAvailable = true;
            qDebug() << "[GeoMap] Loaded GPS points:" << mGpsData.size();
            return;
        }
    }
    qDebug() << "[GeoMap] No valid GPS metadata found";
}

std::vector<unsigned int> GeoMap::getKeyframesFromGps() const {
    std::vector<unsigned int> keyframes;
    for (int i = 0; i < mGpsData.size(); i++) {
        if (mGpsData[i].second) {
            keyframes.push_back(static_cast<unsigned int>(i));
        }
    }
    return keyframes;
}

std::vector<unsigned int> GeoMap::getFramesInsidePolygon() const {
    if (mPolygon.length() == 0) {
        return std::vector<unsigned int>();
    }

    std::vector<unsigned int> keyframes;
    for (int i = 0; i < mGpsData.size(); i++) {
        if (mPolygon.containsPoint(mGpsData[i].first, Qt::OddEvenFill)) {
            keyframes.push_back(static_cast<unsigned int>(i));
        }
    }
    return keyframes;
}

QPointF GeoMap::gpsHashToLatLong(const QVariant& hash, bool* ok) const {
    bool valid = true;
    QHash<QString, QVariant> gpsHash = hash.toHash();

    bool latitudeOk = false;
    bool longitudeOk = false;
    const double latitudeAbs =
        gpsHash.value("GPSLatitude").toDouble(&latitudeOk);
    const double longitudeAbs =
        gpsHash.value("GPSLongitude").toDouble(&longitudeOk);

    const QString latitudeRef = gpsHash.value("GPSLatitudeRef").toString();
    const QString longitudeRef = gpsHash.value("GPSLongitudeRef").toString();

    valid &= latitudeOk && longitudeOk;
    valid &= (latitudeRef == "N" || latitudeRef == "S");
    valid &= (longitudeRef == "E" || longitudeRef == "W");

    if (ok) {
        *ok = valid;
    }
    if (!valid) {
        return QPointF();
    }

    const double latitude =
        (latitudeRef == "N") ? latitudeAbs : -latitudeAbs;
    const double longitude =
        (longitudeRef == "E") ? longitudeAbs : -longitudeAbs;
    return QPointF(latitude, longitude);
}

QGeoCoordinate GeoMap::gpsHashtoGeoCo(const QVariant& hash) const {
    bool ok = false;
    QPointF latLong = gpsHashToLatLong(hash, &ok);
    if (!ok) {
        return QGeoCoordinate();
    }
    return QGeoCoordinate(latLong.x(), latLong.y());
}

double GeoMap::distanceBetweenPoints(int first, int second) const {
    if (first < 0 || second < 0 || first >= mMetaData.size() ||
        second >= mMetaData.size()) {
        return 0.0;
    }

    QGeoCoordinate firstGPS = gpsHashtoGeoCo(mMetaData.at(first));
    QGeoCoordinate secondGPS = gpsHashtoGeoCo(mMetaData.at(second));
    QPointF firstLatLong(firstGPS.latitude(), firstGPS.longitude());
    QPointF secondLatLong(secondGPS.latitude(), secondGPS.longitude());
    return greatCircleDistance(firstLatLong, secondLatLong);
}

double GeoMap::greatCircleDistance(QPointF first, QPointF second) const {
    const int earthRadiusM = 6371008;
    constexpr double pi = 3.14159265358979323846;

    const double lat1 = first.x() * (pi / 180.0);
    const double lat2 = second.x() * (pi / 180.0);
    const double latDiff = (second.x() - first.x()) * (pi / 180.0);
    const double longDiff = (second.y() - first.y()) * (pi / 180.0);

    const double a = std::pow(std::sin(latDiff / 2.0), 2.0) +
                     std::cos(lat1) * std::cos(lat2) *
                         std::pow(std::sin(longDiff / 2.0), 2.0);
    const double distance = 2.0 * earthRadiusM * std::asin(std::sqrt(a));
    return distance;
}
