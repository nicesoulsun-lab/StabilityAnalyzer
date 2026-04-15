#include <QApplication>
#include <QCoreApplication>
#include "widget.h"
#include "logmanager.h"
#include <QDebug>
#include <iostream>
#include <QVector>
#include <QTime>

typedef struct {
    double k;
    double x;
    double y;
    double z;
}TestData;

// 二分查找
int find(double dst, const QVector<TestData> &src)
{
    LOG_TRACE_TIME(Logger::TimingMs,"查找数据",Logger::Console);
    if(src.isEmpty())
        return -1;

    int begin = 0;
    int end = src.size()-1;

    /* 二分法查询 */
    int mid = 0;
    while (begin<end) {
        mid = (end+begin)/2;
        if(src.at(mid).k == dst){
            return mid;
        }else if(src.at(mid).k < dst){
            begin = mid+1;
        }else{
            end = mid-1;
        }
    }
    if(end<0){
        return begin;
    }
    if(begin>=src.size()){
        return end;
    }
    if(qAbs(src.at(begin).k-dst)<qAbs(src.at(end).k-dst))
        return begin;
    else
        return end;
}


int main(int argc, char *argv[])
{
    //QApplication a(argc, argv);
    QCoreApplication ca(argc, argv);
    LogManager log;
    log.InitLog("./log","log",true);
    //Widget w;
    //w.show();
    //qDebug()<<"main run ";
    //qWarning()<<"this is a warning";
    LOG_TRACE(QObject::tr("log trace console"),Logger::Console);
    LOG_INFO(QObject::tr("log info console"),Logger::Console);
    LOG_INFO(QObject::tr("log info file %1").arg("test"),Logger::File);
    //LOG_INFO("log info consolefile",Logger::ConsoleFile);
    //LOG_INFO()<<"this is a test";
    //LOG_DEBUG("this is a debug ",Logger::Console);
    //LoggerTimingHelper timing(cuteLoggerInstance(),Logger::Info,__FILE__,__LINE__,Q_FUNC_INFO);
    //w.test();
    LOG_TRACE_TIME(Logger::TimingMs,"",Logger::Console);
    //LOG_INFO_TIME(Logger::TimingMs,"this is a info timer");
    //LOG_DEBUG_TIME(Logger::TimingMs,"this is a debug timer");
    //LOG_ERROR()<<"";

    /// 二分查找测试 run in terminal
    //    {
    //        QVector<TestData> data;
    //        for(int i = 0; i<10000000; i++){
    //            data.push_back({i*1.111, 0, 0, 0});
    //        }
    //        while (true) {
    //            std::cout << QString("请输入一个数字: ").toLocal8Bit().data();
    //            std::string input;
    //            std::getline(std::cin, input);
    //            double f = std::stod(input);

    //            QTime time;
    //            time.start();

    //            int index = find(f,data);
    //            if(index>=0){
    //                std::cout<< QString("结果: ").toLocal8Bit().data()<<index<<":"<<data.at(index).k<<std::endl;
    //                std::cout<< time.elapsed() << "ms"<<std::endl;
    //            }else{
    //                std::cout << "error";
    //            }
    //        }

    //    }

    return ca.exec();
}
