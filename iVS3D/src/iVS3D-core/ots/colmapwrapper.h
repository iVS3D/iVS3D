#ifndef LIB3D_OTS_COLMAPWRAPPER_H
#define LIB3D_OTS_COLMAPWRAPPER_H

// std
#include <memory>
#include <map>
#include <vector>
#include <functional>

// Qt
#include <QSettings>
#include <QObject>
#include <QDialog>
#include <QAction>
#include <QToolButton>
#include <QPushButton>
#include <QProcess>
#include <QString>
#include <QTimer>
#include <QTemporaryDir>
#include <QTemporaryFile>

// OpenCV
#include <opencv2/core.hpp>

// lib3d_ots
#include "global.h"

namespace lib3d {
namespace ots {

namespace ui {

  // forward decleration
  class ColmapWrapperControlsFactory;

  namespace colmapwrapper {
    class SettingsDialog;
    class ViewWidget;
    class NewProductDialog;
  }
}

/**
 * @brief Wrapperclass for integration of [COLMAP](colmap.github.io).
 * @author Ruf, Boitumelo <boitumelo.ruf@iosb.fraunhofer.de>
 */
class ColmapWrapper : public QObject
{
    Q_OBJECT

    //--- ENUM DECLERATION ---//

  public:

    /**
     * @brief Enum holding the type of connection to COLMAP.
     */
    enum EConnectionType {
      LOCAL = 0, /**< Local connection. I.e. COLMAP is on the same machine as the programming invoking this class. */
      SSH        /**< Remote connection via SSH. */
    };

    /**
     * @brief Enum holding job states.
     */
    enum EJobState {
      JOB_DONE = 0, /**< Job is done. */
      JOB_RUNNING,  /**< Job is running. */
      JOB_PENDING,   /**< Job is waiting to be processed. */
      JOB_FAILED   /**< Job failed to be processed. */

    };

    /**
     * @brief Enum holding worker states.
     */
    enum EWorkerState {
      WORKER_IDLE = 0,  /**< Worker is ideling. */
      WORKER_RUNNING,   /**< Worker is running. I.e. currently processing a job. */
      WORKER_FAILED     /**< Worker has terminated unexpected. */
    };

    /**
     * @brief Enum holding states of workspace.
     */
    enum EWorkspaceStatus {
      IN_SYNC = 0,    /**< Workspace is synchronized. */
      SYNCING         /**< Workspace is currently being synchronized. */
    };

    /**
     * @brief Enum holding product types that can be processed with Agisoft Metashape.
     */
    enum EProductType {
      CAMERA_POSES = 0, /**< Camera poses / Sparse point cloud */
      DENSE_CLOUD,      /**< Dense point cloud */
      MESHED_MODEL      /**< Meshed 3D model */
    };

    enum ESetupTestResult {
        TEST_PENDING = 0,
        TEST_SUCCESSFUL,
        TEST_FAILED
    };

    //--- STRUCT DECLERATION ---//

  public:

    /**
     * @brief Struct holding information of a single job.
     */
    struct SJob {

        /// Name of sequence to which the job / product is assiciated.
        std::string sequenceName;

        /// Product type that is computed by this job.
        EProductType product;

        /// Parameters for computation of the product.
        std::map<std::string, std::string> parameters;

        /// State in which the job is currently in.
        EJobState state = JOB_PENDING;

        /// Progress of this job (in %).
        uint progress = 0;

        /// current step of job.
        uint step = 0;

        /// ETA (expected time of arrival) for current step in ms
        uint eta = 0;
    };

    /**
     * @brief Struct holding information of current worker state.
     */
    struct SWorker {

        /// State of the worker.
        EWorkerState state = WORKER_IDLE;

        /// Pointer of the currently processed job within ColmapWrapper#mJobs.
        SJob *currentlyRunningJob = nullptr;
    };

    /**
     * @brief Struct holding information of a single product.
     */
    struct SProduct {

        /// Type of the product.
        EProductType type;

        /// True, if product is finished.
        bool isFinished;
    };

    /**
     * @brief Struct holding information of a single sequence.
     */
    struct SSequence
    {
        /// Sequence name.
        std::string name;

        /// Path to the images of the sequence.
        std::string imagePath;

