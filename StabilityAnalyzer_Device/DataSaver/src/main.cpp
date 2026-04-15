#include "DataSaverWidget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DataSaverWidget w;
    w.show();
    return a.exec();
}
