#include "visualSimilarity.h"

VisualSimilarity::VisualSimilarity()
{
    QTranslator* translator = new QTranslator();
    translator->load(QLocale::system(), "cosplace", "_", ":/translations", ".qm");
    qApp->installTranslator(translator);

    m_settingsWidget = nullptr;
}

VisualSimilarity::~VisualSimilarity()
{

}

QWidget* VisualSimilarity::getSettingsWidget(QWidget *parent)
{
    if(!m_settingsWidget){
        createSettingsWidget(parent);
    }
    // update widgets with internal parameter values
    return m_settingsWidget;
}

std::vector<uint> VisualSimilarity::sampleImages(const std::vector<unsigned int> &imageList, Progressable *receiver, volatile bool *stopped, bool useCuda, LogFileParent* logFile)
{
    displayMessage(receiver, tr("Loading Neural Network"));
    displayProgress(receiver,0 , tr("Loading Neural Network"));

    logFile->startTimer(LF_TIMER_NN);

    // Read nn
    auto nn = cv::dnn::readNet(m_nnPath.toStdString());
    // activate cuda
    if (useCuda) {
        nn.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
//        nn.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA_FP16);
        nn.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
    }

    cv::Mat totalFeatureVector;

    // calculate feature vectors
    QElapsedTimer timer;
    int frameCount = imageList.size();
    int k = frameCount*(1.0-m_frameReduction/100.0);
    timer.start();
    for (uint idx : imageList) {
        if (*stopped) {
            return imageList;
        }

        // buffer lookup
        auto iter = std::find(m_bufferUsedIdx.begin(), m_bufferUsedIdx.end(), idx);
        cv::Mat out;
        if (iter != m_bufferUsedIdx.end()) {
            // buffered value found
            int vectorPos = iter - m_bufferUsedIdx.begin();
            out = getFeatureVector(m_bufferMat, vectorPos);
        } else {
            // value not found in buffer
            cv::Mat img, inblob;
            img = m_reader->getPic(idx);
            prepareImage(&img, &inblob);
            feedImage(&inblob, &out, &nn);
            inblob.release();
            img.release();
        }
        //

        if (totalFeatureVector.empty()) {
            totalFeatureVector = out;
        }
        cv::vconcat(totalFeatureVector,out,totalFeatureVector);
        out.release();

        // display update
        long duration = timer.elapsed();
        timer.restart();
        std::stringstream ss;
        ss << std::setw(7) << idx << " | "
           << std::setw(7)  << frameCount << " => "
           << std::setw(5) << round(1000.0/duration*100)/100 << "fps";
        int progress = 100.0*(idx-*imageList.begin()) / *(imageList.end()-1);
        displayProgress(receiver, progress, QString::fromStdString(ss.str()));
    }
    timer.invalidate();

    logFile->stopTimer();
    logFile->startTimer(LF_TIMER_KMEANS);

    // k-means
    cv::Mat centers, labels;
    cv::kmeans(totalFeatureVector,
               k,
               labels,
               cv::TermCriteria(cv::TermCriteria::EPS+cv::TermCriteria::COUNT,10,1.0),
               1,
               cv::KmeansFlags::KMEANS_PP_CENTERS,
               centers);

    /*
     * calculate which image is nearest
     * smallest distance to center
     * iterate through all images
     */
    // position in vector is the lable
    std::vector<double> distToCenter(k);
    distToCenter.assign(k, std::numeric_limits<float>::max());
    std::vector<cv::Mat> featureVector(k);
    std::vector<uint> keyframe(k);
    keyframe.assign(k, -1);

    for (int i = 0; i < frameCount; i++) {
        if (*stopped) {
            return imageList;
        }

        int currLabel = labels.at<int>(i);
        cv::Mat currCenter = getFeatureVector(centers,currLabel);
        cv::Mat currFeatureVector = getFeatureVector(totalFeatureVector, i);

        float d = cv::norm(currCenter,currFeatureVector);

        // safe feature vector as nearest
        if (distToCenter[currLabel] > d) {
            distToCenter[currLabel] = d;
            featureVector[currLabel] = currFeatureVector;
            keyframe[currLabel] = imageList[i];
        }
    }

    std::sort(keyframe.begin(), keyframe.end());

    logFile->stopTimer();
    logFile->startTimer(LF_TIMER_BUFFER);

    // Debug logFile entries
    std::stringstream ss;
    for (int i = 0; i < frameCount; i++) {
        ss << std::to_string(labels.at<int>(i));
        if (i < frameCount-1)
            ss << ",";
    }
    logFile->addCustomEntry("labels", QString::fromStdString(ss.str()), "Additional Info");
    //

    // safe to buffer
    sendBuffer(totalFeatureVector, imageList);
    m_bufferMat.release();
    m_bufferMat = totalFeatureVector;
    m_bufferUsedIdx = imageList;

    logFile->stopTimer();

    return keyframe;
}

