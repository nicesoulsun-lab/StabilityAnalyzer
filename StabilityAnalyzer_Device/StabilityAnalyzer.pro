### 平台主框架###
#1 日志模块
#2 单例模式，保证程序不会重复运行
#3 通用数据模块，通用的数据比如结构体可以放在这个位置
#4 日志监控
#5 moudbusrtu通信模块
#6 解压缩模块
#7 曲线封装
#8 主程序模块
#9 子程序模块，需要展示哪个程序就添加哪个子程序
TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    QCuteLogger \
#    QCustomQmlPlugin \
    QSingleapplication \
    CommonData \
    LoggerMonitor\
#    ConfigManager \
#    FileExporter \
    QModbusRTUUnit\
    Algorithm\
#    Quazip\
#    plot\
    DataTransmit \
#    DataSaver \
    SqlOrm \
    TaskScheduler \
    SubApplication \
    Application


