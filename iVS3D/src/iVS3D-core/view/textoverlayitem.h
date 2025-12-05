#pragma once
/**
 * @file textoverlayitem.h
 * @brief QGraphicsItem to render text overlays with background and padding.
 *
 * @author Dominik Wüst
 * @date 2025/12/04
 */
#include "visualization.h"
#include <QGraphicsItem>
#include <QString>
#include <QFont>
#include <QColor>
#include <QRectF>
#include <QPainter>

/**
 * @brief QGraphicsItem that renders a text overlay with background and padding.
 */
class TextOverlayItem : public QGraphicsItem
{
public:
    TextOverlayItem(const QString& text,
                    const QFont& font,
                    const QColor& textColor,
                  const QColor& bgColor,
                  TextAnchor anchor,
                  qreal padding = 3.0,
                  QGraphicsItem* parent = nullptr)
        : QGraphicsItem(parent)
        , m_text(text)
        , m_font(font)
        , m_textColor(textColor)
        , m_bgColor(bgColor)
        , m_anchor(anchor)
        , m_padding(padding)
    {
        setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        updateGeometry();
    }

    QRectF boundingRect() const override { return m_rect; }

    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem*,
               QWidget*) override
    {
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setRenderHint(QPainter::TextAntialiasing, true);
        painter->setFont(m_font);

        // Background
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_bgColor);
        painter->drawRect(m_rect);

        // Text (inside padded area)
        painter->setPen(m_textColor);
        QRectF textRect = m_rect.adjusted(m_padding, m_padding,
                                          -m_padding, -m_padding);
        painter->drawText(textRect,
                          Qt::AlignLeft | Qt::AlignVCenter,
                          m_text);
    }

private:
    void updateGeometry()
    {
        QFontMetrics fm(m_font);
        // Size of the text in item coordinates (i.e. screen pixels)
        QRectF textRect = fm.boundingRect(m_text);

        // Add padding around it
        QRectF boxRect = textRect.adjusted(-m_padding,
                                           -m_padding,
                                           m_padding,
                                           m_padding);

        // Compute anchor point within boxRect
        QPointF anchorPoint;
        switch (m_anchor) {
        case TextAnchor::TopLeft:
            anchorPoint = boxRect.topLeft();
            break;
        case TextAnchor::TopCenter:
            anchorPoint = QPointF(boxRect.center().x(), boxRect.top());
            break;
        case TextAnchor::TopRight:
            anchorPoint = boxRect.topRight();
            break;
        case TextAnchor::CenterLeft:
            anchorPoint = QPointF(boxRect.left(), boxRect.center().y());
            break;
        case TextAnchor::Center:
            anchorPoint = boxRect.center();
            break;
        case TextAnchor::CenterRight:
            anchorPoint = QPointF(boxRect.right(), boxRect.center().y());
            break;
        case TextAnchor::BottomLeft:
            anchorPoint = boxRect.bottomLeft();
            break;
        case TextAnchor::BottomCenter:
            anchorPoint = QPointF(boxRect.center().x(), boxRect.bottom());
            break;
        case TextAnchor::BottomRight:
            anchorPoint = boxRect.bottomRight();
            break;
        }

        // Translate so that anchorPoint becomes (0,0) in item coords.
        m_rect = boxRect.translated(-anchorPoint);
    }

private:
    QString    m_text;
    QFont      m_font;
    QColor     m_textColor;
    QColor     m_bgColor;
    TextAnchor m_anchor;
    qreal      m_padding;
    QRectF     m_rect;   // background rect in item coordinates
};