QString VisualSimilarity::getName() const
{
    return tr("Visual Similarity");
}

void VisualSimilarity::initialize(Reader *reader, QMap<QString, QVariant> buffer, signalObject *so)
{
    // this routine runs if new images have been loaded, it should be performant
    // as it is executed on image load for every plugin

    m_frameReduction = 100.0 - 100.0/(double)reader->getFPS();
    m_frameReduction = std::round(m_frameReduction*100)/100;
    if (m_frameReductionInput) {
        m_frameReductionInput->setValue(m_frameReduction);
    }

    m_signalObject = so; // this is a link to the core which provides updates
    m_reader = reader; // this is our reader for the images

    readBuffer(buffer);
}

void VisualSimilarity::setSettings(QMap<QString, QVariant> settings)
{
    // frame reduction
    QMap<QString, QVariant>::iterator itFrameReduction = settings.find(FRAMEREDUCTION_JSON_NAME);
    if (itFrameReduction != settings.end()) {
        m_frameReduction = itFrameReduction.value().toDouble();
        if (m_frameReductionInput) {
            m_frameReductionInput->setValue(m_frameReduction);
        }
    }

    // nn path
    QMap<QString, QVariant>::iterator itNNPath = settings.find(NNPATH_JSON_NAME);
    if (itNNPath != settings.end()) {
        m_nnPath = itNNPath.value().toString();
        if (m_nnPathInput) {
            m_nnPathInput->setText(m_nnPath);
        }
    }

    // feature vector dimension
    QMap<QString, QVariant>::iterator itFeatureDims = settings.find(FEATUREDIMS_JSON_NAME);
    if (itFeatureDims != settings.end()) {
        m_featureDims = itFeatureDims.value().toInt();
        if (m_featureDimsInput) {
            m_featureDimsInput->setValue(m_featureDims);
        }
    }
}

QMap<QString, QVariant> VisualSimilarity::generateSettings(Progressable *receiver, bool useCuda, volatile bool* stopped)
{
    // this routine runs if the user activly wants to generate optimal settings for the given images. It can be
    // slow or heavy computation here, the user is prompted with an progress bar.
    (void) receiver;
    (void) useCuda;
    (void) stopped;

    return getSettings();
}

QMap<QString, QVariant> VisualSimilarity::getSettings()
{
    QMap<QString, QVariant> settings;
    settings.insert(FRAMEREDUCTION_JSON_NAME, m_frameReduction);
    settings.insert(NNPATH_JSON_NAME, m_nnPath);
    settings.insert(FEATUREDIMS_JSON_NAME, m_featureDims);
    return settings;
}

void VisualSimilarity::slot_frameReductionChanged(double v)
{
    if (v < 0.0) {
        m_frameReductionInput->setValue(0.0);
        return;
    }
    if  (v > 100.0) {
        m_frameReductionInput->setValue(1.0);
        return;
    }

    m_frameReduction = v;
}

void VisualSimilarity::slot_nnPathChanged(QString txt)
{
    m_nnPath = txt;
}

void VisualSimilarity::slot_frameDimsChanged(int v)
{
    m_featureDims = v;
}

