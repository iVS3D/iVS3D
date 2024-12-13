#include "exportthread.h"

ExportThread::ExportThread(Progressable* receiver, ModelInputPictures* mip, QPoint resolution, const QString &path, const QString &name, volatile bool* stopped, QRect roi, const std::vector<ITransform*> &iTransformCopies, LogFile *logFile)
{
    m_receiver = receiver;
    m_reader = mip->getReader()->copy();
    m_keyframes = mip->getAllKeyframes(true);
    m_resolution = resolution;
    m_path = path;
    m_name = name;
    m_stopped = stopped;
    m_roi = roi;
    m_iTransformCopies = iTransformCopies;
    m_logFile = logFile;
    m_progress = 0;

    // log general info
    m_logFile->setInputInfo(m_keyframes);

    m_exportExif = new ExportExif();
}

ExportThread::~ExportThread()
{
    for(int i = 0; i<(int)m_iTransformCopies.size(); i++){
        ITransform *iTr = m_iTransformCopies[i];
        delete iTr;
    }
}


int ExportThread::getResult(){
    return m_result;
}

void ExportThread::run(){
    //Prepare export
    m_result = 0;
    m_receiver->slot_makeProgress(0, tr("Exporting images"));
    m_logFile->startTimer(stringContainer::lfExportPreprocessing);
    int totalTasks = (int)m_keyframes.size();

    bool useRoi = (m_roi.width() > 0 && m_roi.height() > 0);

    cv::Mat originalMat = m_reader->getPic(0);
    QPoint imageSize = QPoint(originalMat.cols, originalMat.rows);

    bool useResize = (imageSize.x() != m_resolution.x()) || (imageSize.y() != m_resolution.y());
    //If the resolution changed the roi Rect has to be scaled
    if (useResize) {   
       //Get scale factor in x & y direction
       float ratioX = (float) m_resolution.x() / (float) imageSize.x();
       float ratioY = (float) m_resolution.y() / (float) imageSize.y();
       //Scale the topLeft point
       QPoint topLeft = m_roi.topLeft();
       int topLeftXWithRatio = topLeft.x() * ratioX;
       int topLeftYWithRatio = topLeft.y() * ratioY;
       QPoint topLeftWitRatio(topLeftXWithRatio, topLeftYWithRatio);
       //Scale the bottomRight point
       QPoint bottomRight = m_roi.bottomRight();
       int bottomRightXWithRatio = bottomRight.x() * ratioX;
       int bottomRightYWithRatio = bottomRight.y() * ratioY;
       QPoint bottomRightWitRatio(bottomRightXWithRatio, bottomRightYWithRatio);
       //create the scaled roi
       m_roi = QRect(topLeftWitRatio, bottomRightWitRatio);
    }

    //prepare roi for exporting images
    cv::Rect roi;
    if (useRoi) {
        int topLeftX = m_roi.topLeft().x();
        int topLeftY = m_roi.topLeft().y();
        int width = m_roi.width();
        int height = m_roi.height();
        roi = cv::Rect(topLeftX, topLeftY, width, height);
        //don't use ROI if its larger then the image
        if (m_resolution.x() < roi.width || m_resolution.y() < roi.height) {
            useRoi = false;
        }
    }

    //directory of export (without /images)
    QString fileName = m_path;
    if(fileName.endsWith("/images"))
        fileName = fileName.left(fileName.length() - QString("/images").length());
    //fileName.replace("/images", "");

    //prepare all iTransform directories and their subdirectories
    for (uint i = 0; i < m_iTransformCopies.size(); ++i) {
        QString prepFileName = fileName;
        prepFileName.append("/").append(m_iTransformCopies[i]->getName()).append("/");
        QStringList iTOutputNames = m_iTransformCopies[i]->getOutputNames();
        for (int t = 0; t < iTOutputNames.length(); ++t) {
            QString iTransformOutDir = prepFileName;
            iTransformOutDir.append(iTOutputNames[i]);
            //create Directory
            if (!QDir(iTransformOutDir).exists()) {
                QDir().mkdir(iTransformOutDir);
            }

        }
    }

    std::vector<std::string> imageFiles;
    bool isDirImages = m_reader->isDir();
    if (isDirImages) {
        imageFiles = m_reader->getFileVector();
    }

    //export normal image and all iTransform images
    int iTransformCopiesSize = static_cast<int>(m_iTransformCopies.size());

    m_logFile->stopTimer();
    //End of preparation

    /*
     * This Code section was intended to export multiple images in parallel, but introduced multiple bugs such as:
     * - Deadlock if less images then threads
     * - Crash if aborting export with only few images
     * Also execution time increased extremely (more than 1 min per 1080p image)
     * Replaced with for loop for simplicity
     *
    m_reader->initMultipleAccess(m_keyframes);

    std::function<void(const int)> doExport = [this, useResize, roi, useRoi, iTransformCopiesSize, fileName, isDirImages, imageFiles](const int &n) {
        if(*m_stopped){
            m_result = 1;
            return;
        }
        if (m_result == -1) {
            return;
        }

        cv::Mat currentImage;
        while (currentImage.empty()) {
           currentImage = m_reader->getPic(n, true);
        }
        cv::Mat imageToExport = resizeCrop(currentImage, cv::Size(m_resolution.x(), m_resolution.y()), useResize, roi, useRoi);
        bool success = exportImages(imageToExport, iTransformCopiesSize, fileName, isDirImages, n, imageFiles);
        if (!success) {
            m_result = -1;
        }
        reportProgress();
        return;
    };
    m_logFile->startTimer(stringContainer::lfExportFrames);
    //calculate blurValues on multiple threads
    QtConcurrent::blockingMap(m_keyframes, doExport);
    m_logFile->stopTimer();
    m_logFile->setResultsInfo(m_keyframes);
    */

    struct ExportStats{
        enum EStep { S_READ = 0, S_RESIZE, S_WRITE };
        enum EType { S_VIDEO = 0, S_IMAGES };

        unsigned long totalTimeMeasured = 0; // ms
        EType type = S_IMAGES;

        struct ExportStepStats {
            unsigned long totalTime = 0; // ms
            unsigned long minTime = ULONG_MAX; // ms
            unsigned long maxTime = 0; // ms
        } steps[3];

        void addStepEntry(unsigned long t, EStep step){
            auto s = &steps[step];
            s->totalTime += t;
            if(t < s->minTime) s->minTime = t;
            if(t > s->maxTime) s->maxTime = t;
        }
    } stats;
    stats.type = isDirImages ? ExportStats::S_IMAGES : ExportStats::S_VIDEO;

//#define EXPORT_SERIAL
#ifdef EXPORT_SERIAL
    QElapsedTimer timer;
    QElapsedTimer totalTimer;
    totalTimer.start();
    for (uint kf : m_keyframes){

        // abort current export if abort flag is set
        if (*m_stopped) {
            m_result = 1;
            return;
        }

        // get image from reader
        timer.start();
        cv::Mat img = m_reader->getPic(kf);
        if (img.empty()) {
            m_result = 1;
            return;
        }
        stats.addStepEntry(timer.restart(), ExportStats::S_READ);

        // resize and crop
        cv::Mat imgToExport = resizeCrop(img, cv::Size(m_resolution.x(), m_resolution.y()), useResize, roi, useRoi);
        stats.addStepEntry(timer.restart(), ExportStats::S_RESIZE);

        // write images and masks
        bool success = exportImages(imgToExport, iTransformCopiesSize, fileName, isDirImages, kf, imageFiles);
        stats.addStepEntry(timer.restart(), ExportStats::S_WRITE);
        if (!success) {
            m_result = -1;
        }
        reportProgress();
    }
    stats.totalTimeMeasured = totalTimer.elapsed();

    if (m_receiver) {
        m_receiver->slot_makeProgress(100, "Exporting images");
        QString message = "Exported " + QString::number(totalTasks) + " images";
        m_receiver->slot_displayMessage(message);
    }
#else
    // create a sequential reader for accessing the images more efficiently
    SequentialReader *seqImages = m_reader->createSequentialReader(m_keyframes);

    // define lambda function to calulate blurValue for multiple images sequentially
    std::function<void(ExportStats*)> writeToDrive = [seqImages, useResize, useRoi, roi, iTransformCopiesSize, fileName, isDirImages, imageFiles, this](ExportStats *stats) {
        QElapsedTimer timer;
        // loop until the computation gets stopped or all images are processed
        std::vector<int> brokenFramesIdx;
        while(true){
            if (*m_stopped) {
                m_result = 1;
                return; // user stopped the computation -> return
            }

            // get the next image, along with its index and progress so far
            cv::Mat mat;
            uint idx;
            int progress;
            // get image from reader
            timer.start();
            if(!seqImages->getNext(mat, idx, progress)) {
                return; // no images left to process -> return
            }
            stats->addStepEntry(timer.restart(), ExportStats::S_READ);

            // empty mats report a broken image
            if (mat.empty()) {
                m_result++;
                brokenFramesIdx.push_back(idx);
                continue;
            }

            bool success = false;
            if (useResize || useRoi || !m_reader->isDir()) {
                // resize and crop
                cv::Mat imgToExport = resizeCrop(mat, cv::Size(m_resolution.x(), m_resolution.y()), useResize, roi, useRoi);
                stats->addStepEntry(timer.restart(), ExportStats::S_RESIZE);

                // write images and masks
                success = exportImages(imgToExport, iTransformCopiesSize, fileName, isDirImages, idx, imageFiles);
                stats->addStepEntry(timer.restart(), ExportStats::S_WRITE);
            } else {
                QString oImageFile = QString::fromStdString(imageFiles[idx]);
                QString nImageFile = fileName;
                nImageFile.append("/images/");
                nImageFile.append(QString::fromStdString(imageFiles[idx]).split("/").last());
                if (QFile::exists(nImageFile)) {
                    QFile::remove(nImageFile);
                }
                success = QFile::copy(oImageFile, nImageFile);
                for (int idxTransform = 0; idxTransform < 0; idxTransform++) {
                    success &= exportITransformImage(fileName, idxTransform, mat, isDirImages, imageFiles, idx);
                }
            }
            if (!success) {
                m_result = -1;
            }
            reportProgress();
        }

        // report broken frames
        if (m_result > 0)
            m_receiver->slot_displayMessage(QString::number(m_result) + tr(" images where skipped."));
        else if (m_result == 0)
            m_receiver->slot_displayMessage(tr("All images exported successfully."));
    };

    std::vector<ExportStats*> concurrentStats;

    QElapsedTimer totalTimer;
    totalTimer.start();

    // start the computation in multiple worker threads
    QFutureSynchronizer<void> synchronizer;
    // use all available threads for now
    for(int i=0; i<QThread::idealThreadCount(); i++){
        concurrentStats.push_back(new ExportStats);
        synchronizer.addFuture(QtConcurrent::run(writeToDrive, concurrentStats[i]));
    }
    synchronizer.waitForFinished();
    stats.totalTimeMeasured = totalTimer.elapsed();

    // cleanup, we have to delete the sequential reader manually
    delete seqImages;

    // combine the messurements from all theads
    for(int i=0; i<QThread::idealThreadCount(); i++){
        stats.steps[ExportStats::S_READ].totalTime += concurrentStats[i]->steps[ExportStats::S_READ].totalTime;
        stats.steps[ExportStats::S_READ].minTime = std::min(stats.steps[ExportStats::S_READ].minTime, concurrentStats[i]->steps[ExportStats::S_READ].minTime);
        stats.steps[ExportStats::S_READ].maxTime = std::max(stats.steps[ExportStats::S_READ].maxTime, concurrentStats[i]->steps[ExportStats::S_READ].maxTime);

        stats.steps[ExportStats::S_RESIZE].totalTime += concurrentStats[i]->steps[ExportStats::S_RESIZE].totalTime;
        stats.steps[ExportStats::S_RESIZE].minTime = std::min(stats.steps[ExportStats::S_RESIZE].minTime, concurrentStats[i]->steps[ExportStats::S_RESIZE].minTime);
        stats.steps[ExportStats::S_RESIZE].maxTime = std::max(stats.steps[ExportStats::S_RESIZE].maxTime, concurrentStats[i]->steps[ExportStats::S_RESIZE].maxTime);

        stats.steps[ExportStats::S_WRITE].totalTime += concurrentStats[i]->steps[ExportStats::S_WRITE].totalTime;
        stats.steps[ExportStats::S_WRITE].minTime = std::min(stats.steps[ExportStats::S_WRITE].minTime, concurrentStats[i]->steps[ExportStats::S_WRITE].minTime);
        stats.steps[ExportStats::S_WRITE].maxTime = std::max(stats.steps[ExportStats::S_WRITE].maxTime, concurrentStats[i]->steps[ExportStats::S_WRITE].maxTime);

        delete concurrentStats[i];
    }
#endif
//#if 1
    qDebug() << "Export stats";
    qDebug() << "------------";
    qDebug() << ">  total (ms):  " << stats.totalTimeMeasured;
    qDebug() << ">  read (ms):   " << stats.steps[ExportStats::S_READ].totalTime << "  min (ms): " << stats.steps[ExportStats::S_READ].minTime << "  max (ms): " << stats.steps[ExportStats::S_READ].maxTime;
    qDebug() << ">  resize (ms): " << stats.steps[ExportStats::S_RESIZE].totalTime << "  min (ms): " << stats.steps[ExportStats::S_RESIZE].minTime << "  max (ms): " << stats.steps[ExportStats::S_RESIZE].maxTime;
    qDebug() << ">  write (ms):  " << stats.steps[ExportStats::S_WRITE].totalTime << "  min (ms): " << stats.steps[ExportStats::S_WRITE].minTime << "  max (ms): " << stats.steps[ExportStats::S_WRITE].maxTime;
    qDebug() << ">  input-type:  " << (stats.type == ExportStats::S_VIDEO ? "video" : "images");
    qDebug() << "";
//#endif

    m_progress = 0;
    return;
}

