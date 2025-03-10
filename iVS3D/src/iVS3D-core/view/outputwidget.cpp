#include "outputwidget.h"

OutputWidget::OutputWidget(QWidget *parent, QString title, QStringList transformList) : QWidget(parent)
{
    m_exportW = new ExportWidget(this, transformList);
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
    m_layout->removeWidget(m_exportW);
    m_exportW->setVisible(false);
    m_progressW->setVisible(true);
    m_layout->addWidget(m_progressW);
}

void OutputWidget::showExportOptions()
{
    m_layout->removeWidget(m_progressW);
    m_progressW->setVisible(false);
    m_exportW->setVisible(true);
    m_layout->addWidget(m_exportW);
}

std::vector<bool> OutputWidget::getSelectedITransformMasks()
{
    return m_exportW->getSelectedITransforms();
}

bool OutputWidget::setSelectedITransformMasks(std::vector<bool> selection)
{
    return m_exportW->setSelectedITransforms(selection);
}

void OutputWidget::enableCreateFilesWidget(bool enable)
{
    m_exportW->enableCreateFilesWidget(enable);
}

void OutputWidget::slot_displayProgress(int progress, QString currentOperation)
{
    m_progressW->slot_displayProgress(progress,currentOperation);
}

void OutputWidget::slot_displayMessage(QString message)
{
    m_progressW->slot_displayMessage(message);
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

