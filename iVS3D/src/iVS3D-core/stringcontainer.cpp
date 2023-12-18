#include "stringcontainer.h"

// config settings (QSettings)
const QString stringContainer::settingsCompany = "IOSB-Fraunhofer";
const QString stringContainer::settingsProgramm = "iVS3D";

// log file strings
const QString stringContainer::dirLogFile = "logs";
// Identifier for UI flag
const char* stringContainer::UIIdentifier = "UI";
const char* stringContainer::OverwriteExport = "OverwriteExport";
// Identifier for the settings maps
const QString stringContainer::OutputPath = "Output path";
const QString stringContainer::Resolution = "Resolution";
const QString stringContainer::ROI = "ROI";
const QString stringContainer::UseROI = "use ROI";
const QString stringContainer::UseITransform = "use ITransform";
const QString stringContainer::ITransformSettings = "ITransform settings";
const QString stringContainer::Export = "Export";
const QString stringContainer::ROISpliter = "x";
// log file import
const QString stringContainer::lfImportProcess = "Import";
const QString stringContainer::lfImportMip = "Model input pictures";
const QString stringContainer::lfImportPath = "Path";
// log file import project
const QString stringContainer::lfImportProject = "Project";
const QString stringContainer::lfImportProjectPath = "Project path";
// log file export
const QString stringContainer::lfExportProcess = "Export";
const QString stringContainer::lfExportPreprocessing = "Preproccssing";
const QString stringContainer::lfExportFrames = "Keyframes and Masks";
// Identifier for project inforamtion
const QString stringContainer::projectNameIdentifier = "Project name";
const QString stringContainer::jsonDelimiter = ",";

// Identifier for ModelInputPictures member
const QString stringContainer::mipIdentifier = "ModelInputPictures";
const QString stringContainer::inputPathIdentifier = "inputPath";
const QString stringContainer::keyframesIdentifier = "keyframes";
const QString stringContainer::boundariesIdentifier = "boundaries";

// Identifier for ModelAlgorithm
const QString stringContainer::maIdentifier = "ModelAlgorithm";

// Identifier for exportData
const QString stringContainer::exportDataIdentifier = "ExportData";
const QString stringContainer::keyframeCountIdentifier = "keyframeCount";
const QString stringContainer::exportResolutionIdentifier = "exportResolution";
const QString stringContainer::exportPathIdentifier = "exportPath";

//Identifier for ApplicationSettings
const QString stringContainer::applicationSettingsIdentifier = "ApplicationSettings";
const QString stringContainer::reconstructSoftwareIdentifier = "reconstructSoftware";
const QString stringContainer::standardInputPathIdentifier = "standardInputPath";
const QString stringContainer::disableChecksIdentifier = "disableChecks";
const QString stringContainer::darkStyleIdentifier = "darkStyle";
const QString stringContainer::useCudaIdentifier = "useCuda";
const QString stringContainer::createLogsIdentifier = "create logs";
const QString stringContainer::interpolateIdentifier = "interpolateMetaData";


// Identifier for LfogFile
const QString stringContainer::logNameIdentifier = "Name";
const QString stringContainer::logIsPluginIdentifier = "isPlugin";
const QString stringContainer::logCreationTimeIdentifier = "Creation time";
const QString stringContainer::logSettingsIdentifier = "Settings";
//      input
const QString stringContainer::logInputIdentifier = "Input";
//      procedure
const QString stringContainer::logTotalProcedureIdentifier = "All procedures";
const QString stringContainer::logProcedureIdentifier = "Procedure";
const QString stringContainer::logStartTimeIdentifier = "Start time";
const QString stringContainer::logStopTimeIdentifier = "Stop time";
const QString stringContainer::logElapsedTimeIdentifier = "Elapsed time";
//      results
const QString stringContainer::logResultsIdentifier = "Results";
const QString stringContainer::logKeyframeCountIdentifier = "Keyframe count";
const QString stringContainer::logKeyframesIdentifier = "Keyframes";
//      custom area
const QString stringContainer::logCustomIdentifier = "Custom";
const QString stringContainer::logTypeIdentifier = "Type";
//      GPS meta data
const QString stringContainer::latitudeIdentifier = "GPSLatitude";
const QString stringContainer::longitudeIdentifier = "GPSLongitude";
const QString stringContainer::altitudeIdentifier = "GPSAltitude";
const QString stringContainer::altitudeRefIdentifier = "GPSAltitudeRef";
const QString stringContainer::latitudeRefIdentifier = "GPSLatitudeRef";
const QString stringContainer::longitudeRefIdentifier = "GPSLongitudeRef";
const QString stringContainer::latitudeSouth = "S";
const QString stringContainer::latitudeNorth = "N";
const QString stringContainer::longitudeEast = "E";
const QString stringContainer::longitudeWest = "W";
const QString stringContainer::altitudeAboveSea = "0";
const QString stringContainer::altitudeBelowSea = "1";
