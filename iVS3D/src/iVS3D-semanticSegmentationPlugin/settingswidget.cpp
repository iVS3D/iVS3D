#include "settingswidget.h"

SettingsWidget::SettingsWidget(QWidget *parent, QStringList ONNXmodelList, float blendAlpha) : QWidget(parent)
{
    this->setLayout(new QVBoxLayout());
    this->layout()->setSpacing(0);
    this->layout()->setMargin(0);

    QFormLayout *w = new QFormLayout(parent);
    w->setSpacing(0);
    w->setMargin(0);

    QLabel *txtModel = new QLabel("Select neural net model to create semantic segmentation");
    txtModel->setStyleSheet(DESCRIPTION_STYLE);
    txtModel->setWordWrap(true);
    this->layout()->addWidget(txtModel);

    m_comboBox = new QComboBox(parent);
    m_comboBox->addItems(ONNXmodelList);
    w->addRow("ONNX model ", m_comboBox);
    connect(m_comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWidget::slot_comboBoxIdxChanged);

    m_alphaSlider = new QSlider(Qt::Horizontal, parent);
    m_alphaSlider->setMinimum(0);
    m_alphaSlider->setMaximum(100);
    m_alphaSlider->setTickInterval(10);
    m_alphaSlider->setValue((int)(blendAlpha * 100));
    connect(m_alphaSlider, &QSlider::valueChanged, this, &SettingsWidget::slot_overlayAlphaChanged);
    w->addRow("Overlay alpha ", m_alphaSlider);

    m_processLabel = new QLabel("");
    m_processLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_processLabel->setStyleSheet("color: green");

    m_processorLabel = new QLabel("");
    m_processorLabel->setStyleSheet("color: green");
    w->addRow(m_processorLabel, m_processLabel);

    QWidget *widget = new QWidget(parent);
    widget->setLayout(w);
    this->layout()->addWidget(widget);



    QLabel *txt = new QLabel("Select classes to include in reconstruction mask");
    txt->setStyleSheet(DESCRIPTION_STYLE);
    txt->setWordWrap(true);
    this->layout()->addWidget(txt);

    // layout space for classes and colors
    m_gridLayout = new QGridLayout(parent);
    QWidget *formWidget = new QWidget(parent);
    formWidget->setLayout(m_gridLayout);
    this->layout()->addWidget(formWidget);

    m_invertButton = new QPushButton("Invert class selection");
    this->layout()->addWidget(m_invertButton);
    connect(m_invertButton, &QPushButton::pressed, this, &SettingsWidget::slot_invertSelectionPressed);

    this->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    this->adjustSize();
}

SettingsWidget::~SettingsWidget()
{

}

void SettingsWidget::setClasses(QBoolList boolList)
{
    for(int i = 0; i < boolList.size(); i++) {
        m_classBoxes[i]->setChecked(boolList[i]);
    }
}

void SettingsWidget::setModel(uint modelIndex)
{
    m_comboBox->setCurrentIndex(modelIndex);
}

void SettingsWidget::slot_classesAndColorsChanged(QStringList classes, QColorList colors, QBoolList selectedClasses)
{
    Q_ASSERT(classes.size() == colors.size());

    for(auto item : m_classBoxes){
        m_gridLayout->removeWidget(item);
        disconnect(item, &QCheckBox::stateChanged, this, &SettingsWidget::slot_selectedClassesChanged);
        delete item;
    }
    m_classBoxes.clear();

    for(int i = 0; i < classes.size(); ++i){
        QCheckBox *cb = new QCheckBox(classes[i], this);
        QString qss = QString("border: 2px solid rgb(%1,%2,%3)").arg(QString::number(colors[i].red()),QString::number(colors[i].green()),QString::number(colors[i].blue()));
        cb->setStyleSheet(qss);
        cb->setChecked(selectedClasses[i]);
        m_classBoxes.push_back(cb);
        m_gridLayout->addWidget(cb,i/2, i%2);
        connect(cb, &QCheckBox::stateChanged, this, &SettingsWidget::slot_selectedClassesChanged);
    }
    this->adjustSize();
}

void SettingsWidget::slot_showTask(QString processor, QString task)
{
    m_processorLabel->setText(processor);
    m_processLabel->setText(task);
}


void SettingsWidget::slot_comboBoxIdxChanged(int idx)
{
    emit sig_selectedONNXIndexChanged(idx);
}

void SettingsWidget::slot_selectedClassesChanged(bool)
{
    QList<bool> classes;
    for(size_t i = 0; i < m_classBoxes.size(); i++){
        classes.push_back(m_classBoxes[i]->isChecked());
    }
    emit sig_selectedClassesChanged(classes);
}

void SettingsWidget::slot_overlayAlphaChanged(int percent)
{
    emit sig_blendAlphaChanged(((float)percent)/100.0f);
}

void SettingsWidget::slot_invertSelectionPressed()
{
    QBoolList classes;
    for(auto *cb : m_classBoxes){
        disconnect(cb, &QCheckBox::stateChanged, this, &SettingsWidget::slot_selectedClassesChanged);
        cb->setChecked(!cb->isChecked());
        classes.push_back(cb->isChecked());
        connect(cb, &QCheckBox::stateChanged, this, &SettingsWidget::slot_selectedClassesChanged);
    }
    emit sig_selectedClassesChanged(classes);
}
