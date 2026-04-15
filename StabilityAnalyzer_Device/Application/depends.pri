
###### 独立库模块 1.引入头文件依赖 2.添加导入库######

# 日志模块
include(../QCuteLogger/QCuteLogger_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLogger
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLoggerd
else:unix:LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLogger

# 单例模块
include(../QSingleapplication/QSinglecoreapplication_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQSingleApplication
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQSingleApplicationd
else:unix:LIBS += -L$$PWD/$$DESTDIR/ -lQSingleApplication

# 通用数据
include(../CommonData/CommonData_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lCommonData
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lCommonDatad
else:unix: LIBS += -L$$PWD/$$DESTDIR/ -lCommonData

# moudbutrtu 通信模块
include(../QModbusRTUUnit/QModbusRTUUnit_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQModbusRTUUnit
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQModbusRTUUnitd
else:unix:LIBS += -L$$PWD/$$DESTDIR/ -lQModbusRTUUnit

# 解压缩
#include(../Quazip/Quazip_inc.pri)
#win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQuazip
#else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQuazipd
#else:unix: LIBS += -L$$PWD/$$DESTDIR/ -lQuazip

# 曲线,暂时不需要了，直接使用mainwindow后面写的曲线界面就行
#include(../plot/plot.pri)

# 任务调度器模块
include(../TaskScheduler/TaskScheduler_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lTaskScheduler
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lTaskSchedulerd
else:unix:LIBS += -L$$PWD/$$DESTDIR/ -lTaskScheduler

# SqlOrm 数据库模块
include(../SqlOrm/SqlOrm_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lSqlOrm
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lSqlOrmd
else:unix: LIBS += -L$$PWD/$$DESTDIR/ -lSqlOrm

# 主窗口 可以根据不同项目加载不同主窗口,qml项目可以使用qml.qrc里面添加对应的界面实现
include(../SubApplication/ANALYZER/MainWindow/MainWindow_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lMainWindow
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lMainWindowd
else:unix: LIBS += -L$$PWD/$$DESTDIR/ -lMainWindow
