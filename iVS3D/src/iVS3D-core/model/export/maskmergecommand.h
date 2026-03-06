#ifndef MASKMERGECOMMAND_H
#define MASKMERGECOMMAND_H

#include <QString>
#include <vector>
#include <optional>

#include "imageprocessor.h"

/**
 * @class MaskMergeCommand
 * @brief Loads an existing mask image from disk and merges it with the current
 *        mask in the context using pixel-wise AND.
 *
 * For binary masks where black(0) means masked and white(255) means unmasked,
 * pixel-wise AND ensures that black dominates:
 * - 0 & 255 -> 0
 * - 0 & 0   -> 0
 * - 255 & 255 -> 255
 * 
 * @author Dominik Wüst (dominik.wuest@iosb.fraunhofer.de)
 * @date March 2026
 */
class MaskMergeCommand : public ImageCommand {
public:
    MaskMergeCommand(QString folderPath, QString format,
                     std::vector<std::string> files = {});

    std::optional<QString> execute(ImageContext& ctx) override;

private:
    QString maskFilePathForIndex(uint index) const;
    static cv::Mat toBinaryMask(const cv::Mat& input);

    QString m_folder;
    QString m_format;
    std::vector<std::string> m_files;
};

#endif // MASKMERGECOMMAND_H
