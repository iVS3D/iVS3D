#include "mainwindow.h"
#include "ui_mainwindow.h"


#include <QDesktopWidget>
#include <QLayout>
#include <QApplication>
#include <QSplitter>
#include "view/about.h"
#include "view/licencedialog.h"
#include "view/darkstyle/DarkStyle.h"
#include <QMimeData>

//for debug
#include <opencv2/core.hpp>
#include <QDebug>



MainWindow::MainWindow(QWidget *parent, bool dark, int cuda, bool createLog, bool horizontal, QStringList algorithmList, QStringList transformList)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->actionuse_DarkStyle->setCheckable(true);

    // --- create GUI Elements VideoPlayer, Timeline and Info-Widgets with Buttons
    // --- for sampling and export of images or 3d-reconstruction

    // all GUI elements have main window as parent
    m_videoplayer = new VideoPlayer(this, dark);
    m_timeline = new Timeline(this);
    m_inputWidget = new InfoWidget(this, "Input", dark);
    m_samplingWidget = new SamplingWidget(this,"Sampling", algorithmList, transformList);
    m_outputWidget = new OutputWidget(this, "Output", transformList);
    m_autoWidget = new AutomaticWidget(this);

    // initialize the layout of GUI elements
    m_videoplayer->addWidgetToLayout(m_timeline); // timeline is part of videoplayer-layout

    m_horizontalLayout = horizontal;
    m_vpSplitter = new QSplitter(Qt::Vertical, this);
    m_vpSplitter->addWidget(m_videoplayer);
    m_infoSplitter = new QSplitter(Qt::Horizontal, this);
    m_infoSplitter->setContentsMargins(10,0,10,0);
    InputAutomaticWidget* inputAutoWidget = new InputAutomaticWidget(this, m_inputWidget, m_autoWidget);
    m_infoSplitter->addWidget(inputAutoWidget);
    m_infoSplitter->addWidget(m_samplingWidget);
    m_infoSplitter->addWidget(m_outputWidget);
    m_vpSplitter->addWidget(m_infoSplitter);
    setCentralWidget(m_vpSplitter);
    if(m_horizontalLayout){
        setHorizontalLayout();
    } else {
        setVerticalLayout();
    }

    // connect input widget
    connect(m_inputWidget, &InfoWidget::sig_openFolderPressed, this, &MainWindow::on_actionOpen_Input_triggered);
    connect(m_inputWidget, &InfoWidget::sig_openVideoPressed, this, &MainWindow::on_actionOpen_Input_Video_triggered);

    //this->setHorizontalLayout();
    this->showProjectTitle();


    //This is to ensure opencv is incorperated correctly
    cv::Mat myMat;

    // --- initialize GUI elements with placeholders
    m_inputWidget->setInfo(QMap<QString,QString>());

    // --- disable all GUI elements
    // --- until some image data is loaded
    m_timeline->setEnabled(false);
    m_videoplayer->setEnabled(false);
    m_inputWidget->setEnabled(true);
    m_samplingWidget->setEnabled(false);
    m_outputWidget->setEnabled(false);
    m_autoWidget->setEnabled(false);

    // --- use dark style if selected
    if(dark){
        QApplication::setStyle(new DarkStyle);
        ui->actionuse_DarkStyle->setChecked(true);
    }
    if(cuda < 0){
        ui->actionUse_CUDA->setEnabled(false);
    } else {
        ui->actionUse_CUDA->setEnabled(true);
        ui->actionUse_CUDA->setChecked(cuda);
    }

    // set create log file
    ui->actionCreate_log_file->setChecked(createLog);

    //enable Drag and Drop
    setAcceptDrops(true);
    //disable automatic execution
    ui->actionStartAutoExec->setEnabled(false);

    QTimer::singleShot(0, this, &MainWindow::showMaximized);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_videoplayer;
    delete m_timeline;
    delete m_inputWidget;
    delete m_samplingWidget;
    delete m_outputWidget;


}


void MainWindow::center()
{
    this->setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            this->size(),
                    qApp->desktop()->geometry()));
}

void MainWindow::showProjectTitle(const QString &title)
{
    this->setWindowTitle(m_appName + " [" + title + "]");
}


