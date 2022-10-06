#include "colmapwrapper.h"

// Std
#include <iostream>
#include <fstream>
#include <algorithm>

// Qt
#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QFileDialog>
#include <QDebug>
#include <QTemporaryDir>
#include <QDirIterator>
#include <QTime>
#include <QDate>
#include <QFile>
#include <QDesktopServices>
#include <QStringList>

#include "colmapwrapper/colmapsettingsdialog.h"
#include "colmapwrapper/colmapviewwidget.h"
#include "colmapwrapper/colmapnewproductdialog.h"


static QString WORK_QUEUE_FILE_NAME = "colmap_work_queue.yaml";
static QString WORKER_STATE_FILE_NAME = "colmap_worker_state.yaml";
static QString IS_RUNNING_FILE_NAME = "_colmapRunning";

static QString SYNC_CMD = "rsync";
static QString SYNC_BASE_ARGS = "-avzuh";

static QString MNT_CMD = "sshfs";
static QString UMNT_CMD = "fusermount";

//--- List of strings representing product type name
static std::vector<QString> PRODUCT_TYPE_STR_LIST = {"CAMERA_POSES", "DENSE_CLOUD", "MESHED_MODEL"};

//--- Map between product type and filenames
static std::map<lib3d::ots::ColmapWrapper::EProductType, QString> PRODUCT_FILENAME_MAP = {
  {lib3d::ots::ColmapWrapper::CAMERA_POSES, "%1_images.bin"},
  {lib3d::ots::ColmapWrapper::DENSE_CLOUD, "%1_dense_cloud.ply"},
  {lib3d::ots::ColmapWrapper::MESHED_MODEL, "%1_meshed_model.ply"}
};

