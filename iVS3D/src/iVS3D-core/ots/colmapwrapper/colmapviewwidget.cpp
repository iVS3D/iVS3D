#include "colmapviewwidget.h"

// Std
#include <algorithm>
#include <iostream>
#include <stdio.h>

// Qt
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QMenu>
#include <QTimer>
#include <QUrl>

#include "ui_colmapviewwidget.h"

#include "colmapnewproductdialog.h"
#include "colmapqueueitem.h"
//#include "jobcreationform.h"
//#include "productindex.h"
//#include "widgetjobedit.h"

namespace lib3d {
namespace ots {
namespace ui {
namespace colmapwrapper {

//==================================================================================================
ViewWidget::ViewWidget(ColmapWrapper *ipMsWrapper, QWidget *parent)
    : QWidget(parent), ui(new Ui::ViewWidget), mpColmapWrapper(ipMsWrapper), mCurrentTheme(LIGHT)
{
    ui->setupUi(this);

    connect(ui->pb_openLogFile,
            &QPushButton::clicked,
            mpColmapWrapper,
            &ColmapWrapper::openColmapLogFile);

    connect(mpColmapWrapper, &ColmapWrapper::jobListUpdate, this, &ViewWidget::refreshJobQueue);
    connect(mpColmapWrapper,
            &ColmapWrapper::workerStateUpdate,
            this,
            &ViewWidget::refreshWorkerState);

    ColmapWrapperControlsFactory *pCtrlFactory = mpColmapWrapper->getOrCreateUiControlsFactory();
    connect(pCtrlFactory,
            &ColmapWrapperControlsFactory::updateToDarkTheme,
            this,
            &ViewWidget::onUpdateToDarkTheme);
    connect(pCtrlFactory,
            &ColmapWrapperControlsFactory::updateToLightTheme,
            this,
            &ViewWidget::onUpdateToLightTheme);

    this->refreshJobQueue();
    this->refreshWorkerState();
}

//==================================================================================================
ViewWidget::~ViewWidget()
{
    delete ui;
}

//==================================================================================================
void ViewWidget::bumpJobUp(const ColmapWrapper::SJob &job)
{
    mpColmapWrapper->moveJobOneUp(job);
    mpColmapWrapper->writeWorkQueueToFile();
    refreshJobQueue();
}

//==================================================================================================
void ViewWidget::bumpJobDown(const ColmapWrapper::SJob &job)
{
    mpColmapWrapper->moveJobOneDown(job);
    mpColmapWrapper->writeWorkQueueToFile();
    refreshJobQueue();
}

//==================================================================================================
void ViewWidget::editJob(const ColmapWrapper::SJob &job)
{
    Q_UNUSED(job)
    //TODO
    //    Job sJob(job);
    //    if(WidgetJobEdit(&sJob).exec()) {
    //        dataset->editJob(sJob);
    //    }
    //    dataset->save_work();
}

//==================================================================================================
void ViewWidget::deleteJob(const ColmapWrapper::SJob &job)
{
    mpColmapWrapper->deleteJob(job);
    mpColmapWrapper->writeWorkQueueToFile();
    refreshJobQueue();
}

//==================================================================================================
void ViewWidget::refreshJobQueue()
{
    ui->lw_queue->clear();
    std::vector<ColmapWrapper::SJob> jobList = mpColmapWrapper->getJobList();

    //--- loop over job list and fill widget
    for (std::vector<ColmapWrapper::SJob>::iterator jobItr = jobList.begin();
         jobItr != jobList.end();
         ++jobItr) {
        QListWidgetItem *item = new QListWidgetItem(ui->lw_queue);
        ui->lw_queue->addItem(item);
        switch (jobItr->state) {
        case ColmapWrapper::JOB_PENDING: {
            QueueItem *jqi = new QueueItem(*jobItr);
            item->setSizeHint(jqi->minimumSize());
            ui->lw_queue->setItemWidget(item, jqi);
            // TODO: Object::connect(jqi, &QueueItem::editJob, this, &ViewWidget::editJob);
            QObject::connect(jqi, &QueueItem::deleteJob, this, &ViewWidget::deleteJob);

            if (mCurrentTheme == DARK)
                jqi->onUpdateToDarkTheme();
            else
                jqi->onUpdateToLightTheme();
        } break;

        case ColmapWrapper::JOB_DONE: {
            QueueItemFinished *jqi = new QueueItemFinished(*jobItr);
            item->setSizeHint(jqi->minimumSize());
            ui->lw_queue->setItemWidget(item, jqi);
            QObject::connect(jqi, &QueueItemFinished::deleteJob, this, &ViewWidget::deleteJob);

            // TODO:
            //          QObject::connect(sqi, SIGNAL(deleteJob(AgisoftIntegration::Job)), this, SLOT(deleteJob(AgisoftIntegration::Job)));

            if (mCurrentTheme == DARK)
                jqi->onUpdateToDarkTheme();
            else
                jqi->onUpdateToLightTheme();

        } break;

        case ColmapWrapper::JOB_RUNNING: {
            QueueItemActive *jqi = new QueueItemActive(*jobItr);
            jqi->setProgress(jobItr->progress);
            ui->lw_queue->setItemWidget(item, jqi);
            item->setSizeHint(jqi->minimumSize());
            if (mCurrentTheme == DARK)
                jqi->onUpdateToDarkTheme();
            else
                jqi->onUpdateToLightTheme();
        } break;

        //case ColmapWrapper::JOB_FAILED:
        default: {
            QueueItemFailed *jqi = new QueueItemFailed(*jobItr);
            item->setSizeHint(jqi->minimumSize());
            ui->lw_queue->setItemWidget(item, jqi);
            item->setSizeHint(jqi->minimumSize());
            QObject::connect(jqi, &QueueItemFailed::deleteJob, this, &ViewWidget::deleteJob);

            if (mCurrentTheme == DARK)
                jqi->onUpdateToDarkTheme();
            else
                jqi->onUpdateToLightTheme();
        } break;
        }
    }
}

//==================================================================================================
void ViewWidget::refreshWorkerState()
{
    switch (mpColmapWrapper->getWorkerState()) {
    default:
    case ColmapWrapper::WORKER_IDLE: {
        ui->l_colmapStatus->setText(tr("Idle"));
        ui->l_colmapStatus->setStyleSheet("QLabel { color : green; }");

    } break;

    case ColmapWrapper::WORKER_RUNNING: {
        ui->l_colmapStatus->setText(tr("Running"));
        ui->l_colmapStatus->setStyleSheet("QLabel { color : orange; }");

    } break;

    case ColmapWrapper::WORKER_FAILED: {
        ui->l_colmapStatus->setText(tr("Failed"));
        ui->l_colmapStatus->setStyleSheet("QLabel { color : red; }");

    } break;
    }
}

//==================================================================================================
void ViewWidget::onClearHistoryClicked()
{
    mpColmapWrapper->removeFinishedJobs();
    refreshJobQueue();
}

//==================================================================================================
void ViewWidget::onUpdateToDarkTheme()
{
    ui->pb_openLogFile->setIcon(QIcon(":/assets/icons/filetypes-4-log-dark.png"));

    mCurrentTheme = DARK;
}

//==================================================================================================
void ViewWidget::onUpdateToLightTheme()
{
    ui->pb_openLogFile->setIcon(QIcon(":/assets/icons/filetypes-4-log.png"));

    mCurrentTheme = LIGHT;
}

//==================================================================================================
void ViewWidget::onOpenActionTriggered(const ColmapWrapper::EProductType iProdType,
                                       const QString iFilePath)
{
    //--- default open routine: open file in explorer
    QDesktopServices::openUrl(QUrl("file:" + iFilePath));
}

} // namespace colmapwrapper
} // namespace ui
} // namespace ots
} // namespace lib3d
