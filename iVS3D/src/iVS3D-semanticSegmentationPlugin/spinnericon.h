#ifndef SPINNERICON_H
#define SPINNERICON_H

#include <QWidget>
#include <QSvgWidget>
#include <QLabel>
#include <QHBoxLayout>

class SpinnerIcon : public QWidget
{
    Q_OBJECT
public:
    explicit SpinnerIcon(QWidget *parent = nullptr);

public slots:
    void setSpinning(const bool spinning);
    void setText(const QString &text);

private:
    QLabel *m_text;
    QSvgWidget *m_spinner;
};

#endif // SPINNERICON_H
