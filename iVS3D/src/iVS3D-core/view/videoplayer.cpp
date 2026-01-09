#include "videoplayer.h"

#include <QGraphicsSvgItem>
#include <QTimer>
#include <QTransform>
#include <iostream>
#include <optional>

#include "overlay.h"

VideoPlayer::VideoPlayer(QWidget* parent, ColorTheme theme)
    : QWidget(parent), ui(new Ui::VideoPlayer) {
    ui->setupUi(this);
    ui->graphicsView->setScene(new QGraphicsScene(this));
    ui->graphicsView->setAcceptDrops(false);
    ui->widget->setLayout(new QVBoxLayout(this));
    ui->widget->layout()->setContentsMargins(0, 0, 0, 0);
    setColorTheme(theme);
    m_nextSC = new QShortcut(QKeySequence(/*Qt::CTRL + */ Qt::Key_Right), this);
    m_prevSC = new QShortcut(QKeySequence(/*Qt::CTRL + */ Qt::Key_Left), this);

    connect(m_nextSC, &QShortcut::activated, this,
            &VideoPlayer::on_pushButton_nextPic_clicked);
    connect(m_prevSC, &QShortcut::activated, this,
            &VideoPlayer::on_pushButton_prevPic_clicked);

    displayDragNDropIcon();

    // Center the item in the view (optional, usually fitInView does this
    // already)
    QGraphicsView* view = ui->graphicsView;
    view->setRenderHint(QPainter::Antialiasing, true);

    // Optionally, you can center the view using `setAlignment` or adjust it
    // further if needed
    view->setAlignment(Qt::AlignCenter);

    ui->graphicsView->show();
    QTimer::singleShot(0, this, [this]() {
        ui->graphicsView->fitInView(ui->graphicsView->scene()->sceneRect(),
                                    Qt::KeepAspectRatio);
    });

    // label for the overlay text displayed in the left blackbar
    m_overlayLabel = new QLabel(this);
    m_overlayLabel->setStyleSheet(
        "color: white; background-color: rgba(100, 100, 100, 0);");
    m_overlayLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    // m_overlayLabel->setWordWrap(true);
    m_overlayLabel->setText("Loading...");
    m_overlayLabel->raise();

    // set opacity effect on overlay label
    m_overlayOpacityEffect = new QGraphicsOpacityEffect(this);
    m_overlayOpacityEffect->setOpacity(1.0);
    m_overlayLabel->setGraphicsEffect(m_overlayOpacityEffect);

    // Align label at the top-left corner of the QGraphicsView
    int margin_l, margin_t;
    this->layout()->getContentsMargins(&margin_l, &margin_t, nullptr, nullptr);
    m_overlayLabel->move(ui->graphicsView->x() + OVERLAY_PADDING + margin_l,
                         ui->graphicsView->y() + OVERLAY_PADDING + margin_t);

    QList<OverlayEntry> info = {
        {"C:\\Some\\Long\\Path\\To\\Images\\Filename.png", false,
         Qt::ElideMiddle},  // Filepath
        {"General", true},
        {"1920 x 1080 pixels"},
        {"204 images"},
        {"Video", true},
        {"12.4 seconds"},
        {"30 fps"},
        {"Metadata", true},
        {"ExifGPS"},              // First metadata entry
        {"CameraModel: Sony A7"}  // Additional metadata
    };

    updateOverlayText(info);

    m_roiItem = nullptr;
    m_imageItem = nullptr;
    m_visItemGroup = nullptr;

    connect(ui->pushButton_roi, &QPushButton::clicked,
            [=]() { emit sig_cropEdit(); });
    connect(ui->checkBox_roi, &QCheckBox::stateChanged,
            [=](int state) { emit sig_useCropChanged(state == Qt::Checked); });
}

VideoPlayer::~VideoPlayer() {
    delete ui->graphicsView->scene();
    delete ui;
}

