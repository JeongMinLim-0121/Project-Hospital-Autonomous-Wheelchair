#include "mainwindow.h"
#include "login.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    qputenv("QT_SCALE_FACTOR", "1");
    QApplication a(argc, argv);
    MainWindow w;
    //Login w;
    w.show();
    return a.exec();
}