        /// Products available (already processed) for the named sequence.
        std::vector<SProduct> products;
    };

    struct SSettings
    {
        /// Absolute path to COLMAP binary on local machine.
        QString localColmapBinPath;

        /// Absolute path to COLMAP binary on remote machine.
        QString remoteColmapBinPath;

        /// Absolute path to workspace on local machine.
        QString localWorkspacePath;

        /// Absolute path to workspace on remote machine
        QString remoteWorkspacePath;

        /// Absolute path to temporary mount location of remote directory.
        QString mntPntRemoteWorkspacePath;

        /// Connection to location of Metashape
        EConnectionType connectionType;

        /// Address of remote server. If empty it assumed that Metashape runs on local machine.
        /// Default = "".
        QString remoteAddr;

        /// User on remote server.
        QString remoteUsr;

        /// Interval of background synchronization between server and client
        int syncInterval;
    };

    struct SSetupResults
    {
        /// Absolute path to COLMAP binary on local machine.
        QPair<ESetupTestResult,QString> localColmapBinPath = {TEST_PENDING, ""};

        /// Absolute path to COLMAP binary on remote machine.
        QPair<ESetupTestResult,QString> remoteColmapBinPath = {TEST_PENDING, ""};

        /// Absolute path to workspace on local machine.
        QPair<ESetupTestResult,QString> localWorkspacePath = {TEST_PENDING, ""};

        /// Absolute path to workspace on remote machine
        QPair<ESetupTestResult,QString> remoteWorkspacePath = {TEST_PENDING, ""};

        /// Absolute path to temporary mount location of remote directory.
        QPair<ESetupTestResult,QString> mntPntRemoteWorkspacePath = {TEST_PENDING, ""};

        /// Connection to server
        QPair<ESetupTestResult,QString> sshConnection = {TEST_PENDING, ""};

        /// remote workspace mounted
        QPair<ESetupTestResult,QString> fileSystemMount = {TEST_PENDING, ""};
    };

    //--- METHOD DECLERATION ---//

  public:

    /**
     * @brief Constructor
     * @param[in] iSettingsFile Path to settings file of ColmapWrapper. Default: "lib3D_ColmapWrapper.ini"
     * @param[in] iSettingsOnly Allows to instantiate class only to edit setting. Wrapper can later
     * be initialize with init();
     */
    explicit ColmapWrapper(const QString iSettingsFile = "lib3D_ColmapWrapper.ini",
                           const bool iSettingsOnly = false);

    /**
     * @brief Destructor
     */
    virtual ~ColmapWrapper();

    /**
     * @brief Initialization routine.
     *
     * Performs the following steps:
     *  1. Initializes the synchronization timers.
     *  2. File system mount of the remote workspace via sshfs if applicable.
     *  3. Synchronizes the workspace and initally populates the list of available products.
     */
    void init();

    bool testSettings(const SSettings* settings, SSetupResults* results);

    void findColmap();

    QMultiMap<QString,QString> imageSequencePaths() const;

    /**
     * @brief Returns absolute path to COLMAP binary on local machine.
     */
    QString localColmapBinPath() const;


    /**
     * @brief Returns absolute path to COLMAP binary on remote machine.
     */
    QString remoteColmapBinPath() const;

    /**
     * @brief Returns absolute path to workspace on local machine.
     */
    QString localWorkspacePath() const;

    /**
     * @brief Returns absolute path to workspace on remote machine.
     */
    QString remoteWorkspacePath() const;

    /**
     * @brief Returns path to mount point of remote workspace on local machine.
     */
    QString mntPntRemoteWorkspacePath() const;

    /**
     * @brief Returns type of connection to COLMAP.
     */
    EConnectionType connectionType() const;

    /**
     * @brief Returns address of remote server.
     */
    QString remoteAddr() const;

    /**
     * @brief Returns user on remote server which is allowed to run COLMAP.
     */
    QString remoteUsr() const;

    /**
     * @brief Returns interval (in seconds) in which the client and server should be synchronized.
     */
    int syncInterval() const;

    bool getSetupSuccessful();

    /**
     * @brief Returns wether robustMode should be used.
     */
    bool useRobustMode() const;

