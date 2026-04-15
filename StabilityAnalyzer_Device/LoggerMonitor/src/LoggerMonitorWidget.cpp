#include "LoggerMonitorWidget.h"
#include "ui_LoggerMonitorWidget.h"
#include "LoggerMonitor.h"
#include <QTimer>
#include <QDateTime>
#include <QDesktopWidget>
#include <QDebug>
#include <QStyle>
#include "logmanager.h"
#include <QThread>

LoggerMonitorWidget::LoggerMonitorWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoggerMonitorWidget)
{
    ui->setupUi(this);
    this->setWindowTitle(tr("事件监控"));
    this->resize(QApplication::desktop()->availableGeometry().width()/2, QApplication::desktop()->availableGeometry().height() - 100);
    move((QApplication::desktop()->width() - this->width())/2,(QApplication::desktop()->height() - this->height())/2);
    ui->plainText_logger->setMaximumBlockCount(1000);


    LOG_INFO() << tr("日志监控模块窗口初始化完成") << QThread::currentThread();
}

LoggerMonitorWidget::~LoggerMonitorWidget()
{
    delete ui;
}

LoggerMonitorWidget *LoggerMonitorWidget::monitorWidget()
{
    static LoggerMonitorWidget* l = nullptr;
    if(l == nullptr){
        l = new LoggerMonitorWidget;
    }
    return l;
}

void LoggerMonitorWidget::showEvent(QShowEvent *event)
{
    emit sig_show();
    ui->plainText_logger->clear();

    /*
     * 使用Qt框架中的QWidget类进行窗口操作，调用了QWidget的move()方法来移动窗口位置，但发现窗口位置发生了偏移，窗口高度似乎增加了一个标题栏的高度。
     * 这可能是由于窗口的位置是相对于窗口的父级部件（如窗口管理器）而不是屏幕的绝对位置。如果您在移动窗口时没有考虑到标题栏的高度，那么窗口的位置可能会发生偏移。
     * 解决此问题的方法是，在调用QWidget的move()方法之前，先调用QWidget的frameGeometry()方法获取窗口在屏幕上的几何位置，
     * 然后计算标题栏的高度并将其纳入考虑，最后再调用move()方法移动窗口
     * frameGeometry() 该方法返回一个QRect对象，该对象描述了窗口在屏幕上的几何位置，包括窗口的位置、大小以及边框和标题栏等部分的大小
     * geometry()方法返回的是一个QRect对象，获取窗口的客户区域（即不包括边框和标题栏的部分）,该对象描述了窗口的客户区域在父级部件坐标系中的位置和大小
     * 同理frameSize与size区别
    */

    // 获取窗口在屏幕上的几何位置
    QRect frame = this->frameGeometry();
    // 计算标题栏的高度
    int titleBarHeight = frame.y() - this->geometry().y();
    // 移动窗口位置，考虑标题栏高度
    move(frame.x(), frame.y() + titleBarHeight);

    ui->plainText_logger->moveCursor(QTextCursor::End);
    QWidget::showEvent(event);
}

void LoggerMonitorWidget::closeEvent(QCloseEvent *event)
{
    emit sig_close();
}
//接收日志
void LoggerMonitorWidget::slot_monitorWidget_appendMsg(QString msg)
{

    //LOG_INFO() << tr("日志监控模块显示日志信息") << QThread::currentThread() << msg;
    ui->plainText_logger->appendHtml(msg);

    //    static int tempsum=0;
    //    tempsum++;
    //    if(tempsum>50)
    //    {

    //        //    ui->plainText_logger->moveCursor(QTextCursor::End);
    //        //    ui->plainText_logger->insertPlainText(msg);
    //        tempsum=0;
    //    }

}

