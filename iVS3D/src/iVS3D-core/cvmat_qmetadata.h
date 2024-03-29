#ifndef CVMAT_QMETADATA_H
#define CVMAT_QMETADATA_H

#include <QObject>
#include <QColor>
#include <QList>
#include "opencv2/core.hpp"

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

#endif // CVMAT_QMETADATA_H