void ExportThread::reportProgress() {
    m_progress++;
    if (m_receiver) {
        int percentProgress = m_progress * 100 / (int)m_keyframes.size();
        m_receiver->slot_makeProgress(percentProgress, tr("Exporting images"));
    }

}

cv::Mat ExportThread::resizeCrop(cv::Mat image, cv::Size resize, bool useResize, cv::Rect crop, bool useCrop) {
    cv::Mat output = image;
    if (useResize) {
        cv::resize(image, output, resize, 0, 0, cv::INTER_AREA);
    }
    if (useCrop) {
        output = output(crop);
    }
    return output;
}

bool ExportThread::exportImages(cv::Mat image, int iTransformCopiesSize, const QString &fileName, bool isDirImages, int currentKeyframe, std::vector<std::string> imageFiles) {

    QString imgPath = fileName;
    //directory of export (without /images)
    //normal picture export
    imgPath.append("/images/");
    //append image specific name (for normal)

    if (isDirImages) {
        //Get name of the current keyframe
        QString keyframeName = QString::fromStdString(imageFiles[currentKeyframe]);
        QStringList splitedName = keyframeName.split("/");
        imgPath.append(splitedName.back());
        //QFileInfo file(keyframeName);
        //QString newFileName = file.baseName().append(".png");
        //imgPath.append(newFileName);
    }
    else {
        //If input is a video images are numbered based on their index
        imgPath.append(QString::number(currentKeyframe, 10).rightJustified(8, '0').append(".png"));
    }
    //write image on disk
    bool exportedImage = cv::imwrite(imgPath.toStdString(), image, JPEG_COMPRESSION_PARAMS);
    if (!exportedImage) {
        qDebug() << "failed to export " + imgPath;
        return false;
    }
    for (int idxTransform = 0; idxTransform < iTransformCopiesSize; ++idxTransform) {
        //iTransform export
        if (!exportITransformImage(fileName, idxTransform, image, isDirImages, imageFiles, currentKeyframe)) {
            return false;
        }
    }

    //save gps data
    QList<MetaDataReader*> mdList =  MetaDataManager::instance().loadAllMetaData();
    bool readerFound = false;
    MetaDataReader* gpsReader;

    for (MetaDataReader* md : mdList) {
        if (md->getName().contains("gps", Qt::CaseInsensitive)) {
            gpsReader = md;
            readerFound= true;
            break;
        }
    }
    if (!readerFound) {
        return true;
    }


    QVariant gpsData = gpsReader->getImageMetaData(currentKeyframe);
    QMutexLocker locker(&m_mutex);

    QElapsedTimer timer;
    timer.start();

    char* exifData = m_exportExif->saveExif(imgPath, gpsData);


    std::FILE *file = std::fopen(imgPath.toStdString().c_str(), "rb");
    if (!file) {
      return false;
    }
    std::fseek(file, 0, SEEK_END);
    unsigned long fileSize = std::ftell(file);
    std::rewind(file);
    unsigned char *fileData = new unsigned char[fileSize];
    if (std::fread(fileData, 1, fileSize, file) != fileSize) {
      delete[] fileData;
      return false;
    }
    std::fclose(file);

    char* newData;
    int size = m_exportExif->getExifSize();
    QFileInfo info(imgPath);
    QString fileExtension = info.completeSuffix().toLower();
    if (fileExtension == "jpeg" || fileExtension == "jpg") {
        //JPEG
        newData= new char[fileSize + size];
        //2Byte Offset : FF D8
        std::memcpy(&newData[0], &fileData[0], 2);
        std::memcpy(&newData[2], &exifData[0], size);
        std::memcpy(&newData[2 + size], &fileData[2], fileSize - 2);
    }
    else if (fileExtension == "png") {
        //PNG
        newData= new char[fileSize + size];
        //8Byte start png + 4byte IHDR length + 4byte IHDR tag + 13byte IHDR data + 4 byte IHDR crc = 33 byte Offset
        std::memcpy(&newData[0], &fileData[0], 33);
        std::memcpy(&newData[33], &exifData[0], size);
        std::memcpy(&newData[33 + size], &fileData[33], fileSize - 33);
    }
    else {
        return true;
    }


    std::ofstream f(imgPath.toStdString().c_str(), std::ofstream::binary);
    f.write((char *)&newData[0], fileSize + size);
    delete[] newData;
    delete[] exifData;
    delete[] fileData;
    //qDebug() << "Time needed: " << timer.elapsed();
    return true;
}

