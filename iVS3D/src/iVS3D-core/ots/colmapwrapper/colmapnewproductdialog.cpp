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

    ui->l_warningSequenceName->setVisible(false);
    ui->l_warningSequenceName->setStyleSheet("QLabel { color: red; }");

    validateImagePath();
    validateSequenceName();

    //--- connections
    connect(ui->pb_selectImagePath,
            &QPushButton::clicked,
            this,
            &NewProductDialog::onPbSelectImageDirectoryClicked);

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
    ui->cb_prodPointCloud->setEnabled(true);
    ui->cb_prodPointCloud->setChecked(false);

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
        ui->cb_prodCameraPoses->setEnabled(false);
        ui->cb_prodCameraPoses->setChecked(true);
        enableSaveButtonState();
    } else {
        ui->cb_prodCameraPoses->setEnabled(true);
        ui->cb_prodCameraPoses->setChecked(false);
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
        ui->cb_prodCameraPoses->setEnabled(false);
        ui->cb_prodCameraPoses->setChecked(true);
        ui->cb_prodPointCloud->setEnabled(false);
        ui->cb_prodPointCloud->setChecked(true);
        enableSaveButtonState();

    } else {
        ui->cb_prodCameraPoses->setEnabled(true);
        ui->cb_prodCameraPoses->setChecked(false);
        ui->cb_prodPointCloud->setEnabled(true);
        ui->cb_prodPointCloud->setChecked(false);
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
void NewProductDialog::updateSettingsVisibility()
{
    ui->gb_settingsCamPoses->setVisible(ui->cb_prodCameraPoses->isChecked());
    ui->gb_settingsPointCloud->setVisible(ui->cb_prodPointCloud->isChecked());
    ui->gb_settingsMesh->setVisible(ui->cb_prodMesh->isChecked());

    //  ui->gb_settingsMesh->setVisible(ui->cb_prodMesh->isChecked());
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

    // TODO: copy image files to REMOTE here, if not done already!!!

    if (mpColmapWrapper->connectionType() == ColmapWrapper::SSH) {
        //--- compute image path from sequence name
        QString genericDirPath = "%1/" + ui->le_sequenceName->text() + ".images";
        QString displayDirPath = genericDirPath.arg(
            QUrl(mpColmapWrapper->remoteWorkspacePath()).toString(QUrl::StripTrailingSlash));
        QString importDirPath = genericDirPath.arg(
            QUrl(mpColmapWrapper->mntPntRemoteWorkspacePath()).toString(QUrl::StripTrailingSlash));

        //--- if custom import function is set, call it.
        //--- Otherwise call dfault import procedure, i.e. copying of image files.
        bool successful = false;
        std::function<bool(std::string, uint)> custImportFn = mpColmapWrapper->customImportFn();
        if (custImportFn) {
            successful = custImportFn(importDirPath.toStdString(), 1);
        } else {
            //--- default import routine: Select directory and copy all image files into destination
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

                //--- create destination directory if not exists
                QDir destDir(importDirPath);
                if (!destDir.exists())
                    destDir.mkpath(".");

                //--- copy files
                if (imgFiles.size() > 0) {
                    QProgressDialog progress(tr("Copying files..."),
                                             tr("Abort Copy"),
                                             0,
                                             imgFiles.size(),
                                             this);
                    progress.setWindowModality(Qt::WindowModal);
                    progress.setMinimumDuration(500);

                    bool isCanceled = false;
                    for (int i = 0; i < imgFiles.size(); i++) {
                        progress.setValue(i);

                        //--- if progress was canceled
                        if (progress.wasCanceled()) {
                            //--- remove already copied files
                            for (int j = i - 1; j >= 0; --j)
                                QFile::remove(importDirPath + QDir::separator() + imgFiles[j]);

                            isCanceled = true;
                            break;
                        }
                        // copy does not overwrite existing, so should be very perfomant if files already exist
                        QFile::copy(srcDirPath + QDir::separator() + imgFiles[i],
                                    importDirPath + QDir::separator() + imgFiles[i]);
                    }
                    progress.setValue(imgFiles.size());

                    successful = !isCanceled;
                } else {
                    QMessageBox msgWarning;
                    msgWarning.setText("WARNING!\n No images found.");
                    msgWarning.setIcon(QMessageBox::Warning);
                    msgWarning.setWindowTitle("Caution");
                    msgWarning.exec();
                    successful = false;
                }
            }
        }

        if (!successful) {
            return;
        }
        ui->le_imagePath->setText(displayDirPath);
    }

    //--- create job to estimate camera poses if applicable
    if (ui->cb_prodCameraPoses->isChecked()) {
        ColmapWrapper::SJob camParamsJob = createJob(ColmapWrapper::CAMERA_POSES);
        camParamsJob.parameters.insert(
            std::pair<std::string, std::string>("image_path",
                                                ui->le_imagePath->text().toStdString()));
        camParamsJob.parameters.insert(
            std::pair<std::string, std::string>("camera_model",
                                                COLMAP_CAM_MODELS[ui->cb_camModel->currentIndex()]));
        camParamsJob.parameters.insert(
            std::pair<std::string, std::string>("single_camera",
                                                ui->cb_singleCam->isChecked() ? "1" : "0"));

        camParamsJob.parameters.insert(
            std::pair<std::string, std::string>("camera_params",
                                                ui->le_intrinsicParameters->text().toStdString()));
        camParamsJob.parameters.insert(std::pair<std::string, std::string>("multiple_models", "1"));
        camParamsJob.parameters.insert(std::pair<std::string, std::string>(
            "gpus", ui->le_poseGpus->text().replace(",", "_").toStdString()));

        camParamsJob.parameters.insert(
            std::pair<std::string, std::string>("max_focal_length_ratio",
                                                ui->cb_highFocalLength->isChecked() ? "1000"
                                                                                    : "10"));
        camParamsJob.parameters.insert(
            std::pair<std::string, std::string>("robust_mode",
                                                std::to_string(ui->cb_useRobustMode->isChecked())));

        mNewJobList.push_back(camParamsJob);
    }

    unsigned short quality = 0;
    if (ui->rb_quality1->isChecked()) {
        quality = 1;
    } else if (ui->rb_quality2->isChecked()) {
        quality = 2;
    } else if (ui->rb_quality3->isChecked()) {
        quality = 3;
    }

    //--- create job to compute dense point cloud if applicable
    if (ui->cb_prodPointCloud->isChecked()) {
        ColmapWrapper::SJob pointCloudJob = createJob(ColmapWrapper::DENSE_CLOUD);

        pointCloudJob.parameters.insert(
            std::pair<std::string, std::string>("cache_size",
                                                std::to_string(ui->sb_cacheSize->value())));

        pointCloudJob.parameters.insert(
            std::pair<std::string, std::string>("quality", std::to_string(quality)));

        pointCloudJob.parameters.insert(std::pair<std::string, std::string>(
            "gpus", ui->le_poseGpus->text().replace(",", "_").toStdString()));
        mNewJobList.push_back(pointCloudJob);
    }

    //--- create job to compute mesh if applicable
    if (ui->cb_prodMesh->isChecked()) {
        ColmapWrapper::SJob meshJob = createJob(ColmapWrapper::MESHED_MODEL);

        meshJob.parameters.insert(
            std::pair<std::string, std::string>("quality", std::to_string(quality)));

        meshJob.parameters.insert(
            std::pair<std::string, std::string>("max_threads",
                                                ui->le_maxThreads->text().toStdString()));

        mNewJobList.push_back(meshJob);
    }
}

