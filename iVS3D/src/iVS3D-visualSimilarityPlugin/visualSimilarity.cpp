#include "visualSimilarity.h"

#include <NeuralUtil.h>

#include <QCoreApplication>
#include <QFile>
#include <QHBoxLayout>
#include <QLocale>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>

using PLUG::ApplySettingsResult;
using PLUG::Error;
using PLUG::ErrorCode;
using PLUG::InputData;
using PLUG::InputLoadedResult;
using PLUG::SelectionData;
using PLUG::SelectionResult;
using PLUG::SettingsWidgetResult;

VisualSimilarity::VisualSimilarity()
    : IBase()
{
    QLocale locale = qApp->property("translation").toLocale();
    auto* translator = new QTranslator();
    translator->load(locale, "visualSimilarity", "_", ":/translations", ".qm");
    qApp->installTranslator(translator);
}

SettingsWidgetResult VisualSimilarity::getSettingsWidget()
{
    auto widget = createSettingsWidget();
    if (!widget)
    {
        return tl::make_unexpected(Error(
            ErrorCode::ResourceUnavailable,
            tr("Failed to create Deep Visual Similarity settings widget.")));
    }
    return widget;
}

QString VisualSimilarity::getName() const
{
    return tr("Deep Visual Similarity");
}

QMap<QString, QVariant> VisualSimilarity::getSettings() const
{
    QMap<QString, QVariant> settings;
    settings.insert(FRAMEREDUCTION_JSON_NAME, m_frameReduction);
    settings.insert(NNNAME_JSON_NAME, m_nnFileName);
    return settings;
}

ApplySettingsResult VisualSimilarity::applySettings(
    const QMap<QString, QVariant>& settings)
{
    const int frameReduction =
        settings.value(FRAMEREDUCTION_JSON_NAME, m_frameReduction).toInt();
    const QString nnFileName =
        settings.value(NNNAME_JSON_NAME, m_nnFileName).toString().trimmed();

    if (frameReduction < 2)
    {
        return tl::make_unexpected(
            Error(ErrorCode::InvalidInput,
                  tr("K must be greater than or equal to 2.")));
    }

    if (nnFileName.isEmpty())
    {
        return tl::make_unexpected(
            Error(ErrorCode::InvalidInput, tr("No neural network was selected.")));
    }

    m_frameReduction = frameReduction;
    m_nnFileName = nnFileName;
    m_neuralNet = nullptr;

    const int maxFrames = m_reader ? std::max(2, int(m_reader->getPicCount())) : 2;
    emit syncSettingsWidget(m_frameReduction,
                            m_nnFileName,
                            collectNns(QCoreApplication::applicationDirPath() +
                                       RESSOURCE_PATH),
                            maxFrames);
    return {};
}

InputLoadedResult VisualSimilarity::onInputLoaded(const InputData& input)
{
    m_reader = input.reader;
    m_neuralNet = nullptr;
    m_featureDims = -1;

    // In-memory cache only for current input.
    m_bufferMat.release();
    m_bufferUsedIdx.clear();

    if (m_reader)
    {
        const int picCount = std::max(1, int(m_reader->getPicCount()));
        const int fps =
            (m_reader->getFPS() > 0.0) ? int(std::round(m_reader->getFPS())) : 30;
        m_frameReduction = std::clamp(fps, 2, std::max(2, picCount));
    }

    const int maxFrames = m_reader ? std::max(2, int(m_reader->getPicCount())) : 2;
    emit syncSettingsWidget(m_frameReduction,
                            m_nnFileName,
                            collectNns(QCoreApplication::applicationDirPath() +
                                       RESSOURCE_PATH),
                            maxFrames);
    return {};
}

void VisualSimilarity::onCudaChanged(bool enabled)
{
    if (m_useCuda == enabled)
    {
        return;
    }
    m_useCuda = enabled;
    m_neuralNet = nullptr;
}

SelectionResult VisualSimilarity::selectImages(const SelectionData& data,
                                               volatile bool& cancelFlag)
{
    if (data.reader)
    {
        m_reader = data.reader;
    }

    if (!m_reader)
    {
        return tl::make_unexpected(
            Error(ErrorCode::InvalidInput, tr("No reader available.")));
    }

    return sampleImages(data.selectedIndices, data.logFile, cancelFlag);
}

