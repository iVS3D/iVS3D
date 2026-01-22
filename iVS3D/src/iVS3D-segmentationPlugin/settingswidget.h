#pragma once

#include <QWidget>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QColor>
#include <QVector>
#include <QString>
#include <QCheckBox>

#include <memory>

#include "segmodel.h"

namespace segmentationplugin {

    class ModelClassView : public QWidget {
        Q_OBJECT
    public:
        explicit ModelClassView(const QVector<ModelClass>& classes, QWidget* parent = nullptr)
            : QWidget(parent)
        {
            auto layout = new QGridLayout(this);
            int row = 0;
            for (const auto& cls : classes) {
                auto checkBox = new QCheckBox(cls.name, this);
                checkBox->setStyleSheet(QString("QCheckBox { color: %1 }").arg(cls.color.name()));
                layout->addWidget(checkBox, row++, 0);
                connect(checkBox, &QCheckBox::stateChanged, this, &ModelClassView::onCheckBoxStateChanged);
                m_classCheckBoxes.push_back(checkBox);
            }
            setLayout(layout);
        }

        ~ModelClassView() {
            std::printf("ModelClassView destroyed\n");
            for (const auto& cb : m_classCheckBoxes) {
                disconnect(cb, &QCheckBox::stateChanged, this, &ModelClassView::onCheckBoxStateChanged);
                layout()->removeWidget(cb);
                delete cb;
            }
        }

    signals:
        void classSelectionChanged(const QVector<bool>& selectedClasses);

    private slots:
        void onCheckBoxStateChanged(int) {
            QVector<bool> selectedClasses;
            for (const auto& cb : m_classCheckBoxes) {
                selectedClasses.push_back(cb->isChecked());
            }
            emit classSelectionChanged(selectedClasses);
        }
    private:
        QVector<QCheckBox*> m_classCheckBoxes;
    };

    class SettingsWidget : public QWidget {
        Q_OBJECT
    public:
        explicit SettingsWidget(QWidget* parent = nullptr, const QVector<ModelInfo>& models = {});
        ~SettingsWidget() override = default;

        void setModel(const ModelInfo& model);
        void setOverlayAlpha(float alpha);
    
    signals:
        void modelChanged(const QString modelName);
        void overlayAlphaChanged(float alpha);
        void classSelectionChanged(const QVector<bool>& selectedClasses);

    private slots:
        void onClassSelectionChanged(const QVector<bool>& selectedClasses) {
            emit classSelectionChanged(selectedClasses);
        }

    private:
        QComboBox *m_modelComboBox;
        QSlider *m_alphaSlider;
        std::unique_ptr<ModelClassView> m_classView;
    };
}