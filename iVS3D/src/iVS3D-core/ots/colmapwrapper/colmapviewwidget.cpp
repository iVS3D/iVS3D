#include "colmapviewwidget.h"

// Std
#include <iostream>
#include <stdio.h>
#include <algorithm>

// Qt
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QTimer>
#include <QDesktopServices>
#include <QUrl>
#include <QMenu>

#include "ui_colmapviewwidget.h"

#include "colmapqueueitem.h"
#include "colmapnewproductdialog.h"
//#include "jobcreationform.h"
//#include "productindex.h"
//#include "widgetjobedit.h"

namespace lib3d {
namespace ots {
namespace ui {
namespace colmapwrapper {

//==================================================================================================
ViewWidget::ViewWidget(ColmapWrapper *ipMsWrapper, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ViewWidget),
    mpColmapWrapper(ipMsWrapper),
    mCurrentTheme(LIGHT)
{
    ui->setupUi(this);
    ui->tw_products->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->tw_products, &QTreeWidget::customContextMenuRequested,
            this, &ViewWidget::customTreeWidgetContextMenu);
    connect(ui->pb_startProcessing, &QPushButton::clicked,
            mpColmapWrapper, &ColmapWrapper::startProcessing);
    connect(ui->pb_syncWorkspace, &QPushButton::clicked,
            mpColmapWrapper, &ColmapWrapper::syncWorkspaceFromServer);
    connect(ui->pb_openLogFile, &QPushButton::clicked,
            mpColmapWrapper, &ColmapWrapper::openColmapLogFile);

    connect(mpColmapWrapper, &ColmapWrapper::sequenceListUpdate,
            this, &ViewWidget::refreshProductList);
    connect(mpColmapWrapper, &ColmapWrapper::jobListUpdate,
            this, &ViewWidget::refreshJobQueue);
    connect(mpColmapWrapper, &ColmapWrapper::workerStateUpdate,
            this, &ViewWidget::refreshWorkerState);
    connect(mpColmapWrapper, &ColmapWrapper::workspaceStatusUpdate,
            this, &ViewWidget::refreshWorkspaceStatus);

    ColmapWrapperControlsFactory* pCtrlFactory = mpColmapWrapper->getOrCreateUiControlsFactory();
    connect(pCtrlFactory, &ColmapWrapperControlsFactory::updateToDarkTheme,
            this, &ViewWidget::onUpdateToDarkTheme);
    connect(pCtrlFactory, &ColmapWrapperControlsFactory::updateToLightTheme,
            this, &ViewWidget::onUpdateToLightTheme);

