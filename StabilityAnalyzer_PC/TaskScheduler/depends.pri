# 通用数据
include(../CommonData/CommonData_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lCommonData
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lCommonDatad
else:unix: LIBS += -L$$PWD/$$DESTDIR/ -lCommonData

# 日志模块
include(../QCuteLogger/QCuteLogger_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLogger
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLoggerd
else:unix:LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLogger

# moudbutrtu通信模块
include(../QModbusRTUUnit/QModbusRTUUnit_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQModbusRTUUnit
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQModbusRTUUnitd
else:unix:LIBS += -L$$PWD/$$DESTDIR/ -lQModbusRTUUnit
################################## TODO可以在这添加UI部分，新增UI子项目然后在这里添加即可 #############################################





