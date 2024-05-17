#include "visualSimilarity.h"

VisualSimilarity::VisualSimilarity()
{
    QTranslator* translator = new QTranslator();
    translator->load(QLocale::system(), "visualSimilarity", "_", ":/translations", ".qm");
    qApp->installTranslator(translator);

    m_settingsWidget = nullptr;
}

VisualSimilarity::~VisualSimilarity() {}

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
    auto nn = cv::dnn::readNet(RESSOURCE_PATH+m_nnFileName.toStdString());
    // activate cuda
    if (useCuda) {
        nn.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
//        nn.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA_FP16);
        nn.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
    }
    size_t weightSize, blobSize;
    nn.getMemoryConsumption(cv::dnn::MatShape({BATCH_SIZE, 3, 512, 512}), weightSize, blobSize);
    qDebug() << "weight:" << weightSize << "blob:" << blobSize;
    qDebug() << "FLOPS:" << nn.getFLOPS(cv::dnn::MatShape({BATCH_SIZE, 3, 512, 512}));

    // calculate feature vectors
    QElapsedTimer timer;
    QFuture<void> futureFeedNN;
    int frameCount = imageList.size();
    timer.start();
    cv::Mat outblob, inblob, totalFeatureVector;
    for (uint i = 0; i < frameCount; i+=BATCH_SIZE) {
        if (*stopped) {
            return imageList;
        }

        // value not found in buffer
        std::vector<cv::Mat> imgVec;
        for (uint j = 0; j < BATCH_SIZE; j++) {
            cv::Mat img = m_reader->getPic(imageList[i+j]);
            imgVec.push_back(img);
        }

        inblob = cv::dnn::blobFromImages(imgVec, 1.0/255.0, m_nnInputSize, NN_MEAN, false, true, CV_32F);
        cv::divide(inblob, NN_STD, inblob);

        futureFeedNN.waitForFinished();
        futureFeedNN = QtConcurrent::run(this, &VisualSimilarity::feedImage, &inblob, &totalFeatureVector, &nn);
//        nn.setInput(inblob);
//        nn.forward(outblob);
//        if (totalFeatureVector.empty()) {
//            totalFeatureVector = outblob;
//        }
//        cv::vconcat(totalFeatureVector,outblob,totalFeatureVector);

        // display update
        long duration = timer.elapsed();
        timer.restart();
//        std::stringstream ss;
//        ss << std::setw(7) << idx << " | "
//           << std::setw(7)  << *(imageList.end()-1) << " => "
//           << std::setw(5) << round(1000.0/duration*100)/100 << "fps";
//        QString progressDesc = QString::fromStdString(ss.str());
        int progress = 100.0*(imageList[i]-*imageList.begin()) / *(imageList.end()-1);
        QString progressDesc = tr("Calculting feature vectors with ")+
                QString::number(round(BATCH_SIZE*1000.0/duration*100)/100).rightJustified(7,' ')+" fps";
        displayProgress(receiver, progress, progressDesc);
    }
    timer.invalidate();
    futureFeedNN.waitForFinished();

    logFile->stopTimer();
    logFile->startTimer(LF_TIMER_KMEANS);

    // k-means
    int targetFrames = imageList.size() / m_frameReduction;
    cv::Mat centers, labels;
    cv::kmeans(totalFeatureVector,
               targetFrames,
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
    std::vector<double> distToCenter(targetFrames);
    distToCenter.assign(targetFrames, std::numeric_limits<float>::max());
    std::vector<cv::Mat> featureVector(targetFrames);
    std::vector<uint> keyframe(targetFrames);
    keyframe.assign(targetFrames, -1);

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

    m_frameReduction = (int)reader->getFPS();

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

    // nn name
    QMap<QString, QVariant>::iterator itNNname = settings.find(NNNAME_JSON_NAME);
    if (itNNname != settings.end()) {
        m_nnFileName = itNNname.value().toString();
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
    return settings;
}

void VisualSimilarity::slot_selectedNNChanged(QString nnName)
{
    QStringList s = nnName.split("_");
    if (s.size() < 3)
        return;
    m_featureDims = s[1].toInt();
    QStringList resList = s[2].split(".")[0].split("x");
    m_nnInputSize = cv::Size(resList[0].toInt(), resList[1].toInt());
    m_nnFileName = nnName;
}

void VisualSimilarity::slot_reloadNN()
{
    QStringList nnNameList = collect_nns(RESSOURCE_PATH);
    m_nnNameInput->clear();
    m_nnNameInput->addItems(nnNameList);
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

double VisualSimilarity::cosineSimilarity(cv::Mat *a, cv::Mat *b)
{
    return a->dot(*b) / 1;
}

void VisualSimilarity::feedImage(cv::Mat *inblob, cv::Mat *totalFeatureVector, cv::dnn::Net *nn)
{
    cv::Mat outblob;
    nn->setInput(*inblob);
    nn->forward(outblob);
    if (totalFeatureVector->empty())
        *totalFeatureVector = outblob;
    cv::vconcat(*totalFeatureVector, outblob, *totalFeatureVector);
}

bool VisualSimilarity::bufferLookup(uint idx, cv::Mat *out)
{
    auto iter = std::find(m_bufferUsedIdx.begin(), m_bufferUsedIdx.end(), idx);
    if (iter != m_bufferUsedIdx.end()) {
        // buffered value found
        int vectorPos = iter - m_bufferUsedIdx.begin();
        *out = getFeatureVector(m_bufferMat, vectorPos);
        return true;
    }
    return false;
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
                for (const QVariant &varUsedIdx : varListUsedIdx) {
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
    for (const QString &xStr : xStrList) {
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

QStringList VisualSimilarity::collect_nns(QString path)
{
    QStringList entries = QDir(path).entryList(QDir::Files);
    return entries.filter(nnNameFormat);
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
    m_frameReductionInput = new QSpinBox(frameReductionWidget);
    frameReductionWidget->setToolTip(UI_FRAMEREDUCTION_DESC);
    m_frameReductionInput->setMinimum(2);
    m_frameReductionInput->setMaximum(m_reader->getPicCount());
    m_frameReductionInput->setSingleStep(1);
    m_frameReductionInput->setAlignment(Qt::AlignRight);
    m_frameReductionInput->setValue(m_frameReduction);
    frameReductionWidget->layout()->addWidget(m_frameReductionInput);
    m_settingsWidget->layout()->addWidget(frameReductionWidget);
    connect(m_frameReductionInput, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int v){m_frameReduction=v;});
    QLabel *frameReductionWidget_txt = new QLabel(frameReductionWidget);
    // nn_name input
    QWidget *nnNameWidget = new QWidget(parent);
    nnNameWidget->setLayout(new QHBoxLayout(parent));
    nnNameWidget->layout()->addWidget(new QLabel(UI_NNNAME_NAME));
    nnNameWidget->setToolTip(UI_NNNAME_DESC);
    m_nnNameInput = new QComboBox(parent);
    // nn_name button
    m_nnNameReloadBt = new QPushButton(parent);
    m_nnNameReloadBt->setToolTip(UI_NNNAME_BT_DESC);
//    m_nnNameReloadBt->setIcon();
    m_nnNameReloadBt->setText("R");
    m_nnNameReloadBt->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(m_nnNameReloadBt, &QPushButton::clicked, this, &VisualSimilarity::slot_reloadNN);
    connect(m_nnNameInput, &QComboBox::currentTextChanged, this, &VisualSimilarity::slot_selectedNNChanged);
    m_nnNameInput->addItems(collect_nns(RESSOURCE_PATH));
    nnNameWidget->layout()->addWidget(m_nnNameInput);
    nnNameWidget->layout()->addWidget(m_nnNameReloadBt);
    m_settingsWidget->layout()->addWidget(nnNameWidget);

    m_settingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_settingsWidget->adjustSize();
}
