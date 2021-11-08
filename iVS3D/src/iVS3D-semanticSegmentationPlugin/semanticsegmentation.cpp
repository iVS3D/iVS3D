#include "semanticsegmentation.h"


SemanticSegmentation::SemanticSegmentation()
{
    QStringList filter("*.onnx");
    QString path = QCoreApplication::applicationDirPath() + MODEL_PATH;
    QDir dir(path);
    qDebug() << dir.currentPath();
    m_ONNXmodelList = dir.entryList(filter);
    m_ONNXmodelIdx = 0;
    if (m_ONNXmodelList.size()) {
        QStringList classes;
        QList<QColor> colors;
        getClassesAndColors(classes,colors);
        for(int i = 0; i<classes.size(); i++){
            m_ONNXselectedClasses.push_back(true);
        }
    }

    m_ONNXmodelLoaded = false;
    m_imageIdx = UINT_MAX;
    m_blendAlpha = 0.5f;
    m_guiUpToDate = false;

    cv::ocl::setUseOpenCL(true);
    m_useCuda = false;
    m_ONNXmodel = nullptr;
    m_settingsWidget = nullptr;
}

SemanticSegmentation::~SemanticSegmentation()
{
    if (m_settingsWidget != nullptr) {
        delete m_settingsWidget;
    }
    if (m_ONNXmodel != nullptr) {
        delete m_ONNXmodel;
    }
}

QWidget* SemanticSegmentation::getSettingsWidget(QWidget *parent)
{
    if (m_settingsWidget != nullptr) {
        return m_settingsWidget;
    }

    // create settings widget and connect to it
    m_settingsWidget = new SettingsWidget(parent, m_ONNXmodelList);
    m_ONNXmodelIdx = 0;
    if(m_ONNXmodelList.size()){
        QStringList classes;
        QList<QColor> colors;
        getClassesAndColors(classes,colors);
        m_ONNXselectedClasses.clear();
        for(int i = 0; i<classes.size(); i++){
            m_ONNXselectedClasses.push_back(true);
        }
        m_settingsWidget->slot_classesAndColorsChanged(classes,colors,m_ONNXselectedClasses);
    }

    connect(m_settingsWidget, &SettingsWidget::sig_selectedONNXIndexChanged, this, &SemanticSegmentation::slot_ONNXindexChanged);
    connect(m_settingsWidget, &SettingsWidget::sig_selectedClassesChanged, this, &SemanticSegmentation::slot_selectedClassesChanged);
    connect(m_settingsWidget, &SettingsWidget::sig_blendAlphaChanged, this, &SemanticSegmentation::slot_blendAlphaChanged);
    connect(this, &SemanticSegmentation::sig_classesAndColorsChanged, m_settingsWidget, &SettingsWidget::slot_classesAndColorsChanged);
    connect(this, &SemanticSegmentation::sig_message, m_settingsWidget, &SettingsWidget::slot_showTask);

    return m_settingsWidget;
}


QString SemanticSegmentation::getName() const
{
    return "SemanticSegmentation";
}

QStringList SemanticSegmentation::getOutputNames()
{
    return QStringList("masks");
}

ITransform *SemanticSegmentation::copy()
{
    auto *copy = new SemanticSegmentation();
    copy->m_ONNXmodelIdx = this->m_ONNXmodelIdx;
    copy->m_ONNXselectedClasses = this->m_ONNXselectedClasses;
    copy->m_useCuda = this->m_useCuda;
    copy->m_ONNXmodel = this->m_ONNXmodel;
    copy->m_ONNXmodelLoaded = this->m_ONNXmodelLoaded;
    this->m_ONNXmodel = nullptr;
    this->m_useCuda = false;
    this->m_ONNXmodelLoaded = false;
    return copy;
}

