#include "exportthread.h"

ExportThread::ExportThread(Progressable* receiver, ModelInputPictures* mip, QPoint resolution, const QString &path, const QString &name, volatile bool* stopped, QRect roi, const std::vector<ITransform*> &iTransformCopies, LogFile *logFile)
{
    m_receiver = receiver;
    m_reader = mip->getReader();
    m_keyframes = mip->getAllKeyframes(mip->getBoundaries());
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
    fileName.replace("/images", "");

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

    m_reader->initMultipleAccess(m_keyframes);

    std::function<void(const int)> doExport = [this, useResize, roi, useRoi, iTransformCopiesSize, fileName, isDirImages, imageFiles](const int &n) {
        if(*m_stopped){
            m_result = 1;
            return;
        }
        if (m_result == -1) {
            return;
        }
        reportProgress();

        cv::Mat currentImage;
        while (currentImage.empty()) {
           currentImage = m_reader->getPic(n, true);
        }
        cv::Mat imageToExport = resizeCrop(currentImage, cv::Size(m_resolution.x(), m_resolution.y()), useResize, roi, useRoi);
        bool success = exportImages(imageToExport, iTransformCopiesSize, fileName, isDirImages, n, imageFiles);
        if (!success) {
            m_result = -1;
        }
        return;
    };
    m_logFile->startTimer(stringContainer::lfExportFrames);
    //calculate blurValues on multiple threads
    QtConcurrent::blockingMap(m_keyframes, doExport);
    m_logFile->stopTimer();
    m_logFile->setResultsInfo(m_keyframes);


    if (m_receiver) {
        m_receiver->slot_makeProgress(100, "Exporting images");
        QString message = "Exported " + QString::number(totalTasks) + " images";
        m_receiver->slot_displayMessage(message);
    }

    m_progress = 0;
    return;
}

void ExportThread::reportProgress() {
    m_progress++;
    if (m_receiver) {
        int percentProgress = m_progress * 100 / (int)m_keyframes.size();
        m_receiver->slot_makeProgress(percentProgress, "Exporting images");
    }

}

cv::Mat ExportThread::resizeCrop(cv::Mat image, cv::Size resize, bool useResize, cv::Rect crop, bool useCrop) {
    cv::Mat output = image;
    if (useResize) {
        cv::resize(image, output, resize);
    }
    if (useCrop) {
        output = output(crop);
    }
    return output;
}

bool ExportThread::exportImages(cv::Mat image, int iTransformCopiesSize, const QString &fileName, bool isDirImages, int currentKeyframe, std::vector<std::string> imageFiles) {

    for (int i = -1; i < iTransformCopiesSize; ++i) {
        //directory of export (without /images)
        if (i < 0) {
            QString imgPath = fileName;

            //normal picture export
            imgPath.append("/images/");
            //append image specific name (for normal)

            if (isDirImages) {
                //Get name of the current keyframe
                QString keyframeName = QString::fromStdString(imageFiles[currentKeyframe]);
                QStringList splitedName = keyframeName.split("/");
                imgPath.append(splitedName.back());
            }
            else {
                //If input is a video images are numbered based on their index
                imgPath.append(QString::number(currentKeyframe, 10).rightJustified(8, '0').append(".png"));
            }
            //write image on disk
            bool exportedImage = cv::imwrite(imgPath.toStdString(), image);
            if (!exportedImage) {
                return false;
            }
        }
        else {
            //iTransform export
            QString iTPath = fileName;
            iTPath.append("/").append(m_iTransformCopies[i]->getName()).append("/");
            QStringList iTOutputNames = m_iTransformCopies[i]->getOutputNames();

            //calculate outputImgs
            ImageList segmanticImgs = m_iTransformCopies[i]->transform(0, image);

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
                cv::imwrite(iTransformOutPath.toStdString(), segmanticImgs[i]);
            }

        }
    }
    return true;
}
