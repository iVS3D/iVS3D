#ifndef CROPEXPORT_H
#define CROPEXPORT_H


#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "ui_cropexport.h"
#include "view/roiselect.h"
#include <QGraphicsProxyWidget>
#include <QDialog>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>


/**
 * @class CropExport
 *
 * @ingroup View
 *
 * @brief The CropExport class is which will open a QDialog on which an roi can be selected
 *
 * @author Daniel Brommer
 *
 * @date 2021/03/29
 */
namespace Ui {
class CropExport;
}

class CropExport : public QDialog
{
    Q_OBJECT

public:
    explicit CropExport(QWidget *parent = nullptr);
    /**
     * @brief CropExport Constructor which will show the given image and draw the given QRect on it, if it isn't a 0 QRect
     *
     * @param img Image to be shown
     * @param roi QRect to be drawn
     */
    CropExport(QWidget *parent, const cv::Mat* img, QRect roi);
    ~CropExport();
    /**
     * @brief triggerResize triggers the Resize slot
     */
    void triggerResize();
    /**
     * @brief Cuts the drawn QRect to the image borders and returns the modified QRect
     * @return QRect with the user selected roi
     */
    QRect getROI();

protected:
    void resizeEvent(QResizeEvent *);


public slots:
    void slot_mousePress(QGraphicsSceneMouseEvent *mouseEvent);
    void slot_mouseMove(QGraphicsSceneMouseEvent *mouseEvent);

private slots:
    void on_pushButton_abort_clicked();
    void on_pushButton_crop_clicked();

private:
    Ui::CropExport *ui;
    void setSavedROI(QRect roi);
    QImage qImageFromCvMat(const cv::Mat *input, bool bgr);
    QPointF start;
    QPointF end;
    int clickCounter = 0;
    roiSelect* m_scene;
    QGraphicsRectItem* m_rect = nullptr;
    QRect image;
    QPoint imageSize;
    QRect m_oldROI;
};

#endif // CROPEXPORT_H
