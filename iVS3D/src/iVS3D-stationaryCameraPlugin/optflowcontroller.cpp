#include "optflowcontroller.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QLocale>
#include <QSignalBlocker>
#include <QVBoxLayout>

using PLUG::ApplySettingsResult;
using PLUG::Error;
using PLUG::ErrorCode;
using PLUG::InputData;
using PLUG::InputLoadedResult;
using PLUG::SelectionData;
using PLUG::SelectionResult;
using PLUG::SettingsWidgetResult;

StationaryCamera::StationaryCamera()
    : IBase()
{
    QLocale locale = qApp->property("translation").toLocale();
    QTranslator* translator = new QTranslator();
    translator->load(locale, "stationary", "_", ":/translations", ".qm");
    qApp->installTranslator(translator);
}

SettingsWidgetResult StationaryCamera::getSettingsWidget()
{
    auto widget = createSettingsWidget();
    if (!widget) {
        return tl::make_unexpected(Error(
            ErrorCode::ResourceUnavailable,
            tr("Failed to create StationaryCamera settings widget.")));
    }
    return widget;
}

QString StationaryCamera::getName() const
{
    return PLUGIN_NAME;
}

QMap<QString, QVariant> StationaryCamera::getSettings() const
{
    QMap<QString, QVariant> settings;
    settings.insert(SETTINGS_SELECTOR_THRESHOLD, m_selectorThreshold);
    return settings;
}

ApplySettingsResult StationaryCamera::applySettings(
    const QMap<QString, QVariant>& settings)
{
    bool ok = true;
    const double threshold =
        settings.value(SETTINGS_SELECTOR_THRESHOLD, m_selectorThreshold)
            .toDouble(&ok);

    if (!ok || threshold < 0.0) {
        return tl::make_unexpected(
            Error(ErrorCode::InvalidInput,
                  tr("Invalid stationary threshold setting.")));
    }

    m_selectorThreshold = threshold;
    emit syncSettingsWidget(m_selectorThreshold * 100.0);
    return {};
}

InputLoadedResult StationaryCamera::onInputLoaded(const InputData& input)
{
    if (!input.reader) {
        return tl::make_unexpected(
            Error(ErrorCode::InvalidInput, tr("Reader is null.")));
    }

    m_reader = input.reader;

    const cv::Mat testPic = m_reader->getPic(0);
    if (testPic.empty()) {
        return tl::make_unexpected(
            Error(ErrorCode::InvalidInput, tr("Input contains no readable frame.")));
    }

    m_inputResolution.setX(testPic.cols);
    m_inputResolution.setY(testPic.rows);

    const int picCount = m_reader->getPicCount();
    int size[2] = {picCount, picCount};
    m_bufferMat = cv::SparseMat(2, size, CV_32F);
    m_bufferMat.clear();

    emit syncSettingsWidget(m_selectorThreshold * 100.0);
    return {};
}

void StationaryCamera::onCudaChanged(bool enabled)
{
    m_useCuda = enabled;
}

