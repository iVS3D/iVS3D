#pragma once

/** @defgroup deepVisualSimilarityPlugin deepVisualSimilarityPlugin
 *
 * @ingroup Plugin
 *
 * @brief This plugin computes a feature vector for each frame using a given neural network.
 *        These identiviers are clustered with k-Means under the cosine similarity metric.
 *        It therefore selects k keyframes based on there visual similarity.
 */

#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QLabel>
#include <QMap>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpinBox>
#include <QTranslator>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent>

#include <memory>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/tracking/tracking_by_matching.hpp>

#include <NeuralNetFactory.h>

#include "ibase.h"
#include "iselection.h"
#include "reader.h"

#define RESSOURCE_PATH "/plugins/resources/neural_network_models/"
#define MEM_THRESEHOLD 0.5f
#define MAX_BATCH 100
#define NN_STD {0.229, 0.224, 0.225}
#define NN_MEAN {0.485, 0.456, 0.406}

// visuals
#define DESCRIPTION_STYLE "color: rgb(58, 58, 58); border-left: 6px solid  rgb(58, 58, 58); border-top-right-radius: 5px; border-bottom-right-radius: 5px; background-color: lightblue;"
#define UI_FRAMEREDUCTION_NAME QObject::tr("Select one frame every K frames")
#define UI_FRAMEREDUCTION_DESC QObject::tr("Reduces the amount of selected frames by the factor K.")
#define UI_NNNAME_NAME QObject::tr("Selected Neural Network")
#define UI_NNNAME_DESC QObject::tr("The drop-down shows all files in plugins/resources/neural_network_models matching ImageEmbedding*.onnx")

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
class VisualSimilarity : public PLUG::IBase, public PLUG::ISelection
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "iVS3D.IBase")
    Q_INTERFACES(PLUG::IBase PLUG::ISelection)

public:
    VisualSimilarity();
    ~VisualSimilarity() override = default;

    // IBase
    PLUG::SettingsWidgetResult getSettingsWidget() override;
    QString getName() const override;
    QMap<QString, QVariant> getSettings() const override;
    PLUG::ApplySettingsResult applySettings(
        const QMap<QString, QVariant>& settings) override;
    PLUG::InputLoadedResult onInputLoaded(
        const PLUG::InputData& input) override;
    void onCudaChanged(bool enabled) override;

    // ISelection
    PLUG::SelectionResult selectImages(const PLUG::SelectionData& data,
                                       volatile bool& cancelFlag) override;

signals:
    void syncSettingsWidget(int frameReduction,
                            QString nnFileName,
                            QStringList availableNns,
                            int maxFrames);


private slots:
    void slot_selectedNNChanged(const QString& nnName);
    void slot_frameReductionChanged(int value);

private:
    std::unique_ptr<QWidget> createSettingsWidget();
    PLUG::SelectionResult sampleImages(const std::vector<uint>& imageList,
                                       std::shared_ptr<LogFileParent> logFile,
                                       volatile bool& cancelFlag);
    bool bufferLookup(uint idx, cv::Mat* out) const;
    cv::Mat getFeatureVector(const cv::Mat& totalVector, int position) const;
    QStringList collectNns(const QString& path) const;
    PLUG::Error mapInferenceError(const NN::NeuralError& err) const;

    cv::Mat m_bufferMat = cv::Mat();
    std::vector<uint> m_bufferUsedIdx;
    Reader* m_reader = nullptr;
    bool m_useCuda = false;

    // parameters
    int m_frameReduction = 30;
    const QRegularExpression m_nnNameFormat = QRegularExpression("^(ImageEmbedding)\\w+.onnx$");
    QString m_nnFileName = "ImageEmbedding_NAME_DIMENSION_WIDTHxHEIGHT.onnx";

    // widgets
    QSpinBox* m_frameReductionInput = nullptr;
    QComboBox* m_nnNameInput = nullptr;

    NN::NeuralNetPtr m_neuralNet = nullptr;
    int m_featureDims = -1;
};
