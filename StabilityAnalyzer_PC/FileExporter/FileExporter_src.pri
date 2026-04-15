INCLUDEPATH += $$PWD/inc/ \

HEADERS += \
    $$PWD/inc/ExcelExporter.h \
    $$PWD/inc/ExportManager.h \
    $$PWD/inc/IExportInterface.h \
    $$PWD/inc/PdfExporter.h  \
    $$PWD/inc/TextExporter.h  \
    $$PWD/inc/fileexporter_global.h 

SOURCES += \
    $$PWD/src/ExcelExporter.cpp \
    $$PWD/src/ExportManager.cpp \
    $$PWD/src/PdfExporter.cpp \
    $$PWD/src/TextExporter.cpp \
    $$PWD/src/main.cpp