ImageList SemanticSegmentation::transform(uint idx, const cv::Mat &img)
{
    QMutexLocker lock(&m_mutex);
    if(!m_useCuda || !m_ONNXmodelLoaded){
        // create preview of image before calculation started
        cv::Mat preview(img.rows, img.cols, img.type());
        preview.setTo(cv::Scalar(255, 255, 255));
        cv::hconcat(img, preview, preview);
        emit sendToGui(idx, preview);
    }

    // update image and index buffer
    m_image = img;
    m_imageIdx = idx;

    // only start calculation if models found
    if(m_ONNXmodelList.size() == 0){
        qDebug() << "No models loaded. Abort.";
        return ImageList();
    }

    // load selected model
    if(!m_ONNXmodelLoaded){
        loadModel();
    }

    slot_computeScore();            // compute NN score

    slot_computeSegmentation();     // create semantic map from score

    slot_computeMask();             // create binary mask from score

    m_guiUpToDate = false;
    slot_sendGuiPreview();          // visualize result on gui

    return ImageList({m_mask});// return the result
}

void SemanticSegmentation::enableCuda(bool enabled)
{
    m_useCuda = enabled;
    if(m_ONNXmodelLoaded){
        if(m_useCuda){
            qDebug() << "loaded. activating cuda...";
            m_ONNXmodel->setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
            m_ONNXmodel->setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
            emit sig_message(HW_NAME(m_useCuda), "Loaded model (cuda)");
            qDebug() << "cuda alive :)";
        } else {
            m_ONNXmodel->setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            m_ONNXmodel->setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
            emit sig_message(HW_NAME(m_useCuda), "Loaded model");
        }
    }
}

void SemanticSegmentation::setSettings(QMap<QString, QVariant> settings)
{
    //find index of the model
    QString modelName = settings.find(USED_MODEL).value().toString();
    int modelIndex = 0;
    for (QString model : m_ONNXmodelList) {
        if (model.compare(modelName) == 0) {
            m_ONNXmodelIdx = modelIndex;
            break;
        }
        modelIndex++;
    }
    QList<QVariant> boolVar = settings.find(SELECTED_CLASSES).value().toList();
    QList<bool> boolList;
    for (QVariant var : boolVar) {
        boolList.append(var.toBool());
    }

    m_ONNXselectedClasses = boolList;
    if (m_settingsWidget) {
        //Disbale class update to prevent classes to be set to default
        m_updateClasses = false;
        m_settingsWidget->setModel(m_ONNXmodelIdx);
        m_settingsWidget->setClasses(m_ONNXselectedClasses);
    }

}

QMap<QString, QVariant> SemanticSegmentation::getSettings()
{
    QMap<QString, QVariant> settings;
    //Save name of the model. Index may change based on order in which models are read
    if (m_ONNXmodelList.size() != 0) {
        settings.insert(USED_MODEL, m_ONNXmodelList[m_ONNXmodelIdx]);
    } else {
        settings.insert(USED_MODEL, "no model selected");
    }
    QList<QVariant> boolList;
    for (bool b : m_ONNXselectedClasses) {
        boolList.append(b);
    }
    QVariant selectedClasses = QVariant(boolList);
    settings.insert(SELECTED_CLASSES, selectedClasses);
    return settings;

}

void SemanticSegmentation::slot_ONNXindexChanged(int n)
{
    // update model
    m_ONNXmodelLoaded = false;
    m_ONNXmodelIdx = n;
    m_guiUpToDate = false;

    // update class checkboxes on gui
    QStringList classes;
    QColorList colors;
    getClassesAndColors(classes, colors);

    if(m_updateClasses) {
        // update local class list
        m_ONNXselectedClasses.clear();
        for(int i = 0; i<classes.size(); i++){
            m_ONNXselectedClasses.push_back(true);
        }
    }
    m_updateClasses = true;

    emit sig_classesAndColorsChanged(classes,colors, m_ONNXselectedClasses);

    // update gui with new model
    if(m_imageIdx == UINT_MAX){
        return;
    }
    QTimer::singleShot(0,this,[=](){transform(m_imageIdx, m_image);});
}

void SemanticSegmentation::slot_selectedClassesChanged(QBoolList classes)
{
    m_ONNXselectedClasses = classes;
    m_guiUpToDate = false;

    if(m_imageIdx == UINT_MAX){
        return;
    }

    QTimer::singleShot(0,this,[=](){
        slot_computeMask();
        slot_sendGuiPreview();
        m_guiUpToDate = true;
    });
}