SelectionResult StationaryCamera::selectImages(
    const SelectionData& data,
    volatile bool& cancelFlag)
{
    const std::vector<uint>& imageList = data.selectedIndices;

    if (imageList.empty()) {
        return std::vector<uint>{};
    }

    if (imageList.size() == 1) {
        return imageList;
    }

    if (data.reader) {
        m_reader = data.reader;
    }
    if (!m_reader) {
        return tl::make_unexpected(
            Error(ErrorCode::InvalidInput, tr("Reader is null.")));
    }

    // invalidate buffer if working resolution changed
    cv::Mat tstImg = m_reader->getPic(0);
    if (tstImg.empty()) {
        return tl::make_unexpected(
            Error(ErrorCode::InvalidInput, tr("Input contains no readable frame.")));
    }
    if (m_inputResolution.x() != tstImg.cols ||
        m_inputResolution.y() != tstImg.rows) {
        m_bufferMat.clear();
        m_inputResolution.setX(tstImg.cols);
        m_inputResolution.setY(tstImg.rows);
    }

    // ---------- create all neccessary components ------------
    reportProgress(tr("Excluding buffered values from computation list"), 0);
    // create a vector that containst only indices that need to be gathered
    // (futureFrames + indices with buffered values = imageList)
    std::vector<uint> futureFrames;
    for (uint imageListIdx = 0; imageListIdx < imageList.size() - 1;
         imageListIdx++) {
        uint fromIdx = imageList[imageListIdx];
        uint toIdx = imageList[imageListIdx + 1];
        double bufferedMovement = m_bufferMat.value<double>(fromIdx, toIdx);
        if (bufferedMovement <= 0.0) {
            futureFrames.push_back(fromIdx);
            futureFrames.push_back(toIdx);
        }
    }
    futureFrames.erase(std::unique(futureFrames.begin(), futureFrames.end()),
                       futureFrames.end());  // remove duplicates
    reportProgress(tr("Creating calculation units"), 0);
    std::tuple<ImageGatherer *, FlowCalculator *, KeyframeSelector *>
        components = Factory::instance().createComponents(
            futureFrames, m_reader, m_useCuda, m_selectorThreshold);
    std::unique_ptr<ImageGatherer> imageGatherer(std::get<0>(components));
    std::unique_ptr<FlowCalculator> flowCalculator(std::get<1>(components));
    std::unique_ptr<KeyframeSelector> keyframeSelector(std::get<2>(components));

    if (!imageGatherer || !flowCalculator || !keyframeSelector) {
        return tl::make_unexpected(
            Error(ErrorCode::ResourceUnavailable,
                  tr("Failed to create stationary camera components.")));
    }

    std::vector<double> flowValues = {};
    auto fromIter = imageList.begin();
    auto toIter = std::next(imageList.begin(), 1);
    std::future<void> flowCalcHandler;
    std::future<QPair<cv::Mat, cv::Mat>> imageGatherHandler;

    // ----------- algorithms definition ------------
    // gather image pair
    std::function<QPair<cv::Mat, cv::Mat>(uint, uint)> gatherImagePairStatic =
        [ig = imageGatherer.get()](uint fromIdx, uint toIdx) {
            QElapsedTimer timer;
            timer.start();
            QPair<cv::Mat, cv::Mat> matPair =
                ig->gatherImagePair(fromIdx, toIdx);
            qDebug() << "gatherDuration=" << timer.elapsed() << "ms";
            return matPair;
        };

    // flow calculation
    std::function<void(cv::Mat, cv::Mat)> calcFlowStatic =
        [fc = flowCalculator.get(), &flowValues](cv::Mat fromMat, cv::Mat toMat) {
            QElapsedTimer timer;
            timer.start();
            // muliplication with down sample factor corrects the reduced
            // resolution
            double flowValue = fc->calculateFlow(fromMat, toMat);
            flowValues.push_back(flowValue);
            qDebug() << "flowDuration=" << timer.elapsed()
                     << "ms\tvalue=" << flowValue;
        };

    // -------------- iterate through all available frame pairs ---------------
    uint usedBufferedValues = 0;
    if (data.logFile) {
        data.logFile->startTimer(LF_TIMER_CORE);
    }
    while (toIter < imageList.end()) {
        // Aborts calculations as result of user interaction
        if (cancelFlag) {
            qDebug() << "Execution was stopped.";
            break;
        }

        // ----------- exectution ------------
        double bufferedMovement = 0.0;
        if (m_bufferMat.size() != 0) {
            bufferedMovement = m_bufferMat.value<double>(*fromIter, *toIter);
        }
        if (bufferedMovement <= 0.0) {
            imageGatherHandler = std::async(
                std::launch::async, gatherImagePairStatic, *fromIter, *toIter);
            QPair<cv::Mat, cv::Mat> matPair = imageGatherHandler.get();
            flowCalcHandler = std::async(std::launch::async, calcFlowStatic,
                                         matPair.first, matPair.second);
        } else {
            // use buffered flow value
            flowValues.push_back(bufferedMovement);
            usedBufferedValues++;
        }
        // -------- progress and debug ------------
        int progress =
            ((toIter - imageList.begin()) * 100) / (int)imageList.size();
        QString currOp = tr("Calculating flow between frame ") +
                         QString::number(*fromIter) + tr(" and ") +
                         QString::number(*toIter);
        reportProgress(currOp, progress);
        // ----------------------------------------
        fromIter = std::next(fromIter, 1);
        toIter = std::next(toIter, 1);
    }
    if (flowCalcHandler.valid()) flowCalcHandler.wait();
    if (data.logFile) {
        data.logFile->stopTimer();
        data.logFile->addCustomEntry(LF_CE_VALUE_USED_BUFFERED, usedBufferedValues,
                                     LF_CE_TYPE_ADDITIONAL_INFO);
    }

    // ------------ select keyframes ----------------
    if (data.logFile) {
        data.logFile->startTimer(LF_SELECT_FRAMES);
    }
    if (flowValues.size() == imageList.size() + 1) {
        return std::vector<uint>{};
    }
    std::vector<uint> keyframes =
        keyframeSelector->select(imageList, flowValues, &cancelFlag);
    if (data.logFile) {
        data.logFile->stopTimer();
    }

    // -------- update buffer --------------
    if (data.logFile) {
        data.logFile->startTimer(LF_TIMER_BUFFER);
    }
    for (uint flowValuesIdx = 0;
         flowValuesIdx + 1 < flowValues.size() && flowValuesIdx + 1 < imageList.size();
         flowValuesIdx++) {
        int progress = (100.0f * flowValues.size()) / (flowValuesIdx + 1);
        reportProgress(tr("Buffering values"), progress);
        if (m_bufferMat.ref<double>(imageList[flowValuesIdx],
                                    imageList[flowValuesIdx + 1]) <= 0.0)
            m_bufferMat.ref<double>(imageList[flowValuesIdx],
                                    imageList[flowValuesIdx + 1]) =
                flowValues[flowValuesIdx];
        // DEBUG write flow values in logFile
        //        logFile->addCustomEntry(LF_CE_NAME_FLOWVALUE,
        //        flowValues[flowValuesIdx], LF_CE_TYPE_DEBUG);
    }

    if (data.logFile) {
        data.logFile->stopTimer();
    }

    return keyframes;
}

