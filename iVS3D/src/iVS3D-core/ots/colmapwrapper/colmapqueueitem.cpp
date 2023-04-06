#include "colmapqueueitem.h"

// std
#include <iostream>
#include <stdio.h>
// Qt
#include <QPushButton>

#include "ui_colmapqueueitem.h"
#include "ui_colmapqueueitem_failed.h"
#include "ui_colmapqueueitem_finished.h"
#include "ui_colmapqueueitem_running.h"

namespace lib3d {
namespace ots {
namespace ui {
namespace colmapwrapper {

//==================================================================================================
QueueItem::QueueItem(ColmapWrapper::SJob job, QWidget *parent)
    : QWidget(parent), ui(new Ui::QueueItem)
{
    mJob = job;
    ui->setupUi(this);
    ui->l_name->setText(QString::fromStdString(job.sequenceName)
                            .append(": ")
                            .append(ColmapWrapper::EProductType2QString(job.product)));

    //--- connect buttons to slots
    connect(ui->btnCancel, &QPushButton::clicked, this, &QueueItem::onBtnCancelClicked);
}

//==================================================================================================
QueueItem::~QueueItem()
{
    delete ui;
}

//==================================================================================================
void QueueItem::onUpdateToDarkTheme()
{
    ui->btnCancel->setIcon(QIcon(":/assets/icons/glyphicons-17-bin-dark.png"));
}

//==================================================================================================
void QueueItem::onUpdateToLightTheme()
{
    ui->btnCancel->setIcon(QIcon(":/assets/icons/glyphicons-17-bin.png"));
}

//==================================================================================================
void QueueItem::onBtnUpClicked()
{
    emit bumpUpJob(mJob);
}

//==================================================================================================
void QueueItem::onBtnDownClicked()
{
    emit bumpDownJob(mJob);
}

//==================================================================================================
void QueueItem::onBtnOptionsClicked()
{
    emit editJob(mJob);
}

//==================================================================================================
void QueueItem::onBtnCancelClicked()
{
    emit deleteJob(mJob);
}

//==================================================================================================
QueueItemActive::QueueItemActive(ColmapWrapper::SJob job, QWidget *parent)
    : QWidget(parent), ui(new Ui::QueueItemRunning)
{
    this->job = job;
    ui->setupUi(this);

    uint milliseconds = job.eta;
    uint seconds = milliseconds / 1000;
    milliseconds = milliseconds % 1000;
    uint minutes = seconds / 60;
    seconds = seconds % 60;
    uint hours = minutes / 60;
    minutes = minutes % 60;

    QTime time;
    time.setHMS(hours, minutes, seconds, milliseconds);

    ui->l_name->setText(QString::fromStdString(job.sequenceName)
                            .append(": ")
                            .append(ColmapWrapper::EProductType2QString(job.product))
                            .append(" ETA for step ")
                            .append(QString::number(job.step))
                            .append(" ")
                            .append(time.toString("hh:mm:ss")));
}

//==================================================================================================
QueueItemActive::~QueueItemActive()
{
    delete ui;
}

//==================================================================================================
void QueueItemActive::setProgress(int progress)
{
    ui->progressBar->setValue(progress);
}

//==================================================================================================
void QueueItemActive::onUpdateToDarkTheme()
{
    ui->btnCancel->setIcon(QIcon(":/assets/icons/glyphicons-17-bin-dark.png"));
}

//==================================================================================================
void QueueItemActive::onUpdateToLightTheme()
{
    ui->btnCancel->setIcon(QIcon(":/assets/icons/glyphicons-17-bin.png"));
}

//==================================================================================================
void QueueItemActive::onBtnOptionsClicked()
{
    emit cancel();
}

//==================================================================================================
QueueItemFinished::QueueItemFinished(ColmapWrapper::SJob job, QWidget *parent)
    : QWidget(parent), ui(new Ui::QueueItemFinished)
{
    this->job = job;
    ui->setupUi(this);

    ui->l_name->setText(QString::fromStdString(job.sequenceName)
                            .append(": ")
                            .append(ColmapWrapper::EProductType2QString(job.product)));
}

//==================================================================================================
QueueItemFinished::~QueueItemFinished()
{
    delete ui;
}

//==================================================================================================
void QueueItemFinished::onUpdateToDarkTheme()
{
    ui->btnDelete->setIcon(QIcon(":/assets/icons/glyphicons-17-bin-dark.png"));
}

//==================================================================================================
void QueueItemFinished::onUpdateToLightTheme()
{
    ui->btnDelete->setIcon(QIcon(":/assets/icons/glyphicons-17-bin.png"));
}

//==================================================================================================
void QueueItemFinished::onBtnDeleteClicked()
{
    emit deleteJob(job);
}

//==================================================================================================
QueueItemFailed::QueueItemFailed(ColmapWrapper::SJob job, QWidget *parent)
    : QWidget(parent), ui(new Ui::QueueItemFailed)
{
    this->job = job;
    ui->setupUi(this);

    ui->l_name->setText(QString::fromStdString(job.sequenceName)
                            .append(": ")
                            .append(ColmapWrapper::EProductType2QString(job.product)));
    connect(ui->btnDelete, &QPushButton::clicked, this, &QueueItemFailed::onBtnDeleteClicked);
}

//==================================================================================================
QueueItemFailed::~QueueItemFailed()
{
    delete ui;
}

//==================================================================================================
void QueueItemFailed::onUpdateToDarkTheme()
{
    ui->btnDelete->setIcon(QIcon(":/assets/icons/glyphicons-17-bin-dark.png"));
}

//==================================================================================================
void QueueItemFailed::onUpdateToLightTheme()
{
    ui->btnDelete->setIcon(QIcon(":/assets/icons/glyphicons-17-bin.png"));
}

//==================================================================================================
void QueueItemFailed::onBtnDeleteClicked()
{
    emit deleteJob(job);
}

} // namespace colmapwrapper
} // namespace ui
} // namespace ots
} // namespace lib3d
