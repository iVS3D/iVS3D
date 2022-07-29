#include "videoplayer.h"


VideoPlayer::VideoPlayer(QWidget *parent, bool dark) :
    QWidget(parent),
    ui(new Ui::VideoPlayer)
{
    ui->setupUi(this);
    ui->graphicsView ->setScene(new QGraphicsScene(this));
    ui->graphicsView->setAcceptDrops(false);
    ui->widget->setLayout(new QVBoxLayout(this));
    ui->widget->layout()->setContentsMargins(0,0,0,0);
    m_dark = dark;
    QString col = m_dark ? "W" : "B";
    ui->pushButton->setIcon(QIcon(":/icons/trashIcon" + col));
    ui->pushButton_firstPic->setIcon(QIcon(":/icons/fastRewindIcon" + col));
    ui->pushButton_lastPic->setIcon(QIcon(":/icons/fastForwardIcon" + col));
    ui->pushButton_nextPic->setIcon(QIcon(":/icons/nextIcon" + col));
    ui->pushButton_prevPic->setIcon(QIcon(":/icons/prevIcon" + col));
    ui->pushButton_playPause->setIcon(QIcon(":/icons/playIcon" + col));

    m_nextSC = new QShortcut(QKeySequence(/*Qt::CTRL + */Qt::Key_Right), this);
    m_prevSC = new QShortcut(QKeySequence(/*Qt::CTRL + */Qt::Key_Left), this);

    connect(m_nextSC, &QShortcut::activated, this, &VideoPlayer::on_pushButton_nextPic_clicked);
    connect(m_prevSC, &QShortcut::activated, this, &VideoPlayer::on_pushButton_prevPic_clicked);
}


VideoPlayer::~VideoPlayer()
{
    delete ui->graphicsView->scene();
    delete ui;
}


void VideoPlayer::showImages(std::vector<cv::Mat*> images)
{
    ui->graphicsView->scene()->clear();

    if(images.size() == 0){
        ui->graphicsView->show();
        return;
    }
    QPixmap pixmap;
    if(images.size()>1){
        // display images as overlay
        cv::Mat img;
        alphaBlend(images[1], images[0], .5, img);

        pixmap = QPixmap::fromImage(qImageFromCvMat(&img, false));
    } else {
        // only one image to display
        pixmap = QPixmap::fromImage(qImageFromCvMat(images[0]));
    }
    ui->graphicsView->scene()->addPixmap(pixmap);
    ui->graphicsView->scene()->setSceneRect(pixmap.rect());
    ui->graphicsView->fitInView(pixmap.rect(), Qt::KeepAspectRatio);
    ui->graphicsView->show();
}

void VideoPlayer::showImage(cv::Mat *image)
{
    std::vector<cv::Mat *> vec;
    vec.push_back(image);
    showImages(vec);
}


void VideoPlayer::setKeyframe(bool isKeyframe)
{
    ui->graphicsView->setStyleSheet(isKeyframe ? "background-color:black; border: 3px solid red;" : "background-color:black; border: 3px solid black;");
    ui->pushButton_setKeyframe->setText(isKeyframe ? "Clear Keyframe" : "Set Keyframe");
}


void VideoPlayer::setKeyframeCount(unsigned int keyframeCount)
{
    ui->label_keyframeCount->setText(QString::number(keyframeCount));
}


void VideoPlayer::setEnabledBackBtns(bool enabled)
{
    ui->pushButton_firstPic->setEnabled(enabled);
    ui->pushButton_prevPic->setEnabled(enabled);
    m_prevSC->setEnabled(enabled);
}


void VideoPlayer::setEnabledForwardBtns(bool enabled)
{
    ui->pushButton_lastPic->setEnabled(enabled);
    ui->pushButton_nextPic->setEnabled(enabled);
    m_nextSC->setEnabled(enabled);
}


void VideoPlayer::setPlaying(bool playing)
{
    QString col = m_dark ? "W" : "B";
    ui->pushButton_playPause->setIcon(playing ? QIcon(":/icons/pauseIcon" + col) : QIcon(":/icons/playIcon" + col));
}


void VideoPlayer::setStepsize(unsigned int stepsize)
{
    ui->spinBox_stepsize->setValue(stepsize);
}

void VideoPlayer::setKeyframesOnly(bool checked)
{
    ui->checkBox_onlyKeyframes->setChecked(checked);
}


void VideoPlayer::addWidgetToLayout(QWidget *widget)
{
    ui->widget->layout()->addWidget(widget);
}


void VideoPlayer::removeWidgetFromLayout(QWidget *widget)
{
    ui->widget->layout()->removeWidget(widget);
}


void VideoPlayer::resizeEvent(QResizeEvent *)
{
    ui->graphicsView->fitInView(ui->graphicsView->scene()->sceneRect(), Qt::KeepAspectRatio);
    ui->graphicsView->show();
}

/*
 * slots for the ui elements, all just delegate their signals
 */

void VideoPlayer::on_pushButton_firstPic_clicked()
{
    emit sig_showFirstImage();
}

void VideoPlayer::on_pushButton_prevPic_clicked()
{
    emit sig_showPreviousImage();
}

void VideoPlayer::on_pushButton_playPause_clicked()
{
    emit sig_play();
}

void VideoPlayer::on_pushButton_nextPic_clicked()
{
    emit sig_showNextImage();
}

void VideoPlayer::on_pushButton_lastPic_clicked()
{
    emit sig_showLastImage();
}

void VideoPlayer::on_checkBox_onlyKeyframes_stateChanged(int arg1)
{
    emit sig_toggleKeyframesOnly(arg1);
}

void VideoPlayer::on_pushButton_setKeyframe_clicked()
{
    emit sig_toggleKeyframes();
}

void VideoPlayer::on_spinBox_stepsize_valueChanged(int arg1)
{
    emit sig_changeStepsize(arg1);
}

void VideoPlayer::on_pushButton_clicked()
{
    emit sig_deleteAllKeyframes();
}

QImage VideoPlayer::qImageFromCvMat(cv::Mat* input, bool bgr)
{
    cv::Mat* rgb = input;

    if(rgb->channels() == 4)
    {
        if (bgr)
        {
            cv::cvtColor(*input, *rgb, cv::COLOR_BGRA2RGBA);
        }

        return QImage(rgb->data, rgb->cols, rgb->rows, static_cast<int>(rgb->step), QImage::Format_RGBA8888).copy();
    }
    else if (rgb->channels() == 3)
    {
        if (bgr)
        {
            cv::cvtColor(*input, *rgb, cv::COLOR_BGR2RGB);
        }

        return QImage(rgb->data, rgb->cols, rgb->rows, static_cast<int>(rgb->step), QImage::Format_RGB888).copy();
    }
    else if (rgb->channels() == 1)
    {
        return QImage(rgb->data, rgb->cols, rgb->rows, static_cast<int>(rgb->step), QImage::Format_Grayscale8).copy();
    }

    return QImage();
}

void VideoPlayer::alphaBlend(cv::Mat *foreground, cv::Mat *background, float alpha, cv::Mat &output)
{
    output = alpha*(*foreground)+(1-alpha)*(*background);
}
