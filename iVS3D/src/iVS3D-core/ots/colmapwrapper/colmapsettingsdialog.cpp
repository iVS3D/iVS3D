#include "colmapsettingsdialog.h"

// Qt
#include <QFileDialog>

#include "ui_colmapsettingsdialog.h"

namespace lib3d {
namespace ots {
namespace ui {
namespace colmapwrapper {

//==================================================================================================
SettingsDialog::SettingsDialog(ColmapWrapper *ipWrapper, QWidget *parent)
    : QDialog(parent), ui(new Ui::SettingsDialog), mpColmapWrapper(ipWrapper)
{
    ui->setupUi(this);
    this->setWindowTitle(QObject::tr("COLMAP Wrapper Settings"));

    //--- connect ui elements to slots
    connect(ui->pb_localColmapBinary,
            &QPushButton::clicked,
            this,
            &SettingsDialog::onLocalColmapBinaryPushButtonPressed);
    connect(ui->pb_localOpenMVSBinaryFolder,
            &QPushButton::clicked,
            this,
            &SettingsDialog::onLocalOpenMVSBinaryPushButtonPressed);
    connect(ui->pb_localWorkspace,
            &QPushButton::clicked,
            this,
            &SettingsDialog::onLocalWorkspacePushButtonPressed);
    connect(ui->pb_mntPnt,
            &QPushButton::clicked,
            this,
            &SettingsDialog::onSelectMountPntPushButtonPressed);
    connect(ui->cb_connection,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            &SettingsDialog::onConnectionComboBoxIdxChanged);
    /*  connect(ui->pb_mount, &QPushButton::clicked,
          this, &SettingsDialog::onMountPushButtonPressed);
  connect(ui->pb_unmount, &QPushButton::clicked,
          this, &SettingsDialog::onUnmountPushButtonPressed);
  connect(ui->pb_installScript, &QPushButton::clicked,
           this, &SettingsDialog::onInstallScriptsPushButtonPressed);*/
    connect(ui->buttonBox->button(QDialogButtonBox::Apply),
            &QPushButton::pressed,
            this,
            &SettingsDialog::onAccepted);

    connect(ui->buttonBox, &QDialogButtonBox::close, this, &SettingsDialog::onCancel);
    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);

  connect(mpColmapWrapper, &ColmapWrapper::setupStatusUpdate, this, &SettingsDialog::onStatusChanged);
  connect(ui->le_localWorkspace, &QLineEdit::textChanged, this, &SettingsDialog::settingsChanged);
  connect(ui->le_localColmapBinary, &QLineEdit::textChanged, this, &SettingsDialog::settingsChanged);
  connect(ui->le_remoteWorkspace, &QLineEdit::textChanged, this, &SettingsDialog::settingsChanged);
  connect(ui->le_remoteAddr, &QLineEdit::textChanged, this, &SettingsDialog::settingsChanged);
  connect(ui->le_remoteUsr, &QLineEdit::textChanged, this, &SettingsDialog::settingsChanged);
  connect(ui->le_remoteColmapBinary, &QLineEdit::textChanged, this, &SettingsDialog::settingsChanged);
  connect(ui->le_mntPnt, &QLineEdit::textChanged, this, &SettingsDialog::settingsChanged);

