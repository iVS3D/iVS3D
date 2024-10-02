#include "spinnericon.h"

SpinnerIcon::SpinnerIcon(QWidget *parent)
    : QWidget{parent}
{
    // Create the SVG spinner
    m_spinner = new QSvgWidget(":/icons/spinner.svg", this);
    //m_spinner->setFixedSize(30, 30);

    // Create the label to display current status as text
    m_text = new QLabel(this);

    // Create a grid layout to overlay the spinner and the icons
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setAlignment(Qt::AlignRight);
    layout->addWidget(m_text);
    layout->addWidget(m_spinner);  // Center spinner

    // Set the layout for this widget
    setLayout(layout);
    setSpinning(false);
}

void SpinnerIcon::setSpinning(const bool spinning)
{
    m_spinner->setVisible(spinning);
}

void SpinnerIcon::setText(const QString &text)
{
    m_text->setText(text);
}
