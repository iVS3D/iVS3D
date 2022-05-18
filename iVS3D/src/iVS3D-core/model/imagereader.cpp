#include "imagereader.h"

ImageReader::ImageReader(QString path)
{
    QDir dir(path);
    QStringList filters;
    filters << "*.png" << "*.bmp" << "*.jpeg" << "*.jpg";
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
}

cv::Mat ImageReader::getPic(unsigned int index, bool)
{
    if(index > getPicCount()){
        cv::Mat empty;
        return empty;
    }

    return cv::imread(m_filePaths.at(index));
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
    return true;
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

ImageReader::ImageReader()
{

}