    /**
     * @brief Returns true, if remote workspace is mounted. False, otherwise.
     */
    bool isRemoteWorkspaceMounted(QString iRemoteWorkspacePath) const;

    /**
     * @brief Method to get or create a pointer to a factory object that allows to create ui controls
     * for the ColmapWrapper.
     * @return Pointer to object.
     */
    ui::ColmapWrapperControlsFactory *getOrCreateUiControlsFactory();

    /**
     * @brief Set fuction implementing custom functionality to import image data into directory.
     *
     * The function needs to be of type bool(std::string, uint). The std::string will hold the path to
     * the directory into which the image data is to be imported. The uint will hold the subsampling
     * rate in form of every-nth-frame to import (Default = 1, i.e. every frame).
     */
    void setCustomImportFn(const std::function<bool (std::string, uint)> &customImportFn);

    /**
     * @brief Get fuction implementing custom functionality to import image data into directory.
     *
     * The function needs is be of type bool(std::string, uint). The std::string will hold the path to
     * the directory into which the image data is to be imported. The uint will hold the subsampling
     * rate in form of every-nth-frame to import (Default = 1, i.e. every frame).
     */
    std::function<bool (std::string, uint)> customImportFn() const;

    /**
     * @brief Clear fuction implementing custom functionality to import image data into directory.
     */
    void clearCustomImportFn();

    /**
     * @brief Set fuction implementing custom functionality to open product.
     *
     * The function needs to be of type void(EProductType, std::string).
     * The std::string will hold the path to the file holding the product of type EProductType.
     */
    void setCustomProductOpenFn(
        const std::function<void (ColmapWrapper::EProductType, std::string)> &customProductOpenFn);

    /**
     * @brief Get fuction implementing custom functionality to open product.
     *
     * The function needs to be of type void(EProductType, std::string).
     * The std::string will hold the path to the file holding the product of type EProductType.
     */
    std::function<void (ColmapWrapper::EProductType, std::string)> customProductOpenFn() const;

    /**
     * @brief Clear fuction implementing custom functionality to open product.
     */
    void clearCustomProductOpenFn();

    /**
     * @brief switchWorkspace tries to switch to the local/remote workspace.
     */
    void applySettings(const SSettings *settings);

  signals:

    /**
     * @brief Signal that the list of available sequences is updated and should be reloaded.
     */
    void sequenceListUpdate();

    /**
     * @brief Signal that the list of outstanding jobs is updated and should be reloaded.
     */
    void jobListUpdate();

    /**
     * @brief Signal that the state of the MetashapeWorker has updated.
     */
    void workerStateUpdate();

    /**
     * @brief Signal that the state of the workspace has updated.
     */
    void workspaceStatusUpdate();

    /**
     * @brief Signal that the setup has changed.
     */
    void setupStatusUpdate();

  public slots:
    void setLocalPresetSequence(QString name, QString path);
    /**
     * @brief Set absolute path to COLMAP binary on local machine.
     */
    void setLocalColmapBinPath(const QString &colmapBinPath);

    /**
     * @brief Set absolute path to COLMAP binary on remote machine.
     */
    void setRemoteColmapBinPath(const QString &remoteColmapBinPath);

    /**
     * @brief Set absolute path to workspace on local machine.
     */
    void setLocalWorkspacePath(const QString &localWorkspacePath);

    /**
     * @brief Set absolute path to workspace on remote machine.
     */
    void setRemoteWorkspacePath(const QString &remoteWorkspacePath);

    /**
     * @brief Set path to mount point of remote workspace.
     */
    void setMntPntRemoteWorkspacePath(const QString &mntPntRemoteWorkspacePath);

    /**
     * @brief Set connection type to Metashape
     */
    void setConnectionType(const EConnectionType &connection);

    /**
     * @brief Set address of remote server. If empty it assumed that Metashape runs
     * on local machine.
     */
    void setRemoteAddr(const QString &remoteAddr);

    /**
     * @brief Set user on remote server.
     */
    void setRemoteUsr(const QString &remoteUsr);

    /**
     * @brief Set interval (in seconds) in which the client and server should be synchronized.
     */
    void setSyncInterval(int intervalInSeconds);

    /**
     * @brief Set wether to use robustMode.
     */
    void setUseRobustMode(bool useRobustMode);