void VisualSimilarity::displayProgress(Progressable *p, int progress, QString msg)
{
    QMetaObject::invokeMethod(p,
                              "slot_makeProgress",
                              Qt::DirectConnection,
                              Q_ARG(int, progress),
                              Q_ARG(QString, msg));
}

void VisualSimilarity::displayMessage(Progressable *p, QString msg)
{
    QMetaObject::invokeMethod(p,
                              "slot_displayMessage",
                              Qt::DirectConnection,
                              Q_ARG(QString, msg));
}

void VisualSimilarity::prepareImage(cv::Mat *img,
                            cv::Mat *outblob,
                            const cv::Scalar &std,
                            const cv::Scalar &mean)
{
    cv::Mat img_resized;
    cv::resize(*img, img_resized, cv::Size(INPUT_W,INPUT_H));
    cv::divide(255.0, img_resized, img_resized);
    cv::subtract(img_resized, mean, img_resized);
//    *outblob = cv::dnn::blobFromImage(img_resized, 1.0, cv::Size(INPUT_W,INPUT_H), false, true, CV_8U);
    *outblob = cv::dnn::blobFromImage(img_resized, 1.0, cv::Size(INPUT_W,INPUT_H), false, true, CV_32F);
    cv::divide(*outblob, std, *outblob);
}

void VisualSimilarity::feedImage(cv::Mat *inblob, cv::Mat *out, cv::dnn::dnn4_v20221220::Net *nn)
{
    nn->setInput(*inblob);
    *out = nn->forward();
}

cv::Mat VisualSimilarity::getFeatureVector(cv::Mat totalVector, int position)
{
    return totalVector(cv::Rect(0,position,m_featureDims,1));
}

void VisualSimilarity::sendBuffer(cv::Mat bufferMat, std::vector<uint> calculatedIdx)
{
    QMap<QString, QVariant> bufferMap;

    // --------------- feature vectors ---------------
    std::stringstream matStream;
    cv::Size size = bufferMat.size();

    // cv::Mat to string
    for (int y = 0; y < size.height; y++) {
        for (int x = 0; x < size.width; x++) {
            matStream << bufferMat.at<float>(x,y);
            if (x < size.width-1)
                matStream << BUFFER_FEATURE_DELIMITER_X;
        }
        if (y < size.height-1)
            matStream << BUFFER_FEATURE_DELIMITER_Y;
    }
    std::string matString = matStream.str();
    //

    QVariant varFeatures(QString::fromStdString(matString));
    bufferMap.insert(BUFFER_NAME_FEATURES, varFeatures);

    // ------------- idx of calculated feature vecotors ----------
    QVariantList varUsedIdx;
    for (uint idx : calculatedIdx) {
        varUsedIdx.append(idx);
    }
    bufferMap.insert(BUFFER_NAME_IDX, varUsedIdx);
    // -----------------------------------------------------------

    emit updateBuffer(bufferMap);
}

void VisualSimilarity::readBuffer(QMap<QString, QVariant> buffer)
{

    m_bufferMat = cv::Mat();
    if (buffer.size() != 0) {
        //Get the QMap from Variant
        QMapIterator<QString, QVariant> mapIt(buffer);
        //Find movementBased buffer in the buffer
        while (mapIt.hasNext()) {
            mapIt.next();
            if (mapIt.key().compare(BUFFER_NAME_FEATURES) == 0) {
                // --------------------- feature vectors ---------------------
                m_bufferMat = stringToBufferMat(mapIt.value().toString());
                // -----------------------------------------------------------
            } else if (mapIt.key().compare(BUFFER_NAME_IDX) == 0) {
                // ------------- idx of calculated feature vecotors ----------
                QVariantList varListUsedIdx = mapIt.value().toList();
                for (QVariant varUsedIdx : varListUsedIdx) {
                    m_bufferUsedIdx.push_back(varUsedIdx.toUInt());
                }
                // -----------------------------------------------------------
            }
        }
    }
}

