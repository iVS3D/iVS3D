#ifndef LIB3D_OTS_UI_COLMAPWRAPPER_VIEWWIDGET_H
#define LIB3D_OTS_UI_COLMAPWRAPPER_VIEWWIDGET_H

// Std
#include <string>
#include <map>

// Qt
#include <QWidget>
#include <QTreeWidget>


#include "../colmapwrapper.h"

namespace lib3d {
namespace ots {
namespace ui {
namespace colmapwrapper {

namespace Ui {
  class ViewWidget;
  class TreeItem;
}

/**
 * @brief Class providing widget interaction with ColmapWrapper.
 * Showing product work queue and finished products.
 * @author Ruf, Boitumelo <boitumelo.ruf@iosb.fraunhofer.de>
 */
class ViewWidget : public QWidget
{
  Q_OBJECT

    //--- METHOD DECLERATION ---//

  public:
    explicit ViewWidget(ColmapWrapper *ipWrapper, QWidget *parent = 0);
    ~ViewWidget();

  public slots:
    void refreshProductList();
    void refreshJobQueue();
    void refreshWorkerState();
    void refreshWorkspaceStatus();
    void bumpJobUp(const ColmapWrapper::SJob &job);
    void bumpJobDown(const ColmapWrapper::SJob &job);
    void editJob(const ColmapWrapper::SJob &job);
    void deleteJob(const ColmapWrapper::SJob  &job);
    void customTreeWidgetContextMenu(const QPoint&);

  private slots:

    void onTreeItemDoubleClicked(QTreeWidgetItem *item, int column);

    void onTreeItemClicked(QTreeWidgetItem *item, int column);

    void onClearHistoryClicked();

    void onUpdateToDarkTheme();
    void onUpdateToLightTheme();

    void onOpenActionTriggered(const ColmapWrapper::EProductType iProdType,
                               const QString iFilePath);

    //--- MEMBER DECLERATION ---//

  private:
    Ui::ViewWidget *ui;

    /// Member pointer to wrapper
    lib3d::ots::ColmapWrapper* mpColmapWrapper;

    std::map<std::string, QTreeWidgetItem *> projectsMap;

    lib3d::ots::ui::ETheme mCurrentTheme;
};

} // namespace colmapwrapper
} // namespace ui
} // namespace ots
} // namespeace lib3d

#endif // LIB3D_OTS_UI_COLMAPWRAPPER_VIEWWIDGET_H
