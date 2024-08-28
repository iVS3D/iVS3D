#ifndef VISUALSIMILARITY_H
#define VISUALSIMILARITY_H

/** @defgroup deepVisualSimilarityPlugin deepVisualSimilarityPlugin
 *
 * @ingroup Plugin
 *
 * @brief This plugin computes a feature vector for each frame using a given neural network.
 *        These identiviers are clustered with k-Means under the cosine similarity metric.
 *        It therefore selects k keyframes based on there visual similarity.
 */

#include <QObject>
#include <QWidget>
#include <QString>
#include <QMap>
#include <QLayout>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QSpacerItem>
#include <QSizePolicy>
#include <QTranslator>
#include <QCoreApplication>
#include <QDialog>
#include <QElapsedTimer>
#include <QDebug>
#include <QDir>
#include <QtConcurrent>

#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/tracking/tracking_by_matching.hpp>

#include "ialgorithm.h"
#include "reader.h"
#include "progressable.h"
#include "signalobject.h"

#define RESSOURCE_PATH "/plugins/resources/neural_network_models/"
#define MEM_THRESEHOLD 0.7f
#define MAX_BATCH 100
#define NN_STD cv::Scalar({0.229, 0.224, 0.225})
#define NN_MEAN cv::Scalar({0.485, 0.456, 0.406})

// visuals
#define DESCRIPTION_STYLE "color: rgb(58, 58, 58); border-left: 6px solid  rgb(58, 58, 58); border-top-right-radius: 5px; border-bottom-right-radius: 5px; background-color: lightblue;"
#define UI_FRAMEREDUCTION_NAME QObject::tr("Select one frame every K framese")
#define UI_FRAMEREDUCTION_DESC QObject::tr("Reduces the amount of selected frames by the factor K.")
#define UI_NNNAME_NAME QObject::tr("Selected Neural Network")
#define UI_NNNAME_DESC QObject::tr("The drop-down shows all files in the folder plugins/ressources/neural_network_models/ that follow the format ImageEmbedding_NAME_DIMENSION_WIDHTxHEIGHT.onnx")
#define UI_NNNAME_BT_DESC QObject::tr("Resets the drop-Down and reloads available neural networks.")

// json settings
#define FRAMEREDUCTION_JSON_NAME "K"
#define NNNAME_JSON_NAME "NN-Name"

// log file
#define LF_TIMER_KEYFRAMES "keyframeSelection"
#define LF_TTHRESHOLD "tDistThreshold"
#define LF_RESULT_INFO_TAG "result_info"
#define LF_COMPUTE_INFO_TAG "compute_info"
#define LF_SELECTION_INFO_TAG "selection_info"
#define LF_TIMER_NN "NN feeding timer"
#define LF_TIMER_KMEANS "kMeans timer"
#define LF_TIMER_BUFFER "safe buffer timer"

// buffer
#define BUFFER_NAME_FEATURES "DeepVisSimilarityFeatureVector"
#define BUFFER_FEATURE_DELIMITER_X ","
#define BUFFER_FEATURE_DELIMITER_Y ";"
#define BUFFER_NAME_IDX "DeepVisSimilarityIdx"

/**
 * @class deepVisualSimilarity
 *
 * @ingroup deepVisualSimilarityPlugin
 *
 * @brief This plugin computes a feature vector for each frame using a given neural network.
 *        These identiviers are clustered with k-Means under the cosine similarity metric.
 *        It therefore selects k keyframes based on there visual similarity.
 *
 * @author Dominic Zahn
 */
class VisualSimilarity : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "iVS3D.IAlgorithm") // implement interface as plugin, use the iid as identifier
    Q_INTERFACES(IAlgorithm)    // declare this as implementation of IAlgorithm interface

