#ifndef DATAMANAGER_H
#define DATAMANAGER_H

/** @defgroup Model Model
 *
 * @ingroup Model
 *
 * @brief Manages image and project data and holds logic for exporting.
 */

#include "modelalgorithm.h"
#include "modelinputpictures.h"
#include "projectmanager.h"
#include "logmanager.h"
#include "history.h"

/**
 * @class DataManager
 *
 * @ingroup Model
 *
 * @brief The DataManager class is a Facade for the data holding classes in the model. It delegates most of it's tasks to model classes
 *
 * @author Patrick Binder
 *
 * @date 2021/02/01
 */
class DataManager
{
public:
    /**
     * @brief DataManager Constructor it will create a new ProjektManager
     */
    explicit DataManager();

    ~DataManager();
    /**
     * @brief open Creates a new ModelInputPictures with the given path
     *
     * @param path Import path
     */
    int open(QString path);
    /**
     * @brief openProject Opens the project by creating an empty ModelInputPictures and ModelAlgorithm and calling the ProjectManager
     *
     * @param path Path of the project
     */
    int openProject(QString path);
    /**
     * @brief saveProject Saves the current project with the ProjectManager
     */
    void saveProject();
    /**
     * @brief saveProject Saves a new project with the ProjectManager
     *
     * @param projectName Name of the project
     * @param projectPath Path of the project
     */
    void saveProjectAs(QString projectName, QString projectPath);
    /**
     * @brief createProject Creates a new project with the ProjectManager
     *
     * @param projectName Name of the project
     * @param projectPath Path of the project
     */
    void createProject(QString projectName, QString projectPath);
    /**
     * @brief isProjectLoaded Returns if a project is loaded
     *
     * @return @a true if a project is loaded @a false otherwise
     */
    bool isProjectLoaded();
    /**
     * @brief getProjectName Returns the current project name from the ProjectManager
     * @return current project name
     */
    QString getProjectName();
    /**
     * @brief getProjectPath Returns the current project path from the ProjectManager
     * @return current project path
     */
    QString getProjectPath();
    /**
     * @brief getModelAlgorithm Returns the current ModelAlgorithm
     * @return The current ModelAlgorithm
     */
    ModelAlgorithm* getModelAlgorithm();
    /**
     * @brief getModelInputPictures Returns the current ModelInputPictures
     * @return The current ModelInputPictures
     */
    ModelInputPictures* getModelInputPictures();
    History* getHistory();

private:
    ModelAlgorithm* m_ma = nullptr;
    ModelInputPictures* m_mip = nullptr;
    ProjectManager* m_projectManager;
    History* m_history;
};



#endif // DATAMANAGER_H