cv::Mat VisualSimilarity::stringToBufferMat(QString string)
{
    QStringList xStrList = string.split(BUFFER_FEATURE_DELIMITER_Y);
    QList<QStringList> strMat;
    for (QString xStr : xStrList) {
        strMat.append(xStr.split(BUFFER_FEATURE_DELIMITER_X));
    }

    cv::Mat mat = cv::Mat(strMat.size(), strMat[0].size(), CV_32F);

    for (int x = 0; x < strMat.size(); x++) {
        for (int y = 0; y < strMat[0].size(); y++) {
            mat.at<float>(x,y) = strMat[x][y].toFloat();
        }
    }

    return mat;
}

void VisualSimilarity::createSettingsWidget(QWidget *parent)
{
    m_settingsWidget = new QWidget(parent);
    m_settingsWidget->setLayout(new QVBoxLayout());
    m_settingsWidget->layout()->setSpacing(0);
    m_settingsWidget->layout()->setMargin(0);
    // frame reduction
    QWidget *frameReductionWidget = new QWidget(parent);
    frameReductionWidget->setLayout(new QHBoxLayout(parent));
    frameReductionWidget->layout()->addWidget(new QLabel(UI_FRAMEREDUCTION_NAME));
    m_frameReductionInput = new QDoubleSpinBox(frameReductionWidget);
    m_frameReductionInput->setDecimals(2);
    m_frameReductionInput->setMinimum(0.0);
    m_frameReductionInput->setMaximum(100.0);
    m_frameReductionInput->setSingleStep(10.0);
    m_frameReductionInput->setAlignment(Qt::AlignRight);
    m_frameReductionInput->setSuffix("%");
    m_frameReductionInput->setValue(m_frameReduction);
    frameReductionWidget->layout()->addWidget(m_frameReductionInput);
    m_settingsWidget->layout()->addWidget(frameReductionWidget);
    connect(m_frameReductionInput, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VisualSimilarity::slot_frameReductionChanged);
    QLabel *frameReductionWidget_txt = new QLabel(frameReductionWidget);
    frameReductionWidget_txt->setText(UI_FRAMEREDUCTION_DESC);
    frameReductionWidget_txt->setStyleSheet(DESCRIPTION_STYLE);
    m_settingsWidget->layout()->addWidget(frameReductionWidget_txt);
    // path to neural network
    QWidget *nnPathWidget = new QWidget(parent);
    nnPathWidget->setLayout(new QVBoxLayout(parent));
    nnPathWidget->layout()->addWidget(new QLabel(UI_NNPATH_NAME));
    m_nnPathInput = new QLineEdit(nnPathWidget);
    m_nnPathInput->setText(m_nnPath);
    m_settingsWidget->layout()->addWidget(m_nnPathInput);
    connect(m_nnPathInput, &QLineEdit::textChanged, this, &VisualSimilarity::slot_nnPathChanged);
    QLabel *nnPathWidget_txt = new QLabel(nnPathWidget);
    nnPathWidget_txt->setText(UI_NNPATH_DESC);
    nnPathWidget_txt->setStyleSheet(DESCRIPTION_STYLE);
    m_settingsWidget->layout()->addWidget(nnPathWidget_txt);
    // feature dimensions
    QWidget *featureDimWidget = new QWidget(parent);
    featureDimWidget->setLayout(new QVBoxLayout(parent));
    featureDimWidget->layout()->addWidget(new QLabel(UI_FEATURE_DIM_NAME));
    m_featureDimsInput = new QSpinBox(featureDimWidget);
    m_featureDimsInput->setValue(m_featureDims);
    m_featureDimsInput->setMaximum(2048);
    m_featureDimsInput->setAlignment(Qt::AlignRight);
    m_settingsWidget->layout()->addWidget(m_featureDimsInput);
    connect(m_featureDimsInput, QOverload<int>::of(&QSpinBox::valueChanged), this, &VisualSimilarity::slot_frameDimsChanged);
    QLabel *featureDimWidget_txt = new QLabel(featureDimWidget);
    featureDimWidget_txt->setText(UI_FEATURE_DIM_DESC);
    featureDimWidget_txt->setStyleSheet(DESCRIPTION_STYLE);
    m_settingsWidget->layout()->addWidget(featureDimWidget_txt);

    m_settingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_settingsWidget->adjustSize();
}
