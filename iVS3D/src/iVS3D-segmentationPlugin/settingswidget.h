#pragma once

#include <QWidget>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QColor>
#include <QVector>
#include <QString>
#include <QScrollArea>
#include <QSpacerItem>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <memory>

#include "segmodel.h"

namespace segmentationplugin {

    class ModelClassView : public QWidget {
        Q_OBJECT
    public:
        explicit ModelClassView(const QVector<ModelClass>& classes, QWidget* parent = nullptr)
            : QWidget(parent)
        {
            auto* mainLayout = new QVBoxLayout(this);
            mainLayout->setSpacing(6);
            mainLayout->setContentsMargins(0, 0, 0, 0);

            auto* gridWidget = new QWidget();
            auto* gridLayout = new QGridLayout(gridWidget);
            gridLayout->setSpacing(6);
            gridLayout->setContentsMargins(0, 0, 0, 0);
            gridWidget->setLayout(gridLayout);

            int displayIdx = 0;
            for (const auto& cls : classes) {
                auto* button = new QPushButton(cls.name, this);
                button->setCheckable(true);
                button->setChecked(cls.selected);
                button->setMinimumHeight(36);
                button->setCursor(Qt::PointingHandCursor);

                // Style the button with color border and background
                QColor lighterColor = cls.color.lighter(180);
                QString styleSheet = QString(
                    "QPushButton {"
                    "    border: 2px solid rgb(%1,%2,%3);"
                    "    border-radius: 4px;"
                    "    padding: 4px;"
                    "    background-color: transparent;"
                    "    font-weight: 500;"
                    "}"
                    "QPushButton:hover {"
                    "    background-color: rgb(%4,%5,%6);"
                    "    background-color: rgba(%4,%5,%6,50);"
                    "}"
                    "QPushButton:pressed {"
                    "    background-color: rgba(%4,%5,%6,100);"
                    "}"
                    "QPushButton:checked {"
                    "    background-color: rgba(%4,%5,%6,150);"
                    "    font-weight: bold;"
                    "}")
                    .arg(cls.color.red()).arg(cls.color.green()).arg(cls.color.blue())
                    .arg(lighterColor.red()).arg(lighterColor.green()).arg(lighterColor.blue());
                
                button->setStyleSheet(styleSheet);
                m_classCheckBoxes.push_back(button);

                // Connect button signal
                connect(button, &QPushButton::toggled,
                        this, [this, displayIdx](bool) { this->onClassStateChanged(); });

                // Add to grid (2 columns)
                int row = displayIdx / 2;
                int col = displayIdx % 2;
                gridLayout->addWidget(button, row, col);
                displayIdx++;
            }

            // Add stretch to fill remaining space
            gridLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding),
                                (displayIdx / 2) + 1, 0, 1, 2);

            mainLayout->addWidget(gridWidget);
            this->setLayout(mainLayout);
        }

        ~ModelClassView() {
            for (const auto& button : m_classCheckBoxes) {
                disconnect(button, nullptr, this, nullptr);
                delete button;
            }
        }

    signals:
        void classSelectionChanged(const QVector<bool>& selectedClasses);

    private slots:
        void onClassStateChanged() {
            QVector<bool> selectedClasses;
            for (const auto& button : m_classCheckBoxes) {
                selectedClasses.push_back(button->isChecked());
            }
            emit classSelectionChanged(selectedClasses);
        }
    private:
        QVector<QPushButton*> m_classCheckBoxes;
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
        QLabel *m_alphaValue;
        std::unique_ptr<ModelClassView> m_classView;
    };
}