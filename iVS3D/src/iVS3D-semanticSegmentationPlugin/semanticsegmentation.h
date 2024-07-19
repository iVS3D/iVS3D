#ifndef NTHFRAME_H
#define NTHFRAME_H

/** @defgroup SemanticSegmentationPlugin SemanticSegmentationPlugin
 *
 * @ingroup Plugin
 *
 * @brief Plugin to create semantic masks for images. The Plugin implements
 * the ITransform interface to be loaded in the core application.
 */

#include <QObject>
#include <QString>
#include <QTimer>
#include <QDir>
#include <QException>
#include <QCoreApplication>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QTranslator>

#include <QDebug>

#include "semanticsegmentation_global.h"
#include "itransform.h"
#include "settingswidget.h"

#include <iostream>
#include <fstream>

#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>

#include <iostream>
#include <iomanip>
#include <regex>

#define MODEL_PATH "/plugins/resources/neural_network_models"
#define HW_NAME(x) x ? "Using GPU (cuda)" : "Using CPU"
#define USED_MODEL "Used Model"
#define SELECTED_CLASSES "Selected classes"

/**
 * @class SemanticSegmentation
 *
 * @ingroup SemanticSegmentationPlugin
 *
 * @brief The SemanticSegmentation class is used to create binary masks for the reconstruction. The masks are
 * based on a semantic map created by an neural network model. The models can be added to the @a models/SemanticSegmentation
 * path. To be recognized and loaded by the plugin they need to be in .onnx format. Plugin allows to use CUDA api with cuDNN
 * to accelerate evaluation of the model using the gpu.
 *
 * @author Dominik Wüst
 *
 * @date 2021/03/10
 */
class SEMANTICSEGMENTATION_EXPORT SemanticSegmentation : public ITransform
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "iVS3D.ITransform")   // implement interface as plugin, use the iid as identifier
    Q_INTERFACES(ITransform)                        // declare this as implementation of ITransform interface

public:
    /**
     * @brief Creates a Semantic Segmentation Plugin and loads the models for image segmentation.
     */
    SemanticSegmentation();
    ~SemanticSegmentation();

    /**
     * @brief getSettingsWidget provides a QWidget for the user. This allows to select one of the models for segmentation.
     * The classes with their associated colors are listed and the ones to include in the output mask can bee selected.
     * The blending alpha for overlaying the semantic map can be adjusted.
     * @param parent The parent for the QWidget
     * @return The QWidget with the settings
     */
    QWidget* getSettingsWidget(QWidget *parent) override;

    /**
     * @brief getName returns a name for displaying this algorithm to the user.
     * @return Semantic Segmentation Algorithm as QString.
     */
    QString getName() const override;

    /**
     * @brief getOutputNames returns a list of folder names containing @a masks to export the output to.
     * @return The output folder names in a QStringList
     */
    QStringList getOutputNames() override;

    /**
     * @brief copy creates a new SemanticSegmentation instance and copies the attributes from this.
     * @return A pointer to the new instance.
     */
    ITransform *copy() override;

    /**
     * @brief transform creates a binary mask for the given image. A nn model is loaded from an .onnx file and used to
     * compute a semantic map. From this a binary mask for the image is computed and returned. The model as well as the
     * classes to include in the mask can be selected in the SettingsWidget accessable through
     * SemanticSegmentation::getSettingsWidget. The ITransform::sendToGui signal is emitted before the calculation is started
     * to present the given image to the user. The gui is once more updated with the finished semantic map and mask side by side.
     * @param idx The index of the image to transform
     * @param img The image to transform
     * @return A list containing the binary mask, or empty if no model selected
     *
     * @see ITransform::sendToGui
     */
    ImageList transform(uint idx, const cv::Mat &img) override;

    /**
     * @brief enableCuda is called if the user toggles the use CUDA flag in the core application.
     * @param enabled allows to ue CUDA api if @a true
     */
    void enableCuda(bool enabled) override;

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

signals:
    /**
     * @brief [signal] sig_classesAndColorsChanged is emitted if a new model with different classes and colors is selected.
     * @param classes The names of the new classes
     * @param colors The colors in the semantic map for the new classes
     * @param selectedClasses bools for the currently selected classes
     */
    void sig_classesAndColorsChanged(QStringList classes, QColorList colors, QBoolList selectedClasses);

    /**
     * @brief [signal] sig_message is emmitted if the algorithm has a message for the user to display.
     * @param processor The processor currently used (cpu or gpu)
     * @param message The information to display as text
     */
    void sig_message(QString processor, QString message = "");

private slots:
    // --- slots for signals from SettingsWidget ---
    void slot_ONNXindexChanged(int n);
    void slot_selectedClassesChanged(QBoolList classes);
    void slot_blendAlphaChanged(float alpha);
    // --- tasks for transforming an image ---
    // (packed as slots to add tasks to event queue)
    void slot_computeScore();           // evaluate selected m_ONNXmodel (input:
    void slot_computeSegmentation();
    void slot_computeMask();
    void slot_sendGuiPreview();

private:
    // --- gui data
    SettingsWidget *m_settingsWidget;   // settings widget for this ITransform
    float m_blendAlpha;                 // alpha for semantic map overlay
    bool m_guiUpToDate;                 // is true if gui is up to date, used to avoid unnessecary updates
    // --- ONNX model data ---
    QStringList m_ONNXmodelList;    // model names
    cv::dnn::Net *m_ONNXmodel;       // active model
    bool m_ONNXmodelLoaded;         // true if selected model is loaded as active model
    int m_ONNXmodelIdx;             // index of selected model in m_ONNXmodelList
    QBoolList m_ONNXselectedClasses;// bool for each class of selected model (true if class selected)
    // --- Buffer for image, nn score, segmentation and mask ---
    uint m_imageIdx;
    cv::Mat m_image;
    cv::Mat m_score;
    cv::Mat m_segmentation;
    cv::Mat m_mask;
    // --- use CUDA acceleration ---
    bool m_useCuda;
    // --- Widget synch ---
    bool m_updateClasses = true;       //Disable updates if classes are set manuel

    // --- get Classes and Colors for each model ---
    void getClassesAndColors(QStringList &classes, QList<QColor> &colors);
    void readClassesAndColorsFile(std::vector<std::string> &classes, std::vector<cv::Vec3b> &colors, const std::string &filepath);
    // --- read size of input layer for each model ---
    void getInputHeightAndWidth(int &inputHeight, int &inputWidth, int modelIdx);
    // --- blend two images using an alpha value ---
    void alphaBlend(const cv::Mat &foreground,const cv::Mat &background, cv::Mat &destionation, float alpha);
    // --- create semanitc map and binary mask from score ---
    void colorizeSegmentationBinary(const cv::Mat score, cv::Mat &segmentation, QList<bool> selectedClasses);
    void colorizeSegmentation(const cv::Mat score, cv::Mat &segmentation, QList<QColor> colorList);
    // --- load model to ram / vram ---
    void loadModel();
    QMutex m_mutex;
};

#endif // NTHFRAME_H
