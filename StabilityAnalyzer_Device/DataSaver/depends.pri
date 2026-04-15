
# 日志模块
include(../QCuteLogger/QCuteLogger_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLogger
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLoggerd
else:unix:LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLogger

# 基础数据结构
include(../CommonData/CommonData_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lCommonData
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lCommonDatad
else:unix:LIBS += -L$$PWD/$$DESTDIR/ -lCommonData

#orm数据库
#include(../sql_orm/sql_orm.pri)

## 数据转发模块
#include(../DataTransmit/DataTransmit_inc.pri)
#win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lDataTransmit
#else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lDataTransmitd
#else:unix:LIBS += -L$$PWD/$$DESTDIR/ -lDataTransmit