    /**
     * @brief Mount remote workspace into path specified with setMntPntRemoteWorkspacePath().
     */
    int mountRemoteWorkspace();

    /**
     * @brief Unmount remote workspace.
     */
    void unmountRemoteWorkspace();

    /**
     * @brief Synchronize workspace from server to local machine.
     */
    void syncWorkspaceFromServer();

    /**
     * @brief Method to check worker state.
     *
     * @return True if running Job has changed. False, otherwise.
     */
    bool checkWorkerState();

    /**
     * @brief Add new job to the end of the list.
     */
    void addJob(const SJob &iJob);

    /**
     * @brief Append list of Jobs to the end of the current list.
     */
    void addJobList( const std::vector<SJob> &iJobList);

    /**
     * @brief Move given Job one up in queue.
     *
     * @return True, if successful. False, if job is not found in list.
     */
    bool moveJobOneUp(const SJob &iJob);

    /**
     * @brief Move given Job one down in queue.
     *
     * @return True, if successful. False, if job is not found in list.
     */
    bool moveJobOneDown(const SJob &iJob);

    /**
     * @brief Delete given Job.
     *
     * @return True, if successful. False, if job is not found in list.
     */
    bool deleteJob(const SJob &iJob);

    /**
     * @brief Returns job at given index.
     */
    SJob* getJobPtrAtIdx(int iIdx);

    /**
     * @brief Returns Number of Jobs in Queue
     */
    int getNumJobs() const;

    /**
     * @brief Remove finished jobs from list.
     *
     * @return Number of removed jobs.
     */
    int removeFinishedJobs();

    /**
     * @brief Remove all available sequences from list.
     */
    void clearAvailableSequenceList();

    /**
     * @return List with available (i.e. already processed) sequences.
     */
    std::vector<SSequence> getFinishedSequenceList() const;

    SSequence getLocalPresetSequence() const;
    /**
     * @brief Get list with available (i.e. already processed) sequences.
     */
    std::vector<SJob> getJobList() const;

    /**
     * @brief Get state of worker.
     */
    EWorkerState getWorkerState() const;

    /**
     * @brief Get status of workspace.
     */
    EWorkspaceStatus getWorkspaceStatus() const;

    /**
     * @brief Get path of file for given product in sequence.
     * @param[in] iSeqName Name of the sequence which holds the product.
     * @param[in] iProdType Product type for which the file path is to be returned.
     */
    QString getProductFilePath(QString iSeqName, const EProductType& iProdType);

    /**
     * @brief Scan workspace for finished products.
     *
     * This will populate the member list holding available sequences (i.e. mSequences).
     */
    void importSeuences();

    /**
     * @brief Method to invoke processing of job queue by COLMAP.
     */
    void startProcessing();

    /**
     * @brief Method to open COLMAP Log File.
     */
    void openColmapLogFile();

    /**
     * @brief Method to install script files into workspace
     */
    void installScriptFilesIntoWorkspace();

    /**
     * @brief check if script files are already installed
     */
    bool hasScriptFilesInstalled();

    /**
     * @brief Read settings from settings-file specified in the ColmapWrapper().
     */
    void readSettings();

    /**
     * @brief Write settings to settings-file specified in the ColmapWrapper().
     */
    void writeSettings();

    /**
     * @brief Write work queue to file (colmap_work_queue.yaml) in workspace.
     */
    void writeWorkQueueToFile();

    /**
     * @brief Clear worker state file (colmap_worker_state.yaml) in workspace.
     */
    void clearWorkerStateFile();

    /**
     * @brief Read work queue from file (colmap_work_queue.yaml) in workspace.
     */
    void readWorkQueueFromFile();

    /**
     * @brief Read worker state from file (colmap_worker_state.yaml) in workspace.
     */
    void readWorkerStateFromFile();


  private:

    /**
     * @brief Export jobs within #mJobs into given [`cv::FileStorage`](https://docs.opencv.org/master/da/d56/classcv_1_1FileStorage.html) object.
     */
    void exportJobs(cv::FileStorage &ioFileStorage) const;

    /**
     * @brief Export given job into given [`cv::FileStorage`](https://docs.opencv.org/master/da/d56/classcv_1_1FileStorage.html) object.
     */
    void exportJob(cv::FileStorage &ioFileStorage, const SJob &iJob) const;

