#include "maskmergecommand.h"

#include <QDir>
#include <QFile>

#include <opencv2/imgproc.hpp>

MaskMergeCommand::MaskMergeCommand(QString folderPath, QString format,
                                   std::vector<std::string> files)
    : m_folder(std::move(folderPath)),
      m_format(std::move(format)),
      m_files(std::move(files)) {}

QString MaskMergeCommand::maskFilePathForIndex(uint index) const {
    if (!m_files.empty() && index < m_files.size() && !m_files[index].empty()) {
        QString sourcePath = QString::fromStdString(m_files[index]);
        QString name = sourcePath.split("/").last();
        name = name.left(name.lastIndexOf('.'));
        name += "." + m_format;
        return QDir::cleanPath(m_folder + "/" + name);
    }

    return QDir::cleanPath(
        m_folder + "/" + QString::number(index, 10).rightJustified(8, '0') +
        "." + m_format);
}

cv::Mat MaskMergeCommand::toBinaryMask(const cv::Mat& input) {
    if (input.empty()) {
        return {};
    }

    cv::Mat gray;
    if (input.channels() == 1) {
        gray = input;
    } else if (input.channels() == 3) {
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    } else if (input.channels() == 4) {
        cv::cvtColor(input, gray, cv::COLOR_BGRA2GRAY);
    } else {
        gray = input;
    }

    cv::Mat u8;
    if (gray.depth() != CV_8U) {
        gray.convertTo(u8, CV_8U);
    } else {
        u8 = gray;
    }

    cv::Mat binary;
    cv::threshold(u8, binary, 127, 255, cv::THRESH_BINARY);
    return binary;
}

std::optional<QString> MaskMergeCommand::execute(ImageContext& ctx) {
    if (ctx.image.empty()) {
        return "ERROR: current mask is empty before merge";
    }

    const QString targetPath = maskFilePathForIndex(ctx.index);
    if (!QFile::exists(targetPath)) {
        return "ERROR: existing mask file not found for merge: " + targetPath;
    }

    cv::Mat existing = cv::imread(targetPath.toStdString(), cv::IMREAD_UNCHANGED);
    if (existing.empty()) {
        return "ERROR: failed to read existing mask for merge: " + targetPath;
    }

    cv::Mat existingMask = toBinaryMask(existing);
    cv::Mat currentMask = toBinaryMask(ctx.image);

    if (existingMask.empty() || currentMask.empty()) {
        return "ERROR: invalid mask image while merging: " + targetPath;
    }

    if (existingMask.size() != currentMask.size()) {
        cv::resize(existingMask, existingMask, currentMask.size(), 0, 0,
                   cv::INTER_NEAREST);
    }

    cv::Mat merged;
    cv::bitwise_and(existingMask, currentMask, merged);
    ctx.image = std::move(merged);

    return std::nullopt;
}
