#include "mainwindow.h"
#include "goserver.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    GoServer server;
    MainWindow w;
    //w.show();
    return a.exec();
}