SelectionResult VisualSimilarity::sampleImages(const std::vector<uint>& imageList,
                                               std::shared_ptr<LogFileParent> logFile,
                                               volatile bool& cancelFlag)
{
    if (imageList.size() < 2)
    {
        return imageList;
    }

    if (cancelFlag)
    {
        return tl::make_unexpected(
            Error(ErrorCode::RuntimeError, tr("Selection cancelled by user.")));
    }

    if (m_nnFileName.trimmed().isEmpty())
    {
        return tl::make_unexpected(
            Error(ErrorCode::InvalidInput, tr("No neural network was selected.")));
    }

    const QString neuralNetAbsolutePath =
        QCoreApplication::applicationDirPath() + RESSOURCE_PATH + m_nnFileName;
    if (!QFile::exists(neuralNetAbsolutePath))
    {
        return tl::make_unexpected(
            Error(ErrorCode::ResourceUnavailable,
                  tr("Neural network model not found: %1").arg(neuralNetAbsolutePath)));
    }

    if (logFile)
    {
        logFile->startTimer(LF_TIMER_NN);
    }
    emit updateProgress(0, tr("Loading Neural Network"));

    if (!m_neuralNet || m_neuralNet->gpuId() != (m_useCuda ? 0 : -1))
    {
        auto createResult =
            NN::NeuralNetFactory::create(neuralNetAbsolutePath.toStdString(), m_useCuda);
        if (!createResult)
        {
            return tl::make_unexpected(Error(
                ErrorCode::ResourceUnavailable,
                tr("Failed to create neural network from file: %1")
                    .arg(neuralNetAbsolutePath)));
        }
        m_neuralNet = createResult.value();
    }

    if (m_neuralNet->inputShape().size() != 4)
    {
        auto neuralNetShape = m_neuralNet->inputShape();
        SHAPE_DEBUG_PRINT(neuralNetShape);
        m_neuralNet = nullptr;
        return tl::make_unexpected(
            Error(ErrorCode::InvalidInput,
                  tr("The selected neural network does not have the expected "
                     "input shape.")));
    }

    int batch = m_neuralNet->inputShape()[0];
    int channels = m_neuralNet->inputShape()[1];
    int height = m_neuralNet->inputShape()[2];
    int width = m_neuralNet->inputShape()[3];

    if (height == -1 || width == -1)
    {
        const cv::Mat firstImage = m_reader->getPic(imageList.front());
        if (firstImage.empty())
        {
            m_neuralNet = nullptr;
            return tl::make_unexpected(
                Error(ErrorCode::InvalidInput,
                      tr("Could not read first input image for dynamic neural "
                         "network shape resolution.")));
        }
        if (height == -1)
        {
            height = firstImage.rows;
        }
        if (width == -1)
        {
            width = firstImage.cols;
        }
    }

    if (channels != 3 || height <= 0 || width <= 0)
    {
        m_neuralNet = nullptr;
        return tl::make_unexpected(
            Error(ErrorCode::InvalidInput,
                  tr("The selected neural network does not have the expected "
                     "input dimensions.")));
    }

    if (m_neuralNet->outputShape().size() != 2)
    {
        auto neuralNetShape = m_neuralNet->outputShape();
        SHAPE_DEBUG_PRINT(neuralNetShape);
        m_neuralNet = nullptr;
        return tl::make_unexpected(
            Error(ErrorCode::InvalidInput,
                  tr("The selected neural network does not have the expected "
                     "output shape.")));
    }

    m_featureDims = m_neuralNet->outputShape()[1];
    if (m_featureDims <= 0)
    {
        m_neuralNet = nullptr;
        return tl::make_unexpected(
            Error(ErrorCode::InvalidInput,
                  tr("The selected neural network has an invalid feature "
                     "dimension.")));
    }

    int batchSize = 1;
    if (batch == -1 && m_useCuda)
    {
        const size_t availableMemory = cv::cuda::DeviceInfo(0).freeMemory();
        const double denom =
            std::max(1.0, double(width) * double(height) * 3.0 * 32.0);
        batchSize =
            int(std::floor((double(availableMemory) / denom) * MEM_THRESEHOLD));
        batchSize = pow(2, std::floor(log2(batchSize)));
        if (batchSize > MAX_BATCH)
        {
            batchSize = MAX_BATCH;
        }
        if (batchSize < 1)
        {
            batchSize = 1;
        }
    }
    std::cout << "[VisualSimilarity] Initial batch size: " << batchSize
              << std::endl;

    NN::Shape nnInputShape = {batchSize, channels, height, width};

    const int frameCount = int(imageList.size());
    cv::Mat totalFeatureVector;

    for (int i = 0; i < frameCount;)
    {
        if (cancelFlag)
        {
            m_neuralNet = nullptr;
            return tl::make_unexpected(
                Error(ErrorCode::RuntimeError, tr("Selection cancelled by user.")));
        }

        const int currentBatchSize = std::min(batchSize, frameCount - i);

        bool fullBatchAvailableInBuffer = true;
        cv::Mat bufferedBatch;
        for (int j = 0; j < currentBatchSize; ++j)
        {
            cv::Mat out;
            if (bufferLookup(imageList[size_t(i + j)], &out))
            {
                if (bufferedBatch.empty())
                {
                    bufferedBatch = out;
                }
                else
                {
                    cv::vconcat(bufferedBatch, out, bufferedBatch);
                }
            }
            else
            {
                fullBatchAvailableInBuffer = false;
                break;
            }
        }

        if (fullBatchAvailableInBuffer)
        {
            if (totalFeatureVector.empty())
            {
                totalFeatureVector = bufferedBatch;
            }
            else
            {
                cv::vconcat(totalFeatureVector, bufferedBatch, totalFeatureVector);
            }
            i += currentBatchSize;
            continue;
        }

        std::vector<cv::Mat> imgVec;
        imgVec.reserve(size_t(currentBatchSize));
        for (int j = 0; j < currentBatchSize; ++j)
        {
            const uint imgIdx = imageList[size_t(i + j)];
            const int denom = std::max(1, frameCount - 1);
            const int progress = int((100.0 * (i + j)) / denom);
            emit updateProgress(
                progress,
                tr("Calculating features for frame %1 (batch: %2)")
                    .arg(imgIdx)
                    .arg(currentBatchSize));

            cv::Mat img = m_reader->getPic(imgIdx);
            if (img.empty())
            {
                m_neuralNet = nullptr;
                return tl::make_unexpected(
                    Error(ErrorCode::InvalidInput,
                          tr("Could not load image %1.").arg(imgIdx)));
            }
            imgVec.push_back(img);
        }

        NN::Shape currentInputShape = nnInputShape;
        currentInputShape[0] = currentBatchSize;

        auto inferenceResult =
            NN::Tensor::fromCvMats(imgVec,
                                   currentInputShape,
                                   1.0 / 255.0,
                                   NN_MEAN,
                                   NN_STD)
                .and_then(NN::Util::bind_inference(m_neuralNet))
                .and_then(NN::Util::bind_selectOutput(0))
                .and_then(NN::Util::bind_squeeze())
                .and_then(NN::Util::bind_toCvMat());

        if (!inferenceResult)
        {
            const auto err = inferenceResult.error();
            if (err.code() == NN::ErrorCode::OutOfMemory && batchSize > 1)
            {
                const int oldBatchSize = batchSize;
                batchSize = std::max(1, batchSize / 2);
                nnInputShape[0] = batchSize;
                std::cout << "[VisualSimilarity] Batch size changed: "
                          << oldBatchSize << " -> " << batchSize << std::endl;
                continue;
            }

            m_neuralNet = nullptr;
            return tl::make_unexpected(mapInferenceError(err));
        }

        const cv::Mat batchFeatures = inferenceResult.value();
        if (batchFeatures.empty())
        {
            m_neuralNet = nullptr;
            return tl::make_unexpected(
                Error(ErrorCode::RuntimeError,
                      tr("Neural network inference returned an empty feature "
                         "matrix.")));
        }

        if (totalFeatureVector.empty())
        {
            totalFeatureVector = batchFeatures;
        }
        else
        {
            cv::vconcat(totalFeatureVector, batchFeatures, totalFeatureVector);
        }

        i += currentBatchSize;
    }

    if (totalFeatureVector.empty())
    {
        m_neuralNet = nullptr;
        return tl::make_unexpected(
            Error(ErrorCode::RuntimeError,
                  tr("No feature vectors were computed for the selected images.")));
    }

    emit updateProgress(99, tr("Waiting for neural network results"));

    if (logFile)
    {
        logFile->stopTimer();
        logFile->startTimer(LF_TIMER_KMEANS);
    }

    cv::Mat normalizedTotalFeatureVector;
    for (int row = 0; row < totalFeatureVector.rows; ++row)
    {
        cv::Mat out;
        cv::normalize(getFeatureVector(totalFeatureVector, row),
                      out,
                      1.0,
                      0.0,
                      cv::NORM_MINMAX);

        if (out.empty())
        {
            m_neuralNet = nullptr;
            return tl::make_unexpected(
                Error(ErrorCode::InvalidInput,
                      tr("One of the resulting feature vectors is empty. The "
                         "selected neural network is likely not suitable.")));
        }

        if (normalizedTotalFeatureVector.empty())
        {
            normalizedTotalFeatureVector = out;
        }
        else
        {
            cv::vconcat(normalizedTotalFeatureVector, out,
                        normalizedTotalFeatureVector);
        }
    }

    totalFeatureVector.release();

    const int targetFrames =
        std::max(1, int(imageList.size()) / std::max(1, m_frameReduction));
    emit updateProgress(99, tr("Selecting images"));

    cv::Mat centers;
    cv::Mat labels;
    try
    {
        cv::kmeans(normalizedTotalFeatureVector,
                   targetFrames,
                   labels,
                   cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT,
                                    10,
                                    1.0),
                   1,
                   cv::KmeansFlags::KMEANS_PP_CENTERS,
                   centers);
    }
    catch (const cv::Exception& e)
    {
        m_neuralNet = nullptr;
        return tl::make_unexpected(
            Error(ErrorCode::RuntimeError,
                  tr("k-means clustering failed: %1")
                      .arg(QString::fromStdString(e.msg))));
    }

    std::vector<double> distToCenter(size_t(targetFrames),
                                     std::numeric_limits<double>::max());
    std::vector<uint> keyframes(size_t(targetFrames),
                                std::numeric_limits<uint>::max());

    for (int i = 0; i < frameCount; ++i)
    {
        if (cancelFlag)
        {
            m_neuralNet = nullptr;
            return tl::make_unexpected(
                Error(ErrorCode::RuntimeError, tr("Selection cancelled by user.")));
        }

        const int currLabel = labels.at<int>(i);
        if (currLabel < 0 || currLabel >= targetFrames)
        {
            m_neuralNet = nullptr;
            return tl::make_unexpected(
                Error(ErrorCode::RuntimeError,
                      tr("k-means produced an invalid label index.")));
        }

        const cv::Mat currCenter = getFeatureVector(centers, currLabel);
        const cv::Mat currFeatureVector =
            getFeatureVector(normalizedTotalFeatureVector, i);

        if (currCenter.empty() || currFeatureVector.empty())
        {
            m_neuralNet = nullptr;
            return tl::make_unexpected(
                Error(ErrorCode::InvalidInput,
                      tr("Feature vector dimensions do not match the selected "
                         "neural network output.")));
        }

        const double d = cv::norm(currCenter, currFeatureVector);
        if (distToCenter[size_t(currLabel)] > d)
        {
            distToCenter[size_t(currLabel)] = d;
            keyframes[size_t(currLabel)] = imageList[size_t(i)];
        }
    }

    keyframes.erase(
        std::remove(keyframes.begin(),
                    keyframes.end(),
                    std::numeric_limits<uint>::max()),
        keyframes.end());

    if (keyframes.empty())
    {
        m_neuralNet = nullptr;
        return tl::make_unexpected(
            Error(ErrorCode::RuntimeError,
                  tr("No keyframes could be selected from clustering results.")));
    }

    std::sort(keyframes.begin(), keyframes.end());

    if (logFile)
    {
        logFile->stopTimer();
        logFile->startTimer(LF_TIMER_BUFFER);

        std::stringstream ss;
        for (int i = 0; i < frameCount; ++i)
        {
            ss << std::to_string(labels.at<int>(i));
            if (i < frameCount - 1)
            {
                ss << ",";
            }
        }
        logFile->addCustomEntry("labels",
                                QString::fromStdString(ss.str()),
                                "Additional Info");

        logFile->stopTimer();
    }

    // In-memory cache only (no persistent storage in interface yet).
    m_bufferMat = normalizedTotalFeatureVector.clone();
    m_bufferUsedIdx = imageList;

    emit updateProgress(100, tr("Selection finished"));

    m_neuralNet = nullptr;

    return keyframes;
}

