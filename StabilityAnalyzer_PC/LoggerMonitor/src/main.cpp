#include "LoggerMonitorWidget.h"
#include "LoggerMonitor.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    LOGGERMONITOR->init();
    LOGGERMONITOR->showMonitor();
    LOGGERMONITOR->appendLogger("HAHAHAHAH", "",0,QDate::currentDate().toString("yyyy-MM-dd"));
    return a.exec();
}
