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
    if (imageList.size() < 2) {
        return imageList;
    }

    logFile->startTimer(LF_TIMER_NN);

    // Read nn
    if (!QFile::exists(QString::fromStdString(RESSOURCE_PATH+m_nnFileName.toStdString()))) {
        displayErrorMessage(tr("No valid neural network was selected."));
        return imageList;
    }
    displayMessage(receiver, tr("Loading Neural Network"));
    displayProgress(receiver,0 , tr("Loading Neural Network"));
    auto nn = cv::dnn::readNet(RESSOURCE_PATH+m_nnFileName.toStdString());
    // activate cuda
    int batchSize = 1;
    if (useCuda) {
        nn.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
        nn.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
        // calculate batch size
        const long availableMemory = cv::cuda::DeviceInfo(0).freeMemory();
        batchSize = round(availableMemory/m_nnInputSize.width/m_nnInputSize.height/3/32*MEM_THRESEHOLD); // RAM/(W*H*C*32)*thresehold <- CV_32F
        std::cout << "Detected " << std::round(availableMemory/10000000.0)/100.0 << "GB free memory." << std::endl;
        std::cout << "Resulting Batch Size: " << batchSize << std::endl;
    }

    // calculate feature vectors
    QFuture<void> futureFeedNN;
    int frameCount = imageList.size();
    cv::Mat inblob, totalFeatureVector;
    for (uint i = 0; i < frameCount; i+=batchSize) {
        if (*stopped) {
            futureFeedNN.cancel();
            return imageList;
        }

        // --------------- use buffered values if posible -------------
        bool fullBatchAvailableInBuffer = true;
        cv::Mat bufferedBatch;
        for (uint j = i ; j < i+batchSize && j < frameCount; j++) {
            cv::Mat out;
            if (bufferLookup(imageList[j], &out)) {
                if (bufferedBatch.empty())
                    bufferedBatch = out;
                else
                    cv::vconcat(bufferedBatch, out, bufferedBatch);
            } else {
                fullBatchAvailableInBuffer = false;
                break;
            }
        }
        if (fullBatchAvailableInBuffer) {
            futureFeedNN.waitForFinished();
            if (totalFeatureVector.empty())
                totalFeatureVector = bufferedBatch;
            else
                cv::vconcat(totalFeatureVector, bufferedBatch, totalFeatureVector);
        }

        // ----------------------- compute batch ------------------------
        if (!fullBatchAvailableInBuffer) {
            std::vector<cv::Mat> imgVec;
            for (uint j = i ; j < i+batchSize && j < frameCount; j++) {
                int progress = 100.0*(imageList[j]-*imageList.begin()) / (*(imageList.end()-1)-*imageList.begin());
                QString progressDesc = tr("Calculating feature vector for frame ") + QString::number(imageList[j]);
                displayProgress(receiver, progress, progressDesc);

                cv::Mat img = m_reader->getPic(imageList[j]);
                imgVec.push_back(img);
            }

            inblob = cv::dnn::blobFromImages(imgVec, 1.0/255.0, m_nnInputSize, NN_MEAN, false, true, CV_32F);
            cv::divide(inblob, NN_STD, inblob);
            futureFeedNN.waitForFinished();
            futureFeedNN = QtConcurrent::run(this, &VisualSimilarity::feedImage, inblob, &totalFeatureVector, &nn);
        }
        // --------------------------------------------------------------
    }
    displayProgress(receiver, 99, tr("Waiting for last batch to finish"));
    futureFeedNN.waitForFinished();

    logFile->stopTimer();
    logFile->startTimer(LF_TIMER_KMEANS);

    // normalizing each dimension
    cv::Mat normalizedTotalFeatureVector;
    cv::Mat out;
    cv::normalize(getFeatureVector(totalFeatureVector, 0), normalizedTotalFeatureVector, 1.0, 0.0, cv::NORM_MINMAX);
    for (int i = 1; i < totalFeatureVector.rows; i++) {
        cv::normalize(getFeatureVector(totalFeatureVector, i), out, 1.0, 0.0, cv::NORM_MINMAX);
        if (out.empty()) {
            displayErrorMessage(tr("One of the resulting feature vectors is empty. This maybe caused by a not suitable neural network."));
            return imageList;
        }
        cv::vconcat(normalizedTotalFeatureVector, out, normalizedTotalFeatureVector);
    }
    totalFeatureVector.release();

    // k-means
    int targetFrames = imageList.size() / m_frameReduction;
    if (targetFrames < 1)
        targetFrames = 1;
    displayProgress(receiver, 99, tr("Selecting Images"));
    cv::Mat centers, labels;
    cv::kmeans(normalizedTotalFeatureVector,
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
        cv::Mat currCenter = getFeatureVector(centers, currLabel);
        cv::Mat currFeatureVector = getFeatureVector(normalizedTotalFeatureVector, i);

        if (currCenter.empty() || currFeatureVector.empty()) {
            displayErrorMessage(tr("Wrong feature vector dimension for selected neural network. (this dimension was specified in the name of the file, which contains the neural network)"));
            return imageList;
        }
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
    sendBuffer(normalizedTotalFeatureVector, imageList);
    m_bufferMat.release();
    m_bufferMat = normalizedTotalFeatureVector;
    m_bufferUsedIdx = imageList;

    logFile->stopTimer();

    return keyframe;
}

