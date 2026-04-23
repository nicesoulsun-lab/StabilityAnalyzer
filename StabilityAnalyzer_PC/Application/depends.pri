###### 独立库模块依赖 ######

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

# Modbus RTU 通信模块
include(../QModbusRTUUnit/QModbusRTUUnit_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQModbusRTUUnit
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQModbusRTUUnitd
else:unix:LIBS += -L$$PWD/$$DESTDIR/ -lQModbusRTUUnit

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

# RNDIS TCP 通信模块
include(../DataTransmit/DataTransmit_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lDataTransmit
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lDataTransmitd
else:unix: LIBS += -L$$PWD/$$DESTDIR/ -lDataTransmit

# 主窗口模块
include(../SubApplication/ANALYZER/MainWindow/MainWindow_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lMainWindow
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lMainWindowd
else:unix: LIBS += -L$$PWD/$$DESTDIR/ -lMainWindow
