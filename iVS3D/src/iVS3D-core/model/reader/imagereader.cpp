#include "imagereader.h"

#include <qfileinfo.h>

#include <opencv2/core.hpp>
#include <opencv2/core/utils/filesystem.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "reader.h"
#include "reader_impl.h"
#include "readererror.h"

ImageReader::ImageReader() {}

ReaderResult ImageReader::init(std::string filePath) {
    ReaderResult parentInit = ReaderImpl::init(filePath);
    if (parentInit != ReaderResult::Success) return parentInit;

    QString qFilePath = QString::fromStdString(filePath);
    QFileInfo fileInfo(qFilePath);
    if (!fileInfo.isDir()) {
        return ReaderResult::FileError;
    }
    QDir dir(qFilePath);
    QStringList filters;
    filters << "*.png" << "*.bmp" << "*.jpeg" << "*.jpg" << "*.tiff" << "*.tif";
    dir.setNameFilters(filters);
    QFileInfoList files = dir.entryInfoList();
    QCollator collator;
    collator.setNumericMode(true);
    m_filePaths.clear();

    std::sort(files.begin(), files.end(),
              [&collator](const QFileInfo& file1, const QFileInfo& file2) {
                  return collator.compare(file1.fileName(), file2.fileName()) <
                         0;
              });

    for (const QFileInfo& info : qAsConst(files)) {
        m_filePaths.push_back(info.absoluteFilePath().toStdString());
    }
    m_imageCount = static_cast<int>(m_filePaths.size());
    if (m_imageCount <= 0) {
        return ReaderResult::FileError;
    }

    return ReaderResult::Success;
}

ReaderResult ImageReader::index2Pts(uint index, double& outPts) const {
    OUTOFBOUND_GUARD(index)
    outPts = m_ptsVec[index];
    return ReaderResult::Success;
}

ReaderResult ImageReader::pts2Index(double pts, uint& index) const {
    if (pts < 0.0 || pts > m_duration) return ReaderResult::OutOfBound;

    for (auto it = m_ptsVec.begin(); it != m_ptsVec.end(); it++) {
        if (*it >= pts) {
            index = it - m_ptsVec.begin();
            return ReaderResult::Success;
        }
    }
    return ReaderResult::OutOfBound;
}

// setter
ReaderResult ImageReader::setAvgFps(double avgFps) {
    if (avgFps <= 0) return ReaderResult::OutOfBound;
    m_avgFps = avgFps;
    m_duration = (double)m_imageCount / avgFps;

    updatePtsVec();
    return ReaderResult::Success;
}
ReaderResult ImageReader::setDuration(double duration) {
    if (duration <= 0) return ReaderResult::OutOfBound;
    m_duration = duration;
    m_avgFps = (double)m_imageCount / duration;

    updatePtsVec();
    return ReaderResult::Success;
}

std::vector<std::string> ImageReader::getFileVector() { return m_filePaths; }

std::pair<ReaderResult, cv::Mat> ImageReader::retrieveImage(uint index) {
    RetrieveResult rr = {ReaderResult::UnkownError, cv::Mat()};
    if (index < 0 || index >= m_imageCount) {
        rr.first = ReaderResult::OutOfBound;
        return rr;
    }

    try {
        rr.second = cv::imread(m_filePaths.at(index));
        rr.first = ReaderResult::Success;
    } catch (cv::Exception& e) {
        rr.second = cv::Mat();
        rr.first = ReaderResult::UnkownError;
    }
    return rr;
}

void ImageReader::updatePtsVec() {
    for (uint i = 0; i < m_imageCount; i++) m_ptsVec[i] = (double)i * m_avgFps;
}