    this->refreshProductList();
    this->refreshJobQueue();
    this->refreshWorkerState();
    this->refreshWorkspaceStatus();
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
void ViewWidget::deleteJob(const ColmapWrapper::SJob &job) {
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
  for(std::vector<ColmapWrapper::SJob>::iterator jobItr = jobList.begin();
      jobItr != jobList.end(); ++jobItr)
  {
    QListWidgetItem* item = new QListWidgetItem(ui->lw_queue);
    ui->lw_queue->addItem(item);
    switch(jobItr->state)
    {
      default:
      case ColmapWrapper::JOB_PENDING:
      {
        QueueItem *jqi = new QueueItem(*jobItr);
        item->setSizeHint(jqi->minimumSize());
        ui->lw_queue->setItemWidget(item, jqi);
        // TODO: Object::connect(jqi, &QueueItem::editJob, this, &ViewWidget::editJob);
        QObject::connect(jqi, &QueueItem::deleteJob, this, &ViewWidget::deleteJob);
        QObject::connect(jqi, &QueueItem::bumpDownJob, this, &ViewWidget::bumpJobDown);
        QObject::connect(jqi, &QueueItem::bumpUpJob, this, &ViewWidget::bumpJobUp);

        if(mCurrentTheme == DARK)
          jqi->onUpdateToDarkTheme();
        else
          jqi->onUpdateToLightTheme();
      }
      break;

      case ColmapWrapper::JOB_DONE:
      {
        QueueItemFinished *jqi = new QueueItemFinished(*jobItr);
        item->setSizeHint(jqi->minimumSize());
        ui->lw_queue->setItemWidget(item, jqi);
        // TODO:
//          QObject::connect(sqi, SIGNAL(deleteJob(AgisoftIntegration::Job)), this, SLOT(deleteJob(AgisoftIntegration::Job)));

        if(mCurrentTheme == DARK)
          jqi->onUpdateToDarkTheme();
        else
          jqi->onUpdateToLightTheme();

      }
      break;

      case ColmapWrapper::JOB_RUNNING:
      {
        QueueItemActive *jqi = new QueueItemActive(*jobItr);
        jqi->setProgress(jobItr->progress);
        ui->lw_queue->setItemWidget(item, jqi);
        item->setSizeHint(jqi->minimumSize());
        if(mCurrentTheme == DARK)
          jqi->onUpdateToDarkTheme();
        else
          jqi->onUpdateToLightTheme();
      }
      break;

    }
  }
}

//==================================================================================================
void ViewWidget::refreshWorkerState()
{
  switch(mpColmapWrapper->getWorkerState())
  {
    default:
    case ColmapWrapper::WORKER_IDLE:
    {
      ui->l_colmapStatus->setText(tr("Idle"));
      ui->l_colmapStatus->setStyleSheet("QLabel { color : green; }");

      ui->pb_startProcessing->setEnabled(true);
    }
    break;

    case ColmapWrapper::WORKER_RUNNING:
    {
      ui->l_colmapStatus->setText(tr("Running"));
      ui->l_colmapStatus->setStyleSheet("QLabel { color : orange; }");

      ui->pb_startProcessing->setEnabled(false);
    }
    break;

    case ColmapWrapper::WORKER_FAILED:
    {
      ui->l_colmapStatus->setText(tr("Failed"));
      ui->l_colmapStatus->setStyleSheet("QLabel { color : red; }");

      ui->pb_startProcessing->setEnabled(true);
    }
    break;
  }
}

//==================================================================================================
void ViewWidget::refreshWorkspaceStatus()
{
  switch(mpColmapWrapper->getWorkspaceStatus())
  {
    default:
    case ColmapWrapper::IN_SYNC:
    {
      ui->l_workspaceStatus->setText(tr("In Sync"));
      ui->l_workspaceStatus->setStyleSheet("QLabel { color : green; }");

      ui->pb_syncWorkspace->setEnabled(true);
    }
    break;

    case ColmapWrapper::SYNCING:
    {
      ui->l_workspaceStatus->setText(tr("Syncing"));
      ui->l_workspaceStatus->setStyleSheet("QLabel { color : orange; }");

      ui->pb_syncWorkspace->setEnabled(false);
    }
    break;
  }
}

//==================================================================================================
void ViewWidget::refreshProductList()
{
  // TODO: improve by using an underlining tree widget model

  std::vector<ColmapWrapper::SSequence> availableSeqs = mpColmapWrapper->getFinishedSequenceList();
  QTreeWidget *pTreeWidget = ui->tw_products;

  //--- get list of expanded tree items
  std::vector<std::string> expandedItemNames;
  for(int i = 0; i < pTreeWidget->topLevelItemCount(); ++i)
  {
    QTreeWidgetItem* pItem = pTreeWidget->topLevelItem(i);
    if(pItem->isExpanded())
      expandedItemNames.push_back(pItem->text(0).toStdString());
  }

  //--- clear Tree widget
  pTreeWidget->clear();

  //--- loop over available seqences, it is assumed that the seqences are listed alphabetically
  int treeSeqIdx = 0; // index of item in treeWidget
  for(std::vector<ColmapWrapper::SSequence>::iterator seqItr = availableSeqs.begin();
      seqItr != availableSeqs.end(); ++seqItr, ++treeSeqIdx)
  {
    //--- create new item and insert at position
    QTreeWidgetItem *pNewItem = new QTreeWidgetItem(
          (QTreeWidget*) nullptr, QStringList(QString::fromStdString(seqItr->name)));

    for(ColmapWrapper::SProduct product : seqItr->products) {
        QTreeWidgetItem *newChild = new QTreeWidgetItem(
              (QTreeWidget*)0, QStringList(ColmapWrapper::EProductType2QString(product.type)));
        pNewItem->addChild(newChild);
    }

    pTreeWidget->insertTopLevelItem(treeSeqIdx, pNewItem);

    //--- if name is in list expand item
    if(std::find(expandedItemNames.begin(), expandedItemNames.end(), seqItr->name)
       != expandedItemNames.end())
      pTreeWidget->topLevelItem(treeSeqIdx)->setExpanded(true);
  }

// old code
#if 0
  //--- loop over items in list. if in available seqences, update item.
  int itemCount = pTreeWidget->topLevelItemCount();
  for(int itemIdx = 0; itemIdx < itemCount; ++itemIdx) {

    QTreeWidgetItem *pItem = pTreeWidget->topLevelItem(pItem);

    for(ColmapWrapper::SSequence seq : seqsToRemove) {
      if(seq.name == pItem->text(0).toStdString()) {
        if(seq.products.size() == 0) {
          pTreeWidget->removeItemWidget(pItem, 0);
          pItem--; // Increment item index only if list has not changed
          itemCount--; // Keep child count consistent with list length

        } else {
          int childCount = pItem->childCount();
          for(int j = 0; j < childCount; j++) {
            std::cout <<j << "\n";
            QTreeWidgetItem *child = pItem->child(j);
            for(ColmapWrapper::SProduct product : seq.products) {
              if(ColmapWrapper::EProductType2QString(product.type) ==
                 child->text(0)) {
                pItem->removeChild(child);
                j--;
                childCount--;
              }
            }
          }
        }
      }
    }
  }

  // Add items
  for(ColmapWrapper::SSequence seq : seqsToAdd) {
    int count = pTreeWidget->topLevelItemCount();
    bool topLevelItemExisted = false;
    for(int i = 0; i < count; i++) {
      QTreeWidgetItem *item = pTreeWidget->topLevelItem(i);
      if(item->text(0).toStdString() == seq.name) {
        for(ColmapWrapper::SProduct product : seq.products) {
          QTreeWidgetItem *newChild = new QTreeWidgetItem(
                (QTreeWidget*)0,
                QStringList(ColmapWrapper::EProductType2QString(product.type)));
          item->addChild(newChild);
          topLevelItemExisted = true;
        }
        break;
      }
    }
    if(topLevelItemExisted) {
        continue;
    }
    QTreeWidgetItem *newItem = new QTreeWidgetItem((QTreeWidget*) 0,
                                                   QStringList(QString::fromStdString(seq.name)));
    for(ColmapWrapper::SProduct product : seq.products) {
        QTreeWidgetItem *newChild = new QTreeWidgetItem(
              (QTreeWidget*)0,
              QStringList(ColmapWrapper::EProductType2QString(product.type)));
        newItem->addChild(newChild);
    }
    ui->tw_products->insertTopLevelItem(0, newItem);
  }
#endif
}

//==================================================================================================
void ViewWidget::customTreeWidgetContextMenu(const QPoint &pos) {
    QTreeWidgetItem *item = ui->tw_products->itemAt(pos);

    if(item->parent() == NULL) {
        // Context menu for folders
        return;
    }


    // Context menu for products
    ColmapWrapper::EProductType prodType = ColmapWrapper::QString2EProductType(item->text(0));
    QString filePath = mpColmapWrapper->getProductFilePath(
          item->parent()->text(0), prodType);
    QString seqName = item->parent()->text(0);

    QAction *openAction = new QAction(tr("Öffnen"), this);
    connect(openAction, &QAction::triggered, this, [this, filePath, prodType, seqName]()
      {
        onOpenActionTriggered(prodType, filePath);
      });

    // TODO
//    QAction *rerenderAction = new QAction(tr("Neu berechnen"), this);
//    rerenderAction->setEnabled(false);
//    connect(rerenderAction, &QAction::triggered, this, [this, filename, projectname, producttype]() {
//        QFile productFile(filename);
//        productFile.rename(productFile.fileName().append(".old"));
//        Job *job = new Job;
//        job->jobName = projectname;
//        job->productType = producttype;
//        if(WidgetJobEdit(job).exec()) {
//                if(!constDataset->editJob(*job)) {
//                        constDataset->addJob(*job);
//            }
//          this->refresh();
//        }
//    });

    //--- create menu
    QMenu *menu=new QMenu(this);
    menu->addAction(openAction);
//    menu->addAction(rerenderAction);

    menu->popup(ui->tw_products->viewport()->mapToGlobal(pos));
}

//==================================================================================================
void ViewWidget::onTreeItemDoubleClicked(QTreeWidgetItem *item,int column)
{  
  Q_UNUSED(column)

    if(item->parent() == NULL) {
        return;
    }

    QString filename = mpColmapWrapper->getProductFilePath(
          item->parent()->text(0), ColmapWrapper::QString2EProductType(item->text(0)));
    QDesktopServices::openUrl(QUrl(filename));
}

//==================================================================================================
void ViewWidget::onTreeItemClicked(QTreeWidgetItem *item, int column)
{
  Q_UNUSED(item)
  Q_UNUSED(column)
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
  ui->pb_startProcessing->setIcon(QIcon(":/assets/icons/glyphicons-174-play-dark.png"));
  ui->pb_syncWorkspace->setIcon(QIcon(":/assets/icons/glyphicons-82-refresh-dark.png"));
  ui->pb_openLogFile->setIcon(QIcon(":/assets/icons/filetypes-4-log-dark.png"));

  mCurrentTheme = DARK;
}

//==================================================================================================
void ViewWidget::onUpdateToLightTheme()
{
  ui->pb_startProcessing->setIcon(QIcon(":/assets/icons/glyphicons-174-play.png"));
  ui->pb_syncWorkspace->setIcon(QIcon(":/assets/icons/glyphicons-82-refresh.png"));
  ui->pb_openLogFile->setIcon(QIcon(":/assets/icons/filetypes-4-log.png"));

  mCurrentTheme = LIGHT;
}

//==================================================================================================
void ViewWidget::onOpenActionTriggered(const ColmapWrapper::EProductType iProdType,
                                       const QString iFilePath)
{
  //--- if custom import function is set, call it.
  //--- Otherwise call dfault import procedure, i.e. copying of image files.
  std::function<void(ColmapWrapper::EProductType, std::string)> custOpenFn =
      mpColmapWrapper->customProductOpenFn();
  if(custOpenFn)
  {
    custOpenFn(iProdType, iFilePath.toStdString());
  }
  else
  {
    //--- default open routine: open file in explorer
    QDesktopServices::openUrl(QUrl("file:"+iFilePath));
  }
}

} // namespace colmapwrapper
} // namespace ui
} // namespace ots
} // namespeace lib3d
