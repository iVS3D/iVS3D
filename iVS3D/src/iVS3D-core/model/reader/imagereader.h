#ifndef IMAGEREADER_H
#define IMAGEREADER_H

#include <qfileinfo.h>

#include <QCollator>
#include <QDir>
#include <QFileInfo>
#include <QObject>
#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>

#include "reader_impl.h"
#include "readererror.h"

/**
 * @class ImageReader
 *
 * @ingroup Model
 *
 * @brief The ImageReader class provides input metadata aswell as on demand
 * image reading/loading
 *
 * @author Daniel Brommer
 * @author Dominic Zahn (REWORK)
 *
 * @date 2021/04/14
 * @date 2026/02/20 (REWORK)
 */
class ImageReader : public ReaderImpl {
   public:
    ImageReader();

    ~ImageReader() override {}

    // image retrieve
    ReaderResult init(std::string filePath) override;

    // getter
    ReaderResult index2Pts(uint index, double& outPts) const override;
    ReaderResult pts2Index(double pts, uint& index) const override;
    // setter
    ReaderResult setAvgFps(double avgFps);
    ReaderResult setDuration(double duration);

    /**
     * @brief getFileVector getter for entire image path list
     * @return list of all image paths, a single entry is one full image path
     */
    std::vector<std::string> getFileVector();

   protected:
    RetrieveResult retrieveImage(uint index) override;

   private:
    void updatePtsVec();

    std::vector<std::string> m_filePaths = {};

    std::vector<double> m_ptsVec = {};
};

#endif  // IMAGEREADER_H
