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

void ExportWidget::setResolutionList(QStringList resList, int idx)
{
    Q_ASSERT(!resList.empty());
    Q_ASSERT(idx>=0);
    Q_ASSERT(idx < resList.size());

    ui->comboBox->clear();
    ui->comboBox->addItems(resList);
    ui->comboBox->setCurrentIndex(idx);
}

void ExportWidget::setResolution(QString resolution)
{
    ui->comboBox->setEditText(resolution);
}

void ExportWidget::setOutputPath(QString path)
{
    ui->lineEdit->setText(path);
}

void ExportWidget::enableExport(bool enabled)
{
    ui->pushButton_export->setEnabled(enabled);
}

void ExportWidget::enableReconstruct(bool enabled)
{
    ui->pushButton_reconstruct->setEnabled(enabled);
}

void ExportWidget::setResolutionValid(bool valid)
{
    QPalette colorPalette = this->ui->comboBox->palette();
    if (valid) {
        ApplicationSettings as = ApplicationSettings::instance();
        if (as.getActiveStyle()) {
            //darkstyle on
            colorPalette.setColor(QPalette::Text, Qt::white);
        }
        else {
            //darkstyle off
            colorPalette.setColor(QPalette::Text, Qt::black);
        }
    }
    else {
        colorPalette.setColor(QPalette::Text, Qt::red);
    }
    this->ui->comboBox->setPalette(colorPalette);
}

bool ExportWidget::getCropStatus()
{
    if (this->ui->checkBox->checkState() == 0) {
        return false;
    }
    return true;
}

void ExportWidget::setCropStatus(bool checked)
{
    if (checked) {
        this->ui->checkBox->setCheckState(Qt::Checked);
        return;
    }
    this->ui->checkBox->setCheckState(Qt::Unchecked);
    return;

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
    QString newPath = QFileDialog::getExistingDirectory(this,"choose output folder", ui->lineEdit->text());
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

void ExportWidget::on_pushButton_cropExport_clicked()
{
    emit sig_cropExport();
}

void ExportWidget::on_pushButton_addAuto_clicked()
{
    emit sig_addAuto();
}

void ExportWidget::on_lineEdit_textChanged(const QString &text)
{
    emit sig_pathChanged(text);
}

void ExportWidget::on_comboBox_currentTextChanged(const QString &text)
{
    emit sig_resChanged(text);
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
