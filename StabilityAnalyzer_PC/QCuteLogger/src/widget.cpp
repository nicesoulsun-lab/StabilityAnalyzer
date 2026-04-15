#include "widget.h"
#include "ui_widget.h"
#include "logmanager.h"
#include <QThread>
Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::test()
{
    LOG_TRACE_TIME(Logger::TimingMs,"",Logger::ConsoleFile);
    QThread::sleep(3);
    //LOG_TRACE_TIME("Foo");
    //LOG_DEBUG_TIME(Logger::TimingMs,"test debug timer");

}
