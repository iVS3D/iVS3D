#include "imagereader.h"

ImageReader::ImageReader(QString path, std::shared_ptr<ReaderParams> readerParams) : m_readerParams(readerParams)
{
    QFileInfo fileInfo(path);
    if (!fileInfo.isDir()) {
        m_isValid = false;
        return;
    }
    QDir dir(path);
    QStringList filters;
    filters << "*.png" << "*.bmp" << "*.jpeg" << "*.jpg" << "*.tiff" << "*.tif";
    dir.setNameFilters(filters);
    QFileInfoList files = dir.entryInfoList();
    QCollator collator;
    collator.setNumericMode(true);
    m_filePaths.clear();

    std::sort(
         files.begin(),
         files.end(),
         [&collator](const QFileInfo &file1, const QFileInfo &file2)
         {
             return collator.compare(file1.fileName(), file2.fileName()) < 0;
         });

    for (const QFileInfo &info : qAsConst(files)) {
        m_filePaths.push_back(info.absoluteFilePath().toStdString());
    }
    m_numImages = static_cast<int>(m_filePaths.size());
    m_folderPath = path.toStdString();
    if (m_numImages > 0) {
        m_isValid = true;
    }
}

cv::Mat ImageReader::getPic(unsigned int index, PictureProcessingFlags flags)
{
    if(index > getPicCount()){
        cv::Mat empty;
        return empty;
    }

    cv::Mat img = cv::imread(m_filePaths.at(index));
    if(flags & PictureProcessingFlags::APPLY_RESIZING){
        m_readerParams->getWorkingResolution().resize(img);
    }
    if(flags & PictureProcessingFlags::APPLY_CROPPING && m_readerParams->getUseRoi()) {
        m_readerParams->getRoi().crop(img);
    }
    return img;
}

unsigned int ImageReader::getPicCount()
{
    return m_numImages;
}

double ImageReader::getFPS()
{
    return -1;
}

double ImageReader::getVideoDuration()
{
    return -1;
}

QString ImageReader::getInputPath()
{
    return QString::fromStdString(m_folderPath);
}

bool ImageReader::isDir()
{
    return m_isValid;
}

std::vector<std::string> ImageReader::getFileVector()
{
    return m_filePaths;
}

ImageReader *ImageReader::copy()
{
    ImageReader *ir = new ImageReader();
    ir->m_folderPath = m_folderPath;
    ir->m_numImages = m_numImages;
    ir->m_filePaths = m_filePaths;
    ir->m_isValid = m_isValid;
    ir->m_readerParams = std::make_shared<ReaderParams>(*m_readerParams);
    ir->addMetaData(m_md);
    return ir;
}

void ImageReader::addMetaData(MetaData *md)
{
    m_md = md;
}

MetaData *ImageReader::getMetaData()
{
    return m_md;
}

bool ImageReader::isValid()
{
    return m_isValid;
}

SequentialReader *ImageReader::createSequentialReader(std::vector<uint> indices, PictureProcessingFlags flags)
{
    return new SequentialReaderImpl(this, indices, false, flags);
}

ImageReader::ImageReader()
{

}