void VideoPlayer::showVisualization(const Visualization& vis) {
    
    assert(m_imageItem != nullptr); // must have an image to overlay on!

    if(m_visItemGroup) {
        ui->graphicsView->scene()->removeItem(m_visItemGroup);
        delete m_visItemGroup;
        m_visItemGroup = nullptr;
    }

    QRectF viewport;
    if (m_roiItem) {
        viewport = m_roiItem->rect();
    } else {
        viewport = m_imageItem->boundingRect();
    }

    m_visItemGroup = new QGraphicsItemGroup(m_imageItem);
    m_visItemGroup->setZValue(2); // on top of image and ROI
    
    int width = 0;
    int height = 0;
    for (const auto& view : vis.views) {
        if (view.style.showTitle && !view.title.isEmpty()) {
            auto view_root = new QGraphicsItemGroup(m_visItemGroup);
            view_root->setZValue(3);
            QRectF viewport_title = (m_roiItem && view.style.viewport == ViewportType::RegionOfInterest)
                                       ? m_roiItem->rect()
                                       : m_imageItem->boundingRect();
            view_root->setTransform(
                QTransform::fromTranslate(width, 0)
                    .scale(viewport_title.width(), viewport_title.height())); // scale from [0,1] to viewport size
            
            TextOverlay titleOverlay;
            titleOverlay.text = view.title;
            titleOverlay.position = QPointF(0.5, 0.02); // top center
            titleOverlay.anchor = TextAnchor::TopCenter;
            titleOverlay.style.fontSize = 16;
            titleOverlay.style.textColor = Qt::white;
            titleOverlay.style.backgroundColor = Qt::darkGray;
            drawOverlay(ui->graphicsView->scene(), view_root, titleOverlay);
        }

        // create a root element correctly scaled and translated for this view
        auto *view_root = new QGraphicsItemGroup(m_visItemGroup);
        int x_offset = 0;
        int y_offset = 0;
        // for FullImage, offset is the viewRect position in the full image
        if (view.style.viewport == ViewportType::FullImage) {
            x_offset = viewport.x();
            y_offset = viewport.y();
        }
        view_root->setTransform(
            QTransform::fromTranslate(x_offset + width, y_offset)
                .scale(viewport.width(), viewport.height())); // scale from [0,1] to viewport size
        
        // keep track of overall scene size to append next view
        auto rect = (view.style.viewport == ViewportType::RegionOfInterest) ? viewport : m_imageItem->boundingRect();
        width += rect.width();
        height = std::max(height, int(rect.height()));

        // draw all overlays for this view. They are in local [0,1] space.
        // Scaling is handled by the view_root transform.
        for (const auto& overlay : view.overlays) {
            std::visit(
                [=](auto&& arg) {
                    drawOverlay(ui->graphicsView->scene(), view_root, arg);
                },
                overlay);
        }
    }

    // set the scene rect to the combined size of all views and fit to view
    // only expand the veiwport, dont shrink it!
    ui->graphicsView->scene()->setSceneRect(0, 0, width, height);
    ui->graphicsView->fitInView(ui->graphicsView->scene()->sceneRect(),
                                Qt::KeepAspectRatio);
    ui->graphicsView->show();

    updateOverlay();
}

void VideoPlayer::showImage(const cv::Mat& image) {
    if (image.empty()) {
        return;
    }
    int w = 0, h = 0;
    if (m_imageItem) {
        w = int(m_imageItem->boundingRect().width());
        h = int(m_imageItem->boundingRect().height());
        ui->graphicsView->scene()->removeItem(m_imageItem);
        delete m_imageItem;
        m_imageItem = nullptr;
        m_visItemGroup = nullptr;
    }
    auto pixmap =
        QPixmap::fromImage(qImageFromCvMat(image, true));
    m_imageItem = ui->graphicsView->scene()->addPixmap(pixmap);

    if (w != pixmap.width() || h != pixmap.height()) {
        // chnaged resolution, need to update scene rect
        ui->graphicsView->scene()->setSceneRect(
            0, 0, pixmap.width(), pixmap.height());
    }
    
    ui->graphicsView->fitInView(ui->graphicsView->scene()->sceneRect(),
                                Qt::KeepAspectRatio);
    ui->graphicsView->show();

    updateOverlay();
}

