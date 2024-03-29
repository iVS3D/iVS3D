#include "cropexport.h"


CropExport::CropExport(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CropExport)
{
    ui->setupUi(this);
}

CropExport::CropExport(QWidget *parent,const cv::Mat* img, QRect roi) :
    QDialog(parent),
    ui(new Ui::CropExport)
{
    ui->setupUi(this);
    m_scene = new roiSelect(this);
    ui->graphicsView ->setScene(m_scene);
    ui->graphicsView->setAcceptDrops(false);
    m_scene->clear();
    m_oldROI = roi;
    QPixmap pixmap;

    pixmap = QPixmap::fromImage(qImageFromCvMat(img, true));

    m_scene->addPixmap(pixmap);
    image = pixmap.rect();
    imageSize = QPoint(img->cols, img->rows);
    m_scene->setSceneRect(pixmap.rect());

    ui->graphicsView->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    ui->graphicsView->show();

    connect(m_scene, &roiSelect::sig_mousePress, this, &CropExport::slot_mousePress);
    connect(m_scene, &roiSelect::sig_mouseMove, this, &CropExport::slot_mouseMove);

    if (roi.width() > 0 && roi.height() > 0 && roi.width() <= imageSize.x() && roi.height() <= imageSize.y()) {
        setSavedROI(roi);
    }



}

CropExport::~CropExport()
{
    delete ui;
}

void CropExport::triggerResize()
{
    QRectF rect = m_scene->sceneRect();
    rect.setHeight(rect.height() + 30);
    rect.setWidth(rect.width() + 30);
    ui->graphicsView->fitInView(rect, Qt::KeepAspectRatio);
    ui->graphicsView->show();
}

QRect CropExport::getROI()
{
    if(m_rect != nullptr && (start != QPointF(0,0) && end != QPointF(0,0))) {
        //Set correct borders
        QRectF drawnROI = QRectF(start, end);

        if (!image.intersects(drawnROI.toRect())) {
            return QRect(0,0,0,0);
        }
        QRectF intersection = image.intersected(drawnROI.toRect());
        m_rect->setRect(intersection);
        return intersection.toRect();

    }
    else if (m_oldROI != QRect(0,0,0,0)) {
        return m_oldROI;
    }
    return QRect(0,0,0,0);
}

QImage CropExport::qImageFromCvMat(const cv::Mat* input, bool bgr)
{
    const cv::Mat* rgb = input;

    if(rgb->channels() == 4)
    {
        if (bgr)
        {
            cv::cvtColor(*input, *rgb, cv::COLOR_BGRA2RGBA);
        }

        return QImage(rgb->data, rgb->cols, rgb->rows, static_cast<int>(rgb->step), QImage::Format_RGBA8888).copy();
    }
    else if (rgb->channels() == 3)
    {
        if (bgr)
        {
            cv::cvtColor(*input, *rgb, cv::COLOR_BGR2RGB);
        }

        return QImage(rgb->data, rgb->cols, rgb->rows, static_cast<int>(rgb->step), QImage::Format_RGB888).copy();
    }
    else if (rgb->channels() == 1)
    {
        return QImage(rgb->data, rgb->cols, rgb->rows, static_cast<int>(rgb->step), QImage::Format_Grayscale8).copy();
    }

    return QImage();
}

void CropExport::resizeEvent(QResizeEvent *)
{
    QRectF rect = m_scene->sceneRect();
    rect.setHeight(rect.height() + 30);
    rect.setWidth(rect.width() + 30);
    ui->graphicsView->fitInView(rect, Qt::KeepAspectRatio);
    ui->graphicsView->show();
}

void CropExport::slot_mousePress(QGraphicsSceneMouseEvent *mouseEvent)
{

    start = mouseEvent->scenePos();
    if(m_rect != nullptr) {
        m_scene->removeItem(m_rect);
        delete m_rect;
    }
    m_rect= new QGraphicsRectItem(QRectF(mouseEvent->scenePos(),QPointF(mouseEvent->scenePos().x() + 10, mouseEvent->scenePos().y() + 10)));
    QPen pen;
    pen.setWidth(5);
    pen.setBrush(Qt::red);
    QColor* color = new QColor(255,255,255,100);
    QBrush* brush = new QBrush(*color);
    m_rect->setBrush(*brush);
    m_rect->setPen(pen);
    m_scene->addItem(m_rect);
    ui->graphicsView->show();

}

void CropExport::slot_mouseMove(QGraphicsSceneMouseEvent *mouseEvent)
{

    QPointF pos = mouseEvent->scenePos();


    if (start.x() < pos.x() && start.y() < pos.y()) {
        end = mouseEvent->scenePos();
        m_rect->setRect(QRectF(start, end));
        ui->graphicsView->show();
    }

}


void CropExport::on_pushButton_abort_clicked()
{
    QDialog::reject();
}

void CropExport::on_pushButton_crop_clicked()
{
    QDialog::accept();
}

void CropExport::setSavedROI(QRect roi)
{
    QPen pen;
    pen.setWidth(5);
    pen.setBrush(Qt::red);
    m_rect = new QGraphicsRectItem(QRectF(roi.topLeft(),roi.bottomRight()));
    QColor* color = new QColor(255,255,255,100);
    QBrush* brush = new QBrush(*color);
    m_rect->setBrush(*brush);
    m_rect->setPen(pen);
    m_scene->addItem(m_rect);
    ui->graphicsView->show();
}



