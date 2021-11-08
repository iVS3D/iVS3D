#ifndef RECONSTRUCTIONTOOLSDIALOG_H
#define RECONSTRUCTIONTOOLSDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QStringListModel>
#include <QAbstractItemView>
#include <QFileDialog>
#include <QInputDialog>

#include "applicationsettings.h"


namespace Ui {
class ReconstructionToolsDialog;
}

/**
 * @class ReconstructionToolsDialog
 *
 * @ingroup View
 *
 * @brief The ReconstructionToolsDialog class is responisble for displaying, adding and removing reconstruction tools.
 *
 * The dialog allows the user to change reconstruction tools stored in ApplicationSettings::instance(). Tools can be added
 * and removed. Their display name within the application can bee changed as well.
 *
 * @author Dominik Wüst
 *
 * @date 2021/02/26
 */
class ReconstructionToolsDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Create a ReconstructionToolsDialog and initialize with the tools listed in ApplicationSettings::instance().
     * @param parent the parent QWidget for displaying the dialog
     */
    explicit ReconstructionToolsDialog(QWidget *parent = nullptr);
    ~ReconstructionToolsDialog();

private slots:
    void on_pushButton_2_clicked();
    void on_pushButton_add_clicked();
    void on_listView_doubleClicked(const QModelIndex &index);

private:
    Ui::ReconstructionToolsDialog *ui;
    QStringListModel *m_model;
    QString connectKeyValue(const QString &key, const QString &value);
    QString getKey(QString keyvalue);
    void onToolsListChanged();
};

#endif // RECONSTRUCTIONTOOLSDIALOG_H
