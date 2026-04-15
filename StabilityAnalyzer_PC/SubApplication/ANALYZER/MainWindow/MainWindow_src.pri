
INCLUDEPATH += $$PWD/inc/ \

HEADERS += \
        $$PWD/inc/Common/systemdata.h \
        $$PWD/inc/Common/update_apk.h \
        $$PWD/inc/Controller/controllerManager.h \
        $$PWD/inc/Controller/experiment_ctrl.h \
        $$PWD/inc/Controller/systemsetting_ctrl.h \
        $$PWD/inc/Controller/user_ctrl.h \
        $$PWD/inc/Controller/data_ctrl.h \
        $$PWD/inc/DataModel/experiment_listmodel.h \
        $$PWD/inc/DataModel/user_sql_listmodel.h \
        $$PWD/inc/mainwindow_global.h \
        $$PWD/inc/MainWindow.h \
        $$PWD/inc/CurveItem.h \
        $$PWD/inc/CurveDataModel.h
SOURCES += \
        $$PWD/src/Common/systemdata.cpp \
        $$PWD/src/Common/update_apk.cpp \
        $$PWD/src/Controller/experiment_ctrl.cpp \
        $$PWD/src/Controller/systemsetting_ctrl.cpp \
        $$PWD/src/Controller/user_ctrl.cpp \
        $$PWD/src/Controller/data_ctrl.cpp \
        $$PWD/src/DataModel/experiment_listmodel.cpp \
        $$PWD/src/DataModel/user_sql_listmodel.cpp \
        $$PWD/src/main.cpp \
        $$PWD/src/MainWindow.cpp \
        $$PWD/src/CurveItem.cpp \
        $$PWD/src/CurveDataModel.cpp

FORMS += \
       $$PWD/ui/MainWindow.ui