void VideoPlayer::setKeyframe(bool isKeyframe) {
    ui->graphicsView->setStyleSheet(
        isKeyframe ? "background-color:black; border: 3px solid red;"
                   : "background-color:black; border: 3px solid black;");
    ui->pushButton_setKeyframe->setText(isKeyframe
                                            ? tr("Deselect current image")
                                            : tr(" Select current image "));
}

void VideoPlayer::setKeyframeCount(unsigned int keyframeCount) {
    ui->label_keyframeCount->setText(QString::number(keyframeCount));
}

void VideoPlayer::setEnabledBackBtns(bool enabled) {
    ui->pushButton_firstPic->setEnabled(enabled);
    ui->pushButton_prevPic->setEnabled(enabled);
    m_prevSC->setEnabled(enabled);
}

void VideoPlayer::setEnabledForwardBtns(bool enabled) {
    ui->pushButton_lastPic->setEnabled(enabled);
    ui->pushButton_nextPic->setEnabled(enabled);
    m_nextSC->setEnabled(enabled);
}

void VideoPlayer::setPlaying(bool playing) {
    QString col = m_colorTheme == DARK ? "W" : "B";
    ui->pushButton_playPause->setIcon(playing
                                          ? QIcon(":/icons/pauseIcon" + col)
                                          : QIcon(":/icons/playIcon" + col));
}

void VideoPlayer::setStepsize(unsigned int stepsize) {
    ui->spinBox_stepsize->setValue(stepsize);
}

void VideoPlayer::setKeyframesOnly(bool checked) {
    ui->checkBox_onlyKeyframes->setChecked(checked);
}

void VideoPlayer::addWidgetToLayout(QWidget* widget) {
    ui->widget->layout()->addWidget(widget);
}

void VideoPlayer::removeWidgetFromLayout(QWidget* widget) {
    ui->widget->layout()->removeWidget(widget);
}

void VideoPlayer::setColorTheme(ColorTheme theme) {
    m_colorTheme = theme;
    QString col = m_colorTheme == DARK ? "W" : "B";
    ui->pushButton_resetKeyframes->setIcon(QIcon(":/icons/resetIcon" + col));
    ui->pushButton_firstPic->setIcon(QIcon(":/icons/fastRewindIcon" + col));
    ui->pushButton_lastPic->setIcon(QIcon(":/icons/fastForwardIcon" + col));
    ui->pushButton_nextPic->setIcon(QIcon(":/icons/nextIcon" + col));
    ui->pushButton_prevPic->setIcon(QIcon(":/icons/prevIcon" + col));
    ui->pushButton_playPause->setIcon(QIcon(":/icons/playIcon" + col));
}

void VideoPlayer::resizeEvent(QResizeEvent*) {
    ui->graphicsView->fitInView(ui->graphicsView->scene()->sceneRect(),
                                Qt::KeepAspectRatio);
    ui->graphicsView->show();
    updateOverlay();
}

/*
 * slots for the ui elements, all just delegate their signals
 */

void VideoPlayer::on_pushButton_firstPic_clicked() {
    emit sig_showFirstImage();
}

void VideoPlayer::on_pushButton_prevPic_clicked() {
    emit sig_showPreviousImage();
}

void VideoPlayer::on_pushButton_playPause_clicked() { emit sig_play(); }

void VideoPlayer::on_pushButton_nextPic_clicked() { emit sig_showNextImage(); }

void VideoPlayer::on_pushButton_lastPic_clicked() { emit sig_showLastImage(); }

void VideoPlayer::on_checkBox_onlyKeyframes_stateChanged(int arg1) {
    emit sig_toggleKeyframesOnly(arg1);
}