    /**
     * @brief Import job from given [`cv::FileNode`](https://docs.opencv.org/master/da/d56/classcv_1_1FileStorage.html) object.
     * @param[out] oJob Job that is imported from file.
     */
    void importJob(const cv::FileNode &iFileNode, SJob &oJob) const;

    /**
     * @brief Returns true, if sequence with given sequence name is available.
     * @param[in] iSeqName Name of sequence that is to be queried.
     */
    bool hasSequence(const std::string iSeqName) const;

    /**
     * @brief Returns true, if product with given of given type is available in sequence.
     * @param[in] iSeqName Name of sequence for which a product is to be queried.
     * @param[in] iPdctType Type of product that is to be queried.
     */
    bool hasProduct(const std::string iSeqName, const ColmapWrapper::EProductType iPdctType) const;

    /**
     * @brief Move job in queue.
     * @param[in] fromidx Index pointing to job that is to be moved.
     * @param[in] toIdx Index to which the job is to be moved.
     */
    void moveJobInQueue(const int fromidx, const int toIdx);

    /**
     * @return Returns index of given job in queue.
     *
     * Returns -1 if job is not in queue.
     */
    int getIndexOfJob(const SJob &iJob);

    //--- FUNCTION DECLERATION ---//

  public:

    /**
     * @brief Convert product type to string.
     */
    static QString EProductType2QString(const ColmapWrapper::EProductType iType);

    /**
     * @brief Convert string to product type.
     */
    static ColmapWrapper::EProductType QString2EProductType(const QString iStr);

    //--- MEMBER DECLERATION ---//

  private:


    /// Settings object.
    QSettings mSettings;

    /// Pointer to temporary directory.
    QTemporaryDir* mpTempDir;

    /// Process for Python worker.
    QProcess* mpPyWorkerProcess;

    /// Process for mounting operations.
    QProcess* mpMountProcess;

    /// Process for synchronization operations.
    QProcess* mpSyncProcess;

    /// Timer object to chech worker file server and client
    QTimer mCheckWorkerTimer;

    /// Member object of Python worker.
    SWorker mPyWorker;

    /// Arguments for Python worker.
    QStringList mPyWorkerArguments;

    /// Absolute path to COLMAP binary on local machine.
    QString mLocalColmapBinPath;

    /// Absolute path to COLMAP binary on remote machine.
    QString mRemoteColmapBinPath;

    /// Absolute path to workspace on local machine.
    QString mLocalWorkspacePath;

    /// Absolute path to workspace on remote machine
    QString mRemoteWorkspacePath;

    /// Absolute path to temporary mount location of remote directory.
    QString mMntPntRemoteWorkspacePath;

    /// Connection to location of Metashape
    EConnectionType mConnectionType;

    /// Address of remote server. If empty it assumed that Metashape runs on local machine.
    /// Default = "".
    QString mRemoteAddr;

    /// User on remote server.
    QString mRemoteUsr;

    /// Interval of background synchronization between server and client
    int mSyncInterval;

    /// Use robust mode for higher probability for success versus quality
    bool mUseRobustMode;

    /// Status of workspace
    EWorkspaceStatus mWorkspaceStatus;

    /// List of available sequences
    std::vector<SSequence> mAvailableSequences;

    ColmapWrapper::SSequence mLocalPresetSequence;

    /// List of job pointers
    std::vector<SJob> mJobs;

    /// Pointer to UiControlsFactory object
    ui::ColmapWrapperControlsFactory*    mpUiControls;

    /// Member for custom functionality to import image data
    std::function<bool(std::string, uint)> mCustomImportFn;

    /// Member for custom functionality to open product
    std::function<void(ColmapWrapper::EProductType, std::string)> mCustomProductOpenFn;

    /// current status of the setup, true if colmap can be used
    bool mSetupSuccessful = false;
};

namespace ui {

  /**
   * @brief Class providing ui elements for controlling ColmapWrapper
   * @author Ruf, Boitumelo <boitumelo.ruf@iosb.fraunhofer.de>
   */
  class ColmapWrapperControlsFactory : public QObject
  {
      Q_OBJECT

    public:
      friend class lib3d::ots::ColmapWrapper;