void StationaryCamera::reportProgress(const QString& op, int progress)
{
    emit updateProgress(progress, op);
}

void StationaryCamera::slot_selectorThresholdChanged(double value)
{
    m_selectorThreshold = value / 100.0;
}

std::unique_ptr<QWidget> StationaryCamera::createSettingsWidget()
{
    // selector layout
    auto settingsWidget = std::make_unique<QWidget>(nullptr);
    settingsWidget->setLayout(new QVBoxLayout(settingsWidget.get()));
    settingsWidget->layout()->setSpacing(0);
    settingsWidget->layout()->setContentsMargins(0, 0, 0, 0);

    QWidget* selectorLayout = new QWidget(settingsWidget.get());
    selectorLayout->setLayout(new QHBoxLayout(selectorLayout));
    selectorLayout->layout()->addWidget(new QLabel(SELECTOR_LABEL_TEXT, settingsWidget.get()));
    selectorLayout->layout()->setContentsMargins(0, 0, 0, 0);
    selectorLayout->layout()->setSpacing(0);

    // selector spinBox
    m_selectorThresholdSpinBox = new QDoubleSpinBox(settingsWidget.get());
    m_selectorThresholdSpinBox->setValue(m_selectorThreshold * 100.0);
    m_selectorThresholdSpinBox->setDecimals(2);
    m_selectorThresholdSpinBox->setMinimum(0.0);
    m_selectorThresholdSpinBox->setMaximum(200.0);
    m_selectorThresholdSpinBox->setSuffix("%");
    m_selectorThresholdSpinBox->setSingleStep(1.0);
    m_selectorThresholdSpinBox->setAlignment(Qt::AlignRight);
    QObject::connect(m_selectorThresholdSpinBox,
                     QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                     this,
                     &StationaryCamera::slot_selectorThresholdChanged);
    selectorLayout->layout()->addWidget(m_selectorThresholdSpinBox);

    // selector description
    QLabel* selectorLabel = new QLabel(SELECTOR_DESCRIPTION, settingsWidget.get());
    selectorLabel->setStyleSheet(DESCRIPTION_STYLE);
    selectorLabel->setWordWrap(true);

    // add elements
    settingsWidget->layout()->addWidget(selectorLayout);
    settingsWidget->layout()->addWidget(selectorLabel);

    QObject::connect(this, &StationaryCamera::syncSettingsWidget, settingsWidget.get(),
                     [this](double selectorThresholdPercent) {
                         if (m_selectorThresholdSpinBox) {
                             QSignalBlocker blocker(m_selectorThresholdSpinBox);
                             m_selectorThresholdSpinBox->setValue(selectorThresholdPercent);
                         }
                     },
                     Qt::QueuedConnection);

    QObject::connect(settingsWidget.get(), &QObject::destroyed, this, [this]() {
        m_selectorThresholdSpinBox = nullptr;
    });

    emit syncSettingsWidget(m_selectorThreshold * 100.0);

    settingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    settingsWidget->adjustSize();
    return settingsWidget;
}
