#include "samplingwidget.h"

#include "applicationsettings.h"

SamplingWidget::SamplingWidget(QWidget* parent)
    : QWidget(parent), ui(new Ui::SamplingWidget) {
    ui->setupUi(this);

    connect(ui->comboBoxAlgo, &QComboBox::currentTextChanged, this,
            &SamplingWidget::slot_selectedPluginChanged);

    // setup area for algo settings widget
    m_currentPluginSettings = std::make_shared<QWidget>(this);
    ui->scrollAreaWidgetContents->setLayout(new QVBoxLayout(this));
    ui->scrollAreaWidgetContents->layout()->setContentsMargins(0, 0, 0, 0);
    ui->scrollAreaWidgetContents->layout()->addWidget(
        m_currentPluginSettings.get());

    m_startSelectionBtn = new QPushButton(tr("Start Selection"), this);
    m_startSelectionBtn->setVisible(false);
    connect(m_startSelectionBtn, &QPushButton::pressed, this,
            &SamplingWidget::slot_startSamplingPressed);

    m_previewCB = new QCheckBox(tr("Enable preview"), this);
    m_previewCB->setVisible(false);
    connect(m_previewCB, &QCheckBox::stateChanged, this,
            &SamplingWidget::slot_enablePreviewChanged);

    m_addMaskBtn = new QPushButton(tr("Add Mask"), this);
    m_addMaskBtn->setVisible(false);
    connect(m_addMaskBtn, &QPushButton::pressed, this,
            &SamplingWidget::slot_addMaskPressed);

    connect(ui->comboBoxResolution, &QComboBox::currentTextChanged,
            [=](const QString& text) { emit sig_resChanged(text); });

    QHBoxLayout* layout = ui->btnLayout;
    std::vector<QWidget*> btns = {m_startSelectionBtn, m_addMaskBtn,
                                  m_previewCB};
    for (QWidget* btn : btns) {
        layout->addWidget(btn);
    }
}

SamplingWidget::~SamplingWidget() { delete ui; }

void SamplingWidget::showPluginSettings(
    std::shared_ptr<QWidget> settingsWidget) {
    ui->scrollAreaWidgetContents->layout()->replaceWidget(
        m_currentPluginSettings.get(), settingsWidget.get());
    m_currentPluginSettings->setVisible(false);
    settingsWidget->setVisible(true);
    m_currentPluginSettings = settingsWidget;
}

void SamplingWidget::slot_startSamplingPressed() { emit sig_startSampling(); }

void SamplingWidget::slot_enablePreviewChanged(bool enabled) {
    emit sig_enablePreviewChanged(enabled);
}

void SamplingWidget::slot_addMaskPressed() {
    emit sig_addMask();
}

void SamplingWidget::slot_selectedPluginChanged(QString name) {
    emit sig_selectedPluginChanged(name);
}

void SamplingWidget::setResolutionList(QStringList resList, int idx) {
    Q_ASSERT(!resList.empty());
    Q_ASSERT(idx >= 0);
    Q_ASSERT(idx < resList.size());

    ui->comboBoxResolution->clear();
    ui->comboBoxResolution->addItems(resList);
    ui->comboBoxResolution->setCurrentIndex(idx);
}

void SamplingWidget::setResolution(QString resolution) {
    ui->comboBoxResolution->setEditText(resolution);
}

void SamplingWidget::setResolutionValid(bool valid) {
    QPalette colorPalette = ui->comboBoxResolution->palette();
    if (valid) {
        ApplicationSettings as = ApplicationSettings::instance();
        if (as.getColorTheme() == DARK) {
            // darkstyle on
            colorPalette.setColor(QPalette::Text, Qt::white);
        } else {
            // darkstyle off
            colorPalette.setColor(QPalette::Text, Qt::black);
        }
    } else {
        colorPalette.setColor(QPalette::Text, Qt::red);
    }
    ui->comboBoxResolution->setPalette(colorPalette);
}

void SamplingWidget::setPluginList(const QStringList& pluginNames) {
    ui->comboBoxAlgo->clear();
    ui->comboBoxAlgo->addItems(pluginNames);
}

void SamplingWidget::setSelectedPlugin(const QString& pluginName) {
    int index = ui->comboBoxAlgo->findText(pluginName);
    if (index != -1) {
        ui->comboBoxAlgo->setCurrentIndex(index);
    }
}

void SamplingWidget::setPluginActionVisible(PluginActions action,
                                            bool visible) {
    switch (action) {
        case PluginActions::START_SELECTION:
            m_startSelectionBtn->setVisible(visible);
            break;
        case PluginActions::PREVIEW_TOGGLE:
            m_previewCB->setVisible(visible);
            break;
        case PluginActions::ADD_MASK:
            m_addMaskBtn->setVisible(visible);
            break;
        case PluginActions::ALL_ACTIONS:
            m_startSelectionBtn->setVisible(visible);
            m_previewCB->setVisible(visible);
            m_addMaskBtn->setVisible(visible);
            break;

        default:
            break;
    }
}

void SamplingWidget::setPreviewEnabled(bool enabled) {
    m_previewCB->setChecked(enabled);
}