public:
    VisualSimilarity();
    ~VisualSimilarity();

    /**
     * @brief returns a widget to change parameters of this plugin.
     * @param parent The parent for the SettingsWidget.
     */
    QWidget* getSettingsWidget(QWidget *parent) override;

    /**
     * @brief sampleImages Create an keyframe list with indices of the keyframes. The algorithm reports progress
     * to the Progressable *receiver by calling Progressable::slot_makeProgress. If the *stopped bool is set to @a true
     * the algorithm should abort the calculations and exit.
     *
     * @param imageList is a preselection of frames
     * @param receiver is a progressable, which displays the progress made so far
     * @param stopped Pointer to a bool indication if user wants to stop the computation
     * @param useCuda defines if the compution should run on graphics card
     * @param logFile can be used to protocoll progress or problems
     * @return A list of indices, which represent the selected keyframes.
     */
    std::vector<uint> sampleImages(const std::vector<unsigned int> &imageList, Progressable *receiver, volatile bool *stopped, bool useCuda, LogFileParent* logFile) override;
    /**
     * @brief getName Returns the plugin name
     * @return "orbslam"
     */
    QString getName() const override;
    /**
     * @brief initialize is called after new images have been loaded. the plugin parameters can be selected based on the images.
     * @param reader for the images
     * @param buffer contains persistently stored plugin data from the project file. This includes data stored using getSettings() and generateSettings()
     * @param sigObj provides signals from the core
     */
    void initialize(Reader* reader, QMap<QString, QVariant> buffer, signalObject* sigObj) override;
    /**
     * @brief setter for plugin's settings
     * @param QMap with the settings
     */
    void setSettings(QMap<QString, QVariant> settings) override;
    /**
     * @brief generateSettings tries to generate the best settings for the current input
     * @param receiver is a progressable, which displays the already made progress
     * @param buffer QVariant with the buffered data form last call to sampleImages
     * @param useCuda @a true if cv::cuda can be used
     * @param stopped is set if the algorithm should abort
     * @return QMap with the settings
     */
    QMap<QString, QVariant> generateSettings(Progressable *receiver, bool useCuda, volatile bool* stopped) override;
    /**
     * @brief getter for plugin's settings
     * @return QMap with the settings
     *
     */
    QMap<QString, QVariant> getSettings() override;


private slots:
    void slot_selectedNNChanged(QString nnName);
    void slot_reloadNN(int index);

private:
    // functions
    static void displayProgress(Progressable *p, int progress, QString msg);
    static void displayMessage(Progressable *p, QString msg);
    void feedImage(cv::Mat inblob, cv::Mat *totalFeatureVector, cv::dnn::Net *nn);
    bool bufferLookup(uint idx, cv::Mat *out);
    cv::Mat getFeatureVector(cv::Mat totalVector, int position);
    void sendBuffer(cv::Mat bufferMat, std::vector<uint> calculatedIdx);
    void readBuffer(QMap<QString,QVariant> buffer);
    cv::Mat stringToBufferMat(QString string);
    QStringList collect_nns(QString path);
    void displayErrorMessage(QString message);
    //

    cv::Mat m_bufferMat = cv::Mat();
    std::vector<uint> m_bufferUsedIdx;
    Reader *m_reader = nullptr;
    signalObject *m_signalObject = nullptr;
    // parameters
    int m_frameReduction = -1;
    int m_featureDims = -1;
    cv::Size m_nnInputSize = cv::Size(-1,-1);
    const QRegularExpression m_nnNameFormat = QRegularExpression("^(ImageEmbedding)\\w+_(?<featureDims>\\d+)_(?<width>\\d+)x(?<height>\\d+).onnx$");
    QString m_nnFileName = "ImageEmbedding_NAME_DIMENSION_WIDTHxHEIGHT.onnx";
    // widgets
    QWidget *m_settingsWidget = nullptr;
    QSpinBox *m_frameReductionInput = nullptr;
    QComboBox *m_nnNameInput = nullptr;

    void createSettingsWidget(QWidget *parent);
};

#endif // VISUALSIMILARITY_H
