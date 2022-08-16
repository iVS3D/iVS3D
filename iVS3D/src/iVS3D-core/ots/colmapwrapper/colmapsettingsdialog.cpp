#include "colmapsettingsdialog.h"

// Qt
#include <QFileDialog>

#include "ui_colmapsettingsdialog.h"

namespace lib3d {
namespace ots {
namespace ui {
namespace colmapwrapper {

//==================================================================================================
SettingsDialog::SettingsDialog(ColmapWrapper *ipWrapper,
                               QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SettingsDialog),
  mpColmapWrapper(ipWrapper)
{
  ui->setupUi(this);
  this->setWindowTitle(QObject::tr("COLMAP Wrapper Settings"));

  //--- connect ui elements to slots
  connect(ui->pb_localColmapBinary, &QPushButton::clicked,
          this, &SettingsDialog::onLocalBinaryPushButtonPressed);
  connect(ui->pb_localWorkspace, &QPushButton::clicked,
          this, &SettingsDialog::onLocalWorkspacePushButtonPressed);
  connect(ui->pb_mntPnt, &QPushButton::clicked,
          this, &SettingsDialog::onSelectMountPntPushButtonPressed);
  connect(ui->cb_connection, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this, &SettingsDialog::onConnectionComboBoxIdxChanged);
  connect(ui->pb_mount, &QPushButton::clicked,
          this, &SettingsDialog::onMountPushButtonPressed);
  connect(ui->pb_unmount, &QPushButton::clicked,
          this, &SettingsDialog::onUnmountPushButtonPressed);
  connect(ui->pb_installScript, &QPushButton::clicked,
           this, &SettingsDialog::onInstallScriptsPushButtonPressed);
  connect(ui->buttonBox, &QDialogButtonBox::accepted,
          this, &SettingsDialog::onAccepted);

  //--- hide remote settings (default)
  ui->f_remote->setVisible(false);
}

//==================================================================================================
SettingsDialog::~SettingsDialog()
{
  delete ui;
}

//==================================================================================================
void SettingsDialog::onLocalBinaryPushButtonPressed()
{
  QString filePath = ui->le_localColmapBinary->text();

  filePath = QFileDialog::getOpenFileName(this, tr("Select COLMAP binary"),
                                           filePath.isEmpty() ?
                                           QApplication::applicationDirPath() :
                                           QFileInfo(filePath).absoluteDir().absolutePath());

  if(!filePath.isEmpty())
    ui->le_localColmapBinary->setText(filePath);
}

//==================================================================================================
void SettingsDialog::onLocalWorkspacePushButtonPressed()
{
  QString dirPath = ui->le_localWorkspace->text();

  dirPath = QFileDialog::getExistingDirectory(this, tr("Select workspace directory"),
                                         dirPath.isEmpty() ?
                                           QApplication::applicationDirPath() :
                                           dirPath);

  if(!dirPath.isEmpty())
    ui->le_localWorkspace->setText(dirPath);

}

//==================================================================================================
void SettingsDialog::onSelectMountPntPushButtonPressed()
{
  QString dirPath = ui->le_mntPnt->text();

  dirPath = QFileDialog::getExistingDirectory(this, tr("Select mountpoint of remote  workspace directory"),
                                         dirPath.isEmpty() ?
                                           QApplication::applicationDirPath() :
                                           dirPath);

  if(!dirPath.isEmpty())
    ui->le_mntPnt->setText(dirPath);
}

//==================================================================================================
void SettingsDialog::onConnectionComboBoxIdxChanged(int currentIdx)
{
  switch(static_cast<ColmapWrapper::EConnectionType>(currentIdx))
  {
    default:
    case ColmapWrapper::LOCAL:
    {
      ui->f_local->setVisible(true);
      ui->f_remote->setVisible(false);
    }
    break;

    case ColmapWrapper::SSH:
    {
      ui->f_local->setVisible(false);
      ui->f_remote->setVisible(true);
    }
    break;
  }
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
  if(static_cast<ColmapWrapper::EConnectionType>(
       ui->cb_connection->currentIndex()) == ColmapWrapper::LOCAL)
  {
    mpColmapWrapper->setLocalWorkspacePath(ui->le_localWorkspace->text());
  }
  else
  {
    mpColmapWrapper->setMntPntRemoteWorkspacePath(ui->le_mntPnt->text());
    mpColmapWrapper->mountRemoteWorkspace();
  }

  mpColmapWrapper->installScriptFilesIntoWorkspace();
}

//==================================================================================================
void SettingsDialog::onAccepted()
{
  mpColmapWrapper->setLocalColmapBinPath(ui->le_localColmapBinary->text());
  mpColmapWrapper->setLocalWorkspacePath(ui->le_localWorkspace->text());
  mpColmapWrapper->setConnectionType(static_cast<ColmapWrapper::EConnectionType>(
                                   ui->cb_connection->currentIndex()));
  mpColmapWrapper->setRemoteAddr(ui->le_remoteAddr->text());
  mpColmapWrapper->setRemoteUsr(ui->le_remoteUsr->text());
  mpColmapWrapper->setRemoteColmapBinPath(ui->le_remoteColmapBinary->text());
  mpColmapWrapper->setRemoteWorkspacePath(ui->le_remoteWorkspace->text());
  mpColmapWrapper->setMntPntRemoteWorkspacePath(ui->le_mntPnt->text());
  mpColmapWrapper->setSyncInterval(ui->sb_syncInterval->value());
  mpColmapWrapper->writeSettings();
}

//==================================================================================================
void SettingsDialog::onShow()
{
  ui->le_localColmapBinary->setText(mpColmapWrapper->localColmapBinPath());
  ui->le_localWorkspace->setText(mpColmapWrapper->localWorkspacePath());
  ui->cb_connection->setCurrentIndex(static_cast<int>(mpColmapWrapper->connectionType()));
  ui->le_remoteAddr->setText(mpColmapWrapper->remoteAddr());
  ui->le_remoteUsr->setText(mpColmapWrapper->remoteUsr());
  ui->le_remoteColmapBinary->setText(mpColmapWrapper->remoteColmapBinPath());
  ui->le_remoteWorkspace->setText(mpColmapWrapper->remoteWorkspacePath());
  ui->le_mntPnt->setText(mpColmapWrapper->mntPntRemoteWorkspacePath());
  ui->sb_syncInterval->setValue(mpColmapWrapper->syncInterval());
}


} // namespace colmapwrapper
} // namespace ui
} // namespace ots
} // namespeace lib3d
