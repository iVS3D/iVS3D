#include "colmapnewproductdialog.h"
#include "ui_colmapnewproductdialog.h"

// Std
#include <iostream>

// Qt
#include <QUrl>
#include <QDebug>
#include <QFileDialog>
#include <QProgressDialog>

#include "../subsamplingdialog.h"

namespace lib3d {
namespace ots {
namespace ui {
namespace colmapwrapper {

const std::vector<std::string> COLMAP_CAM_MODELS = {"OPENCV", "RADIAL", "PINHOLE"};

//==================================================================================================
NewProductDialog::NewProductDialog(ColmapWrapper *ipWrapper, QWidget *parent) :
  QDialog(parent),
  ui(new Ui::NewProductDialog),
  mpColmapWrapper(ipWrapper)
{
  ui->setupUi(this);

  //--- connections
  connect(ui->pb_selectImagePath, &QPushButton::clicked,
          this, &NewProductDialog::onPbSelectImageDirectoryClicked);
  connect(ui->pb_importImagePath, &QPushButton::clicked,
          this, &NewProductDialog::onPbImportImagesClicked);
  connect(ui->cb_sequenceName, &QComboBox::editTextChanged,
          this, &NewProductDialog::onCbEditTextChanged);
  connect(ui->cb_sequenceName, &QComboBox::currentTextChanged,
          this, &NewProductDialog::onCbCurrentTextChanged);
  connect(ui->cb_prodCameraPoses, &QCheckBox::clicked,
          this, &NewProductDialog::onProdCameraPosesClicked);
  connect(ui->cb_prodPointCloud, &QCheckBox::clicked,
          this, &NewProductDialog::onProdPointCloudClicked);
  connect(ui->cb_prodMesh, &QCheckBox::clicked,
          this, &NewProductDialog::onProdMeshClicked);
  connect(this, &NewProductDialog::accepted,
          this, &NewProductDialog::onAccepted);

  ColmapWrapperControlsFactory* pCtrlFactory = mpColmapWrapper->getOrCreateUiControlsFactory();
  connect(pCtrlFactory, &ColmapWrapperControlsFactory::updateToDarkTheme,
          this, &NewProductDialog::onUpdateToDarkTheme);
  connect(pCtrlFactory, &ColmapWrapperControlsFactory::updateToLightTheme,
          this, &NewProductDialog::onUpdateToLightTheme);

  updateSettingsVisibility();
}

//==================================================================================================
NewProductDialog::~NewProductDialog()
{
  delete ui;
}

//==================================================================================================
void NewProductDialog::onCbCurrentTextChanged(QString currentText)
{
  //--- get current index of text
  int currIdx = ui->cb_sequenceName->findText(currentText);

  //--- if current index is -1, i.e. new name, clear image path.
  //--- Otherwise, load image path according to sequence.
  if(currIdx == -1)
    ui->le_imagePath->setText("");
  else
    ui->le_imagePath->setText(QString::fromStdString(mAvailableSeqs.at(currIdx).imagePath));
}

//==================================================================================================
void NewProductDialog::onProdCameraPosesClicked()
{
  ui->cb_prodPointCloud->setEnabled(true);
  ui->cb_prodPointCloud->setChecked(false);
  // TODO
//  ui->cb_prodMesh->setEnabled(true);
//  ui->cb_prodMesh->setChecked(false);

  updateSettingsVisibility();
}

//==================================================================================================
void NewProductDialog::onProdPointCloudClicked()
{
  if(ui->cb_prodPointCloud->isChecked())
  {
    ui->cb_prodCameraPoses->setEnabled(false);
    ui->cb_prodCameraPoses->setChecked(true);
  }
  else
  {
    ui->cb_prodCameraPoses->setEnabled(true);
    ui->cb_prodCameraPoses->setChecked(false);
  }
  // TODO
//  ui->cb_prodMesh->setEnabled(true);
//  ui->cb_prodMesh->setChecked(false);

  updateSettingsVisibility();
}

//==================================================================================================
void NewProductDialog::onProdMeshClicked()
{
  if(ui->cb_prodMesh->isChecked())
  {
    ui->cb_prodCameraPoses->setEnabled(false);
    ui->cb_prodCameraPoses->setChecked(true);
    ui->cb_prodPointCloud->setEnabled(false);
    ui->cb_prodPointCloud->setChecked(true);
  }
  else
  {
    ui->cb_prodCameraPoses->setEnabled(true);
    ui->cb_prodCameraPoses->setChecked(false);
    ui->cb_prodPointCloud->setEnabled(true);
    ui->cb_prodPointCloud->setChecked(false);
  }

  updateSettingsVisibility();
}

//==================================================================================================
void NewProductDialog::onPbSelectImageDirectoryClicked()
{
  QString dirPath = ui->le_imagePath->text();

  dirPath = QFileDialog::getExistingDirectory(this, tr("Select image directory"),
                                              dirPath.isEmpty() ?
                                              QApplication::applicationDirPath() :
                                              dirPath);

  if(!dirPath.isEmpty())
    ui->le_imagePath->setText(dirPath);
}

//==================================================================================================
void NewProductDialog::onPbImportImagesClicked()
{
  ui->le_imagePath->clear();

  //--- compute image path from sequence name
  QString genericDirPath = "%1/" + ui->cb_sequenceName->currentText() + ".images";
  QString displayDirPath = genericDirPath.arg(
        QUrl((mpColmapWrapper->connectionType() == ColmapWrapper::LOCAL) ?
          mpColmapWrapper->localWorkspacePath() : mpColmapWrapper->remoteWorkspacePath()).
        toString(QUrl::StripTrailingSlash));
  QString importDirPath = genericDirPath.arg(
        QUrl((mpColmapWrapper->connectionType() == ColmapWrapper::LOCAL) ?
          mpColmapWrapper->localWorkspacePath() : mpColmapWrapper->mntPntRemoteWorkspacePath()).
        toString(QUrl::StripTrailingSlash));

  //--- subsampling
  uint nthFrameToImport = 1;

  //--- if custom import function is set, call it.
  //--- Otherwise call dfault import procedure, i.e. copying of image files.
  bool successful = false;
  std::function<bool(std::string, uint)> custImportFn = mpColmapWrapper->customImportFn();
  if(custImportFn)
  {
    //--- subsampling
    QSharedPointer<SubsamplingDialog> subsampl =
        QSharedPointer<SubsamplingDialog>(new SubsamplingDialog(this));
    subsampl->setModal(true);

    int retVal = subsampl->exec();

    if(static_cast<QDialog::DialogCode>(retVal) == QDialog::Accepted)
    {
      //--- get subsampling rate if accepted
      if(subsampl->isSubsamplingActivated())
        nthFrameToImport = subsampl->getNthFrameValue();

      successful = custImportFn(importDirPath.toStdString(), nthFrameToImport);
    }
  }
  else
  {
    //--- default import routine: Select directory and copy all image files into destination
    QString srcDirPath = QFileDialog::getExistingDirectory(this, tr("Select image directory"),
                                                        QApplication::applicationDirPath());

    if(!srcDirPath.isEmpty())
    {
      QDir srcDir(srcDirPath);

      //--- create destination directory if not exists
      QDir destDir(importDirPath);
      if(!destDir.exists())
        destDir.mkpath(".");

      //--- list all image files in directory
      QStringList imgFiles = srcDir.entryList(QStringList() << "*.jpg" << "*.JPG"
                                                            << "*.jepg" << "*.JEPG"
                                                            << "*.png" << "*.PNG"
                                                            << "*.bmp" << "*.BMP"
                                                            << "*.tiff" << "*.tiff",
                                              QDir::Files);

      //--- copy files
      if(imgFiles.size() > 0)
      {
        //--- subsampling
        QSharedPointer<SubsamplingDialog> subsampl =
            QSharedPointer<SubsamplingDialog>(new SubsamplingDialog(this));
        subsampl->setModal(true);

        int retVal = subsampl->exec();
        if(static_cast<QDialog::DialogCode>(retVal) == QDialog::Accepted)
        {
          if(subsampl->isSubsamplingActivated())
            nthFrameToImport = subsampl->getNthFrameValue();

          QProgressDialog progress(tr("Copying files..."), tr("Abort Copy"), 0, imgFiles.size(), this);
          progress.setWindowModality(Qt::WindowModal);
          progress.setMinimumDuration(500);

          bool isCanceled = false;
          for(int i = 0; i < imgFiles.size(); i += nthFrameToImport)
          {
            progress.setValue(i);

            //--- if progress was canceled
            if (progress.wasCanceled())
            {
              //--- remove already copied files
              for(int j = i - 1; j >= 0; --j)
                QFile::remove(importDirPath + QDir::separator() + imgFiles[j]);

              isCanceled = true;
              break;
            }

            QFile::copy(srcDirPath + QDir::separator() + imgFiles[i], importDirPath + QDir::separator() + imgFiles[i]);
          }
          progress.setValue(imgFiles.size());


          successful = (true && !isCanceled);
        }
      }
    }
  }

  if(successful)
    ui->le_imagePath->setText(displayDirPath);
}

//==================================================================================================
void NewProductDialog::updateSettingsVisibility()
{
  ui->gb_settingsCamPoses->setVisible(ui->cb_prodCameraPoses->isChecked());
  ui->gb_settingsPointCloud->setVisible(ui->cb_prodPointCloud->isChecked());
//  ui->gb_settingsMesh->setVisible(ui->cb_prodMesh->isChecked());
}

//==================================================================================================
void NewProductDialog::onAccepted()
{
  mNewJobList.clear();

  //--- if no sequence name and image path is provided no jobs are stored
  if(ui->cb_sequenceName->currentText().isEmpty() && ui->le_imagePath->text().isEmpty())
    return;

  //--- lambda to creat new job
  auto createJob = [this](ColmapWrapper::EProductType productType) -> ColmapWrapper::SJob {
    ColmapWrapper::SJob newJob;
    newJob.sequenceName = this->ui->cb_sequenceName->currentText().toStdString();
    newJob.product = productType;
    newJob.state = ColmapWrapper::JOB_PENDING;
    newJob.progress = 0;
    return newJob;
  };

  //--- create job to estimate camera poses if applicable
  if(ui->cb_prodCameraPoses->isChecked())
  {
    ColmapWrapper::SJob camParamsJob = createJob(ColmapWrapper::CAMERA_POSES);
    camParamsJob.parameters.insert(
          std::pair<std::string, std::string>("image_path", ui->le_imagePath->text().toStdString()));
    camParamsJob.parameters.insert(
          std::pair<std::string, std::string>("camera_model", COLMAP_CAM_MODELS[
                                              ui->cb_camModel->currentIndex()]));
    camParamsJob.parameters.insert(
          std::pair<std::string, std::string>("single_camera",
                                              ui->cb_singleCam->isChecked() ? "1" : "0" ));
    camParamsJob.parameters.insert(
          std::pair<std::string, std::string>("multiple_models",
                                              ui->cb_multiModels->isChecked() ? "1" : "0" ));
    camParamsJob.parameters.insert(
          std::pair<std::string, std::string>("gpus", ui->le_poseGpus->text().toStdString()));
    mNewJobList.push_back(camParamsJob);
  }

  //--- create job to compute dense point cloud if applicable
  if(ui->cb_prodPointCloud->isChecked())
  {
    ColmapWrapper::SJob pointCloudJob = createJob(ColmapWrapper::DENSE_CLOUD);

    pointCloudJob.parameters.insert(
          std::pair<std::string, std::string>("max_img_size",
                                              QString::number(ui->sb_maxImgSize->value()).toStdString()));
    pointCloudJob.parameters.insert(
          std::pair<std::string, std::string>("cache_size",
                                              QString::number(ui->sb_cacheSize->value()).toStdString()));
    pointCloudJob.parameters.insert(
          std::pair<std::string, std::string>("gpus", ui->le_cloudGpus->text().toStdString()));
    mNewJobList.push_back(pointCloudJob);
  }

  //--- create job to compute mesh if applicable
  if(ui->cb_prodMesh->isChecked())
  {
    ColmapWrapper::SJob meshJob = createJob(ColmapWrapper::MESHED_MODEL);

    mNewJobList.push_back(meshJob);
  }
}

//==================================================================================================
void NewProductDialog::onUpdateToDarkTheme()
{
  ui->pb_importImagePath->setIcon(QIcon(":/assets/icons/glyphicons-359-file-import-dark.png"));
  ui->pb_selectImagePath->setIcon(QIcon(":/assets/icons/glyphicons-145-folder-open-dark.png"));
}

//==================================================================================================
void NewProductDialog::onUpdateToLightTheme()
{
  ui->pb_importImagePath->setIcon(QIcon(":/assets/icons/glyphicons-359-file-import.png"));
  ui->pb_selectImagePath->setIcon(QIcon(":/assets/icons/glyphicons-145-folder-open.png"));
}

//==================================================================================================
void NewProductDialog::onShow()
{
  //--- clear previous configuration
  mAvailableSeqs.clear();
  ui->cb_prodCameraPoses->setChecked(false);
  ui->cb_prodCameraPoses->setEnabled(true);
  ui->cb_prodPointCloud->setChecked(false);
  ui->cb_prodPointCloud->setEnabled(true);
  ui->cb_prodMesh->setChecked(false);
  ui->gb_settingsCamPoses->setVisible(false);
  ui->gb_settingsPointCloud->setVisible(false);
  ui->gb_settingsMesh->setVisible(false);

  //--- get list of sequences already defined, i.e. in finished seq and pending jobs
  mAvailableSeqs = mpColmapWrapper->getFinishedSequenceList();
  std::vector<ColmapWrapper::SJob> jobList = mpColmapWrapper->getJobList();
  for(ColmapWrapper::SJob job : jobList)
  {
    ColmapWrapper::SSequence seq;
    seq.name = job.sequenceName;
    seq.imagePath = (mpColmapWrapper->connectionType() == ColmapWrapper::LOCAL) ?
        mpColmapWrapper->localWorkspacePath().toStdString() + seq.name + ".images" :
        mpColmapWrapper->remoteWorkspacePath().toStdString() + seq.name + ".images" ;
    mAvailableSeqs.push_back(seq);
  }

  //--- enable pushbutton only if connection type is local
  ui->pb_selectImagePath->setEnabled(mpColmapWrapper->connectionType() == ColmapWrapper::LOCAL);

  //--- add squence list to combo box
  for(ColmapWrapper::SSequence seq : mAvailableSeqs)
  {
    if(ui->cb_sequenceName->findText(QString::fromStdString(seq.name)) == -1)
      ui->cb_sequenceName->addItem(QString::fromStdString(seq.name));
  }
  ui->cb_sequenceName->setCurrentIndex(-1);
}

//==================================================================================================
void NewProductDialog::onCbEditTextChanged(QString editText)
{
  //--- if edit text is not empty, enable push button to import images
  if(!editText.isEmpty())
    ui->pb_importImagePath->setEnabled(true);
  else
    ui->pb_importImagePath->setEnabled(false);
}

//==================================================================================================
std::vector<ColmapWrapper::SJob> NewProductDialog::getNewJobList() const
{
  return mNewJobList;
}

} // namespace colmapwrapper
} // namespace ui
} // namespace ots
} // namespace lib3d
