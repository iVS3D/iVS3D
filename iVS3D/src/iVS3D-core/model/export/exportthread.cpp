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
    for (int i = -1; i < iTransformCopiesSize; ++i) {
        //directory of export (without /images)
        if (i < 0) {


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
                qDebug() << "failed to export " + imgPath;
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


    QPointF gpsData = gpsReader->getImageMetaData(currentKeyframe).toPointF();
    QMutexLocker locker(&m_mutex);

    QElapsedTimer timer;
    timer.start();

    double latitude = gpsData.x();
    double degreeLatitude = int(latitude);
    double minutesLatitude = int((latitude - degreeLatitude) * 60);
    double secondsLatitude = int((latitude - degreeLatitude - (minutesLatitude/60)) * 3600);
    //If latitude > 0 it is in the north, south otherwise
    char latitudeRef = (latitude > 0) ? 'N' : 'S';


    double longitude = gpsData.y();
    double degreeLongitude = int(longitude);
    double minutesLongitude= int((longitude - degreeLongitude) * 60);
    double secondsLongitude= int((longitude - degreeLongitude - (minutesLongitude/60)) * 3600);
    //If longitude > 0 it is in the east, south west
    char longitudeRef = (longitude > 0) ? 'E' : 'W';


    unsigned char * exifData = new unsigned char[140];
    //4Bytes length of Data Only (without this, tag and crc) --> 137 - 9 = 128 (Big endian)
    exifData[0] = 0x00;
    exifData[1] = 0x00;
    exifData[2] = 0x00;
    exifData[3] = 0x80;
    //eXIf Tag PNG --> eXIf
    exifData[4] = 101;
    exifData[5] = 88;
    exifData[6] = 73;
    exifData[7] = 102;
    //Start of Data
    // II tag
    exifData[8] = 0x49;
    exifData[9] = 0x49;
    //Algin bytes 0x2A00
    exifData[10] = 0x2A;
    exifData[11] = 0x00;
    //Offset to first ifd (4 Bytes) --> total of 8 bytes offset
    exifData[12] = 0x08;
    exifData[13] = 0x00;
    exifData[14] = 0x00;
    exifData[15] = 0x00;
    //Count of IFD, 2 bytes --> only gps ifd
    exifData[16] = 0x01;
    exifData[17] = 0x00;
    //GPS IFD Tag , 12Bytes
    exifData[18] = 0x25;
    exifData[19] = 0x88;
    //Type
    exifData[20] = 0x04;
    exifData[21] = 0x00;
    //Count
    exifData[22] = 0x01;
    exifData[23] = 0x00;
    exifData[24] = 0x00;
    exifData[25] = 0x00;
    //Value -> Pointer to gps tags
    exifData[26] = 0x1A;
    exifData[27] = 0x00;
    exifData[28] = 0x00;
    exifData[29] = 0x00;
    //4 Byte offset to new segment
    exifData[30] = 0x00;
    exifData[31] = 0x00;
    exifData[32] = 0x00;
    exifData[33] = 0x00;
    //Count of new segment = 4 (Lat, LatRef, Long, LongRef)
    exifData[34] = 0x04;
    exifData[35] = 0x00;
    //GPS LatitudeRef -> 12 bytes, tag 01
    exifData[36] = 0x01;
    exifData[37] = 0x00;
    //Ascii type
    exifData[38] = 0x02;
    exifData[39] = 0x00;
    //Count 2
    exifData[40] = 0x02;
    exifData[41] = 0x00;
    exifData[42] = 0x00;
    exifData[43] = 0x00;
    //Value -> N or S
    exifData[44] = latitudeRef;
    exifData[45] = 0x00;
    exifData[46] = 0x00;
    exifData[47] = 0x00;
    //GPS Latitude -> 12 bytes, tag 02
    exifData[48] = 0x02;
    exifData[49] = 0x00;
    //rational type
    exifData[50] = 0x05;
    exifData[51] = 0x00;
    //Count 3 (deg,h,s)
    exifData[52] = 0x03;
    exifData[53] = 0x00;
    exifData[54] = 0x00;
    exifData[55] = 0x00;
    //Pointer to actual values (This should always fit here)
    exifData[56] = 0x50;
    exifData[57] = 0x00;
    exifData[58] = 0x00;
    exifData[59] = 0x00;
    //GPS LongitudeRef -> 12 bytes, tag 03
    exifData[60] = 0x03;
    exifData[61] = 0x00;
    //Ascii type
    exifData[62] = 0x02;
    exifData[63] = 0x00;
    //Count 2
    exifData[64] = 0x02;
    exifData[65] = 0x00;
    exifData[66] = 0x00;
    exifData[67] = 0x00;
    //Value -> E or W
    exifData[68] = longitudeRef;
    exifData[69] = 0x00;
    exifData[70] = 0x00;
    exifData[71] = 0x00;
    //GPS Longitude -> 12 bytes, tag 04
    exifData[72] = 0x04;
    exifData[73] = 0x00;
    //Rational type
    exifData[74] = 0x05;
    exifData[75] = 0x00;
    //Count 3 (deg,h,s)
    exifData[76] = 0x03;
    exifData[77] = 0x00;
    exifData[78] = 0x00;
    exifData[79] = 0x00;
    //Pointer to actual values (This should always fit here)
    exifData[80] = 0x68;
    exifData[81] = 0x00;
    exifData[82] = 0x00;
    exifData[83] = 0x00;
    //4 byte offset to data segment
    exifData[84] = 0x00;
    exifData[85] = 0x00;
    exifData[86] = 0x00;
    exifData[87] = 0x00;
    //Pointer to Latitude Data -> 3 rational with 2x4 bytes each -> second 4 byte block is always 1
    //Degree numerator
    exifData[88] = degreeLatitude;
    exifData[89] = 0x00;
    exifData[90] = 0x00;
    exifData[91] = 0x00;
    //Degree denumerator
    exifData[92] = 0x01;
    exifData[93] = 0x00;
    exifData[94] = 0x00;
    exifData[95] = 0x00;
    //Minutes numerator TODO
    exifData[96] = minutesLatitude;
    exifData[97] = 0x00;
    exifData[98] = 0x00;
    exifData[99] = 0x00;
    //Minutes denumerator
    exifData[100] = 0x01;
    exifData[101] = 0x00;
    exifData[102] = 0x00;
    exifData[103] = 0x00;
    //Seconds numerator
    exifData[104] = secondsLatitude;
    exifData[105] = 0x00;
    exifData[106] = 0x00;
    exifData[107] = 0x00;
    //Seconds denumerator
    exifData[108] = 0x01;
    exifData[109] = 0x00;
    exifData[110] = 0x00;
    exifData[111] = 0x00;
    //Pointer to Longitude Data -> 3 rational with 2x4 bytes each -> second 4 byte block is always 1
    //Degree numerator TODO
    exifData[112] = degreeLongitude;
    exifData[113] = 0x00;
    exifData[114] = 0x00;
    exifData[115] = 0x00;
    //Degree denumerator
    exifData[116] = 0x01;
    exifData[117] = 0x00;
    exifData[118] = 0x00;
    exifData[119] = 0x00;
    //Minutes numerator
    exifData[120] = minutesLongitude;
    exifData[121] = 0x00;
    exifData[122] = 0x00;
    exifData[123] = 0x00;
    //Minutes denumerator
    exifData[124] = 0x01;
    exifData[125] = 0x00;
    exifData[126] = 0x00;
    exifData[127] = 0x00;
    //Seconds numerator
    exifData[128] = secondsLongitude;
    exifData[129] = 0x00;
    exifData[130] = 0x00;
    exifData[131] = 0x00;
    //Seconds denumerator
    exifData[132] = 0x01;
    exifData[133] = 0x00;
    exifData[134] = 0x00;
    exifData[135] = 0x00;
    //End of Data
    //CRC over exifData --> without length field
    exifData[136] = 0x00;//0x26;
    exifData[137] = 0x00;//0x69;
    exifData[138] = 0x00;//0x47;
    exifData[139] = 0x00;//0x8B;

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

    char* newData= new char[fileSize + 140];
    //8Byte start png + 4byte IHDR length + 4byte IHDR tag + 13byte IHDR data + 4 byte IHDR crc = 33 byte
    std::memcpy(&newData[0], &fileData[0], 33);
    std::memcpy(&newData[33], &exifData[0], 140);
    std::memcpy(&newData[33+140], &fileData[33], fileSize - 33);

    std::ofstream f(imgPath.toStdString().c_str());
    f.write((char *)&newData[0], fileSize + 140);
    qDebug() << "Time needed: " << timer.elapsed();
    return true;
}