void SemanticSegmentation::slot_blendAlphaChanged(float alpha)
{
    qDebug() << "Blend alpha set to " << QString::number(alpha);
    m_blendAlpha = alpha;
    m_guiUpToDate = false;
    if(m_imageIdx == UINT_MAX){
        return;
    }
    // update preview
    QTimer::singleShot(0,this,&SemanticSegmentation::slot_sendGuiPreview);
}

void SemanticSegmentation::slot_computeScore()
{
    qDebug() << "start prediction";
    double scaleFactor = 1.0/255;
    cv::Scalar mean = cv::Scalar(0.485, 0.456, 0.406);
    bool swapRB = true;
    bool crop = false;

    // preprocess the image
    int w,h;
    getInputHeightAndWidth(h,w,m_ONNXmodelIdx);

    cv::Mat blob;
    cv::dnn::blobFromImage(m_image, blob, scaleFactor, cv::Size(w,h), mean, swapRB, crop, CV_32F);

    // make prediction
    qDebug() << "Start image prediction.";
    if(m_useCuda){
        QString im = QString("[") + QString::number(m_imageIdx) + QString("]");
        std::cout << std::right << std::setw(30) << im.toStdString() << std::left << " free mem: " << cv::cuda::DeviceInfo().freeMemory() << "\n";
    }
    emit sig_message(HW_NAME(m_useCuda) ,"Computing preview...");
    auto start = std::chrono::high_resolution_clock::now(); // start clock

    m_ONNXmodel->setInput(blob);                             // set model input ...
    m_score = m_ONNXmodel->forward();                    // ... and predict output

    auto end = std::chrono::high_resolution_clock::now();   // stop clock
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    qDebug() << "Predicted the image in " << QString::number(durationMs) << "ms";
    emit sig_message(HW_NAME(m_useCuda), "Finished preview in " + QString::number(durationMs) + "ms");
}

void SemanticSegmentation::slot_computeSegmentation()
{
    // fetch classes and colors for visualizing segmentation
    QStringList classes;
    QColorList colors;
    getClassesAndColors(classes, colors);

    // colorize the score
    cv::Mat segmentation_score;
    m_score.copyTo(segmentation_score);
    colorizeSegmentation(segmentation_score, m_segmentation, colors);

    // resize the result to the original width and height
    if(m_segmentation.cols != m_image.cols || m_segmentation.rows != m_image.rows) {
        cv::resize(m_segmentation, m_segmentation, cv::Size(m_image.cols, m_image.rows), 0, 0, cv::INTER_NEAREST);
    }
}

void SemanticSegmentation::slot_computeMask()
{
    // colorize selected classes in score
    cv::Mat mask_score;
    m_score.copyTo(mask_score);
    colorizeSegmentationBinary(mask_score, m_mask, m_ONNXselectedClasses);


    // single-channel image has to be converted to 3 channel image
    cv::cvtColor(m_mask, m_mask, cv::COLOR_GRAY2RGB);

    // resize the result to the original width and height
    if(m_mask.cols != m_image.cols || m_mask.rows != m_image.rows) {
        cv::resize(m_mask, m_mask, cv::Size(m_image.cols, m_image.rows), 0, 0, cv::INTER_NEAREST);
    }
}

void SemanticSegmentation::slot_sendGuiPreview()
{
    if(m_guiUpToDate){
        return;
    }
    cv::Mat preview;
    alphaBlend(m_segmentation, m_image, preview, m_blendAlpha);
    cv::hconcat(preview, m_mask, preview);
    emit sendToGui(m_imageIdx, preview);
    m_guiUpToDate = true;
}

void SemanticSegmentation::getClassesAndColors(QStringList &cl, QColorList &co)
{
    std::vector<std::string> classes;
    std::vector<cv::Vec3b> colors;

    std::string modelName = m_ONNXmodelList[m_ONNXmodelIdx].toStdString();
    if(modelName.find("voc") != std::string::npos) {
        getVOCClassesAndColors(classes, colors);
    }
    else if(modelName.find("cityscapes") != std::string::npos) {
        getCityscapesClassesAndColors(classes, colors);
    }
    else if(modelName.find("ade20k") != std::string::npos) {
        getAde20KClassesAndColors(classes, colors);
    }
    else {
        qDebug() << "The dataset, the model got trained with, is not supported. Choose another model!";
    }

    for(int i = 0; i<(int)classes.size(); i++){
        cl.push_back(QString::fromStdString(classes[i]));
        co.push_back(QColor::fromRgb(colors[i][0], colors[i][1],colors[i][2]));
    }
}


