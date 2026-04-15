#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QTextCodec>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "modbustaskscheduler.h"
#include "portmanager.h"
#include "device.h"
#include "task.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "TaskScheduler 测试程序启动";
    
    // 创建任务调度器
    ModbusTaskScheduler scheduler;
    
    // 设置配置文件路径
    QString path = QCoreApplication::applicationDirPath() + "/config/";
    scheduler.setConfigFile(path);
    
    // 测试配置加载
    if (scheduler.loadConfiguration()) {
        qDebug() << "配置文件加载成功";
        
        // 执行初始化任务
        scheduler.executeInitTasks( );
        
        qDebug() << "初始化任务执行完成";
        
        // 启动调度器
        if (scheduler.startScheduler()) {
            qDebug() << "调度器已启动";
        } else {
            qDebug() << "调度器启动失败";
        }
        
        // 10秒后停止调度器并退出程序
        //        QTimer::singleShot(10000, [&]() {
        //            qDebug() << "停止调度器...";
        //            scheduler.stopScheduler();
        //            qDebug() << "程序退出";
        //            app.quit();
        //        });
        
    } else {
        qDebug() << "配置文件加载失败";
        return -1;
    }
    
    return app.exec();
}
