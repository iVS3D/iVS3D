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

    // Set up the custom commands table
    ui->tw_customCommands->setColumnCount(3);
    ui->tw_customCommands->setHorizontalHeaderLabels(QStringList() << "Name" << "Command" << "");

    ui->tw_customCommands->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->tw_customCommands->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tw_customCommands->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    // Load existing custom commands
    loadCustomCommands();


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
    connect(ui->buttonBox->button(QDialogButtonBox::Apply),
            &QPushButton::pressed,
            this,
            &SettingsDialog::onApply);
    connect(ui->buttonBox->button(QDialogButtonBox::RestoreDefaults),
            &QPushButton::pressed,
            this,
            &SettingsDialog::onRestoreDefaults);

    connect(ui->buttonBox, &QDialogButtonBox::close, this, &SettingsDialog::onCancel);
    connect(ui->pb_addRow,             &QPushButton::clicked,
            this,
            &SettingsDialog::addEmptyRow);

    connect(mpColmapWrapper,
            &ColmapWrapper::setupStatusUpdate,
            this,
            &SettingsDialog::onStatusChanged);
    connect(ui->le_localWorkspace, &QLineEdit::textChanged, this, &SettingsDialog::settingsChanged);
    connect(ui->le_localColmapBinary,
            &QLineEdit::textChanged,
            this,
            &SettingsDialog::settingsChanged);
    connect(ui->le_localOpenMVSBinaryFolder,
            &QLineEdit::textChanged,
            this,
            &SettingsDialog::settingsChanged);
    connect(ui->le_remoteWorkspace, &QLineEdit::textChanged, this, &SettingsDialog::settingsChanged);
    connect(ui->le_remoteAddr, &QLineEdit::textChanged, this, &SettingsDialog::settingsChanged);
    connect(ui->le_remoteUsr, &QLineEdit::textChanged, this, &SettingsDialog::settingsChanged);
    connect(ui->le_remoteColmapBinary,
            &QLineEdit::textChanged,
            this,
            &SettingsDialog::settingsChanged);
    connect(ui->le_remoteOpenMVSBinaryFolder,
            &QLineEdit::textChanged,
            this,
            &SettingsDialog::settingsChanged);
    connect(ui->le_mntPnt, &QLineEdit::textChanged, this, &SettingsDialog::settingsChanged);

    ColmapWrapperControlsFactory *pCtrlFactory = mpColmapWrapper->getOrCreateUiControlsFactory();
    connect(pCtrlFactory,
            &ColmapWrapperControlsFactory::updateToDarkTheme,
            this,
            &SettingsDialog::onUpdateToDarkTheme);
    connect(pCtrlFactory,
            &ColmapWrapperControlsFactory::updateToLightTheme,
            this,
            &SettingsDialog::onUpdateToLightTheme);

    //--- hide remote settings (default)
    ui->f_remote->setVisible(false);
    //updateStatusMsg();
}

//==================================================================================================
SettingsDialog::~SettingsDialog()
{
  delete ui;
}

void SettingsDialog::onUpdateToDarkTheme()
{
  settingsChanged();
}

void SettingsDialog::onUpdateToLightTheme()
{
  settingsChanged();
}

void SettingsDialog::onAddCustomCommand()
{
    int rowCount = ui->tw_customCommands->rowCount();
    ui->tw_customCommands->insertRow(rowCount);
    ui->tw_customCommands->setItem(rowCount, 0, new QTableWidgetItem(""));
    ui->tw_customCommands->setItem(rowCount, 1, new QTableWidgetItem(""));

}


void SettingsDialog::onRemoveCustomCommand(int row)
{
    ui->tw_customCommands->removeRow(row);
    ui->tw_customCommands->update();

    updateRemoveButtonCallbacks();
}

void SettingsDialog::updateRemoveButtonCallbacks()
{
    int rowCount = ui->tw_customCommands->rowCount();
    for (int i = 0; i < rowCount - 1; ++i) {
        QWidget *widget = ui->tw_customCommands->cellWidget(i, 2);
        if (QPushButton *removeButton = qobject_cast<QPushButton*>(widget)) {
            // Disconnect previous connections
            removeButton->disconnect();

            // Reconnect with the updated row index
            connect(removeButton, &QPushButton::clicked, [this, i]() {
                this->onRemoveCustomCommand(i);
            });
        }
    }
}

void SettingsDialog::loadCustomCommands()
{
    QList<QPair<QString, QString>> customCommands = mpColmapWrapper->customCommands();

    for(int i = 0; i< ui->tw_customCommands->rowCount();i++)
        ui->tw_customCommands->removeRow(i);

    ui->tw_customCommands->update();
    ui->tw_customCommands->setRowCount(0);

    for (QList<QPair<QString, QString>>::const_iterator it = customCommands.constBegin(); it != customCommands.constEnd(); ++it) {
        int rowCount = ui->tw_customCommands->rowCount();
        ui->tw_customCommands->insertRow(rowCount);

        const QPair<QString, QString> &pair = *it;
        ui->tw_customCommands->setItem(rowCount, 0, new QTableWidgetItem(pair.first));
        ui->tw_customCommands->setItem(rowCount, 1, new QTableWidgetItem(pair.second));

        QPushButton *removeButton = new QPushButton("Remove");
        connect(removeButton, &QPushButton::clicked, [this, rowCount]() {
            this->onRemoveCustomCommand(rowCount);
        });
        ui->tw_customCommands->setCellWidget(rowCount, 2, removeButton);
    }

}

