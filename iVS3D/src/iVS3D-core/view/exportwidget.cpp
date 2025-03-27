#include "exportwidget.h"
#include "ui_exportwidget.h"


ExportWidget::ExportWidget(QWidget *parent, QStringList transformList) :
    QWidget(parent),
    ui(new Ui::ExportWidget)
{
    ui->setupUi(this);
    for(const auto &t : transformList){
        auto *cb = new QCheckBox(t);
        m_checkboxes.push_back(cb);
        ui->verticalLayout_transforms->addWidget(cb);
    }
}

ExportWidget::~ExportWidget()
{
    for(auto cb : m_checkboxes){
        delete cb;
    }
    delete ui;
}

void ExportWidget::setOutputPath(QString path)
{
    ui->lineEdit->setText(path);
}

void ExportWidget::enableExport(bool enabled)
{
    ui->pushButton_export->setEnabled(enabled);
}

void ExportWidget::enableExportPathEditable(bool enabled)
{
    ui->lineEdit->setEnabled(enabled);
    ui->lineEdit->setToolTip(enabled ? tr("location to export keyframes to.") : tr("export location ha been passed as a start argument. Thus it can not be changed!"));
    ui->pushButton_browse->setEnabled(enabled);
    ui->pushButton_browse->setToolTip(enabled ? tr("location to export keyframes to.") : tr("export location ha been passed as a start argument. Thus it can not be changed!"));
}

void ExportWidget::enableReconstruct(bool enabled)
{

    (void)enabled;

}

std::vector<bool> ExportWidget::getSelectedITransforms()
{
    std::vector<bool> transforms;
    for(const auto &cb : m_checkboxes){
        transforms.push_back(cb->isChecked());
    }
    return transforms;
}

void ExportWidget::on_pushButton_browse_clicked()
{
    QString newPath = QFileDialog::getExistingDirectory(this,tr("choose output folder"), ui->lineEdit->text(), QFileDialog::DontUseNativeDialog);
    if(!newPath.isEmpty()){
        ui->lineEdit->setText(newPath);
    }
}

void ExportWidget::on_pushButton_export_clicked()
{
    emit sig_export();
}

void ExportWidget::on_pushButton_reconstruct_clicked()
{
    emit sig_reconstruct();
}

void ExportWidget::on_pushButton_addAuto_clicked()
{
    emit sig_addAuto();
}

void ExportWidget::on_lineEdit_textChanged(const QString &text)
{
    emit sig_pathChanged(text);
}

bool ExportWidget::setSelectedITransforms(std::vector<bool> selection)
{
    if(m_checkboxes.size() != selection.size()){
        return false;
    }
    for(int i = 0; i<(int)m_checkboxes.size();i++){
        m_checkboxes[i]->setChecked(selection[i]);
    }
    return true;
}

void ExportWidget::enableCreateFilesWidget(bool enable)
{
    if (enable) {
        ui->createFilesWidget->show();
    }
    else {
        ui->createFilesWidget->hide();
    }
}

