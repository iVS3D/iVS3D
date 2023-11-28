#include "infowidget.h"
#include "ui_infowidget.h"

/*!
 * \fn InfoWidget::InfoWidget(QWidget *parent, QString title)
 * \param parent
 * \param title
 *
 * Creates an InfoWidget with and displays the given \a title at the top.
 */
InfoWidget::InfoWidget(QWidget *parent, QString title, bool dark) :
    QWidget(parent),
    ui(new Ui::InfoWidget)
{
    ui->setupUi(this);
    ui->toolButton_folder->setIcon(QIcon(dark ? ":/icons/openFolderIconW" : ":/icons/openFolderIconB"));
    ui->toolButton_video->setIcon( QIcon(dark ? ":/icons/openVideoIconW"  : ":/icons/openVideoIconB"));
    ui->toolButton_meta->setIcon( QIcon(dark ? ":/icons/openMetaIconW"  : ":/icons/openMetaIconB"));
    ui->toolButton_meta->setEnabled(false);
}

InfoWidget::~InfoWidget()
{
    delete ui;
}

/*!
 * \fn void InfoWidget::setInfo(QMap<QString, QString> info)
 * \param info
 *
 * Displays the given key-value-pairs in a table.
 */
void InfoWidget::setInfo(QMap<QString, QString> info)
{
    // create html skeleton for displaying infos in table
    QString str = "";
    // add each entry separatly to the table
    foreach (QString key, info.keys()) {
        if(key.compare(stringContainer::inputPathIdentifier) == 0){
            QString input = "<tr><td style=\"word-wrap: break-word\" colspan=\"2\">" + info.value(key) + "</td></tr><tr><td></td></tr>";
            str = input + str;
        } else {
            QString value = info.value(key);
            QRegularExpression reg("(\\d*)(\\D)(.*)");
            QRegularExpressionMatch match = reg.match(key);
            if(match.hasMatch()){
                key = match.captured(2) + match.captured(3);
            }
            str += "<tr><td>" + key + "</td><td style=\"word-wrap: break-word\">" + value + "</td></tr>";
        }
    }

    // finish html skeleton and send to display
    QString tableHead = "<html><style>td{padding:0 0px;} th{text-align:left}</style><body><table style=\"table-layout: fixed; width: 100%\">";
    tableHead += str;
    tableHead += "</table></body></html>";
    ui->label->setText(tableHead);
}

void InfoWidget::enableOpenMetaData(bool status, QString tooltip)
{
    ui->toolButton_meta->setEnabled(status);
    if (!tooltip.isEmpty()) ui->toolButton_meta->setToolTip(tooltip);
}

void InfoWidget::enableOpenImages(bool status, QString tooltip)
{
    ui->toolButton_folder->setEnabled(status);
    if (!tooltip.isEmpty()) ui->toolButton_folder->setToolTip(tooltip);
}

void InfoWidget::enableOpenVideo(bool status, QString tooltip)
{
    ui->toolButton_video->setEnabled(status);
    if (!tooltip.isEmpty()) ui->toolButton_video->setToolTip(tooltip);
}

void InfoWidget::on_toolButton_folder_clicked()
{
    emit sig_openFolderPressed();
}

void InfoWidget::on_toolButton_video_clicked()
{
    emit sig_openVideoPressed();
}

void InfoWidget::on_toolButton_meta_clicked()
{
    qDebug("openmeta");
    emit sig_openMetaPressed();
}