void SemanticSegmentation::colorizeSegmentation(const cv::Mat score, cv::Mat &segmentation, QList<QColor> colorList)
{
    std::vector<cv::Vec3b> colors;
    for(int i = 0; i<colorList.size(); i++){
        colors.push_back(cv::Vec3b(colorList[i].blue(),colorList[i].green(),colorList[i].red()));
    }
    const int rows = score.size[2];
    const int cols = score.size[3];
    const int chns = score.size[1];

    if (chns != static_cast<int>(colors.size()))
    {
        CV_Error(cv::Error::StsError, cv::format("Number of output classes does not match "
                                                 "number of colors (%d != %d)", chns, (int)colors.size()));
    }
    cv::Mat maxCl = cv::Mat::zeros(rows, cols, CV_8UC1);
    cv::Mat maxVal(rows, cols, CV_32FC1, score.data);
    for (int ch = 1; ch < chns; ch++)
    {
        for (int row = 0; row < rows; row++)
        {
            const float *ptrScore = score.ptr<float>(0, ch, row);
            uint8_t *ptrMaxCl = maxCl.ptr<uint8_t>(row);
            float *ptrMaxVal = maxVal.ptr<float>(row);
            for (int col = 0; col < cols; col++)
            {
                if (ptrScore[col] > ptrMaxVal[col])
                {
                    ptrMaxVal[col] = ptrScore[col];
                    ptrMaxCl[col] = static_cast<uchar>(ch);
                }
            }
        }
    }
    segmentation.create(rows, cols, CV_8UC3);
    for (int row = 0; row < rows; row++)
    {
        const uchar *ptrMaxCl = maxCl.ptr<uchar>(row);
        cv::Vec3b *ptrSegm = segmentation.ptr<cv::Vec3b>(row);
        for (int col = 0; col < cols; col++)
        {
            if(1/*mShowClassBoxes.at(static_cast<int>(ptrMaxCl[col]))->isChecked()*/) {
                ptrSegm[col] = colors[ptrMaxCl[col]];
            }
            else {
                ptrSegm[col] = cv::Vec3b(0, 0, 0);
            }
        }
    }
}