void SettingsDialog::addEmptyRow()
{
    int rowCount = ui->tw_customCommands->rowCount();
    ui->tw_customCommands->insertRow(rowCount);
    ui->tw_customCommands->setItem(rowCount, 0, new QTableWidgetItem(""));
    ui->tw_customCommands->setItem(rowCount, 1, new QTableWidgetItem(""));

    QPushButton *removeButton = new QPushButton("Remove");
    connect(removeButton, &QPushButton::clicked, [this, rowCount]() {
        this->onRemoveCustomCommand(rowCount);
    });
    ui->tw_customCommands->setCellWidget(rowCount, 2, removeButton);
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
    QString folderPath = ui->le_localOpenMVSBinaryFolder->text();

    folderPath = QFileDialog::getExistingDirectory(this,
                                            tr("Select OpenMVS binary folder"),
                                            folderPath.isEmpty()
                                                ? QApplication::applicationDirPath()
                                                : QFileInfo(folderPath).absoluteDir().absolutePath());


    if (!folderPath.isEmpty()) {
        ui->le_localOpenMVSBinaryFolder->setText(folderPath);
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
void SettingsDialog::onInstallScriptsPushButtonPressed()
{
    if (static_cast<ColmapWrapper::EConnectionType>(ui->cb_connection->currentIndex())
        == ColmapWrapper::LOCAL) {
        mpColmapWrapper->setLocalWorkspacePath(ui->le_localWorkspace->text());
    } else {
        mpColmapWrapper->setMntPntRemoteWorkspacePath(ui->le_mntPnt->text());
    }

    mpColmapWrapper->installScriptFilesIntoWorkspace();
}

//==================================================================================================
void SettingsDialog::onApply()
{
    ui->l_error->setText("Testing connection... this may take up to 10 seconds");
    ui->l_error->setStyleSheet("QLabel { border : 1px solid orange; color : orange; }");
    this->repaint();

    // Collect custom commands from the table
    QList<QPair<QString, QString>> customCommands;
    int rowCount = ui->tw_customCommands->rowCount();
    for (int i = 0; i < rowCount; ++i) {
        QTableWidgetItem *nameItem = ui->tw_customCommands->item(i, 0);
        QTableWidgetItem *commandItem = ui->tw_customCommands->item(i, 1);
        if (nameItem && commandItem) {
            QString name = nameItem->text();
            QString command = commandItem->text();
            if (!name.trimmed().isEmpty() && !command.trimmed().isEmpty()) {
                customCommands.append(QPair(name, command));
            }
        }
    }




  ColmapWrapper::SSettings *settings = new ColmapWrapper::SSettings{
      ui->le_localColmapBinary->text(),
      ui->le_remoteColmapBinary->text(),
      ui->le_localOpenMVSBinaryFolder->text(),
      ui->le_remoteOpenMVSBinaryFolder->text(),
      ui->le_localWorkspace->text(),
      ui->le_remoteWorkspace->text(),
      ui->le_mntPnt->text(),
      static_cast<ColmapWrapper::EConnectionType>(ui->cb_connection->currentIndex()),
      ui->le_remoteAddr->text(),
      ui->le_remoteUsr->text(),
      customCommands,
      ui->sb_syncInterval->value()
  };
  mSetupResults = ColmapWrapper::SSetupResults();
  // maybe do this in a thread
  if(!mpColmapWrapper->testSettings(settings, &mSetupResults)){
      // something went wrong!
      delete settings;
      return;
  }

    mpColmapWrapper->setCustomCommands(customCommands);


  ui->l_error->setStyleSheet("QLabel { border : 1px solid green; color : green; }");
  ui->l_error->setText(tr("Setup successfull"));
  mpColmapWrapper->applySettings(settings);
  mpColmapWrapper->writeSettings();
  delete settings;
}

void SettingsDialog::onCancel()
{
    mpColmapWrapper->readSettings();
}

void SettingsDialog::onRestoreDefaults()
{
    mpColmapWrapper->restoreDefaultSettings();

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

    loadCustomCommands();
}

void SettingsDialog::settingsChanged()
{
    ui->l_error->setStyleSheet("QLabel { border : none; }");
    ui->l_error->setText("");
    QString textColor = this->palette().text().color().name();
    QString css = "QLineEdit { color : " + textColor +"; }";
    ui->le_localWorkspace->setStyleSheet(css);
    ui->le_localColmapBinary->setStyleSheet(css);
    ui->le_remoteColmapBinary->setStyleSheet(css);
    ui->le_remoteWorkspace->setStyleSheet(css);
    ui->le_mntPnt->setStyleSheet(css);
}

void SettingsDialog::updateStatusMsg(const QPair<ColmapWrapper::ESetupTestResult,QString> &test, QLineEdit *input){

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

    loadCustomCommands();
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

void SettingsDialog::onQuit()
{
    this->close();
}

} // namespace colmapwrapper
} // namespace ui
} // namespace ots
} // namespace lib3d
