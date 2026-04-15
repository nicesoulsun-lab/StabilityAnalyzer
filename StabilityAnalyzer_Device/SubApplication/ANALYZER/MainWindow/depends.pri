# 通用数据
include(../../../CommonData/CommonData_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lCommonData
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lCommonDatad
else:unix: LIBS += -L$$PWD/$$DESTDIR/ -lCommonData

# 日志模块
include(../../../QCuteLogger/QCuteLogger_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLogger
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLoggerd
else:unix:LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLogger

# moudbutrtu 通信模块
include(../../../QModbusRTUUnit/QModbusRTUUnit_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQModbusRTUUnit
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQModbusRTUUnitd
else:unix:LIBS += -L$$PWD/$$DESTDIR/ -lQModbusRTUUnit

# ORM 数据库模块
include(../../../SqlOrm/SqlOrm_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lSqlOrm
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lSqlOrmd
else:unix:LIBS += -L$$PWD/$$DESTDIR/ -lSqlOrm

# 任务调度器模块
include(../../../TaskScheduler/TaskScheduler_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lTaskScheduler
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lTaskSchedulerd
else:unix:LIBS += -L$$PWD/$$DESTDIR/ -lTaskScheduler
