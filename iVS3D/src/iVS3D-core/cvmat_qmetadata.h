#ifndef CVMAT_QMETADATA_H
#define CVMAT_QMETADATA_H

/**
 * @defgroup iVS3D iVS3D Main App
 */

#include <QObject>
#include <QColor>
#include <QList>
#include "opencv2/core.hpp"
#include "resolution.h"
#include "roi.h"

Q_DECLARE_METATYPE(cv::Mat)

Q_DECLARE_METATYPE(QStringList)

typedef QList<cv::Mat> ImageList;
Q_DECLARE_METATYPE(ImageList)

typedef QList<QColor> QColorList;
Q_DECLARE_METATYPE(QColorList)

typedef QList<bool> QBoolList;
Q_DECLARE_METATYPE(QBoolList)

Q_DECLARE_METATYPE(std::vector<uint>)

/**
 * @brief Color theme: Light or dark mode
 */
enum ColorTheme {
    LIGHT = 0,  /**< Light theme. */
    DARK        /**< Dark theme. */
};
Q_DECLARE_METATYPE(ColorTheme)

Q_DECLARE_METATYPE(Resolution)

Q_DECLARE_METATYPE(ROI)

#endif // CVMAT_QMETADATA_H