void VideoPlayer::on_pushButton_setKeyframe_clicked() {
    emit sig_toggleKeyframes();
}

void VideoPlayer::on_spinBox_stepsize_valueChanged(int arg1) {
    emit sig_changeStepsize(arg1);
}

void VideoPlayer::on_pushButton_resetKeyframes_clicked() {
    emit sig_deleteAllKeyframes();
}

QImage VideoPlayer::qImageFromCvMat(const cv::Mat& input, bool bgr) {
    cv::Mat rgb = input;
    if (input.channels() == 4) {
        if (bgr) {
            cv::cvtColor(input, rgb, cv::COLOR_BGRA2RGBA);
        }

        return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step),
                      QImage::Format_RGBA8888)
            .copy();
    } else if (rgb.channels() == 3) {
        if (bgr) {
            cv::cvtColor(input, rgb, cv::COLOR_BGR2RGB);
        }

        return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step),
                      QImage::Format_RGB888)
            .copy();
    } else if (rgb.channels() == 1) {
        return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step),
                      QImage::Format_Grayscale8)
            .copy();
    }

    return QImage();
}

void VideoPlayer::alphaBlend(cv::Mat* foreground, cv::Mat* background,
                             float alpha, cv::Mat& output) {
    output = alpha * (*foreground) + (1 - alpha) * (*background);
}

void VideoPlayer::updateOverlay() {
    // Convert image rect to viewport coordinates
    int imageWidth =
        ui->graphicsView->mapFromScene(ui->graphicsView->sceneRect())
            .boundingRect()
            .width();
    int imageHeight =
        ui->graphicsView->mapFromScene(ui->graphicsView->sceneRect())
            .boundingRect()
            .height();

    // get the margins around the graphics view
    int margin_l, margin_t;
    this->layout()->getContentsMargins(&margin_l, &margin_t, nullptr, nullptr);

    // Calculate available space on the left and top
    int availableWidth = (ui->graphicsView->width() - imageWidth) / 2;
    availableWidth -= OVERLAY_PADDING + margin_l;
    int availableHeight = (ui->graphicsView->height() - imageHeight) / 2;
    availableHeight -= OVERLAY_PADDING + margin_t;

    // Create formatted text
    QString text;
    QString tooltip;
    QFontMetrics metrics(m_overlayLabel->font());

    for (const auto& entry : m_overlayEntries) {
        QString truncated =
            metrics.elidedText(entry.text, entry.elidMode, availableWidth);
        if (entry.isHeader) {
            // Bold header (e.g., "Metadata")
            text += QString("<br><b>%1</b><br>").arg(truncated);
            tooltip += QString("<br><b>%1</b><br>").arg(entry.text);
        } else {
            // Normal text only (e.g., file path)
            text += truncated + "<br>";
            tooltip += entry.text + "<br>";
        }
    }

    if (availableWidth >= OVERLAY_MIN_WIDTH) {  // blackbars on the left is wide
                                                // enough, display there
        m_overlayLabel->setText(text);
        m_overlayOpacityEffect->setOpacity(1.0);
    } else if (availableHeight >=
               metrics.height()) {  // blackbar on the top is high enough,
                                    // display there
        QString horizontal_text;
        bool insert_separator = false;
        for (const auto& entry : m_overlayEntries) {
            if (entry.isHeader) {
                // Bold header (e.g., "Metadata")
                horizontal_text += QString(" <b>| %1</b>: ").arg(entry.text);
                insert_separator = false;
            } else {
                // Normal text only (e.g., file path)
                if (insert_separator) horizontal_text += ",";
                horizontal_text += " " + entry.text;
                insert_separator = true;
            }
        }
        m_overlayLabel->setText(horizontal_text);
        m_overlayOpacityEffect->setOpacity(1.0);
    } else if (!m_overlayEntries.empty()) {  // neither bar is big enough, fall
                                             // back to info-icon overlay
        m_overlayLabel->setText(
            "<img src=':/icons/infoIconW' width='24' height='24'>");
        m_overlayOpacityEffect->setOpacity(0.7);
    }
    m_overlayLabel->setToolTip(tooltip);
    m_overlayLabel->adjustSize();
    m_overlayLabel->show();
}