bool ExportThread::exportITransformImage(QString fileName, int idxTransform, cv::Mat image, bool isDirImages, std::vector<std::string> imageFiles, int currentKeyframe)
{
    QString iTPath = fileName;
    iTPath.append("/").append(m_iTransformCopies[idxTransform]->getName()).append("/");
    QStringList iTOutputNames = m_iTransformCopies[idxTransform]->getOutputNames();

    //calculate outputImgs
    ImageList segmanticImgs = m_iTransformCopies[idxTransform]->transform(0, image);

    if (segmanticImgs.length() != iTOutputNames.length()) {
        return false;
    }

    for (int t = 0; t < iTOutputNames.length(); ++t) {
        QString iTransformOutPath = iTPath;
        iTransformOutPath.append(iTOutputNames[t]).append("/");
        //directory was created earlier so we can just use it

        //append image specific name (iTransforms)

        if (isDirImages) {
            //Get name of the current keyframe
            QString keyframeName = QString::fromStdString(imageFiles[currentKeyframe]);
            QStringList splitedName = keyframeName.split("/");
            iTransformOutPath.append(splitedName.back());
        }
        else {
            iTransformOutPath.append(QString::number(currentKeyframe, 10).rightJustified(8, '0').append(".png"));
        }

        //write image on disk
        if (!cv::imwrite(iTransformOutPath.toStdString(), segmanticImgs[idxTransform], JPEG_COMPRESSION_PARAMS)) {
            return false;
        }
    }
    return true;
}
