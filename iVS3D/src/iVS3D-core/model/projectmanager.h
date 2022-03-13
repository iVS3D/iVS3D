#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include "stringcontainer.h"
#include "modelalgorithm.h"
#include "modelinputpictures.h"


#include <QVariant>
#include <QJsonObject>
#include <QFile>
#include <QJsonDocument>
#include <QObject>


/**
 * @class ProjectManager
 *
 * @ingroup Model
 *
 * @brief The ProjectManager class saves and imports Projektfiles containing tha classes ModelAlgorithm, ModelInputPictures and ExportDataInformation
 *
 * @author Daniel Brommer
 *
 * @date 2021/02/03
 */

class ProjectManager
{
public:
    /**
     * @brief Default constructor
     *
     */
    ProjectManager();
    /**
     * @brief Saves ModelAlgorithm, ModelInputPictures and all ExportDataInformation using the already defined project path and project name
     *
     * @param mip A pointer to the ModelInputPictures to be saved
     * @param ma A pointer to the ModelAlgorithm to be saved
     *
     */
    bool saveProject(ModelInputPictures* mip, ModelAlgorithm* ma);
    /**
     * @brief Saves ModelAlgorithm, ModelInputPictures and all ExportDataInformation, when the project is saved for the first time
     *
     * @param mip A pointer to the ModelInputPictures to be saved
     * @param ma A pointer to the ModelAlgorithm to be saved
     * @param path Path to where to project will be saved
     * @param name Name of the project
     */
    bool saveProjectAs(ModelInputPictures* mip, ModelAlgorithm* ma, QString path, const QString &name);
    /**
     * @brief Loades ModelAlgorithm, ModelInputPictures and all ExportDataInformation from the given file
     *
     * @param mip A pointer to an empty ModelInputPictures
     * @param ma A pointer to an empty ModelAlgorithm
     * @param path Path to the project file
     */
    bool loadProject(ModelInputPictures* mip, ModelAlgorithm* ma, QString path);
    /**
     * @brief Returns if currently a project exists
     *
     * @return @a true if a project is loaded @a false otherwise
     */
    bool isProjectLoaded();
    /**
     * @brief Returns the project name
     * @return QString with the project name
     */
    QString getProjectName();
    /**
     * @brief Returns the project path
     * @return QString with the project path
     */
    QString getProjectPath();

    bool createProject(ModelInputPictures *mip, ModelAlgorithm *ma, const QString &path, const QString &name);
private:
    QString m_projectPath = "";
    QString m_projectName = "";
};

#endif // PROJECTMANAGER_H
