#ifndef STRINGCONTAINER_H
#define STRINGCONTAINER_H

#include <QObject>

/**
 * @ingroup model
 *
 * @brief The stringContainer struct defines global string identifiers
 *
 * @author Daniel Brommer
 *
 * @date 2021/07/21
 */

struct stringContainer
{
    // log
    static const QString dirLogFile;
    // Identifier for UI flag
    static const char* UIIdentifier;
    // Identifier for the settings maps
    static const QString OutputPath;
    static const QString Resolution;
    static const QString ROI;
    static const QString UseROI;
    static const QString UseITransform;
    static const QString ITransformSettings;
    static const QString Export;
    static const QString ROISpliter;

    // log file import
    static const QString lfImportProcess;
    static const QString lfImportMip;
    static const QString lfImportPath;
    // log file import project
    static const QString lfImportProject;
    static const QString lfImportProjectPath;
    // log file export
    static const QString lfExportProcess;
    static const QString lfExportPreprocessing;
    static const QString lfExportFrames;
    // log file rotation
    static const QString lfRotationBuffer;
    static const QString lfRotationOptFlowTotal;
};

#endif // STRINGCONTAINER_H