  //--- hide remote settings (default)
  ui->f_remote->setVisible(false);
  //updateStatusMsg();
}

//==================================================================================================
SettingsDialog::~SettingsDialog()
{
    delete ui;
}

//==================================================================================================
void SettingsDialog::onLocalColmapBinaryPushButtonPressed()
{
    QString filePath = ui->le_localColmapBinary->text();

    filePath = QFileDialog::getOpenFileName(this,
                                            tr("Select COLMAP binary"),
                                            filePath.isEmpty()
                                                ? QApplication::applicationDirPath()
                                                : QFileInfo(filePath).absoluteDir().absolutePath());

    if (!filePath.isEmpty()) {
        ui->le_localColmapBinary->setText(filePath);
        settingsChanged();
    }
}

//==================================================================================================
void SettingsDialog::onLocalOpenMVSBinaryPushButtonPressed()
{
    QString filePath = ui->le_localOpenMVSBinaryFolder->text();

    filePath = QFileDialog::getOpenFileName(this,
                                            tr("Select OpenMVS binary folder"),
                                            filePath.isEmpty()
                                                ? QApplication::applicationDirPath()
                                                : QFileInfo(filePath).absoluteDir().absolutePath());

    if (!filePath.isEmpty()) {
        ui->le_localOpenMVSBinaryFolder->setText(filePath);
        settingsChanged();
    }
}

//==================================================================================================
void SettingsDialog::onLocalWorkspacePushButtonPressed()
{
    QString dirPath = ui->le_localWorkspace->text();

    dirPath = QFileDialog::getExistingDirectory(this,
                                                tr("Select workspace directory"),
                                                dirPath.isEmpty()
                                                    ? QApplication::applicationDirPath()
                                                    : dirPath);

    if (!dirPath.isEmpty()) {
        ui->le_localWorkspace->setText(dirPath);
        settingsChanged();
    }
}

//==================================================================================================
void SettingsDialog::onSelectMountPntPushButtonPressed()
{
    QString dirPath = ui->le_mntPnt->text();

    dirPath
        = QFileDialog::getExistingDirectory(this,
                                            tr("Select mountpoint of remote  workspace directory"),
                                            dirPath.isEmpty() ? QApplication::applicationDirPath()
                                                              : dirPath);

    if (!dirPath.isEmpty()) {
        ui->le_mntPnt->setText(dirPath);
        settingsChanged();
    }
}

//==================================================================================================
void SettingsDialog::onConnectionComboBoxIdxChanged(int currentIdx)
{
    switch (static_cast<ColmapWrapper::EConnectionType>(currentIdx)) {
    default:
    case ColmapWrapper::LOCAL: {
        ui->f_local->setVisible(true);
        ui->f_remote->setVisible(false);
    } break;

    case ColmapWrapper::SSH: {
        ui->f_local->setVisible(false);
        ui->f_remote->setVisible(true);
    } break;
    }
    settingsChanged();
}

//==================================================================================================
void SettingsDialog::onMountPushButtonPressed()
{
    mpColmapWrapper->setMntPntRemoteWorkspacePath(ui->le_mntPnt->text());
    mpColmapWrapper->mountRemoteWorkspace();
}

//==================================================================================================
void SettingsDialog::onUnmountPushButtonPressed()
{
    mpColmapWrapper->setMntPntRemoteWorkspacePath(ui->le_mntPnt->text());
    mpColmapWrapper->unmountRemoteWorkspace();
}

//==================================================================================================
void SettingsDialog::onInstallScriptsPushButtonPressed()
{
    if (static_cast<ColmapWrapper::EConnectionType>(ui->cb_connection->currentIndex())
        == ColmapWrapper::LOCAL) {
        mpColmapWrapper->setLocalWorkspacePath(ui->le_localWorkspace->text());
    } else {
        mpColmapWrapper->setMntPntRemoteWorkspacePath(ui->le_mntPnt->text());
        mpColmapWrapper->mountRemoteWorkspace();
    }

    mpColmapWrapper->installScriptFilesIntoWorkspace();
}

//==================================================================================================
void SettingsDialog::onAccepted()
{
    ui->l_error->setText("Testing connection... this may take up to 10 seconds");
    ui->l_error->setStyleSheet("QLabel { border : 1px solid orange; color : orange; }");
    this->repaint();

  ColmapWrapper::SSettings *settings = new ColmapWrapper::SSettings{
      ui->le_localColmapBinary->text(),
      ui->le_remoteColmapBinary->text(),
      ui->le_localWorkspace->text(),
      ui->le_remoteWorkspace->text(),
      ui->le_mntPnt->text(),
      static_cast<ColmapWrapper::EConnectionType>(ui->cb_connection->currentIndex()),
      ui->le_remoteAddr->text(),
      ui->le_remoteUsr->text(),
      ui->sb_syncInterval->value()
  };
  mSetupResults = ColmapWrapper::SSetupResults();
  // maybe do this in a thread
  if(!mpColmapWrapper->testSettings(settings, &mSetupResults)){
      // something went wrong!
      delete settings;
      return;
  }

  ui->l_error->setStyleSheet("QLabel { border : 1px solid green; color : green; }");
  ui->l_error->setText(tr("Setup successfull"));
  mpColmapWrapper->applySettings(settings);
  mpColmapWrapper->writeSettings();
  ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
  delete settings;
}

void SettingsDialog::onCancel()
{
    mpColmapWrapper->readSettings();
    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void SettingsDialog::settingsChanged()
{
    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->l_error->setStyleSheet("QLabel { border : none; }");
    ui->l_error->setText("");
    ui->le_localWorkspace->setStyleSheet("QLineEdit { color : black; }");
    ui->le_localColmapBinary->setStyleSheet("QLineEdit { color : black; }");
    ui->le_remoteColmapBinary->setStyleSheet("QLineEdit { color : black; }");
    ui->le_remoteWorkspace->setStyleSheet("QLineEdit { color : black; }");
    ui->le_mntPnt->setStyleSheet("QLineEdit { color : black; }");
}

void SettingsDialog::updateStatusMsg(const QPair<ColmapWrapper::ESetupTestResult,QString> &test, QLineEdit *input)
{
    /*if(mpColmapWrapper->getSetupStatus() != ColmapWrapper::SETUP_OK) {

        QString msg;
        switch (mpColmapWrapper->getSetupStatus()) {
        case ColmapWrapper::ERR_EXE:
            msg = tr("colmap executabel was not found or is not executable");
            break;
        case ColmapWrapper::ERR_SSH:
            msg = tr("ssh connection failed");
            break;
        case ColmapWrapper::ERR_PATH:
            msg = tr("path to workspace or local mount point does not exist");
            break;
        case ColmapWrapper::ERR_MOUNT:
            msg = tr("remote workspace was not mounted correctly");
            break;
        default:
            msg = tr("unknown reason!");
        }
        ui->l_error->setStyleSheet("QLabel { border : 1px solid red; color : red; }");
        ui->l_error->setText(tr("Setup failed: ") + msg);
    } else {
        ui->l_error->setStyleSheet("QLabel { border : 1px solid green; color : green; }");
        ui->l_error->setText(tr("Setup successfull"));
        mpColmapWrapper->writeSettings();
        ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }*/
    if(test.first == ColmapWrapper::TEST_SUCCESSFUL){
        if(input) input->setStyleSheet("QLineEdit { color: green; }");
        return;
    }
    if(test.first == ColmapWrapper::TEST_FAILED){
        if(input) input->setStyleSheet("QLineEdit { color: red; }");
        ui->l_error->setStyleSheet("QLabel { border : 1px solid red; color : red; }");
        ui->l_error->setText(tr("Setup failed: ") + test.second);
    }

}

//==================================================================================================
void SettingsDialog::onShow()
{
    ui->le_localColmapBinary->setText(mpColmapWrapper->localColmapBinPath());
    ui->le_localOpenMVSBinaryFolder->setText(mpColmapWrapper->localOpenMVSBinPath());
    ui->le_localWorkspace->setText(mpColmapWrapper->localWorkspacePath());
    ui->cb_connection->setCurrentIndex(static_cast<int>(mpColmapWrapper->connectionType()));
    ui->le_remoteAddr->setText(mpColmapWrapper->remoteAddr());
    ui->le_remoteUsr->setText(mpColmapWrapper->remoteUsr());
    ui->le_remoteColmapBinary->setText(mpColmapWrapper->remoteColmapBinPath());
    ui->le_remoteOpenMVSBinaryFolder->setText(mpColmapWrapper->remoteOpenMVSBinPath());
    ui->le_remoteWorkspace->setText(mpColmapWrapper->remoteWorkspacePath());
    ui->le_mntPnt->setText(mpColmapWrapper->mntPntRemoteWorkspacePath());
    ui->sb_syncInterval->setValue(mpColmapWrapper->syncInterval());

    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

  //updateStatusMsg();
}

void SettingsDialog::onStatusChanged()
{
    updateStatusMsg(mSetupResults.localWorkspacePath, ui->le_localWorkspace);
    updateStatusMsg(mSetupResults.localColmapBinPath, ui->le_localColmapBinary);
    updateStatusMsg(mSetupResults.mntPntRemoteWorkspacePath, ui->le_mntPnt);
    updateStatusMsg(mSetupResults.sshConnection);
    updateStatusMsg(mSetupResults.fileSystemMount);
    updateStatusMsg(mSetupResults.remoteColmapBinPath, ui->le_remoteColmapBinary);
    updateStatusMsg(mSetupResults.remoteWorkspacePath, ui->le_remoteWorkspace);
}

} // namespace colmapwrapper
} // namespace ui
} // namespace ots
} // namespace lib3d