void VisualSimilarity::slot_selectedNNChanged(const QString& nnName)
{
    m_nnFileName = nnName;
    m_neuralNet = nullptr;
}

void VisualSimilarity::slot_frameReductionChanged(int value)
{
    m_frameReduction = std::max(2, value);
}

bool VisualSimilarity::bufferLookup(uint idx, cv::Mat* out) const
{
    auto iter = std::find(m_bufferUsedIdx.begin(), m_bufferUsedIdx.end(), idx);
    if (iter == m_bufferUsedIdx.end())
    {
        return false;
    }

    const int vectorPos = int(iter - m_bufferUsedIdx.begin());
    *out = getFeatureVector(m_bufferMat, vectorPos);
    return !out->empty();
}

cv::Mat VisualSimilarity::getFeatureVector(const cv::Mat& totalVector,
                                           int position) const
{
    if (m_featureDims <= 0 || totalVector.empty())
    {
        return cv::Mat();
    }

    if (position < 0 || position >= totalVector.rows)
    {
        return cv::Mat();
    }

    if (totalVector.cols != m_featureDims)
    {
        return cv::Mat();
    }

    return totalVector(cv::Rect(0, position, m_featureDims, 1));
}

QStringList VisualSimilarity::collectNns(const QString& path) const
{
    QStringList entries = QDir(path).entryList(QDir::Files);
    entries = entries.filter(m_nnNameFormat);
    entries.sort(Qt::CaseInsensitive);
    return entries;
}

