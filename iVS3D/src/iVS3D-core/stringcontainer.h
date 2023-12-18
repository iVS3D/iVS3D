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
    // config settings (QSettings)
    static const QString settingsCompany;
    static const QString settingsProgramm;
    // log
    static const QString dirLogFile;
    // Identifier for UI flag
    static const char* UIIdentifier;
    static const char* OverwriteExport;
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
    // Identifier for project inforamtion
    static const QString projectNameIdentifier;
    static const QString jsonDelimiter;
    //Identifier for mip member
    static const QString mipIdentifier;
    static const QString inputPathIdentifier;
    static const QString keyframesIdentifier;
    static const QString boundariesIdentifier;
    //Identifier for ma
    static const QString maIdentifier;
    //Identifier for exportData
    static const QString exportDataIdentifier;
    static const QString keyframeCountIdentifier;
    static const QString exportResolutionIdentifier;
    static const QString exportPathIdentifier;
    //Identifier for ApplicationSettings
    static const QString applicationSettingsIdentifier;
    static const QString reconstructSoftwareIdentifier;
    static const QString standardInputPathIdentifier;
    static const QString disableChecksIdentifier;
    static const QString colorThemeIdentifier;
    static const QString useCudaIdentifier;
    static const QString createLogsIdentifier;
    static const QString interpolateIdentifier;
    // Identifier for LogFile
    static const QString logNameIdentifier;
    static const QString logIsPluginIdentifier;
    static const QString logCreationTimeIdentifier;
    static const QString logSettingsIdentifier;
    //      input
    static const QString logInputIdentifier;
    //      procedure
    static const QString logTotalProcedureIdentifier;
    static const QString logProcedureIdentifier;
    static const QString logStartTimeIdentifier;
    static const QString logStopTimeIdentifier;
    static const QString logElapsedTimeIdentifier;
    //      results
    static const QString logResultsIdentifier;
    static const QString logKeyframeCountIdentifier;
    static const QString logKeyframesIdentifier;
    //      custom area
    static const QString logCustomIdentifier;
    static const QString logTypeIdentifier;
    //      GPS meta data
    static const QString latitudeIdentifier;
    static const QString longitudeIdentifier;
    static const QString altitudeIdentifier;
    static const QString latitudeRefIdentifier;
    static const QString longitudeRefIdentifier;
    static const QString altitudeRefIdentifier;
    static const QString latitudeSouth;
    static const QString latitudeNorth;
    static const QString longitudeEast;
    static const QString longitudeWest;
    static const QString altitudeAboveSea;
    static const QString altitudeBelowSea;
};

#endif // STRINGCONTAINER_H
