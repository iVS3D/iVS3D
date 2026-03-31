#ifndef IVS3D_GEOMAPPLUGIN_H
#define IVS3D_GEOMAPPLUGIN_H

// Std
#include <cmath>
#include <algorithm>
#include <memory>

// Qt
#include <QCoreApplication>
#include <QLabel>
#include <QLayout>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QPushButton>
#include <QTranslator>
#include <QVBoxLayout>
#include <QWidget>

// iVS3D-core
#include "metadata.h"

// iVS3D-pluginInterface
#include "ibase.h"
#include "iselection.h"

#include "maphandler.h"

#define DESCRIPTION_STYLE "color: rgb(58, 58, 58); border-left: 6px solid  rgb(58, 58, 58); " \
                          "border-top-right-radius: 5px; border-bottom-right-radius: 5px; "   \
                          "background-color: lightblue;"

#define NAME_Polygon "Polygon"
/**
 * @class GeoMap
 *
 * @ingroup GeoMapPlugin
 *
 * @brief The GeoMapPlugin allows the user to select the images, which are to be used for the 3D
 * reconstruction, based on their geographic position.
 *
 * In this, the location (derived from the meta data) of the images are displayed on a map. The
 * user is then prompted to draw a region of interest in the form of a polygon. All images whose
 * geographic location lie inside this polygon are than exported and prepared for the 3D
 * reconstruction.
 *
 * @author Daniel Brommer
 * @author Boitumelo Ruf
 */
class GeoMap : public PLUG::IBase, public PLUG::ISelection
{
    Q_OBJECT

    // implement interface as plugin, use the iid as identifier
    Q_PLUGIN_METADATA(IID "iVS3D.IBase")

    // declare this as implementation of IBase and ISelection interfaces
    Q_INTERFACES(PLUG::IBase PLUG::ISelection)

    //--- METHOD DECLARATION ---//

  public:
    /**
     * @brief Construct and initialize plugin with default parameterization.
     *
     * Default construction.
     */
    GeoMap();

    /**
     * @brief Destroy object of GeoMapPlugin.
     */
    ~GeoMap() override = default;

    /**
     * @brief getSettingsWidget is provides an QWidget to display plugin specific settings to the
     * user. The Widget is not deleted by the core application, so storage management is duty of the
     * plugin.
     *
     * @return The QWidget with the plugin settings
     */
    PLUG::SettingsWidgetResult getSettingsWidget() override;

    /**
     * @brief getName returns the display name for the plugin. This name is presented to the user.
     * @return The name to display
     */
    QString getName() const override;

    QMap<QString, QVariant> getSettings() const override;
    PLUG::ApplySettingsResult applySettings(
        const QMap<QString, QVariant>& settings) override;
    PLUG::InputLoadedResult onInputLoaded(const PLUG::InputData& input) override;
    PLUG::MetaDataLoadedResult onMetaDataLoaded(
      const PLUG::InputMetaData& inputMetaData) override;
    void onSelectedImagesChanged(
        const std::vector<uint>& selectedImages) override;
    void onIndexChanged(uint index) override;

    PLUG::SelectionResult selectImages(const PLUG::SelectionData& data,
                                 volatile bool& cancelFlag) override;

  signals:
    void syncMapData(const GpsDataList& gpsData, const QPolygonF& polygon);
    void syncMapPointUpdates(const GpsPointStateList& changedGpsData,
                             const QPolygonF& polygon);
    void syncMapPolygon(const QPolygonF& polygon);
    void syncCurrentIndex(uint index);
    void syncPartialSelectionMode(bool enabled);

  public slots:

    /**
     * @brief onGpsClicked slot triggered when gps point on the map is clicked
     * @param gpsPoint gps point which has been clicked
     * @param used @a true if points is used AFTER click @a false otherwise
     */
    void onGpsClicked(QPointF gpsPoint, bool used);

    /**
     * @brief onGpsSelected Slot is triggered when a new user selcted polyogn is created
     * @param polyF Perimeter of the polyogn
     */
    void onGpsSelected(QPolygonF polyF);


  private:
    std::unique_ptr<QWidget> createSettingsWidget();
    void loadPersistentSettings();
    void savePersistentSettings() const;

    void readMetaData(MetaData* metaData);
    /// these are the localy stored keyframes
    std::vector<unsigned int> getKeyframesFromGps() const;
    /// these are all frames inside the user created polygon
    std::vector<unsigned int> getFramesInsidePolygon() const;

    QPointF gpsHashToLatLong(const QVariant& hash, bool* ok = nullptr) const;
    QGeoCoordinate gpsHashtoGeoCo(const QVariant& hash) const;

    double distanceBetweenPoints(int first, int second) const;
    double greatCircleDistance(QPointF first, QPointF second) const;
    //--- MEMBER DECLARATION ---//

  private:
    Reader* mReader = nullptr;

    /// List of available meta data
    QList<QVariant> mMetaData;

    /// List of GPS data associated with the images
    QList<QPair<QPointF, bool>> mGpsData;

    QPolygonF mPolygon;

    /// Flag indicating if GPS data is available
    bool mIsGpsAvailable = false;
    bool mPartialSelectionEnabled = GEOMAP_ENABLE_PARTIAL_SELECTION != 0;
};

#endif // IVS3D_GEOMAPPLUGIN_H
