#ifndef IVS3D_GEOMAPPLUGIN_H
#define IVS3D_GEOMAPPLUGIN_H

// Std
#include <cmath>
#include <memory>
#include <algorithm>

// Qt
#include <QCheckBox>
#include <QLabel>
#include <QLayout>
#include <QMap>
#include <QObject>
#include <QPushButton>
#include <QSpinBox>
#include <QWidget>
#include <QTabWidget>

// iVS3D-core
#include "../iVS3D-core/model/progressable.h"
#include "../iVS3D-core/model/reader/reader.h"
#include "../iVS3D-core/plugin/signalobject.h"

// iVS3D-pluginInterface
#include "../iVS3D-pluginInterface/ialgorithm.h"

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
class GeoMap : public IAlgorithm
{
    Q_OBJECT

    // implement interface as plugin, use the iid as identifier
    Q_PLUGIN_METADATA(IID "iVS3D.IAlgorithm")

    // declare this as implementation of IAlgorithm interface
    Q_INTERFACES(IAlgorithm)

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
    virtual ~GeoMap();

    /**
     * @brief getSettingsWidget is provides an QWidget to display plugin specific settings to the
     * user. The Widget is not deleted by the core application, so storage management is duty of the
     * plugin.
     *
     * @param parent The parent for the QWidget
     * @return The QWidget with the plugin settings
     */
    QWidget* getSettingsWidget(QWidget* parent) override;

    /**
     * @brief getName returns the display name for the plugin. This name is presented to the user.
     * @return The name to display
     */
    QString getName() const override;

    /**
     * @brief sampleImages selects the keyframes from the given images. The computation is based on
     * the images provided by the given Reader. The imageList provides indices for the currently
     * selected keyframes.
     *
     * @param imageList Index list of images to compute, but indices in between can be used for
     * computation
     * @param receiver The Progressable to invoke to report progress
     * @param stopped Flag @a true if the computation should abort, @a false if it should continue
     * @param useCuda @a true if cv::cuda can be used
     * @param logFile can be used to protocol progress or problems
     * @return The indices of the selected keyframes
     */
    std::vector<uint> sampleImages(const std::vector<uint>& imageList,
                                   Progressable* receiver, volatile bool* stopped,
                                   bool useCuda, LogFileParent* logFile) override;

    /**
     * @brief initialize the the IAlgorithm and the settings widget with plausible values from the
     * Reader.
     * @param reader The reader with the images
     * @param buffer QVariant with the buffered data form last call to sampleImages
     * @param sigObj provides signals from the core application
     */
    void initialize(Reader* reader, QMap<QString, QVariant> buffer, signalObject* sigObj) override;

    /**
     * @brief setter for plugin's settings
     * @param QMap with the settings
     */
    void setSettings(QMap<QString, QVariant> settings) override;

    /**
     * @brief getter for plugin's settings
     * @return QMap with the settings
     */
    QMap<QString, QVariant> getSettings() override;

    /**
     * @brief generateSettings tries to generate the best settings for the current input
     * @param receiver is a progressable, which displays the already made progress
     * @param buffer QVariant with the buffered data form last call to sampleImages
     * @param useCuda @a true if cv::cuda can be used
     * @param stopped is set if the algorithm should abort
     * @return QMap with the settings
     */
    QMap<QString, QVariant> generateSettings(Progressable* receiver, bool useCuda,
                                             volatile bool* stopped) override;

  public slots:

    /**
     * @brief onGpsClicked slot triggered when gps point on the map is clicked
     * @param gpsPoint gps point which has been clicked
     * @param used @a true if points is used AFTER click @a false otherwise
     */
    void onGpsClicked(QPointF gpsPoint, bool used);

    /**
     * @brief onNewMetaData Slot is triggerd if the core loads new meta data
     */
    void onNewMetaData();
    /**
     * @brief onKeyframesChanged Slot is triggerd if the core emtis new keyframes
     * @param keyframes Keyframe vector
     */
    void onKeyframesChanged(std::vector<uint> keyframes);

    /**
     * @brief onGpsSelected Slot is triggered when a new user selcted polyogn is created
     * @param polyF Perimeter of the polyogn
     */
    void onGpsSelected(QPolygonF polyF);


  private:
    void createSettingsWidget(QWidget* parent);

    void readMetaData();
    /// these are the localy stored keyframes
    std::vector<unsigned int> getKeyframesFromGps();
    /// these are all frames inside the user created polygon
    std::vector<unsigned int> getFramesInsidePolygon();

    QPointF gpsHashToLatLong(QVariant hash);
    QGeoCoordinate gpsHashtoGeoCo(QVariant hash);

    void initializeQmlMap();
    void reinitializeQmlMap();

    double distanceBetweenPoints(int first, int second);
    double greatCircleDistance(QPointF first, QPointF second);
    //--- MEMBER DECLARATION ---//

  private:
    /// Pointer to signal object of iVS3D-core
    signalObject* mpSigObj;

    /// Pointer to reader object of iVS3D-core
    Reader* mpReader;

    /// Pointer to QML container widget
    QWidget* mpQuickViewContainerWidget;

    /// Pointer to the map widget
    QWidget* mpMapWidget;

    /// Pointer to map handler
    std::shared_ptr<MapHandler> mpMapHandler;

    /// Buffered data form last call to sampleImages
    QMap<QString, QVariant> mBuffer;

    /// List of available meta data
    QList<QVariant> mMetaData;

    /// List of GPS data associated with the images
    QList<QPair<QPointF, bool>> mGpsData;

    QPolygonF mPolygon;

    bool mIsQmlMapInitialized;

    /// Flag indicating if GPS data is available
    bool mIsGpsAvailable;

};

#endif // IVS3D_GEOMAPPLUGIN_H