//==================================================================================================
void NewProductDialog::onUpdateToDarkTheme()
{
    ui->pb_selectImagePath->setIcon(QIcon(":/assets/icons/glyphicons-145-folder-open-dark.png"));
}

//==================================================================================================
void NewProductDialog::onUpdateToLightTheme()
{
    ui->pb_selectImagePath->setIcon(QIcon(":/assets/icons/glyphicons-145-folder-open.png"));
}

//==================================================================================================
void NewProductDialog::onShow()
{
    //--- clear previous configuration
    mAvailableSeqs.clear();
    ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
    ui->cb_prodCameraPoses->setChecked(false);
    ui->cb_prodCameraPoses->setEnabled(true);
    ui->cb_prodPointCloud->setChecked(false);
    ui->cb_prodPointCloud->setEnabled(true);
    ui->cb_useRobustMode->setChecked(mpColmapWrapper->useRobustMode());
    ui->cb_prodMesh->setChecked(false);
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

    //--- get list of sequences already defined, i.e. in finished seq and pending jobs
    mAvailableSeqs = mpColmapWrapper->getFinishedSequenceList();
    std::vector<ColmapWrapper::SJob> jobList = mpColmapWrapper->getJobList();
    for (ColmapWrapper::SJob job : jobList) {
        ColmapWrapper::SSequence seq;
        seq.name = job.sequenceName;
        seq.imagePath = (mpColmapWrapper->connectionType() == ColmapWrapper::LOCAL)
                            ? mpColmapWrapper->localWorkspacePath().toStdString() + seq.name
                                  + ".images"
                            : mpColmapWrapper->remoteWorkspacePath().toStdString() + seq.name
                                  + ".images";
        mAvailableSeqs.push_back(seq);
    }

    //--- enable pushbutton only if connection type is local
    //ui->pb_selectImagePath->setEnabled(mpColmapWrapper->connectionType() == ColmapWrapper::LOCAL);

    ui->le_imagePath->setText("");
    if (!mpColmapWrapper->getLocalPresetSequence().name.empty()) {
        ui->le_sequenceName->setText(
            QString::fromStdString(mpColmapWrapper->getLocalPresetSequence().name));
        ui->le_imagePath->setText(
            QString::fromStdString(mpColmapWrapper->getLocalPresetSequence().imagePath));
    }
}

//==================================================================================================
std::vector<ColmapWrapper::SJob> NewProductDialog::getNewJobList() const
{
    return mNewJobList;
}

void NewProductDialog::enableSaveButtonState()
{
    ui->buttonBox->button(QDialogButtonBox::Save)
        ->setEnabled(ui->cb_prodCameraPoses->isChecked() && isImagePathValid && isSequenceNameValid);
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
    QDir workSpaceDirectory(mpColmapWrapper->mLocalWorkspacePath);
    QStringList msProjFiles = workSpaceDirectory.entryList(QStringList() << "*.db",
                                                           QDir::Files,
                                                           QDir::Name);

    if (!msProjFiles.contains(sequenceName + ".db") && sequenceName != ""
        && !sequenceName.contains(" ")) {
        ui->le_sequenceName->setStyleSheet("");
        isSequenceNameValid = true;
        enableSaveButtonState();
        ui->l_warningSequenceName->setVisible(false);
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
