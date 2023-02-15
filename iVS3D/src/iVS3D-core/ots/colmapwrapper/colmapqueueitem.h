#ifndef LIB3D_OTS_UI_COLMAPWRAPPER_QUEUEITEM_H
#define LIB3D_OTS_UI_COLMAPWRAPPER_QUEUEITEM_H

// Qt
#include <QWidget>
#include <QTime>

#include "../colmapwrapper.h"

namespace lib3d {
namespace ots {
namespace ui {
namespace colmapwrapper {

namespace Ui {
  class QueueItem;
  class QueueItemRunning;
  class QueueItemFinished;
  class QueueItemFailed;

}

/**
 * @brief Class providing queue item for work queue widget
 * @author Ruf, Boitumelo <boitumelo.ruf@iosb.fraunhofer.de>
 */
class QueueItem : public QWidget
{
  Q_OBJECT

    //--- METHOD DECLERATION ---//
  public:
    explicit QueueItem(ColmapWrapper::SJob mJob, QWidget *parent = 0);
    ~QueueItem();


  signals:
    void bumpDownJob(const ColmapWrapper::SJob mJob);
    void bumpUpJob(const ColmapWrapper::SJob mJob);
    void cancelJob(const ColmapWrapper::SJob mJob);
    void deleteJob(const ColmapWrapper::SJob mJob);
    void editJob(const ColmapWrapper::SJob mJob);

  public slots:
    void onUpdateToDarkTheme();
    void onUpdateToLightTheme();


  private slots:
    void onBtnUpClicked();
    void onBtnDownClicked();
    void onBtnOptionsClicked();
    void onBtnCancelClicked();

    //--- MEMBER DECLERATION ---//

  private:
    ColmapWrapper::SJob mJob;
    Ui::QueueItem *ui;
};

/**
 * @brief Class providing running queue item for work queue widget
 * @author Wickersheim, Dennis
 * @author Ruf, Boitumelo <boitumelo.ruf@iosb.fraunhofer.de>
 */
class QueueItemActive : public QWidget
{
  Q_OBJECT

    //--- METHOD DECLERATION ---///

  public:
    explicit QueueItemActive(ColmapWrapper::SJob job, QWidget *parent = 0);
    ~QueueItemActive();

    void setProgress(int progress);

  signals:
    void cancel();

  public slots:
    void onUpdateToDarkTheme();
    void onUpdateToLightTheme();

  private slots:
    void onBtnOptionsClicked();

    //--- MEMBER DECLERATION ---//

  private:
    ColmapWrapper::SJob job;
    Ui::QueueItemRunning *ui;
};

/**
 * @brief Class providing finished queue item for work queue widget
 * @author Ruf, Boitumelo <boitumelo.ruf@iosb.fraunhofer.de>
 */
class QueueItemFinished : public QWidget
{
  Q_OBJECT

    //--- METHOD DECLERATION ---///

  public:
    explicit QueueItemFinished(ColmapWrapper::SJob job, QWidget *parent = 0);
    ~QueueItemFinished();

    void set(ColmapWrapper::SJob job);

  signals:
    void deleteJob(const ColmapWrapper::SJob);

  public slots:
    void onUpdateToDarkTheme();
    void onUpdateToLightTheme();

  private slots:

    void onBtnDeleteClicked();

    //--- MEMBER DECLERATION ---//

  private:
    ColmapWrapper::SJob job;
    Ui::QueueItemFinished *ui;
};

/**
 * @brief Class providing failed queue item for work queue widget
 * @author Hermann, Max <max.hermann@iosb.fraunhofer.de>
 */
class QueueItemFailed : public QWidget
{
  Q_OBJECT

    //--- METHOD DECLERATION ---///

  public:
    explicit QueueItemFailed(ColmapWrapper::SJob job, QWidget *parent = 0);
    ~QueueItemFailed();

    void set(ColmapWrapper::SJob job);

  signals:
    void deleteJob(const ColmapWrapper::SJob);

  public slots:
    void onUpdateToDarkTheme();
    void onUpdateToLightTheme();

  private slots:

    void onBtnDeleteClicked();

    //--- MEMBER DECLERATION ---//

  private:
    ColmapWrapper::SJob job;
    Ui::QueueItemFailed *ui;
};

} // namespace colmapwrapper
} // namespace ui
} // namespace ots
} // namespeace lib3d

#endif // LIB3D_OTS_UI_COLMAPWRAPPER_QUEUEITEM_H