void VideoPlayer::updateOverlayText(const QList<OverlayEntry>& content) {
    m_overlayEntries = content;
    updateOverlay();
}

void VideoPlayer::updateRoi(const QRect& roi) {
    if (roi.size() == QSize(0, 0)) {
        if (m_roiItem) {
            ui->graphicsView->scene()->removeItem(m_roiItem);
            delete m_roiItem;
            m_roiItem = nullptr;
        }
        return;
    }
    if (!m_roiItem) {
        m_roiItem = new QGraphicsRectItem(roi);
        m_roiItem->setZValue(1);  // ensure ROI is on top of image

        QPen pen;
        pen.setWidth(3);
        pen.setBrush(Qt::green);
        pen.setCosmetic(true);
        QColor color = Qt::transparent;
        QBrush brush = QBrush(color);
        m_roiItem->setBrush(brush);
        m_roiItem->setPen(pen);

        ui->graphicsView->scene()->addItem(m_roiItem);
    } else {
        m_roiItem->setRect(roi);
    }
}

void VideoPlayer::clear() {
    ui->graphicsView->scene()->clear();
    ui->graphicsView->scene()->setSceneRect(0, 0, 0, 0);
    m_imageItem = nullptr;
    m_roiItem = nullptr;
    m_visItemGroup = nullptr;
}

bool VideoPlayer::checkOverlap() {
    QRectF imageRect =
        ui->graphicsView
            ->sceneRect();  // Image bounding box in scene coordinates
    QRect labelRect =
        m_overlayLabel
            ->geometry();  // Overlay label geometry in widget coordinates

    // Convert image rect to viewport coordinates
    QRectF imageInView =
        ui->graphicsView->mapFromScene(imageRect).boundingRect();

    // Check if label overlaps the image area
    return labelRect.intersects(imageInView.toRect());
}

void VideoPlayer::displayDragNDropIcon() {
    ui->graphicsView->scene()->clear();

    ui->graphicsView->scene()->setSceneRect(0, 0, 1.0, 1.0);

    // Create a text item with your message
    QGraphicsTextItem* textItem = new QGraphicsTextItem(
        tr("Drag and drop images, videos, or project files here to open"));

    QFont font("Arial", 16);
    textItem->setFont(font);
    textItem->setDefaultTextColor(Qt::white);

    // Make sure the text item is not affected by view transformations
    textItem->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);

    textItem->setTransformOriginPoint(
        textItem->boundingRect().width() / 2.0,
        0);  // Center the origin in horizontal direction

    textItem->setPos(0,0.15);  // top middle

    // Add the text item to the scene
    ui->graphicsView->scene()->addItem(textItem);

    // Load the SVG icon
    QGraphicsSvgItem* svgItem = new QGraphicsSvgItem(":/icons/dragndropIconW");
    svgItem->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);

    svgItem->setScale(0.5);  // Adjust scale as needed

    svgItem->setTransformOriginPoint(
        svgItem->boundingRect().width() / 2.0,
        0);  // Center the origin top middle
    
    svgItem->setPos(0, 0.4);  // Position at the top-middle

    // Add the item to the scene
    ui->graphicsView->scene()->addItem(svgItem);

    // Ensure you're calling fitInView with the entire scene's bounding rect
    ui->graphicsView->fitInView(ui->graphicsView->scene()->sceneRect(),
                                Qt::KeepAspectRatio);
}

bool VideoPlayer::getCropStatus() {
    return ui->checkBox_roi->checkState() == Qt::Checked;
}

void VideoPlayer::setCropStatus(bool checked) {
    ui->checkBox_roi->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
}
