#include "colmapnewproductdialog.h"
#include "ui_colmapnewproductdialog.h"

// Std
#include <iostream>

// Qt
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QUrl>

namespace lib3d {
namespace ots {
namespace ui {
namespace colmapwrapper {

const std::vector<std::string> COLMAP_CAM_MODELS = {"OPENCV", "RADIAL", "PINHOLE"};

//==================================================================================================
NewProductDialog::NewProductDialog(ColmapWrapper *ipWrapper, QWidget *parent)
    : QDialog(parent), ui(new Ui::NewProductDialog), mpColmapWrapper(ipWrapper),
      isImagePathValid(false), isSequenceNameValid(false)

{
    ui->setupUi(this);
    ui->l_warningNoImages->setVisible(false);
    ui->l_warningNoImages->setStyleSheet("QLabel { color: red; }");

    ui->l_warningNoMasks->setVisible(false);
    ui->l_warningNoMasks->setStyleSheet("QLabel { color: red; }");

    ui->l_warningSequenceName->setVisible(false);
    ui->l_warningSequenceName->setStyleSheet("QLabel { color: red; }");

    //--- connections
    connect(ui->pb_selectImagePath,
            &QPushButton::clicked,
            this,
            &NewProductDialog::onPbSelectImageDirectoryClicked);

    connect(ui->pb_selectMaskPath,
            &QPushButton::clicked,
            this,
            &NewProductDialog::onPbSelectMaskDirectoryClicked);

    connect(ui->le_maskPath,
            &QLineEdit::editingFinished,
            this,
            &NewProductDialog::validateMaskPath); 
    connect(ui->cb_prodCameraPoses,
            &QCheckBox::clicked,
            this,
            &NewProductDialog::onProdCameraPosesClicked);
    connect(ui->cb_prodPointCloud,
            &QCheckBox::clicked,
            this,
            &NewProductDialog::onProdPointCloudClicked);
    connect(ui->cb_prodMesh, &QCheckBox::clicked, this, &NewProductDialog::onProdMeshClicked);
    connect(this, &NewProductDialog::accepted, this, &NewProductDialog::onAccepted);

    ColmapWrapperControlsFactory *pCtrlFactory = mpColmapWrapper->getOrCreateUiControlsFactory();
    connect(pCtrlFactory,
            &ColmapWrapperControlsFactory::updateToDarkTheme,
            this,
            &NewProductDialog::onUpdateToDarkTheme);
    connect(pCtrlFactory,
            &ColmapWrapperControlsFactory::updateToLightTheme,
            this,
            &NewProductDialog::onUpdateToLightTheme);
    connect(ui->le_sequenceName,
            &QLineEdit::editingFinished,
            this,
            &NewProductDialog::validateSequenceName);
    connect(ui->le_imagePath,
            &QLineEdit::editingFinished,
            this,
            &NewProductDialog::validateImagePath);

    updateSettingsVisibility();
}

//==================================================================================================
NewProductDialog::~NewProductDialog()
{
    delete ui;
}


//==================================================================================================
void NewProductDialog::onProdCameraPosesClicked()
{
    if (ui->cb_prodCameraPoses->isChecked()) {
        enableSaveButtonState();
    } else {
        ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
    }

    // TODO
    //  ui->cb_prodMesh->setEnabled(true);
    //  ui->cb_prodMesh->setChecked(false);

    updateSettingsVisibility();
}

//==================================================================================================
void NewProductDialog::onProdPointCloudClicked()
{
    if (ui->cb_prodPointCloud->isChecked()) {
        if(!mpColmapWrapper->areChecksDisabled())
        {
            ui->cb_prodCameraPoses->setEnabled(false);
            ui->cb_prodCameraPoses->setChecked(true);
        }
        enableSaveButtonState();
    } else {
        if(!mpColmapWrapper->areChecksDisabled())
        {
            ui->cb_prodCameraPoses->setEnabled(true);
            ui->cb_prodCameraPoses->setChecked(false);
        }
        ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
    }
    // TODO
    //  ui->cb_prodMesh->setEnabled(true);
    //  ui->cb_prodMesh->setChecked(false);

    updateSettingsVisibility();
}

//==================================================================================================
void NewProductDialog::onProdMeshClicked()
{
    if (ui->cb_prodMesh->isChecked()) {
        if(!mpColmapWrapper->areChecksDisabled())
        {
            ui->cb_prodCameraPoses->setChecked(true);
            ui->cb_prodCameraPoses->setEnabled(false);
            ui->cb_prodPointCloud->setChecked(true);
            ui->cb_prodPointCloud->setEnabled(false);
        }
        enableSaveButtonState();

    } else {
        if(!mpColmapWrapper->areChecksDisabled())
        {
            ui->cb_prodCameraPoses->setChecked(false);
            ui->cb_prodCameraPoses->setEnabled(true);
            ui->cb_prodPointCloud->setChecked(false);
            ui->cb_prodPointCloud->setEnabled(true);
        }
        ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
    }

    updateSettingsVisibility();
}

//==================================================================================================
void NewProductDialog::onPbSelectImageDirectoryClicked()
{
    QString dirPath = ui->le_imagePath->text();

    dirPath = QFileDialog::getExistingDirectory(this,
                                                tr("Select image directory"),
                                                dirPath.isEmpty()
                                                    ? QApplication::applicationDirPath()
                                                    : dirPath);

    if (!dirPath.isEmpty()) {
        ui->le_imagePath->setText(dirPath);
        validateImagePath();
    }
}

//==================================================================================================
void NewProductDialog::onPbSelectMaskDirectoryClicked()
{
    QString dirPath = ui->le_maskPath->text();

    dirPath = QFileDialog::getExistingDirectory(this,
                                                tr("Select mask directory"),
                                                dirPath.isEmpty()
                                                    ? QApplication::applicationDirPath()
                                                    : dirPath);

    if (!dirPath.isEmpty()) {
        ui->le_maskPath->setText(dirPath);
        validateMaskPath();
    }
}

void NewProductDialog::validateMaskPath()
{
    QString maskDirPath = ui->le_maskPath->text().trimmed();

    // If empty, that's fine (optional)
    if (maskDirPath.isEmpty()) {
        ui->le_maskPath->setStyleSheet("");
        ui->l_warningNoMasks->setVisible(false);
        return;
    }

    QDir maskDir(maskDirPath);
    if (!maskDir.exists()) {
        ui->le_maskPath->setStyleSheet("QLineEdit { border: 1px solid red; }");
        ui->l_warningNoMasks->setText(tr("Mask directory does not exist."));
        ui->l_warningNoMasks->setVisible(true);
        return;
    }

    // List mask files
    QStringList maskFiles = maskDir.entryList(QStringList() << "*.png"
                                                            << "*.PNG"
                                                            << "*.bmp"
                                                            << "*.BMP"
                                                            << "*.tiff"
                                                            << "*.TIFF"
                                                            << "*.jpg"
                                                            << "*.JPG"
                                                            << "*.jpeg"
                                                            << "*.JPEG",
                                              QDir::Files);

    // Get image count for comparison
    QString imageDirPath = ui->le_imagePath->text().trimmed();
    int imageCount = 0;
    if (!imageDirPath.isEmpty()) {
        QDir imageDir(imageDirPath);
        QStringList imgFiles = imageDir.entryList(QStringList() << "*.jpg"
                                                                << "*.JPG"
                                                                << "*.jpeg"
                                                                << "*.JPEG"
                                                                << "*.png"
                                                                << "*.PNG"
                                                                << "*.bmp"
                                                                << "*.BMP"
                                                                << "*.tiff"
                                                                << "*.TIFF",
                                                  QDir::Files);
        imageCount = imgFiles.size();
    }

    if (imageCount > 0 && maskFiles.size() != imageCount) {
        ui->le_maskPath->setStyleSheet("QLineEdit { border: 1px solid orange; }");
        ui->l_warningNoMasks->setText(tr("Warning: Number of masks (%1) does not match number of images (%2).")
                                     .arg(maskFiles.size()).arg(imageCount));
        ui->l_warningNoMasks->setVisible(true);
        return;
    }

    // Valid
    ui->le_maskPath->setStyleSheet("");
    ui->l_warningNoMasks->setVisible(false);
}

//==================================================================================================
void NewProductDialog::updateSettingsVisibility()
{
    ui->gb_settingsCamPoses->setVisible(ui->cb_prodCameraPoses->isChecked());
    ui->gb_settingsPointCloud->setVisible(ui->cb_prodPointCloud->isChecked());
    ui->gb_settingsMesh->setVisible(ui->cb_prodMesh->isChecked());
}

//==================================================================================================
void NewProductDialog::onAccepted()
{
    mNewJobList.clear();

    //--- if no sequence name and image path is provided no jobs are stored
    if (ui->le_sequenceName->text().isEmpty() && ui->le_imagePath->text().isEmpty())
        return;

    //--- lambda to creat new job
    auto createJob = [this](ColmapWrapper::EProductType productType) -> ColmapWrapper::SJob {
        ColmapWrapper::SJob newJob;
        newJob.sequenceName = this->ui->le_sequenceName->text().toStdString();
        newJob.product = productType;
        newJob.state = ColmapWrapper::JOB_PENDING;
        newJob.progress = 0;
        return newJob;
    };

    // Helper lambda to copy files from a source directory to a destination directory
    auto copyFilesToWorkspace = [this](const QString& srcDirPath, const QString& destDirPath, QWidget* parent) -> bool {
        if (srcDirPath.isEmpty())
            return false;

        QDir srcDir(srcDirPath);

        // List all image files in directory
        QStringList imgFiles = srcDir.entryList(QStringList() << "*.jpg"
                                                              << "*.JPG"
                                                              << "*.jpeg"
                                                              << "*.JPEG"
                                                              << "*.png"
                                                              << "*.PNG"
                                                              << "*.bmp"
                                                              << "*.BMP"
                                                              << "*.tiff"
                                                              << "*.TIFF",
                                                QDir::Files);

        // Create destination directory if not exists
        QDir destDir(destDirPath);
        if (destDir.exists()) {
            destDir.removeRecursively();
        }
        destDir.mkpath(".");

        // Copy files
        if (imgFiles.size() > 0) {
            QProgressDialog progress(tr("Copying files..."),
                                     tr("Abort Copy"),
                                     0,
                                     imgFiles.size(),
                                     parent);
            progress.setWindowModality(Qt::WindowModal);
            progress.show();
            bool isCanceled = false;
            for (int i = 0; i < imgFiles.size(); i++) {
                progress.setValue(i);

                // If progress was canceled
                if (progress.wasCanceled()) {
                    // Remove already copied files
                    for (int j = i - 1; j >= 0; --j)
                        QFile::remove(QDir::toNativeSeparators(destDirPath + QDir::separator() + imgFiles[j]));

                    isCanceled = true;
                    break;
                }
                QFile::copy(QDir::toNativeSeparators(srcDirPath + QDir::separator() + imgFiles[i]),
                            QDir::toNativeSeparators(destDirPath + QDir::separator() + imgFiles[i]));
            }
            progress.setValue(imgFiles.size());

            return !isCanceled;
        } else {
            QMessageBox msgWarning;
            msgWarning.setText("WARNING!\n No images found.");
            msgWarning.setIcon(QMessageBox::Warning);
            msgWarning.setWindowTitle("Caution");
            msgWarning.exec();
            return false;
        }
    };

    //--- compute image path from sequence name
    QString genericDirPath = "%1/" + ui->le_sequenceName->text() + ".images";
    QString displayDirPath, importDirPath;
    if (mpColmapWrapper->connectionType() == ColmapWrapper::SSH) {
        displayDirPath = QDir::toNativeSeparators(genericDirPath.arg(
            mpColmapWrapper->remoteWorkspacePath()));
        importDirPath = QDir::toNativeSeparators(genericDirPath.arg(
            mpColmapWrapper->mntPntRemoteWorkspacePath()));
    } else {
        displayDirPath = QDir::toNativeSeparators(genericDirPath.arg(
            mpColmapWrapper->localWorkspacePath()));
        importDirPath = displayDirPath;
    }

    QString srcDirPath = ui->le_imagePath->text();

    // Copy images
    bool successful = copyFilesToWorkspace(srcDirPath, importDirPath, this);

    if (!successful) {
        return;
    }
    ui->le_imagePath->setText(displayDirPath);

    // --- Copy masks only if a valid folder is selected in the UI ---
    QString mask_path;
    QString maskDirPath = ui->le_maskPath->text().trimmed();
    bool hasMasks = false;
    if (!maskDirPath.isEmpty()) {
        QDir maskDir(maskDirPath);
        if (maskDir.exists()) {
            // Prepare destination path in workspace
            QString genericMaskDirPath = "%1/" + ui->le_sequenceName->text() + ".masks";
            QString importMaskDirPath;
            if (mpColmapWrapper->connectionType() == ColmapWrapper::SSH) {
                importMaskDirPath = QDir::toNativeSeparators(genericMaskDirPath.arg(
                    mpColmapWrapper->mntPntRemoteWorkspacePath()));
            } else {
                importMaskDirPath = QDir::toNativeSeparators(genericMaskDirPath.arg(
                    mpColmapWrapper->localWorkspacePath()));
            }
            // Copy mask files
            bool maskCopySuccess = copyFilesToWorkspace(maskDirPath, importMaskDirPath, this);
            if (maskCopySuccess) {
                hasMasks = true;
                mask_path = importMaskDirPath;
                if (mpColmapWrapper->connectionType() == ColmapWrapper::SSH) {
                    mask_path = QDir::fromNativeSeparators(mask_path);
                }
            }
        }
    }

    unsigned short quality = 0;
    if (ui->rb_quality1->isChecked()) {
        quality = 1;
    } else if (ui->rb_quality2->isChecked()) {
        quality = 2;
    } else if (ui->rb_quality3->isChecked()) {
        quality = 3;
    }

    ColmapWrapper::SJob customCommandJob = createJob(ColmapWrapper::CUSTOM_COMMAND);

    QString image_path = ui->le_imagePath->text();

    if (mpColmapWrapper->connectionType() == ColmapWrapper::SSH) {
        image_path = QDir::fromNativeSeparators(image_path);
    }

    customCommandJob.parameters.insert(
        std::pair<std::string, std::string>("custom_command",
                                            mpColmapWrapper->customCommands()
                                                .at(ui->cob_customCommand->currentIndex())
                                                .second.toStdString()));

    customCommandJob.displayName = mpColmapWrapper->customCommands()
                                       .at(ui->cob_customCommand->currentIndex())
                                       .first.toStdString();

    customCommandJob.parameters.insert(
        std::pair<std::string, std::string>("image_path", image_path.toStdString()));
    if (hasMasks) {
        customCommandJob.parameters.insert(
            std::pair<std::string, std::string>("mask_path", mask_path.toStdString()));
    }
    customCommandJob.parameters.insert(
        std::pair<std::string, std::string>("camera_model",
                                            COLMAP_CAM_MODELS[ui->cb_camModel->currentIndex()]));
    customCommandJob.parameters.insert(
        std::pair<std::string, std::string>("single_camera",
                                            ui->cb_singleCam->isChecked() ? "1" : "0"));

    customCommandJob.parameters.insert(
        std::pair<std::string, std::string>("camera_params",
                                            ui->le_intrinsicParameters->text().toStdString()));
    customCommandJob.parameters.insert(std::pair<std::string, std::string>("multiple_models", "1"));
    customCommandJob.parameters.insert(
        std::pair<std::string, std::string>("gpus",
                                            ui->le_poseGpus->text().replace(",", "_").toStdString()));

    customCommandJob.parameters.insert(
        std::pair<std::string, std::string>("max_focal_length_ratio",
                                            ui->cb_highFocalLength->isChecked() ? "1000" : "10"));
    customCommandJob.parameters.insert(
        std::pair<std::string, std::string>("robust_mode",
                                            std::to_string(ui->cb_useRobustMode->isChecked())));

    customCommandJob.parameters.insert(
        std::pair<std::string, std::string>("quality", std::to_string(quality)));

    customCommandJob.parameters.insert(
        std::pair<std::string, std::string>("max_threads", ui->le_maxThreads->text().toStdString()));

    customCommandJob.parameters.insert(
        std::pair<std::string, std::string>("cache_size",
                                            std::to_string(ui->sb_cacheSize->value())));

    customCommandJob.parameters.insert(
        std::pair<std::string, std::string>("run_sparse",
                                            std::to_string(ui->cb_prodCameraPoses->isChecked())));

    customCommandJob.parameters.insert(
        std::pair<std::string, std::string>("run_dense",
                                            std::to_string(ui->cb_prodPointCloud->isChecked())));

    customCommandJob.parameters.insert(
        std::pair<std::string, std::string>("run_mesh",
                                            std::to_string(ui->cb_prodMesh->isChecked())));

    mNewJobList.push_back(customCommandJob);
}

//==================================================================================================
void NewProductDialog::onUpdateToDarkTheme()
{
    ui->pb_selectImagePath->setIcon(QIcon(":/assets/icons/glyphicons-145-folder-open-dark.png"));
    ui->pb_selectMaskPath->setIcon(QIcon(":/assets/icons/glyphicons-145-folder-open-dark.png"));
}

//==================================================================================================
void NewProductDialog::onUpdateToLightTheme()
{
    ui->pb_selectImagePath->setIcon(QIcon(":/assets/icons/glyphicons-145-folder-open.png"));
    ui->pb_selectMaskPath->setIcon(QIcon(":/assets/icons/glyphicons-145-folder-open.png"));
}

//==================================================================================================
void NewProductDialog::onShow()
{
    //--- clear previous configuration
    ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
    ui->cb_prodCameraPoses->setChecked(false);
    ui->cb_prodCameraPoses->setEnabled(true);
    ui->cb_prodPointCloud->setChecked(false);
    ui->cb_prodPointCloud->setEnabled(true);
    ui->cb_useRobustMode->setChecked(mpColmapWrapper->useRobustMode());
    ui->cb_prodMesh->setChecked(false);
    ui->cob_customCommand->setEnabled(true);
    ui->cob_customCommand->clear();

    for(auto pair: mpColmapWrapper->customCommands())
        ui->cob_customCommand->addItem(pair.first);

    if (mpColmapWrapper->connectionType() == ColmapWrapper::LOCAL) {
        ui->cb_prodMesh->setEnabled(mpColmapWrapper->localOpenMVSBinPath() != "");
    } else {
        ui->cb_prodMesh->setEnabled(mpColmapWrapper->remoteOpenMVSBinPath() != "");
    }

    ui->gb_settingsCamPoses->setVisible(false);
    ui->gb_settingsPointCloud->setVisible(false);
    //ui->gb_settingsMesh->setVisible(false);

    ui->rb_quality0->setChecked(false);
    ui->rb_quality1->setChecked(true);
    ui->rb_quality2->setChecked(false);
    ui->rb_quality3->setChecked(false);

    //--- enable pushbutton only if connection type is local
    //ui->pb_selectImagePath->setEnabled(mpColmapWrapper->connectionType() == ColmapWrapper::LOCAL);

    ui->le_imagePath->setText("");
    if (!mpColmapWrapper->getLocalPresetSequence().name.empty()) {
        ui->le_sequenceName->setText(
            QString::fromStdString(mpColmapWrapper->getLocalPresetSequence().name));
        ui->le_imagePath->setText(
            QString::fromStdString(mpColmapWrapper->getLocalPresetSequence().imagePath));
    }

    validateImagePath();
    validateSequenceName();
}

//==================================================================================================
std::vector<ColmapWrapper::SJob> NewProductDialog::getNewJobList() const
{
    return mNewJobList;
}

void NewProductDialog::enableSaveButtonState()
{
    bool isEnabled = isImagePathValid && isSequenceNameValid;

    //--- if in expert mode, one of the products needs to be checked.
    if(mpColmapWrapper->areChecksDisabled())
        isEnabled &= (ui->cb_prodCameraPoses->isChecked() || ui->cb_prodPointCloud->isChecked()
                      || ui->cb_prodMesh->isChecked());
    else
        isEnabled &= (ui->cb_prodCameraPoses->isChecked());

    ui->buttonBox->button(QDialogButtonBox::Save)
        ->setEnabled(isEnabled);
}

//==================================================================================================
void NewProductDialog::validateImagePath()
{
    QString srcDirPath = ui->le_imagePath->text();

    if (!srcDirPath.isEmpty()) {
        QDir srcDir(srcDirPath);

        //--- list all image files in directory
        QStringList imgFiles = srcDir.entryList(QStringList() << "*.jpg"
                                                              << "*.JPG"
                                                              << "*.jepg"
                                                              << "*.JEPG"
                                                              << "*.png"
                                                              << "*.PNG"
                                                              << "*.bmp"
                                                              << "*.BMP"
                                                              << "*.tiff"
                                                              << "*.tiff",
                                                QDir::Files);

        if (imgFiles.size() > 0) {
            ui->le_imagePath->setStyleSheet("");
            isImagePathValid = true;
            enableSaveButtonState();
            ui->l_warningNoImages->setVisible(false);
            enableSaveButtonState();

            // --- Auto-select sibling masks folder if it exists and mask path is empty or invalid
            QDir imageDir(srcDirPath);
            QString maskDirPath = imageDir.filePath("../masks");
            QDir maskDir(maskDirPath);
            if (maskDir.exists() && (ui->le_maskPath->text().isEmpty() || !maskDir.exists(ui->le_maskPath->text()))) {
                ui->le_maskPath->setText(maskDir.absolutePath());
                validateMaskPath();
            }

            return;
        }
    }
    ui->le_imagePath->setStyleSheet("QLineEdit { border: 1px solid red; }");
    ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
    isImagePathValid = false;
    ui->l_warningNoImages->setVisible(true);
}

//==================================================================================================
void NewProductDialog::validateSequenceName()
{
    QString sequenceName = ui->le_sequenceName->text();

    //--- get all colmap project files files from local workspace
    QDir workSpaceDirectory;
    if (mpColmapWrapper->connectionType() == ColmapWrapper::SSH) {
        workSpaceDirectory = QDir(mpColmapWrapper->mntPntRemoteWorkspacePath());
    } else {
        workSpaceDirectory = QDir(mpColmapWrapper->mLocalWorkspacePath);
    }

    QStringList msProjFiles = workSpaceDirectory.entryList(QStringList() << "*.db",
                                                           QDir::Files,
                                                           QDir::Name);

    if ((!msProjFiles.contains(sequenceName + ".db") || mpColmapWrapper->areChecksDisabled()) // if in expert mode, the sequence may already exist
        && sequenceName != ""  && !sequenceName.contains(" ")) 
    {
        ui->le_sequenceName->setStyleSheet("");
        isSequenceNameValid = true;
        ui->l_warningSequenceName->setVisible(false);
        enableSaveButtonState();
    } else {
        ui->le_sequenceName->setStyleSheet("QLineEdit { border: 1px solid red; }");
        ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
        isSequenceNameValid = false;
        ui->l_warningSequenceName->setVisible(true);
    }
}

} // namespace colmapwrapper
} // namespace ui
} // namespace ots
} // namespace lib3d
