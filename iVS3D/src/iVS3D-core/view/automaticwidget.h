#ifndef AUTOMATICWIDGET_H
#define AUTOMATICWIDGET_H

#include <QWidget>
#include <QListWidgetItem>
#include <QUuid>

#include "automaticlistwidget.h"

namespace Ui {
class AutomaticWidget;
}
/**
 * @class AutomaticWidget
 *
 * @ingroup View
 *
 * @brief The AutomaticWidget class is used to show the current settings of the automatic execution.
 * The execution can be started.
 * One Algorithm can be removed.
 * The current configuration can be saved and loaded.
 *
 * @author Daniel Brommer
 *
 * @date 2021/07/19
 */

class AutomaticWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief AutomaticWidget Constructor connects to the AutomaticListWidget and enables Drag&Drop
     * @param parent
     */
    explicit AutomaticWidget(QWidget *parent = nullptr);
    ~AutomaticWidget();
    /**
     * @brief updateSelectedPlugins Show the given QStrings on the ListWidget
     * @param pluginList QStrings to be shown
     */
    void updateSelectedPlugins(QStringList pluginList);

private slots:
    void on_remove_clicked();
    void on_start_clicked();
    void on_save_clicked();
    void on_load_clicked();
    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);

public slots:
    /**
     * @brief slot_itemDrop Connected to the AutomaticListWidget, called when a drop occured to get the items that changed
     */
    void slot_itemDrop();

signals:
    /**
     * @brief sig_startAutomaticExec Connected to AutomaticExecutor, emited when Start automatic is clicked
     */
    void sig_startAutomaticExec();
    /**
     * @brief sig_removedPlugin Connected to AutomaticExecSettings, emited a algorithm is removed
     * @param row Index of the removed algorithm
     */
    void sig_removedPlugin(int row);
    /**
     * @brief sig_saveConfiguration Connected to AutomaticExecSettings, emited when Save configuration is clicked
     */
    void sig_saveConfiguration();
    /**
     * @brief sig_loadConfiguration Connected to AutomaticExecSettings, emited when Load configuration is clicked
     */
    void sig_loadConfiguration();
    /**
     * @brief sig_doubleClickedItem Connected to AutomaticExecSettings, emited when a algorithm is double clicked
     */
    void sig_doubleClickedItem(int row);
    /**
     * @brief sig_autoOrderChanged Connected to AutomaticExecSettings, emited when two algorithm changed their place
     * @param first First element to swap
     * @param second Second element to swap
     */
    void sig_autoOrderChanged(int first, int second);

private:
    Ui::AutomaticWidget *ui;
    QList<QUuid> m_idList;
};

#endif // AUTOMATICWIDGET_H
