#include "view/mainwindow.h"
#include "controller.h"
#include "nouicontroller.h"
#include "stringcontainer.h"

#include <QApplication>
#include "view/darkstyle/DarkStyle.h"
#include "cvmat_qmetadata.h"

#include <QDebug>
#include <QFlags>

#include <stdio.h>
#include <iostream>



void ignoreMessages(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    (void) type;
    (void) context;
    (void) msg;
}

int main(int argc, char *argv[])
{

    qRegisterMetaType<cv::Mat>("cvMat");
    qRegisterMetaType<ImageList>("ImageList");
    qRegisterMetaType<QStringList>("QStringList");
    qRegisterMetaType<QColorList>("QColorList");
    qRegisterMetaType<QBoolList>("QBoolList");

    int num = argc;
    if (num < 2) {
        //FreeConsole();
        QApplication a( argc, argv );
        Controller mainController;
        qApp->setProperty(stringContainer::UIIdentifier, true);

        return a.exec();
    }
    else {
        //Disable all Messages comming from Qt
        qInstallMessageHandler(ignoreMessages);
        QCoreApplication a( argc, argv );
        qApp->setProperty(stringContainer::UIIdentifier, false);
        noUIController* noUI = new noUIController(qApp->arguments());
        QTimer::singleShot(0, noUI, SLOT(exec()));
        return a.exec();
    }

}


