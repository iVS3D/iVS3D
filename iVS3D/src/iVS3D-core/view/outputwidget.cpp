#include "outputwidget.h"

OutputWidget::OutputWidget(QWidget *parent, QString title) : QWidget(parent)
{
    m_exportW = new ExportWidget(this);
    m_exportW->setVisible(true);
    m_progressW = new ProgressWidget(this);
    m_progressW->setVisible(false);

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0,0,0,0);
    m_layout->addWidget(m_exportW);

    setLayout(m_layout);

    connect(m_exportW, &ExportWidget::sig_export, this, &OutputWidget::slot_export);
    connect(m_exportW, &ExportWidget::sig_reconstruct, this, &OutputWidget::slot_reconstruct);
    connect(m_exportW, &ExportWidget::sig_pathChanged, this, &OutputWidget::slot_pathChanged);
    connect(m_exportW, &ExportWidget::sig_addAuto, this, &OutputWidget::slot_addAuto);
    connect(m_progressW, &ProgressWidget::sig_abort, this, &OutputWidget::slot_abort);
    connect(m_exportW, &ExportWidget::sig_resChanged, [=](QString res) {
        emit sig_resChanged(res);
    });
    connect(m_exportW, &ExportWidget::sig_altitudeChanged, [=](double altitude) {
        emit sig_altitudeChanged(altitude);
    });

    this->setMinimumSize(200,250);
}

void OutputWidget::setOutputPath(QString path)
{
    m_exportW->setOutputPath(path);
}

void OutputWidget::enableExport(bool enabled)
{
    m_exportW->enableExport(enabled);
}

void OutputWidget::enableExportPathChange(bool enabled)
{
    m_exportW->enableExportPathEditable(enabled);
}

void OutputWidget::enableReconstruct(bool enabled)
{
    m_exportW->enableReconstruct(enabled);
}

void OutputWidget::showProgress()
{
    m_progressW->slot_clearWarnings();
    m_layout->removeWidget(m_exportW);
    m_exportW->setVisible(false);
    m_progressW->setVisible(true);
    m_layout->addWidget(m_progressW);
}

void OutputWidget::showExportOptions()
{
    m_progressW->slot_clearWarnings();
    m_layout->removeWidget(m_progressW);
    m_progressW->setVisible(false);
    m_exportW->setVisible(true);
    m_layout->addWidget(m_exportW);
}

void OutputWidget::setResolutionList(QStringList resList, int idx)
{
    m_exportW->setResolutionList(resList,idx);
}

void OutputWidget::setResolution(QString resolution)
{
    m_exportW->setResolution(resolution);
}

void OutputWidget::setResolutionValid(bool valid)
{
    m_exportW->setResolutionValid(valid);
}

QString OutputWidget::getExportFormat()
{
    return m_exportW->getExportFormat();
}

ExportFormat OutputWidget::getExportFormatEnum()
{
    return m_exportW->getExportFormatEnum();
}

bool OutputWidget::setOutputFormat(QString format)
{
    return m_exportW->setOutputFormat(format);
}

bool OutputWidget::setOutputFormat(ExportFormat format)
{
    return m_exportW->setOutputFormat(format);
}

void OutputWidget::enableFormat(QString format, bool enable)
{
    m_exportW->enableFormat(format, enable);
}

void OutputWidget::enableFormat(ExportFormat format, bool enable)
{
    m_exportW->enableFormat(format, enable);
}

std::shared_ptr<MaskStackView> OutputWidget::getMaskStackView() {
    return m_exportW->getMaskStackView();
}

void OutputWidget::setAltitudeVisible(bool visible)
{
    m_exportW->setAltitudeVisible(visible);
}

void OutputWidget::setAltitude(double altitude)
{
    m_exportW->setAltitude(altitude);
}

double OutputWidget::getAltitude()
{
    return m_exportW->getAltitude();
}

void OutputWidget::slot_displayProgress(int progress, QString currentOperation)
{
    m_progressW->slot_displayProgress(progress,currentOperation);
}

void OutputWidget::slot_displayMessage(QString message)
{
    m_progressW->slot_displayMessage(message);
}

void OutputWidget::slot_displayWarning(QString warning)
{
    m_progressW->slot_displayWarning(warning);
}

void OutputWidget::slot_clearWarnings()
{
    m_progressW->slot_clearWarnings();
}

void OutputWidget::slot_pathChanged(const QString &path)
{
    emit sig_pathChanged(path);
}

void OutputWidget::slot_addAuto()
{
    emit sig_addAuto();
}

void OutputWidget::slot_export()
{
    emit sig_export();
}

void OutputWidget::slot_reconstruct()
{
    emit sig_reconstruct();
}

void OutputWidget::slot_abort()
{
    emit sig_abort();
}

