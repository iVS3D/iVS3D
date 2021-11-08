#include "reconstructiontoolsdialog.h"
#include "ui_reconstructiontoolsdialog.h"

ReconstructionToolsDialog::ReconstructionToolsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReconstructionToolsDialog)
{
    ui->setupUi(this);
    QMap<QString, QString> tools = ApplicationSettings::instance().getReconstructPath();
    QStringList toolList;
    QList<QString> keys = tools.keys();
    for(const QString &key : qAsConst(keys)){
        toolList.append(connectKeyValue(key,tools.value(key)));
    }
    m_model = new QStringListModel(this);
    m_model->setStringList(toolList);
    ui->listView->setModel(m_model);
    onToolsListChanged();
}

ReconstructionToolsDialog::~ReconstructionToolsDialog()
{
    delete ui;
    delete m_model;
}

void ReconstructionToolsDialog::on_pushButton_2_clicked()
{
    QVariant variant = m_model->data(ui->listView->currentIndex());
    QString key = getKey(variant.toString());
    if(ApplicationSettings::instance().removeReconstructPath(key)){
        m_model->removeRows(ui->listView->currentIndex().row(), 1);
        onToolsListChanged();
    }
}

void ReconstructionToolsDialog::on_pushButton_add_clicked()
{
    QString reconstructPath = QFileDialog::getOpenFileName (this, "Choose reconstruction software", ApplicationSettings::instance().getStandardInputPath(), "*.exe *.bat");
    if (reconstructPath == nullptr) {
        return;
    }
    int slashIndex = reconstructPath.lastIndexOf("/");
    QString name = reconstructPath.mid(slashIndex + 1);
    name.chop(4);
    ApplicationSettings::instance().addReconstructPath(name, reconstructPath);

    int row = m_model->rowCount();
    m_model->insertRows(row,1);
    QModelIndex index = m_model->index(row);
    m_model->setData(index, connectKeyValue(name,reconstructPath));
    onToolsListChanged();
}

QString ReconstructionToolsDialog::connectKeyValue(const QString &key, const QString &value)
{
    return key + "\t[" + value + "]";
}

QString ReconstructionToolsDialog::getKey(QString keyvalue)
{
    return keyvalue.split("\t[", QString::SplitBehavior::SkipEmptyParts)[0];
}

void ReconstructionToolsDialog::onToolsListChanged()
{
    if(int row = m_model->rowCount()){
        ui->pushButton_2->setEnabled(true);
        QModelIndex index = m_model->index(row-1);
        ui->listView->setCurrentIndex(index);
    } else {
        ui->pushButton_2->setEnabled(false);
    }
}

void ReconstructionToolsDialog::on_listView_doubleClicked(const QModelIndex &index)
{
    bool ok;
    QString name = getKey(m_model->data(index).toString());
    QString text = QInputDialog::getText(this, tr("Change display name"),
                                             QString("Display name for ").append(name), QLineEdit::Normal,
                                             name, &ok);
    if (ok && !text.isEmpty()){
        QString path = ApplicationSettings::instance().getReconstructPath()[name];
        ApplicationSettings::instance().removeReconstructPath(name);
        ApplicationSettings::instance().addReconstructPath(text, path);
        m_model->setData(index,connectKeyValue(text,path));
    }
}