QString VisualSimilarity::getName() const
{
    return tr("Deep Visual Similarity");
}

void VisualSimilarity::initialize(Reader *reader, QMap<QString, QVariant> buffer, signalObject *so)
{
    // this routine runs if new images have been loaded, it should be performant
    // as it is executed on image load for every plugin

    double fps = reader->getFPS();
    m_frameReduction = fps > 0.0 ? (int)fps : 30;
    m_frameReduction = m_frameReduction < reader->getPicCount() ? m_frameReduction : 2;

    m_signalObject = so; // this is a link to the core which provides updates
    m_reader = reader; // this is our reader for the images

    readBuffer(buffer);
}

void VisualSimilarity::setSettings(QMap<QString, QVariant> settings)
{
    // frame reduction
    QMap<QString, QVariant>::iterator itFrameReduction = settings.find(FRAMEREDUCTION_JSON_NAME);
    if (itFrameReduction != settings.end()) {
        m_frameReduction = itFrameReduction.value().toInt();
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
    QRegularExpressionMatch match = m_nnNameFormat.match(nnName);
    m_featureDims = match.captured("featureDims").toInt();
    int w = match.captured("width").toInt();
    int h = match.captured("height").toInt();
    m_nnInputSize = cv::Size(w, h);
    m_nnFileName = nnName;
}

void VisualSimilarity::slot_reloadNN(int index)
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

void VisualSimilarity::feedImage(cv::Mat inblob, cv::Mat *totalFeatureVector, cv::dnn::Net *nn)
{
    cv::Mat outblob;
    nn->setInput(inblob);
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
        if (out->empty()) {
            return false;
        }
        return true;
    }
    return false;
}

cv::Mat VisualSimilarity::getFeatureVector(cv::Mat totalVector, int position)
{
    if (totalVector.cols != m_featureDims) {
        return cv::Mat();
    }
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
    return entries.filter(m_nnNameFormat);
}

void VisualSimilarity::displayErrorMessage(QString message)
{
    QDialog *errorDialog = new QDialog();
    errorDialog->setWindowTitle(tr("Error"));
    QLabel *txtLabel = new QLabel(message);
    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayout->addWidget(txtLabel);
    errorDialog->setLayout(mainLayout);
    errorDialog->exec();
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
    // nn_name input
    QWidget *nnNameWidget = new QWidget(parent);
    nnNameWidget->setLayout(new QHBoxLayout(parent));
    nnNameWidget->layout()->addWidget(new QLabel(UI_NNNAME_NAME));
    nnNameWidget->setToolTip(UI_NNNAME_DESC);
    m_nnNameInput = new QComboBox(parent);
    connect(m_nnNameInput, QOverload<int>::of(&QComboBox::activated), this, &VisualSimilarity::slot_reloadNN);
    connect(m_nnNameInput, &QComboBox::currentTextChanged, this, &VisualSimilarity::slot_selectedNNChanged);
    m_nnNameInput->addItems(collect_nns(RESSOURCE_PATH));
    nnNameWidget->layout()->addWidget(m_nnNameInput);
    m_settingsWidget->layout()->addWidget(nnNameWidget);

    m_settingsWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_settingsWidget->adjustSize();
}