void SemanticSegmentation::loadModel()
{
    QString modelPath = QCoreApplication::applicationDirPath() + MODEL_PATH + "/" + m_ONNXmodelList[m_ONNXmodelIdx];
    std::cout << "\n\n<---------- LOADING MODEL ----------->\n";
    if(m_useCuda){
        std::cout << std::left << std::setw(30) << "Target:" << "GPU" << "\n";
        std::cout << std::left << std::setw(30) << "Target name:" << cv::cuda::DeviceInfo().name() << "\n";
        std::cout << std::left << std::setw(30) << "Target memory:" << cv::cuda::DeviceInfo().totalMemory() << "\n";
        std::cout << std::left << std::setw(30) << "Target memory free:" << cv::cuda::DeviceInfo().freeMemory() << "\n";
    }
    else{
        std::cout << std::left << std::setw(30) << "Target:" << "CPU" << "\n";
    }
    std::cout << std::left << std::setw(30) << "Model name:" << m_ONNXmodelList[m_ONNXmodelIdx].toStdString() << "\n";
    std::cout << std::left << std::setw(30) << "Model path:" << modelPath.toStdString() << "\n";

    qDebug() << "Try to load model from " << modelPath;
    emit sig_message(HW_NAME(m_useCuda), "Loading model...");
    auto start = std::chrono::high_resolution_clock::now();


    try {
        if(m_ONNXmodel){
            delete m_ONNXmodel;
            m_ONNXmodel = nullptr;
        }
        m_ONNXmodel = new cv::dnn::Net(cv::dnn::readNetFromONNX(modelPath.toStdString()));
        if(m_useCuda){
            m_ONNXmodel->setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
            m_ONNXmodel->setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
            emit sig_message(HW_NAME(m_useCuda), "Loaded model (cuda)");
        } else {
            m_ONNXmodel->setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            m_ONNXmodel->setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
            emit sig_message(HW_NAME(m_useCuda), "Loaded model");
        }
        m_ONNXmodelLoaded = true;
    }  catch (QException &e) {
        qDebug() <<  "failed to load model: " << e.what();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << std::left << std::setw(30) << "Model loaded in:" << durationMs << "\n";

    qDebug() << "Successfully loaded model from " << modelPath;
    qDebug() << "Successfully loaded model in " << QString::number(durationMs) << "ms";

}

void SemanticSegmentation::colorizeSegmentationBinary(const cv::Mat score, cv::Mat &segmentation, QList<bool> selectedClasses)
{
    std::vector<bool> classes;
    for(int i = 0; i<selectedClasses.size(); i++){
        classes.push_back(selectedClasses[i]);
    }

    const int rows = score.size[2];
    const int cols = score.size[3];
    const int chns = score.size[1];

    if (chns != static_cast<int>(classes.size()))
    {
        CV_Error(cv::Error::StsError, cv::format("Number of output classes does not match "
                                                 "number of colors (%d != %d)", chns, (int)classes.size()));
    }
    cv::Mat maxCl = cv::Mat::zeros(rows, cols, CV_8UC1);
    cv::Mat maxVal(rows, cols, CV_32FC1, score.data);
    for (int ch = 1; ch < chns; ch++)
    {
        for (int row = 0; row < rows; row++)
        {
            const float *ptrScore = score.ptr<float>(0, ch, row);
            uint8_t *ptrMaxCl = maxCl.ptr<uint8_t>(row);
            float *ptrMaxVal = maxVal.ptr<float>(row);
            for (int col = 0; col < cols; col++)
            {
                if (ptrScore[col] > ptrMaxVal[col])
                {
                    ptrMaxVal[col] = ptrScore[col];
                    ptrMaxCl[col] = static_cast<uchar>(ch);
                }
            }
        }
    }
    segmentation.create(rows, cols, CV_8UC1);
    for (int row = 0; row < rows; row++)
    {
        const uchar *ptrMaxCl = maxCl.ptr<uchar>(row);
        uchar *ptrSegm = segmentation.ptr<uchar>(row);
        for (int col = 0; col < cols; col++)
        {
            if(classes[ptrMaxCl[col]]) {
                ptrSegm[col] = 255;
            }
            else {
                ptrSegm[col] = 0;
            }
        }
    }
}

void SemanticSegmentation::getVOCClassesAndColors(std::vector<std::string> &classes, std::vector<cv::Vec3b> &colors)
{
    QString path = QCoreApplication::applicationDirPath() + MODEL_PATH;
    std::string filepath = path.toUtf8().constData();
    filepath.append("/VOC_Dataset.txt");

    // open the file
    std::fstream modelTextFile;
    modelTextFile.open(filepath, std::ios::in);

    // if the file is open read lines
    if (modelTextFile.is_open()) {
        std::string line;
        //read data from file object and put it into string.
        while(std::getline(modelTextFile, line)) {
            // get the classes and colors
            std::stringstream lineStream(line);
            std::string segment;
            std::vector<std::string> classesAndColors;
            while(std::getline(lineStream, segment, ';'))
            {
                classesAndColors.push_back(segment);
            }
            std::string classLabel = classesAndColors.at(0);
            std::string colorString = classesAndColors.at(1);

            // add class label to classes
            classes.push_back(classLabel);

            // get color as cv::Vec3b
            std::stringstream colorStream(colorString);
            std::string rgbSegment;
            std::vector<std::string> rgb;
            while(std::getline(colorStream, rgbSegment, ','))
            {
                rgb.push_back(rgbSegment);
            }
            colors.push_back(cv::Vec3b(std::stoi(rgb.at(0)), std::stoi(rgb.at(1)), std::stoi(rgb.at(2))));
        }

        //close the file object.
        modelTextFile.close();
    }
}

// =============================================================================

void SemanticSegmentation::getCityscapesClassesAndColors(std::vector<std::string> &classes, std::vector<cv::Vec3b> &colors)
{
    QString path = QCoreApplication::applicationDirPath() + MODEL_PATH;
    std::string filepath = path.toUtf8().constData();
    filepath.append("/Cityscapes_Dataset.txt");

    // open the file
    std::fstream modelTextFile;
    modelTextFile.open(filepath, std::ios::in);

    // if the file is open read lines
    if (modelTextFile.is_open()) {
        std::string line;
        //read data from file object and put it into string.
        while(std::getline(modelTextFile, line)) {
            // get the classes and colors
            std::stringstream lineStream(line);
            std::string segment;
            std::vector<std::string> classesAndColors;
            while(std::getline(lineStream, segment, ';'))
            {
                classesAndColors.push_back(segment);
            }
            std::string classLabel = classesAndColors.at(0);
            std::string colorString = classesAndColors.at(1);

            // add class label to classes
            classes.push_back(classLabel);

            // get color as cv::Vec3b
            std::stringstream colorStream(colorString);
            std::string rgbSegment;
            std::vector<std::string> rgb;
            while(std::getline(colorStream, rgbSegment, ','))
            {
                rgb.push_back(rgbSegment);
            }
            colors.push_back(cv::Vec3b(std::stoi(rgb.at(0)), std::stoi(rgb.at(1)), std::stoi(rgb.at(2))));
        }

        //close the file object.
        modelTextFile.close();
    }
}

// =============================================================================

void SemanticSegmentation::getAde20KClassesAndColors(std::vector<std::string> &classes, std::vector<cv::Vec3b> &colors)
{
    QString path = QCoreApplication::applicationDirPath() + MODEL_PATH;
    std::string filepath = path.toUtf8().constData();
    filepath.append("/Ade20K_Dataset.txt");

    // open the file
    std::fstream modelTextFile;
    modelTextFile.open(filepath, std::ios::in);

    // if the file is open read lines
    if (modelTextFile.is_open()) {
        std::string line;
        //read data from file object and put it into string.
        while(std::getline(modelTextFile, line)) {
            // get the classes and colors
            std::stringstream lineStream(line);
            std::string segment;
            std::vector<std::string> classesAndColors;
            while(std::getline(lineStream, segment, ';'))
            {
                classesAndColors.push_back(segment);
            }
            std::string classLabel = classesAndColors.at(0);
            std::string colorString = classesAndColors.at(1);

            // add class label to classes
            classes.push_back(classLabel);

            // get color as cv::Vec3b
            std::stringstream colorStream(colorString);
            std::string rgbSegment;
            std::vector<std::string> rgb;
            while(std::getline(colorStream, rgbSegment, ','))
            {
                rgb.push_back(rgbSegment);
            }
            colors.push_back(cv::Vec3b(std::stoi(rgb.at(0)), std::stoi(rgb.at(1)), std::stoi(rgb.at(2))));
        }

        //close the file object.
        modelTextFile.close();
    }
}

void SemanticSegmentation::getInputHeightAndWidth(int &inputHeight, int &inputWidth, int modelIdx)
{
    // get model name and erase everything except the size part
    std::string modelName = m_ONNXmodelList[modelIdx].toStdString();
    modelName.erase(0, 40);
    modelName.erase(modelName.length()-5, 5);

    // get the size
    std::stringstream inputSizeOfModelString(modelName);
    std::string segment;
    std::vector<std::string> sizes;
    while(std::getline(inputSizeOfModelString, segment, 'x'))
    {
        sizes.push_back(segment);
    }
    std::string heightString = sizes.at(0);
    std::string widthString = sizes.at(1);

    inputHeight = std::stoi(heightString);
    inputWidth = std::stoi(widthString);
}

void SemanticSegmentation::alphaBlend(const cv::Mat &foreground, const cv::Mat &background, cv::Mat &destionation, float alpha)
{
    destionation = alpha * foreground + (1.0f - alpha) * background;
}
