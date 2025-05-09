#include <QToolButton>
#include <QResizeEvent>
#include <QIcon>
#include <QMap>
#include <QFontMetrics>
#include "cvmat_qmetadata.h"

/**
 * @class AdaptiveToolButton provides a QToolButton that adapts its label based on the available space.
 * It can show a full text, a short text, or just an icon depending on the width of the button.
 * It also supports changing icons based on a color theme (light/dark).
 */
class AdaptiveToolButton : public QToolButton
{
public:
    /**
     * @brief AdaptiveToolButton constructor
     * @param fullText The full text to display when there is enough space
     * @param shortText The short text to display when space is limited (optional)
     * @param parent The parent widget (optional)
     */
    AdaptiveToolButton(const QString &fullText, const QString &shortText = "", QWidget *parent = nullptr)
        : QToolButton(parent), mFullText(fullText), mShortText(shortText), mColorTheme(ColorTheme::LIGHT)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMinimumSize(30, 30);
        setIconSize(QSize(25, 25));
        setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
        setText(fullText);
    }

    /**
     * @brief setIconForTheme sets the icon for a specific color theme
     * @param icon The icon to set
     * @param theme The color theme (light/dark)
     */
    void setIconForTheme(const QIcon &icon, ColorTheme theme)
    {
        mIcons[theme] = icon;
        if (mColorTheme == theme) {
            setIcon(icon);
            adjustLabel();
        }
    }

    /**
     * @brief setColorTheme sets the current color theme and updates the icon accordingly
     * @param theme The color theme (light/dark)
     */
    void setColorTheme(ColorTheme theme)
    {
        mColorTheme = theme;
        if(mIcons.contains(theme)) {
            setIcon(mIcons.value(theme));
            adjustLabel();
        }
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QToolButton::resizeEvent(event);
        adjustLabel();
    }

private:
    void adjustLabel()
    {
        QFontMetrics fm(font());
        int textWidth = fm.horizontalAdvance(mFullText);
        int availableWidth = width() - iconSize().width() - 10;  // margin/padding

        if (textWidth > availableWidth) {
            if (!mShortText.isEmpty() && fm.horizontalAdvance(mShortText) <= availableWidth) {
                setText(mShortText);
                setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            } else {
                setToolButtonStyle(Qt::ToolButtonIconOnly);
            }
        } else {
            setText(mFullText);
            setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        }
    }

    QString mFullText;
    QString mShortText;
    ColorTheme mColorTheme;
    QMap<ColorTheme, QIcon> mIcons;
};