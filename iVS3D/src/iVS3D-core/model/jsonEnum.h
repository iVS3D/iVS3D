#ifndef JSONENUM_H
#define JSONENUM_H

#include <QObject>

/**
 * @ingroup model
 *
 * @brief The jsonEnum struct defines global identifiers which are used in some json-file (e.x. project file)
 *
 * @author Daniel Brommer
 *
 * @date 2021/02/08
 */
struct jsonEnum
{
    // Identifier for project inforamtion
    static const QString projectNameIdentifier;
    static const QString jsonDelimiter;

    //Identifier for mip member
    static const QString mipIdentifier;
    static const QString inputPathIdentifier;
    static const QString keyframesIdentifier;

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
    static const QString darkStyleIdentifier;
    static const QString useCudaIdentifier;
    static const QString createLogsIdentifier;

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
    // Identifiers used for logging in core
};




#endif // JSONENUM_H