void MainWindow::setHorizontalLayout()
{
    m_vpSplitter->setOrientation(Qt::Vertical);
    m_infoSplitter->setOrientation(Qt::Horizontal);
    adjustSize();
}


void MainWindow::setVerticalLayout()
{

    m_vpSplitter->setOrientation(Qt::Horizontal);
    m_infoSplitter->setOrientation(Qt::Vertical);
    adjustSize();
}


void MainWindow::slot_displayStatusMessage(QString message)
{
    statusBar()->showMessage(message);
}


VideoPlayer *MainWindow::getVideoPlayer()
{
    return m_videoplayer;
}


Timeline *MainWindow::getTimeline()
{
    return m_timeline;
}

OutputWidget *MainWindow::getOutputWidget()
{
    return m_outputWidget;
}

AutomaticWidget *MainWindow::getAutoWidget()
{
    return m_autoWidget;
}


InfoWidget *MainWindow::getInputWidget()
{
    return m_inputWidget;
}


SamplingWidget *MainWindow::getSamplingWidget()
{
    return m_samplingWidget;
}


void MainWindow::enableSaveProject(bool status)
{
    ui->actionSave_Project->setEnabled(status);
    ui->actionSave_Project_As->setEnabled(status);
}


void MainWindow::on_actionOpen_Project_triggered()
{
    emit sig_openProject();
}

void MainWindow::on_actionSave_Project_triggered()
{
    emit sig_saveProject();
}

void MainWindow::on_actionSave_Project_As_triggered()
{
    emit sig_saveProjectAs();
}

void MainWindow::on_actionOpen_Input_triggered()
{
    emit sig_openInputFolder();
}

void MainWindow::on_actionSet_Reconstructiontool_Path_triggered()
{
    emit sig_changeReconstructPath();
}

void MainWindow::on_actionSet_Input_Path_triggered()
{
    emit sig_changeDefaultInputPath();
}

void MainWindow::on_actionChange_layout_style_triggered()
{
    m_horizontalLayout = !m_horizontalLayout;
    if (m_horizontalLayout) {
        setHorizontalLayout();
    }
    else {
        setVerticalLayout();
    }
    emit sig_changeLayoutStyle(m_horizontalLayout);
}

void MainWindow::on_actionInfo_triggered()
{
    //open About Dialog
    QDialog *aboutDialog = new About(this);
    aboutDialog->exec();
}

void MainWindow::on_actionOpen_Input_Video_triggered()
{
    emit sig_openInputVideo();
}

void MainWindow::on_actionHelp_triggered()
{
    //open Help Dialog
    QDialog *help = new helpDialog(this);
    help->exec();
}


std::vector<uint> MainWindow::generateKeyframes(uint totalFrames, uint keyframeCount)
{
    std::vector<uint> keyframes;
    // reset vector
    keyframes.clear();

    while (keyframeCount != 0) {
        uint index = rand() % totalFrames;
        if (keyframes.empty() || std::find(keyframes.begin(), keyframes.end(), index) == keyframes.end()) {
            // the frame is not a keyframe
            keyframes.push_back(index);
            keyframeCount--;
        }
    }

    std::sort(keyframes.begin(), keyframes.end());

    return keyframes;
}

void MainWindow::on_actionuse_DarkStyle_toggled(bool checked)
{
    emit sig_changeDarkStyle(checked);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QList<QUrl> &url = event->mimeData()->urls();
    if (url.length() <= 0) {
        return;
    }
    QString filePath = url[0].toLocalFile();
    qDebug() << "Dropped file:" << filePath;
    emit sig_openVideoDragAndDrop(filePath);
}

void MainWindow::on_actionUse_CUDA_triggered()
{
    emit sig_changeUseCuda(ui->actionUse_CUDA->isChecked());
}

void MainWindow::on_actionLicence_triggered()
{
    //open licence Dialog
    QDialog *licence = new LicenceDialog(this);
    licence->exec();
}


void MainWindow::on_actionCreate_log_file_triggered()
{
    emit sig_changeCreateLogFile(ui->actionCreate_log_file->isChecked());
}