Error VisualSimilarity::mapInferenceError(const NN::NeuralError& err) const
{
    if (err.code() == NN::ErrorCode::OutOfMemory)
    {
        return Error(
            ErrorCode::GpuOutOfMemory,
            tr("Not enough memory available to run neural network inference. "
               "Reduce working resolution, select a smaller neural network, or "
               "disable CUDA.\nDetails: %1")
                .arg(QString::fromStdString(err.message())));
    }

    return Error(ErrorCode::RuntimeError,
                 tr("An error occurred during neural network inference: %1")
                     .arg(QString::fromStdString(err.message())));
}

std::unique_ptr<QWidget> VisualSimilarity::createSettingsWidget()
{
    auto settingsWidget = std::make_unique<QWidget>(nullptr);
    settingsWidget->setLayout(new QVBoxLayout());
    settingsWidget->layout()->setSpacing(0);
    settingsWidget->layout()->setContentsMargins(0, 0, 0, 0);

    const QString modelsPath =
        QCoreApplication::applicationDirPath() + RESSOURCE_PATH;
    const QStringList availableNNs = collectNns(modelsPath);

    auto* descriptionLabel = new QLabel(
        tr("Computes feature vectors with a neural network and uses k-means "
           "to select diverse keyframes."),
        settingsWidget.get());
    descriptionLabel->setStyleSheet(DESCRIPTION_STYLE);
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setMargin(10);
    settingsWidget->layout()->addWidget(descriptionLabel);

    auto* frameReductionWidget = new QWidget(settingsWidget.get());
    frameReductionWidget->setLayout(new QHBoxLayout());
    frameReductionWidget->layout()->setSpacing(0);
    frameReductionWidget->layout()->setContentsMargins(0, 0, 0, 0);
    frameReductionWidget->layout()->addWidget(
        new QLabel(UI_FRAMEREDUCTION_NAME, settingsWidget.get()));

    m_frameReductionInput = new QSpinBox(settingsWidget.get());
    m_frameReductionInput->setMinimum(2);
    m_frameReductionInput->setMaximum(
        m_reader ? std::max(2, int(m_reader->getPicCount())) : 999999);
    m_frameReductionInput->setSingleStep(1);
    m_frameReductionInput->setAlignment(Qt::AlignRight);
    m_frameReductionInput->setValue(m_frameReduction);
    frameReductionWidget->setToolTip(UI_FRAMEREDUCTION_DESC);
    frameReductionWidget->layout()->addWidget(m_frameReductionInput);
    settingsWidget->layout()->addWidget(frameReductionWidget);

    connect(m_frameReductionInput,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            &VisualSimilarity::slot_frameReductionChanged);

    auto* nnNameWidget = new QWidget(settingsWidget.get());
    nnNameWidget->setLayout(new QHBoxLayout());
    nnNameWidget->layout()->setSpacing(0);
    nnNameWidget->layout()->setContentsMargins(0, 0, 0, 0);
    nnNameWidget->layout()->addWidget(
        new QLabel(UI_NNNAME_NAME, settingsWidget.get()));
    nnNameWidget->setToolTip(UI_NNNAME_DESC);

    m_nnNameInput = new QComboBox(settingsWidget.get());
    m_nnNameInput->addItems(availableNNs);
    nnNameWidget->layout()->addWidget(m_nnNameInput);
    settingsWidget->layout()->addWidget(nnNameWidget);

    connect(m_nnNameInput,
            &QComboBox::currentTextChanged,
            this,
            &VisualSimilarity::slot_selectedNNChanged);

    auto* modelHintLabel = new QLabel(settingsWidget.get());
    modelHintLabel->setWordWrap(true);
    modelHintLabel->setTextInteractionFlags(Qt::TextSelectableByMouse |
                                            Qt::LinksAccessibleByMouse);
    modelHintLabel->setOpenExternalLinks(true);
    settingsWidget->layout()->addWidget(modelHintLabel);

    connect(this,
            &VisualSimilarity::syncSettingsWidget,
            settingsWidget.get(),
            [this, modelHintLabel](int frameReduction,
                                   const QString& nnFileName,
                                   const QStringList& nns,
                                   int maxFrames) {
                if (!m_frameReductionInput || !m_nnNameInput)
                {
                    return;
                }

                {
                    const QSignalBlocker blocker(m_frameReductionInput);
                    m_frameReductionInput->setMaximum(std::max(2, maxFrames));
                    m_frameReductionInput->setValue(std::clamp(
                        frameReduction, 2, std::max(2, maxFrames)));
                }

                {
                    const QSignalBlocker blocker(m_nnNameInput);
                    m_nnNameInput->clear();
                    m_nnNameInput->addItems(nns);

                    const int idx = m_nnNameInput->findText(nnFileName);
                    if (idx >= 0)
                    {
                        m_nnNameInput->setCurrentIndex(idx);
                    }
                    else if (!nns.isEmpty())
                    {
                        m_nnNameInput->setCurrentIndex(0);
                    }
                }

                if (nns.isEmpty())
                {
                    modelHintLabel->setText(
                        tr("<div style='border: 2px solid red; padding: 10px;'>"
                           "<p><b>%1</b></p>"
                           "<p>%2</p>"
                           "</div><br>"
                           "<p>%3</p>"
                           "<code>%4</code><br><br>"
                           "<p>%5 <a href='https://github.com/iVS3D/iVS3D-models'>%6</a>."
                           "</p>")
                            .arg(tr("ERROR:"),
                                 tr("No neural network models for deep visual "
                                    "similarity were found."),
                                 tr("Please add your models to the plugin "
                                    "resources directory and restart iVS3D:"),
                                 QCoreApplication::applicationDirPath() +
                                     RESSOURCE_PATH,
                                 tr("You can find our neural network models"),
                                 tr("here")));
                }
                else
                {
                    modelHintLabel->setText(QString());
                }
            },
            Qt::QueuedConnection);

    connect(settingsWidget.get(), &QObject::destroyed, this, [this]() {
        m_frameReductionInput = nullptr;
        m_nnNameInput = nullptr;
    });

    if (!availableNNs.isEmpty() && !availableNNs.contains(m_nnFileName))
    {
        m_nnFileName = availableNNs.front();
    }

    emit syncSettingsWidget(m_frameReduction,
                            m_nnFileName,
                            availableNNs,
                            m_reader ? std::max(2, int(m_reader->getPicCount())) : 2);

    settingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    settingsWidget->adjustSize();
    return settingsWidget;
}
