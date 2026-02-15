#include "spinnericon.h"

SpinnerIcon::SpinnerIcon(QWidget *parent)
    : QWidget{parent}
{
    // Create the label to display current status as text
    m_text = new QLabel(this);
    auto line_height = m_text->height();

    // Create the SVG spinner
    m_spinner = new QSvgWidget(":/icons/spinnerIcon", this);
    m_spinner->setFixedSize(line_height/2, line_height/2);

    // Create a grid layout to overlay the spinner and the icons
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setAlignment(Qt::AlignRight);
    layout->addWidget(m_text);
    layout->addWidget(m_spinner);  // Center spinner

    // Set the layout for this widget
    setLayout(layout);

    // set a fixed hight to avoid resizing issues
    setFixedHeight(line_height);

    // Initially hide the spinner
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