namespace lib3d {
namespace ots {

//==================================================================================================
ColmapWrapper::ColmapWrapper(const QString iSettingsFile, const bool iSettingsOnly)  :
  QObject(),
  mSettings(iSettingsFile, QSettings::IniFormat),
  mpTempDir(new QTemporaryDir()),
  mpPyWorkerProcess(new QProcess()),
  mpMountProcess(new QProcess()),
  mpSyncProcess(new QProcess()),
  mCheckWorkerTimer(),
  mLocalColmapBinPath(""),
  mRemoteColmapBinPath(""),
  mLocalWorkspacePath(""),
  mRemoteWorkspacePath(""),
  mMntPntRemoteWorkspacePath(""),
  mConnectionType(LOCAL),
  mRemoteAddr(""),
  mRemoteUsr(""),
  mSyncInterval(10),
  mWorkspaceStatus(IN_SYNC),
  mpUiControls(nullptr)
{
  this->readSettings();

  //--- if settings only == false init
  if(!iSettingsOnly)
    init();
}

//==================================================================================================
ColmapWrapper::~ColmapWrapper()
{
  // BUG: Causes crash in Abul4Configurator: ABUL-7398
  // this->writeSettings();
    if(mConnectionType != LOCAL && isRemoteWorkspaceMounted(this->mMntPntRemoteWorkspacePath)){
        unmountRemoteWorkspace();
    }
  delete mpTempDir;
}

//==================================================================================================
void ColmapWrapper::init()
{
  //--- initialize synchornization timer
  mCheckWorkerTimer.setInterval(mSyncInterval * 1000);
  connect(&mCheckWorkerTimer, &QTimer::timeout, this, &ColmapWrapper::checkWorkerState);

  //--- connect slots to end of processes
  connect(mpSyncProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
          this, &ColmapWrapper::importSeuences);

  testSetup();

  if(mConnectionType == LOCAL && !mLocalWorkspacePath.isEmpty() && !hasScriptFilesInstalled()){
      installScriptFilesIntoWorkspace();
  }

  //--- check worker state file
  //--- initial sync and import of sequence is done at the end of checkRunningJobs
  bool runningJobChanged = checkWorkerState();

  //--- if running job has not changed, i.e. currently no running job in state file, sync
  //--- workspace from server.
  if(!runningJobChanged && mConnectionType != LOCAL)
    syncWorkspaceFromServer();
  else if( mConnectionType == LOCAL)
    importSeuences();

  mCheckWorkerTimer.start();
}

void ColmapWrapper::testSetup()
{
    /// if CONNECTION_TYPE is LOCAL, then:
    /// -- check if workspace exists
    /// -- colmap exe exists and is executable
    if(mConnectionType == LOCAL){
        if(!QDir(mLocalWorkspacePath).exists()){
            mSetupStatus = ERR_PATH;
            emit setupStatusUpdate();
            return;
        }
        if(!QFile(mLocalColmapBinPath).exists() || !QFileInfo(mLocalColmapBinPath).isExecutable()){
            mSetupStatus = ERR_EXE;
            emit setupStatusUpdate();
            return;
        }
        mSetupStatus = SETUP_OK;
        emit setupStatusUpdate();
        return;
    }
    /// if CONNECTION_TYPE is SSH, then:
    /// -- check if local workspace exists
    /// -- check if mount point exists
    /// -- try to run ssh command
    /// -- try to write and read from mount point
    if(mConnectionType == SSH){
        if(!QDir(mLocalWorkspacePath).exists() || !QDir(mMntPntRemoteWorkspacePath).exists()){
            mSetupStatus = ERR_PATH;
            emit setupStatusUpdate();
            return;
        }
        QStringList args;
        args << mRemoteUsr + QString("@") + mRemoteAddr << "nvidia-smi";
        QProcess p;
        p.start("ssh", args);
        if(!p.waitForFinished(5000)){  // maximum 5 sec to respond
            // something went wrong!
            mSetupStatus = ERR_SSH;
            emit setupStatusUpdate();
            qDebug() << p.readAllStandardError();
            qDebug() << p.readAllStandardOutput();
            return;
        } else {
            QString err(p.readAllStandardError());
            if(!err.isEmpty()){
                mSetupStatus = ERR_SSH;
                emit setupStatusUpdate();
                qDebug() << "SSH error: " << p.readAllStandardError();
                return;
            }
        }
        //--- mount remote workspace
        if(!isRemoteWorkspaceMounted(mMntPntRemoteWorkspacePath))
        {
            if(mountRemoteWorkspace() != 0){
                mSetupStatus = ERR_MOUNT;
                emit setupStatusUpdate();
                return;
            }
        }
        // TODO: instead copy a file with scp to the remote folder directly and see
        // if it is available at the local mount point too!
        QFile testFile(mMntPntRemoteWorkspacePath + QDir::separator() + "testFile.txt");
        if(!testFile.open(QIODevice::WriteOnly)){
            mSetupStatus = ERR_MOUNT;
            emit setupStatusUpdate();
            return;
        }
        testFile.write(QByteArray("This is a test"));
        testFile.close();
        // eventually wait??
        if(!testFile.exists()){
            mSetupStatus = ERR_MOUNT;
            emit setupStatusUpdate();
            return;
        }

        mSetupStatus = SETUP_OK;
        emit setupStatusUpdate();
    }
}

//==================================================================================================
void ColmapWrapper::readSettings()
{
  qDebug() << "==================================================================================";
  qDebug() << "Time: " << QDateTime::currentDateTime().toString();
  qDebug() << __PRETTY_FUNCTION__;

  mSettings.beginGroup("ColmapWrapper");
  setLocalColmapBinPath(mSettings.value("LocalColmapBinPath").toString());
  setRemoteColmapBinPath(mSettings.value("RemoteColmapBinPath").toString());
  setLocalWorkspacePath(mSettings.value("LocalWorkspacePath").toString());
  setRemoteWorkspacePath(mSettings.value("RemoteWorkspacePath").toString());
  setMntPntRemoteWorkspacePath(mSettings.value("MntPntRemoteWorkspacePath").toString());
  setConnectionType(static_cast<EConnectionType>(mSettings.value("ConnectionType").toInt()));
  setRemoteAddr(mSettings.value("RemoteAddr").toString());
  setRemoteUsr(mSettings.value("RemoteUsr").toString());
  setSyncInterval(mSettings.value("SyncInterval").toInt());
  mSettings.endGroup();
}

//==================================================================================================
void ColmapWrapper::writeSettings()
{
  qDebug() << "==================================================================================";
  qDebug() << "Time: " << QDateTime::currentDateTime().toString();
  qDebug() << __PRETTY_FUNCTION__;

  mSettings.beginGroup("ColmapWrapper");
  mSettings.setValue("LocalColmapBinPath", mLocalColmapBinPath);
  mSettings.setValue("RemoteColmapBinPath", mRemoteColmapBinPath);
  mSettings.setValue("LocalWorkspacePath", mLocalWorkspacePath);
  mSettings.setValue("RemoteWorkspacePath", mRemoteWorkspacePath);
  mSettings.setValue("MntPntRemoteWorkspacePath", mMntPntRemoteWorkspacePath);
  mSettings.setValue("ConnectionType", static_cast<int>(mConnectionType));
  mSettings.setValue("RemoteAddr", mRemoteAddr);
  mSettings.setValue("RemoteUsr", mRemoteUsr);
  mSettings.setValue("SyncInterval", mSyncInterval);
  mSettings.endGroup();
}

//==================================================================================================
void ColmapWrapper::writeWorkQueueToFile()
{
  qDebug() << "==================================================================================";
  qDebug() << "Time: " << QDateTime::currentDateTime().toString();
  qDebug() << __PRETTY_FUNCTION__;

  //--- if remote connection use mounting point
  QDir outputDir;
  if(mConnectionType == LOCAL)
  {
    outputDir.setPath(mLocalWorkspacePath);
    outputDir.mkpath(mLocalWorkspacePath);
  }
  else
  {
    if(!isRemoteWorkspaceMounted(mMntPntRemoteWorkspacePath))
    {
      qDebug() << "Error: Remote Workspace is not mounted!";
      return;
    }

    outputDir.setPath(mMntPntRemoteWorkspacePath);
  }

  //--- initialize file storage and export jobs
  QString outputFile = outputDir.absolutePath() + QDir::separator() + WORK_QUEUE_FILE_NAME;
  cv::FileStorage fs(outputFile.toStdString(), cv::FileStorage::WRITE);
  fs << "workspace" << ((mConnectionType == LOCAL) ? mLocalWorkspacePath.toStdString() :
                                                    mRemoteWorkspacePath.toStdString());
  fs << "queue" << "[";
  exportJobs(fs);
  fs << "]";

  fs.release();

  qDebug() << "Info: " << mJobs.size() << " jobs written to work queue.";
}

//==================================================================================================
void ColmapWrapper::readWorkQueueFromFile()
{
  qDebug() << "==================================================================================";
  qDebug() << "Time: " << QDateTime::currentDateTime().toString();
  qDebug() << __PRETTY_FUNCTION__;

  //--- if remote connection use mounting point
  QDir inputDir;
  if(mConnectionType == LOCAL)
  {
    inputDir.setPath(mLocalWorkspacePath);
    if(!inputDir.exists())
    {
      qDebug() << "Error: Local Workspace does not exist!";
      return;
    }
  }
  else
  {
    if(!isRemoteWorkspaceMounted(mMntPntRemoteWorkspacePath))
    {
      qDebug() << "Error: Remote Workspace is not mounted!";
      return;
    }

    inputDir.setPath(mMntPntRemoteWorkspacePath);
  }

  //--- initialize file storage and import jobs
  QString inputFile = inputDir.absolutePath() + QDir::separator() + WORK_QUEUE_FILE_NAME;
  cv::FileStorage fs = cv::FileStorage(inputFile.toStdString(), cv::FileStorage::READ);
  cv::FileNode queueNode = fs["queue"];

  int elementsInQueue = queueNode.size();

  mJobs.clear();
  for(cv::FileNodeIterator itr = queueNode.begin(); itr != queueNode.end(); ++itr) {
      SJob newPendingJob;
      importJob(*itr, newPendingJob);
      mJobs.push_back(newPendingJob);
  }
  fs.release();

  qDebug() << "Info: " << elementsInQueue << " jobs read from file.";
}

//==================================================================================================
void ColmapWrapper::readWorkerStateFromFile()
{
  qDebug() << "==================================================================================";
  qDebug() << "Time: " << QDateTime::currentDateTime().toString();
  qDebug() << __PRETTY_FUNCTION__;

  //--- if remote connection use mounting point
  QDir inputDir;
  if(mConnectionType == LOCAL)
  {
    inputDir.setPath(mLocalWorkspacePath);
    if(!inputDir.exists())
    {
      qDebug() << "Error: Local Workspace does not exist!";
      return;
    }
  }
  else
  {
    if(!isRemoteWorkspaceMounted(mMntPntRemoteWorkspacePath))
    {
      qDebug() << "Error: Remote Workspace is not mounted!";
      return;
    }

    inputDir.setPath(mMntPntRemoteWorkspacePath);
  }

  //--- initialize file storage and import jobs
  QString inputFile = inputDir.absolutePath() + QDir::separator() + WORKER_STATE_FILE_NAME ;

  //--- if input file does not exist return
  if(!QFileInfo(inputFile).exists())
  {
    qDebug() << "Error: Worker state file does not exist!";
    return;
  }

  /*
   * This reading access is needed to prevent opencv from crashing.
   * Seems to be an error related to syncing lokal and remote workspace.
   */
  QFile f(inputFile);
  f.open(QIODevice::ReadOnly);
  f.readAll();
  f.close();

  //--- read information from file
  ColmapWrapper::SJob runningJob;
  cv::FileStorage fs = cv::FileStorage(inputFile.toStdString(), cv::FileStorage::READ);
  cv::FileNode jobNode = fs["runningJob"];

  int nRunningJob = jobNode.size();
  if(nRunningJob > 0)
  {
    importJob(jobNode[0], runningJob);
    mJobs.insert(mJobs.begin(),runningJob);
    mPyWorker.currentlyRunningJob = &(*mJobs.begin());
  }
  else
  {
    mPyWorker.currentlyRunningJob = nullptr;
  }

  fs.release();

  //--- compute worker state
  bool isRunningFileExists = QFileInfo(inputDir.absolutePath() + QDir::separator() + IS_RUNNING_FILE_NAME).exists();
  if(mPyWorker.currentlyRunningJob == nullptr && !isRunningFileExists)
    mPyWorker.state = WORKER_IDLE;
  else if(mPyWorker.currentlyRunningJob != nullptr && isRunningFileExists)
    mPyWorker.state = WORKER_RUNNING;
  else if(mPyWorker.currentlyRunningJob != nullptr && !isRunningFileExists)
    mPyWorker.state = WORKER_FAILED;

  qDebug() << "Info: " << nRunningJob << " job read from file.";
}

//==================================================================================================
void ColmapWrapper::exportJobs(cv::FileStorage &ioFileStorage) const
{
  for(std::vector<SJob>::const_iterator itr = mJobs.begin();
      itr != mJobs.end(); ++itr)
  {
    //--- do not export running job as this is stored in worker state
    if( itr->state != JOB_RUNNING)
      exportJob(ioFileStorage, (*itr));
  }
}

//==================================================================================================
void ColmapWrapper::exportJob(cv::FileStorage &ioFileStorage,
                                 const ColmapWrapper::SJob &iJob) const
{
  ioFileStorage << "{";
  ioFileStorage << "sequenceName" << iJob.sequenceName;
  ioFileStorage << "productType" << static_cast<int>(iJob.product);
  ioFileStorage << "jobState" << static_cast<int>(iJob.state);
  ioFileStorage << "progress" << static_cast<int>(iJob.progress);
  ioFileStorage << "parameters" << "{";
  for(std::map<std::string,std::string>::const_iterator itr = iJob.parameters.begin();
      itr != iJob.parameters.end(); ++itr)
  {
      ioFileStorage << std::string(itr->first) << std::string(itr->second);
  }
  ioFileStorage << "}";
  ioFileStorage << "}";
}

//==================================================================================================
void ColmapWrapper::importJob(const cv::FileNode &iFileNode,
                                 ColmapWrapper::SJob &oJob) const
{
  oJob.sequenceName = std::string(iFileNode["sequenceName"]);
  oJob.product = static_cast<EProductType>(static_cast<int>(iFileNode["productType"]));
  oJob.state = static_cast<EJobState>(static_cast<int>(iFileNode["jobState"]));
  oJob.progress = static_cast<uint>(static_cast<int>(iFileNode["progress"]));
  cv::FileNode params = iFileNode["parameters"];
  for(cv::FileNodeIterator itr = params.begin(); itr != params.end(); ++itr)
  {
      cv::FileNode item = *itr;
      oJob.parameters[item.name()] = std::string(item);
  }
}

//==================================================================================================
bool ColmapWrapper::hasSequence(const std::string iSeqName) const
{
  for(SSequence seq : mAvailableSequences) {
    if(seq.name == iSeqName) {
      return true;
    }
  }
  return false;
}

//==================================================================================================
bool ColmapWrapper::hasProduct(const std::string iSeqName,
                                  const ColmapWrapper::EProductType iPdctType) const
{
  for(SSequence seq : mAvailableSequences) {
    if(seq.name == iSeqName) {
      for(SProduct product : seq.products) {
        if(product.type == iPdctType) {
            return true;
        }
      }
    }
  }
  return false;
}

//==================================================================================================
void ColmapWrapper::moveJobInQueue(const int fromidx, const int toIdx)
{
  std::iter_swap(mJobs.begin() + fromidx, mJobs.begin() + toIdx);
}

//==================================================================================================
int ColmapWrapper::getIndexOfJob(const ColmapWrapper::SJob &iJob)
{
  int i = 0;
  for(SJob job : mJobs) {
      if(job.sequenceName == iJob.sequenceName && job.product == iJob.product)
      {
          return i;
      }
      i++;
  }
  return -1;
}

//==================================================================================================
QString ColmapWrapper::EProductType2QString(const ColmapWrapper::EProductType iType)
{
  return PRODUCT_TYPE_STR_LIST[static_cast<int>(iType)];
}

//==================================================================================================
ColmapWrapper::EProductType ColmapWrapper::QString2EProductType(const QString iStr)
{
  std::vector<QString>::iterator itr = std::find(PRODUCT_TYPE_STR_LIST.begin(),
                                                 PRODUCT_TYPE_STR_LIST.end(),
                                                 iStr);
  int idx = std::max(0, static_cast<int>(itr - PRODUCT_TYPE_STR_LIST.begin()));
  return static_cast<EProductType>(idx);
}

//==================================================================================================
std::function<void (ColmapWrapper::EProductType, std::string)> ColmapWrapper::customProductOpenFn() const
{
  return mCustomProductOpenFn;
}

//==================================================================================================
void ColmapWrapper::clearCustomProductOpenFn()
{
    mCustomProductOpenFn = std::function<void(EProductType, std::string)>();
}

void ColmapWrapper::switchWorkspace()
{
    testSetup();

    if(!hasScriptFilesInstalled()){
        installScriptFilesIntoWorkspace();
    }
    //--- check worker state file
    //--- initial sync and import of sequence is done at the end of checkRunningJobs
    bool runningJobChanged = checkWorkerState();

    //--- if running job has not changed, i.e. currently no running job in state file, sync
    //--- workspace from server.
    if(!runningJobChanged && mConnectionType != LOCAL)
      syncWorkspaceFromServer();
    else if( mConnectionType == LOCAL)
      importSeuences();
}

//==================================================================================================
void ColmapWrapper::setCustomProductOpenFn(const std::function<void (EProductType, std::string)> &customProductOpenFn)
{
  mCustomProductOpenFn = customProductOpenFn;
}

//==================================================================================================
void ColmapWrapper::setCustomImportFn(const std::function<bool(std::string, uint)> &customImportFn)
{
  mCustomImportFn = customImportFn;
}

//==================================================================================================
std::function<bool (std::string, uint)> ColmapWrapper::customImportFn() const
{
  return mCustomImportFn;
}

//==================================================================================================
void ColmapWrapper::clearCustomImportFn()
{
  mCustomImportFn = std::function<bool(std::string, uint)>();
}

//==================================================================================================
ColmapWrapper::EWorkspaceStatus ColmapWrapper::getWorkspaceStatus() const
{
    return mWorkspaceStatus;
}

ColmapWrapper::ESetupStatus ColmapWrapper::getSetupStatus() const
{
    return mSetupStatus;
}

//==================================================================================================
std::vector<ColmapWrapper::SSequence> ColmapWrapper::getFinishedSequenceList() const
{
    return mAvailableSequences;
}

ColmapWrapper::SSequence ColmapWrapper::getLocalPresetSequence() const
{
    return mLocalPresetSequence;
}

//==================================================================================================
std::vector<ColmapWrapper::SJob> ColmapWrapper::getJobList() const
{
  return mJobs;
}

//==================================================================================================
ColmapWrapper::EWorkerState ColmapWrapper::getWorkerState() const
{
  return mPyWorker.state;
}

//==================================================================================================
QString ColmapWrapper::mntPntRemoteWorkspacePath() const
{
  return mMntPntRemoteWorkspacePath;
}

//==================================================================================================
void ColmapWrapper::setMntPntRemoteWorkspacePath(const QString &mntPntRemoteWorkspacePath)
{
  mMntPntRemoteWorkspacePath = mntPntRemoteWorkspacePath;
}

//==================================================================================================
int ColmapWrapper::syncInterval() const
{
  return mSyncInterval;
}

//==================================================================================================
void ColmapWrapper::setSyncInterval(int syncInterval)
{
  if(syncInterval == 0)
    return;

  int oldSyncInterv = mSyncInterval;

  mSyncInterval = syncInterval;

  if(mSyncInterval != oldSyncInterv)
  {
    mCheckWorkerTimer.setInterval(mSyncInterval * 1000);
  }
}

//==================================================================================================
bool ColmapWrapper::isRemoteWorkspaceMounted(QString iRemoteWorkspacePathMntPath) const
{
  //--- workspace is mounted if mount point is not empty and worker state file exists
  return (!iRemoteWorkspacePathMntPath.isEmpty() &&
          QFileInfo(iRemoteWorkspacePathMntPath + QDir::separator() + WORKER_STATE_FILE_NAME).exists());
}

//==================================================================================================
int ColmapWrapper::mountRemoteWorkspace()
{
  qDebug() << "==================================================================================";
  qDebug() << "Time: " << QDateTime::currentDateTime().toString();
  qDebug() << __PRETTY_FUNCTION__;

  //--- if already mounted or path not set, return
  if(isRemoteWorkspaceMounted(mMntPntRemoteWorkspacePath) || mMntPntRemoteWorkspacePath.isEmpty())
  {
    qDebug() << "Info: Remote Workspace already mounted!";
    return 0;
  }

  //--- construct temporary mount directory
  QDir dir;
  dir.mkpath(mMntPntRemoteWorkspacePath);

  //--- construct arguments to synchronize from server to local workspace
  QStringList args;
  args << QString(mRemoteUsr + "@" + mRemoteAddr + ":" +
                                    mRemoteWorkspacePath)
       << mMntPntRemoteWorkspacePath;


  //--- write info to log file
  qDebug() << "Cmd: " << MNT_CMD << args;

  //--- call sync cmd
  /*mpMountProcess->start(MNT_CMD, args);
  mpMountProcess->waitForFinished();
  qDebug() << mpMountProcess->exitCode();
  QString stdOut = mpSyncProcess->readAllStandardOutput();
  QString stdErr = mpSyncProcess->readAllStandardError();
  if(!stdOut.isEmpty()) qDebug() << stdOut;
  if(!stdErr.isEmpty()) qDebug() << stdErr;*/
  int exitCode = QProcess::execute(MNT_CMD, args);
  qDebug() << "Mount exit code: " << exitCode;
  // if mounted successfully, install scripts if missing
  if(exitCode == 0 && !hasScriptFilesInstalled()){
      qDebug() << "Installing script files";
      installScriptFilesIntoWorkspace();
  }
  return exitCode;
}

//==================================================================================================
void ColmapWrapper::unmountRemoteWorkspace()
{
  qDebug() << "==================================================================================";
  qDebug() << "Time: " << QDateTime::currentDateTime().toString();
  qDebug() << __PRETTY_FUNCTION__;

  //--- if not mounted or path not set, return
  if(!isRemoteWorkspaceMounted(mMntPntRemoteWorkspacePath) || mMntPntRemoteWorkspacePath.isEmpty())
  {
    qDebug() << "Info: Remote Workspace is not mounted!";
    return;
  }

  //--- construct arguments to synchronize from server to local workspace
  QStringList args;
  args << "-u"
       << mMntPntRemoteWorkspacePath
       /*<< QString("&>> " + mSshLogFilePath)*/;

  //--- write info to log file
  qDebug() << "Cmd: " << UMNT_CMD << args;

  //--- call sync cmd
  mpMountProcess->start(UMNT_CMD, args);
  mpMountProcess->waitForFinished();

  QString stdOut = mpSyncProcess->readAllStandardOutput();
  QString stdErr = mpSyncProcess->readAllStandardError();
  if(!stdOut.isEmpty()) qDebug() << stdOut;
  if(!stdErr.isEmpty()) qDebug() << stdErr;
}

//==================================================================================================
void ColmapWrapper::syncWorkspaceFromServer()
{
  qDebug() << "==================================================================================";
  qDebug() << "Time: " << QDateTime::currentDateTime().toString();
  qDebug() << __PRETTY_FUNCTION__;

  //--- return if connection type is local
  if(mConnectionType == LOCAL)
  {
    qDebug() << "Warning: Connection type is LOCAL!";
    return;
  }

  //--- update workspace status
  mWorkspaceStatus = SYNCING;
  emit workspaceStatusUpdate();

  //--- construct arguments to synchronize from server to local workspace
  QStringList args;
  args << SYNC_BASE_ARGS
       << "--delete"
       << mMntPntRemoteWorkspacePath + QDir::separator()
       << mLocalWorkspacePath;

  //--- write info to log file
  qDebug() << "Cmd: " << SYNC_CMD << args;

  //--- call sync cmd
  mpSyncProcess->setParent(this);
  mpSyncProcess->start(SYNC_CMD, args);

  QString stdOut = mpSyncProcess->readAllStandardOutput();
  QString stdErr = mpSyncProcess->readAllStandardError();
  if(!stdOut.isEmpty()) qDebug() << stdOut;
  if(!stdErr.isEmpty()) qDebug() << stdErr;
}

//==================================================================================================
bool ColmapWrapper::checkWorkerState()
{
  qDebug() << "==================================================================================";
  qDebug() << "Time: " << QDateTime::currentDateTime().toString();
  qDebug() << __PRETTY_FUNCTION__;

  //--- store currently referenced job
  SJob previouslyRunningJob;
  bool isPreviouslyRunningJobNull = true;
  if( mPyWorker.currentlyRunningJob != nullptr)
  {
    previouslyRunningJob = *mPyWorker.currentlyRunningJob;
    isPreviouslyRunningJobNull = false;
  }

  //--- read wokr queue and worker state
  readWorkQueueFromFile();
  readWorkerStateFromFile();
  emit jobListUpdate();
  emit workerStateUpdate();

  bool runningJobChanged = false;

  //--- if current job has changed read work queue and sync directory
  if(mPyWorker.currentlyRunningJob == nullptr && !isPreviouslyRunningJobNull)
    runningJobChanged = true;
  else if(mPyWorker.currentlyRunningJob != nullptr &&
          (mPyWorker.currentlyRunningJob->sequenceName != previouslyRunningJob.sequenceName ||
            mPyWorker.currentlyRunningJob->product != previouslyRunningJob.product))
    runningJobChanged = true;

  if(runningJobChanged)
  {
    qDebug() << "Info: Running Job has changed.";

    if(mConnectionType == LOCAL)
      importSeuences();
    else
      syncWorkspaceFromServer();
  }
  else{
    qDebug() << "Info: Running Job unchanged.";
  }

  return runningJobChanged;
}

//==================================================================================================
ColmapWrapper::EConnectionType ColmapWrapper::connectionType() const
{
  return mConnectionType;
}

//==================================================================================================
void ColmapWrapper::setConnectionType(const ColmapWrapper::EConnectionType &connection)
{
  mConnectionType = connection;
}

//==================================================================================================
QString ColmapWrapper::localColmapBinPath() const
{
  return mLocalColmapBinPath;
}

//==================================================================================================
void ColmapWrapper::setLocalPresetSequence(QString name, QString path){
  mLocalPresetSequence.name = name.toStdString();
  mLocalPresetSequence.imagePath = path.toStdString();
}

//==================================================================================================
void ColmapWrapper::setLocalColmapBinPath(const QString &colmapBinPath)
{
  //--- check if path is valid
  /*QFileInfo fileInfo(colmapBinPath);

  if(!colmapBinPath.isEmpty() && !fileInfo.isExecutable())
  {
    std::cerr << "Specified COLMAP binary on local machine is no executable: "
              << fileInfo.absoluteFilePath().toStdString();
    return;
  }

  mLocalColmapBinPath = fileInfo.absoluteFilePath();*/
    mLocalColmapBinPath = colmapBinPath;
}

//==================================================================================================
QString ColmapWrapper::remoteColmapBinPath() const
{
  return mRemoteColmapBinPath;
}

//==================================================================================================
void ColmapWrapper::setRemoteColmapBinPath(const QString &remoteColmapBinPath)
{
  //--- check if path is valid
  QFileInfo fileInfo(remoteColmapBinPath);
  mRemoteColmapBinPath = fileInfo.absoluteFilePath();
}

//==================================================================================================
QString ColmapWrapper::localWorkspacePath() const
{
  return mLocalWorkspacePath;
}

//==================================================================================================
void ColmapWrapper::setLocalWorkspacePath(const QString &localWorkspacePath)
{
  //--- check if path is valid
/*  QFileInfo fileInfo(localWorkspacePath);

  if(!localWorkspacePath.isEmpty() && !fileInfo.isDir())
  {
    std::cerr << "Specified Workspace on local machine is no directory: "
              << fileInfo.absoluteFilePath().toStdString();
    return;
  }
  mLocalWorkspacePath = fileInfo.absoluteFilePath();*/
    mLocalWorkspacePath = localWorkspacePath;
}

//==================================================================================================
QString ColmapWrapper::remoteWorkspacePath() const
{
  return mRemoteWorkspacePath;
}

//==================================================================================================
void ColmapWrapper::setRemoteWorkspacePath(const QString &remoteWorkspacePath)
{
  //--- check if path is valid
  QFileInfo fileInfo(remoteWorkspacePath);
  mRemoteWorkspacePath = fileInfo.absoluteFilePath();
}

//==================================================================================================
QString ColmapWrapper::remoteAddr() const
{
  return mRemoteAddr;
}

//==================================================================================================
void ColmapWrapper::setRemoteAddr(const QString &remoteAddr)
{
  mRemoteAddr = remoteAddr;
}

//==================================================================================================
QString ColmapWrapper::remoteUsr() const
{
  return mRemoteUsr;
}

//==================================================================================================
void ColmapWrapper::setRemoteUsr(const QString &remoteUsr)
{
  mRemoteUsr = remoteUsr;
}

//==================================================================================================
void ColmapWrapper::addJob(const ColmapWrapper::SJob &iJob)
{
  qDebug() << "==================================================================================";
  qDebug() << "Time: " << QDateTime::currentDateTime().toString();
  qDebug() << __PRETTY_FUNCTION__;

  mJobs.push_back(iJob);
}

//==================================================================================================
void ColmapWrapper::addJobList(const std::vector<SJob> &iJobList)
{
  qDebug() << "==================================================================================";
  qDebug() << "Time: " << QDateTime::currentDateTime().toString();
  qDebug() << __PRETTY_FUNCTION__;

  mJobs.insert(mJobs.end(), iJobList.begin(), iJobList.end());

  emit jobListUpdate();
}

//==================================================================================================
bool ColmapWrapper::moveJobOneUp(const ColmapWrapper::SJob &iJob)
{
  qDebug() << "==================================================================================";
  qDebug() << "Time: " << QDateTime::currentDateTime().toString();
  qDebug() << __PRETTY_FUNCTION__;

  int jobIdx = getIndexOfJob(iJob);
  if(jobIdx == -1) {
      return false;
  }
  else if(jobIdx == 0) {
      return true;
  }
  // if job in front is running, then do nothing
  else if (jobIdx > 0 && mJobs[jobIdx-1].state == JOB_RUNNING)
  {
    return true;
  }
  // if job in front is of same sequence and a prerequisite, then do nothing
  else if (jobIdx > 0 && mJobs[jobIdx-1].sequenceName == iJob.sequenceName &&
           static_cast<int>(mJobs[jobIdx-1].product) < static_cast<int>(iJob.product))
  {
    return true;
  }

  moveJobInQueue(jobIdx, jobIdx-1);
  return true;
}

//==================================================================================================
bool ColmapWrapper::moveJobOneDown(const ColmapWrapper::SJob &iJob)
{
  qDebug() << "==================================================================================";
  qDebug() << "Time: " << QDateTime::currentDateTime().toString();
  qDebug() << __PRETTY_FUNCTION__;

  int jobIdx = getIndexOfJob(iJob);
  if(jobIdx == -1) {
      return false;
  }
  else if(jobIdx == 0) {
      return true;
  }
  // if job behind is of same sequence and dependent of this job, then do nothing
  else if (jobIdx >= 0 && mJobs[jobIdx+1].sequenceName == iJob.sequenceName &&
           static_cast<int>(mJobs[jobIdx+1].product) > static_cast<int>(iJob.product))
  {
     return true;
  }

  moveJobInQueue(jobIdx, jobIdx+1);
  return true;
}

//==================================================================================================
bool ColmapWrapper::deleteJob(const ColmapWrapper::SJob &iJob)
{
  qDebug() << "==================================================================================";
  qDebug() << "Time: " << QDateTime::currentDateTime().toString();
  qDebug() << __PRETTY_FUNCTION__;

  int jobIdx = getIndexOfJob(iJob);
  if(jobIdx == -1) {
      return false;
  }
  mJobs.erase(mJobs.begin() + jobIdx);

  //--- erase all jobs of the same sequence that depend on this job
  for(std::vector<ColmapWrapper::SJob>::iterator itr = mJobs.begin() + jobIdx;
      itr != mJobs.end();)
  {
    if(itr->sequenceName == iJob.sequenceName &&
       static_cast<int>(itr->product) > static_cast<int>(iJob.product))
    {
      mJobs.erase(itr);
    }
    else
    {
      ++itr;
    }
  }

  return true;
}

//==================================================================================================
ColmapWrapper::SJob* ColmapWrapper::getJobPtrAtIdx(int iIdx)
{
  return &mJobs.at(iIdx);
}

//==================================================================================================
int ColmapWrapper::getNumJobs() const
{
  return static_cast<int>(mJobs.size());
}

//==================================================================================================
int ColmapWrapper::removeFinishedJobs()
{
  qDebug() << "==================================================================================";
  qDebug() << "Time: " << QDateTime::currentDateTime().toString();
  qDebug() << __PRETTY_FUNCTION__;

  int i = 0;
  for(std::vector<SJob>::iterator itr = mJobs.begin(); itr != mJobs.end();) {
      if((*itr).state == JOB_DONE) {
          itr = mJobs.erase(itr);
          i++;
      } else {
          itr++;
      }
  }

  writeWorkQueueToFile();

  return i;
}

//==================================================================================================
void ColmapWrapper::clearAvailableSequenceList()
{
  mAvailableSequences.clear();
}

//==================================================================================================
QString ColmapWrapper::getProductFilePath(QString iSeqName,
                                             const ColmapWrapper::EProductType &iProdType)
{
  //--- strip ending
  if(iSeqName.endsWith(".psx")) {
      iSeqName = iSeqName.left(iSeqName.lastIndexOf("."));
  }

  //--- construct product filepath
  QString productFilePath = QString("%1/%2.output/%3").arg(mLocalWorkspacePath, iSeqName,
                                                PRODUCT_FILENAME_MAP[iProdType].arg(iSeqName));
  return productFilePath;
}

//==================================================================================================
void ColmapWrapper::importSeuences()
{
  qDebug() << "==================================================================================";
  qDebug() << "Time: " << QDateTime::currentDateTime().toString();
  qDebug() << __PRETTY_FUNCTION__;

  //--- clear member variable holding all sequences
  mAvailableSequences.clear();

  //--- get all colmap project files files from local workspace
  QDir workSpaceDirectory(mLocalWorkspacePath);
  QStringList msProjFiles = workSpaceDirectory.entryList(QStringList() << "*.db" << "*.DB", QDir::Files,
                                                QDir::Name);
  for (QString msProjFileName : msProjFiles)
  {
    SSequence seq;

    //--- assign file name to sequence
    QString projectFileName = msProjFileName.left(msProjFileName.lastIndexOf("."));
    seq.name = projectFileName.toStdString();

    //--- if directory with images does exist, then populate image path according to connection type.
    //--- Continue, otherwise
    if(QDir(mLocalWorkspacePath + QDir::separator() + projectFileName + ".images").exists())
    {
      QString adjustedWorkspacePath = (mConnectionType == LOCAL) ? mLocalWorkspacePath :
                                                                   mRemoteWorkspacePath;
      if(!adjustedWorkspacePath.endsWith(QDir::separator()))
        adjustedWorkspacePath += QDir::separator();

      seq.imagePath = QString(adjustedWorkspacePath + projectFileName + ".images").toStdString();
    }
    else
    {
      continue;
    }

    //--- loop over all product types and check if exists in corresponding subdirectory
    for(auto filenameItr = PRODUCT_FILENAME_MAP.begin();
        filenameItr != PRODUCT_FILENAME_MAP.end(); ++filenameItr)
    {
      QString productFilePath =
          getProductFilePath(QString::fromStdString(seq.name), filenameItr->first);

      //--- if product exists add corresponding type to sequence struct
      if(QFile(productFilePath).exists()) {
        SProduct product;
        product.type = filenameItr->first;
        product.isFinished = true;

        seq.products.push_back(product);
      }
    }

    //--- add sequence to member list
    mAvailableSequences.push_back(seq);
  }

  qDebug() << "Already finished sequences: " << mAvailableSequences.size();

  //--- update workspace status
  mWorkspaceStatus = IN_SYNC;
  emit workspaceStatusUpdate();
  emit sequenceListUpdate();
}

//==================================================================================================
void ColmapWrapper::startProcessing()
{
  qDebug() << "==================================================================================";
  qDebug() << "Time: " << QDateTime::currentDateTime().toString();
  qDebug() << __PRETTY_FUNCTION__;

  //--- if temporary directory not valid, create new one
  if(!mpTempDir->isValid())
  {
    if(mpTempDir != nullptr)
      delete mpTempDir;

    mpTempDir = new QTemporaryDir();
  }

  //--- depending on connection type copy bootstrap scrip
  if(mConnectionType == LOCAL)
  {
    QFile::copy(":/ots/colmapwrapper/localBootstrapper.sh",
                mpTempDir->path() + "/colmapbootstrapper.sh");
  }
  else
  {
    QFile::copy(":/ots/colmapwrapper/remoteBootstrapper.sh",
                mpTempDir->path() + "/colmapbootstrapper.sh");
  }
  QFile::setPermissions(mpTempDir->path() + "/colmapbootstrapper.sh",
                        QFileDevice::ReadUser|QFileDevice::WriteUser|QFileDevice::ExeUser);

  //--- adjust content of script
  QByteArray fileData;
  QFile file(mpTempDir->path() + "/colmapbootstrapper.sh");
  file.open(QIODevice::ReadWrite|QIODevice::Text); // open for read and write
  fileData = file.readAll(); // read all the data into the byte array

  QString text(fileData); // add to text string for easy string replace

  // replace text in string
  if(mConnectionType == LOCAL)
  {
    text.replace(QString("<+workspace+>"), mLocalWorkspacePath);
    text.replace(QString("<+bin+>"), mLocalColmapBinPath);
  }
  else
  {
    text.replace(QString("<+user+>"), mRemoteUsr);
    text.replace(QString("<+address+>"), mRemoteAddr);
    text.replace(QString("<+workspace+>"), mRemoteWorkspacePath);
    text.replace(QString("<+bin+>"), mRemoteColmapBinPath);
  }

  file.seek(0); // go to the beginning of the file
  file.write(text.toUtf8()); // write the new text back to the file
  file.close(); // close the file handle.

  //--- call sync cmd
//  mpPyWorkerProcess->setParent(this);
  mpPyWorkerProcess->startDetached(mpTempDir->path() + "/colmapbootstrapper.sh");
  qDebug() << mpPyWorkerProcess->error();
  qDebug() << mpPyWorkerProcess->errorString();

  QString stdOut = mpPyWorkerProcess->readAllStandardOutput();
  QString stdErr = mpPyWorkerProcess->readAllStandardError();
  if(!stdOut.isEmpty()) qDebug() << stdOut;
  if(!stdErr.isEmpty()) qDebug() << stdErr;

  mPyWorker.state = WORKER_RUNNING;
  emit workerStateUpdate();

  checkWorkerState();
}

//==================================================================================================
void ColmapWrapper::openColmapLogFile()
{
  QString logFilePath = QString("%1/ColmapWorker.log");

  if(mConnectionType == LOCAL)
  {
    logFilePath = logFilePath.arg(mLocalWorkspacePath);
  }
  else
  {
    logFilePath = logFilePath.arg(mMntPntRemoteWorkspacePath);
  }

  QDesktopServices::openUrl(QUrl(logFilePath));
}

//==================================================================================================
void ColmapWrapper::installScriptFilesIntoWorkspace()
{
  qDebug() << "==================================================================================";
  qDebug() << "Time: " << QDateTime::currentDateTime().toString();
  qDebug() << __PRETTY_FUNCTION__;

  //--- defince ressource list that needs to be installed
  QStringList ressourceList;
  ressourceList << ":/ots/colmapwrapper/ColmapWorker"
           << ":/ots/colmapwrapper/colmap_work_queue.yaml"
           << ":/ots/colmapwrapper/colmap_worker_state.yaml";

  QString outputPath = (mConnectionType == LOCAL) ?
        mLocalWorkspacePath : mMntPntRemoteWorkspacePath;
  for(QString res : ressourceList)
  {
    //--- compute output file path
    QString outputFile = outputPath + QDir::separator() + QFileInfo(res).baseName();
    outputFile += (QFileInfo(res).completeSuffix() != "") ? "." + QFileInfo(res).completeSuffix() : "";

    //--- copy file and set permissions
    if(QFile::exists(outputFile))
      QFile::remove(outputFile);
    QFile::copy(res, outputFile);
    QFile::setPermissions(outputFile,
                          QFileDevice::ReadUser|QFileDevice::WriteUser|QFileDevice::ExeUser);

    //--- adjust content of script
    QByteArray fileData;
    QFile file(outputFile);
    file.open(QIODevice::ReadWrite|QIODevice::Text); // open for read and write
    fileData = file.readAll(); // read all the data into the byte array

    QString text(fileData); // add to text string for easy string replace

    // replace text in string
    text.replace(QString("<+workspace+>"), (mConnectionType == LOCAL) ?
                   QUrl(mLocalWorkspacePath).toString(QUrl::StripTrailingSlash) :
                   QUrl(mRemoteWorkspacePath).toString(QUrl::StripTrailingSlash));

    file.seek(0); // go to the beginning of the file
    file.write(text.toUtf8()); // write the new text back to the file
    file.close(); // close the file handle.
  }
}

bool ColmapWrapper::hasScriptFilesInstalled()
{
    //--- defince ressource list that needs to be installed
      QStringList ressourceList;
      ressourceList << ":/ots/colmapwrapper/ColmapWorker"
               << ":/ots/colmapwrapper/colmap_work_queue.yaml"
               << ":/ots/colmapwrapper/colmap_worker_state.yaml";

      QString outputPath = (mConnectionType == LOCAL) ?
            mLocalWorkspacePath : mMntPntRemoteWorkspacePath;
      for(QString res : ressourceList)
      {
          //--- compute output file path
          QString outputFile = outputPath + QDir::separator() + QFileInfo(res).baseName();
          outputFile += (QFileInfo(res).completeSuffix() != "") ? "." + QFileInfo(res).completeSuffix() : "";

          //--- copy file and set permissions
          if(!QFile::exists(outputFile))
            return false;
      }
      return true;
}

//==================================================================================================
ui::ColmapWrapperControlsFactory *ColmapWrapper::getOrCreateUiControlsFactory()
{
  if(!mpUiControls)
    mpUiControls = new ui::ColmapWrapperControlsFactory(this);

  return mpUiControls;
}


//==================================================================================================
QPushButton *ui::ColmapWrapperControlsFactory::createNewProductPushButton(ui::ETheme iTheme,
                                                                             QWidget *parent,
                                                                             QPushButton *rhs)
{
  QPushButton* pPushButton = (!rhs) ? new QPushButton(parent) : rhs;
  pPushButton->setToolTip(QObject::tr("Create new product to be processed by COLMAP"));
//  pPushButton->setFixedSize( 28, 28 );
  pPushButton->setIconSize( QSize( 16, 16 ) );
  pPushButton->setText(tr(" New Product"));
  if(iTheme == DARK)
  {
    pPushButton->setIcon(QIcon(":/assets/icons/glyphicons-37-file-dark.png"));
  }
  else
  {
    pPushButton->setIcon(QIcon(":/assets/icons/glyphicons-37-file.png"));
  }

  //--- initialize dialog
  if(mpMsWrapperNewProductDialog == nullptr)
  {
    mpMsWrapperNewProductDialog = QSharedPointer<colmapwrapper::NewProductDialog>(
          new colmapwrapper::NewProductDialog(mpMsWrapper, parent));
    mpMsWrapperNewProductDialog->setModal(false);
  }

  connect(pPushButton, &QPushButton::clicked,
          this, &ColmapWrapperControlsFactory::showNewProductDialog);

  connect(this, &ColmapWrapperControlsFactory::enableNewProductButtons,
          pPushButton, &QPushButton::setEnabled);

  pPushButton->setEnabled(mpMsWrapper->getSetupStatus() == ColmapWrapper::SETUP_OK);

  return pPushButton;
}

//==================================================================================================
QToolButton *ui::ColmapWrapperControlsFactory::createNewProductToolButton(ui::ETheme iTheme,
                                                                             QWidget *parent,
                                                                             QToolButton *rhs)
{
  QToolButton* pToolButton = (!rhs) ? new QToolButton(parent) : rhs;
  pToolButton->setToolTip(QObject::tr("Create new product to be processed by COLMAP"));
  pToolButton->setFixedSize( 16, 16 );
  pToolButton->setIconSize( QSize( 16, 16 ) );
  if(iTheme == DARK)
  {
    pToolButton->setIcon(QIcon(":/assets/icons/glyphicons-37-file-dark.png"));
  }
  else
  {
    pToolButton->setIcon(QIcon(":/assets/icons/glyphicons-37-file.png"));
  }

  //--- initialize dialog
  if(mpMsWrapperNewProductDialog == nullptr)
  {
    mpMsWrapperNewProductDialog = QSharedPointer<colmapwrapper::NewProductDialog>(
          new colmapwrapper::NewProductDialog(mpMsWrapper, parent));
    mpMsWrapperNewProductDialog->setModal(false);
  }

  connect(pToolButton, &QToolButton::clicked,
          this, &ColmapWrapperControlsFactory::showNewProductDialog);

  return pToolButton;
}

//==================================================================================================
QPushButton *ui::ColmapWrapperControlsFactory::createSettingsPushButton(ETheme iTheme,
                                                                           QWidget *parent,
                                                                           QPushButton *rhs)
{
  QPushButton* pPushButton = (!rhs) ? new QPushButton(parent) : rhs;
  pPushButton->setToolTip(QObject::tr("Open COLMAP Wrapper Settings"));
//  pPushButton->setFixedSize( 28, 28 );
  pPushButton->setIconSize( QSize( 27, 27 ) );
  pPushButton->setText(tr("Settings"));
  if(iTheme == DARK)
  {
    pPushButton->setIcon(QIcon(":/assets/icons/glyphicons-281-settings-dark.png"));
  }
  else
  {
    pPushButton->setIcon(QIcon(":/assets/icons/glyphicons-281-settings.png"));
  }

  //--- initialize dialog
  if(mpMsWrapperSettingsDialog == nullptr)
    mpMsWrapperSettingsDialog = QSharedPointer<colmapwrapper::SettingsDialog>(
          new colmapwrapper::SettingsDialog(mpMsWrapper, parent));

  connect(pPushButton, &QPushButton::clicked,
          this, &ColmapWrapperControlsFactory::showSettingsDialog);

  return pPushButton;
}

//==================================================================================================
QToolButton *ui::ColmapWrapperControlsFactory::createSettingsToolButton(ETheme iTheme,
                                                                   QWidget *parent,
                                                                   QToolButton *rhs)
{
  QToolButton* pToolButton = (!rhs) ? new QToolButton(parent) : rhs;
  pToolButton->setToolTip(QObject::tr("Open COLMAP Wrapper Settings"));
  pToolButton->setFixedSize( 16, 16 );
  pToolButton->setIconSize( QSize( 16, 16 ) );
  if(iTheme == DARK)
  {
    pToolButton->setIcon(QIcon(":/assets/icons/glyphicons-281-settings-dark.png"));
  }
  else
  {
    pToolButton->setIcon(QIcon(":/assets/icons/glyphicons-281-settings.png"));
  }

  //--- initialize dialog
  if(mpMsWrapperSettingsDialog == nullptr)
    mpMsWrapperSettingsDialog = QSharedPointer<colmapwrapper::SettingsDialog>(
          new colmapwrapper::SettingsDialog(mpMsWrapper, parent));

  connect(pToolButton, &QToolButton::clicked,
          this, &ColmapWrapperControlsFactory::showSettingsDialog);

  return pToolButton;
}

//=================================================================================================
QAction *ui::ColmapWrapperControlsFactory::createSettingsAction(ui::ETheme iTheme, QWidget *parent, QAction *rhs)
{
    QAction* pAction = (!rhs) ? new QAction(parent) : rhs;
    pAction->setToolTip(QObject::tr("Open COLMAP Wrapper Settings"));
    pAction->setCheckable(false);
    pAction->setText("Colmap Wrapper Settings");

    if(iTheme == DARK)
    {
      pAction->setIcon(QIcon(":/assets/icons/glyphicons-281-settings-dark.png"));
    }
    else
    {
      pAction->setIcon(QIcon(":/assets/icons/glyphicons-281-settings.png"));
    }

    //--- initialize dialog
    if(mpMsWrapperSettingsDialog == nullptr)
      mpMsWrapperSettingsDialog = QSharedPointer<colmapwrapper::SettingsDialog>(
            new colmapwrapper::SettingsDialog(mpMsWrapper, parent));

    connect(pAction, &QAction::triggered,
            this, &ColmapWrapperControlsFactory::showSettingsDialog);

    return pAction;
}


//==================================================================================================
QWidget *ui::ColmapWrapperControlsFactory::createViewWidget(QWidget *parent)
{
  ui::colmapwrapper::ViewWidget* pViewWidget =
      new ui::colmapwrapper::ViewWidget(mpMsWrapper,parent);

  return static_cast<QWidget*>(pViewWidget);
}

//==================================================================================================
void ui::ColmapWrapperControlsFactory::updateIconTheme(ui::ETheme iTheme)
{
  if(iTheme == DARK)
    emit updateToDarkTheme();
  else
    emit updateToLightTheme();
}

//==================================================================================================
ui::ColmapWrapperControlsFactory::ColmapWrapperControlsFactory(ColmapWrapper *ipColmapWrapper) :
  QObject(),
  mpMsWrapper(ipColmapWrapper),
  mpMsWrapperSettingsDialog(nullptr)
{
    connect(mpMsWrapper, &ColmapWrapper::setupStatusUpdate, this, &ColmapWrapperControlsFactory::onSetupChanged);
}

//==================================================================================================
void ui::ColmapWrapperControlsFactory::showSettingsDialog()
{
  if(mpMsWrapperSettingsDialog == nullptr)
    return;

  mpMsWrapperSettingsDialog->show();
  mpMsWrapperSettingsDialog->onShow();
}

//==================================================================================================
void ui::ColmapWrapperControlsFactory::showNewProductDialog()
{
  if(mpMsWrapperNewProductDialog == nullptr)
    return;

  mpMsWrapperNewProductDialog->onShow();
  mpMsWrapperNewProductDialog->exec();

  std::vector<ColmapWrapper::SJob> newJobList = mpMsWrapperNewProductDialog->getNewJobList();
  if(!newJobList.empty())
  {
    mpMsWrapper->addJobList(newJobList);

    mpMsWrapper->writeWorkQueueToFile();
    mpMsWrapper->startProcessing();
  }
}

void ui::ColmapWrapperControlsFactory::onSetupChanged()
{
    emit enableNewProductButtons(mpMsWrapper->getSetupStatus() == ColmapWrapper::SETUP_OK);
}

} // namespace ots
} // namespace lib3d