      /**
       * @brief Create a QPushButton that opens the "New Product" dialog.
       * @param[in] iTheme Theme for Icon to use.
       * @param[in] rhs Pointer to already instatiated QPushButton. If null, a new Object of a
       * QPushButton will be instatiated. Can be used to adjust the icon.
       * @return Pointer to instatiated object.
       */
      QPushButton* createNewProductPushButton(ETheme iTheme = LIGHT,
                                              QWidget* parent = nullptr, QPushButton* rhs = nullptr);

      /**
       * @brief Create a QToolButton that opens the "New Product" dialog.
       * @param[in] iTheme Theme for Icon to use.
       * @param[in] rhs Pointer to already instatiated QToolButton. If null, a new Object of a
       * QToolButton will be instatiated. Can be used to adjust the icon.
       * @return Pointer to instatiated object.
       */
      QToolButton* createNewProductToolButton(ETheme iTheme = LIGHT,
                                    QWidget* parent = nullptr, QToolButton* rhs = nullptr);

      /**
       * @brief Create a QPushButton that opens the settings dialog.
       * @param[in] iTheme Theme for Icon to use.
       * @param[in] rhs Pointer to already instatiated QPushButton. If null, a new Object of a
       * QPushButton will be instatiated. Can be used to adjust the icon.
       * @return Pointer to instatiated object.
       */
      QPushButton* createSettingsPushButton(ETheme iTheme = LIGHT,
                                    QWidget* parent = nullptr, QPushButton* rhs = nullptr);

      /**
       * @brief Create a QToolButton that opens the settings dialog.
       * @param[in] iTheme Theme for Icon to use.
       * @param[in] rhs Pointer to already instatiated QToolButton. If null, a new Object of a
       * QToolButton will be instatiated. Can be used to adjust the icon.
       * @return Pointer to instatiated object.
       */
      QToolButton* createSettingsToolButton(ETheme iTheme = LIGHT,
                                    QWidget* parent = nullptr, QToolButton* rhs = nullptr);

      /**
       * @brief Create a QAction that opens the settings dialog.
       * @param[in] iTheme Theme for Icon to use.
       * @param[in] rhs Pointer to already instatiated QToolButton. If null, a new Object of a
       * QToolButton will be instatiated. Can be used to adjust the icon.
       * @return Pointer to instatiated object.
       */
      QAction* createSettingsAction(ETheme iTheme = LIGHT,
                                    QWidget* parent = nullptr, QAction* rhs = nullptr);

      /**
       * @brief Create ViewWidget for interaction with MetashapeWrapper.
       * @param[in] parent Pointer to parent object.
       * @return Pointer to ViewWidget.
       */
      QWidget* createViewWidget(QWidget* parent = nullptr);

      /**
       * @brief Method to update icon themes.
       */
      void updateIconTheme(ETheme iTheme);

    signals:

      /**
       * @brief Signal to update ui to light theme.
       */
      void updateToLightTheme();

      /**
       * @brief Signal to update ui to dark theme.
       */
      void updateToDarkTheme();

      void enableNewProductButtons(bool enabled);

    private:

      /**
       * @brief Private cosntructor.
       * @param[in] ipColmapWrapper Pointer to object of MetashapeWrapper.
       */
      ColmapWrapperControlsFactory(lib3d::ots::ColmapWrapper* ipColmapWrapper);

    private slots:

      /**
       * @brief Slot to show "Settings" dialog.
       */
      void showSettingsDialog();

      /**
       * @brief Slot to show "New Product" dialog.
       */
      void showNewProductDialog();

      void onSetupChanged();

    private:
      /// Member pointer to MetashapeWrapper
      lib3d::ots::ColmapWrapper* mpMsWrapper = nullptr;

      //! MetashapeWrapperNewProductDialog Member
      QSharedPointer<colmapwrapper::NewProductDialog> mpMsWrapperNewProductDialog = nullptr;

      //! MetashapeWrapperSettingsDialog Member
      QSharedPointer<colmapwrapper::SettingsDialog> mpMsWrapperSettingsDialog = nullptr;
  };

} // namespace ui

} // namespace ots
} // namespace lib3d

#endif // LIB3D_OTS_COLMAPWRAPPER_H
