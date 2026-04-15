/********************************************************************************
** Form generated from reading UI file 'LoggerMonitorWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.12.12
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOGGERMONITORWIDGET_H
#define UI_LOGGERMONITORWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_LoggerMonitorWidget
{
public:
    QGridLayout *gridLayout_2;
    QGridLayout *gridLayout;
    QPlainTextEdit *plainText_logger;

    void setupUi(QWidget *LoggerMonitorWidget)
    {
        if (LoggerMonitorWidget->objectName().isEmpty())
            LoggerMonitorWidget->setObjectName(QString::fromUtf8("LoggerMonitorWidget"));
        LoggerMonitorWidget->resize(549, 426);
        gridLayout_2 = new QGridLayout(LoggerMonitorWidget);
        gridLayout_2->setSpacing(2);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        gridLayout_2->setContentsMargins(2, 2, 0, 2);
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        plainText_logger = new QPlainTextEdit(LoggerMonitorWidget);
        plainText_logger->setObjectName(QString::fromUtf8("plainText_logger"));
        plainText_logger->setFrameShape(QFrame::NoFrame);
        plainText_logger->setLineWidth(0);
        plainText_logger->setReadOnly(true);

        gridLayout->addWidget(plainText_logger, 0, 0, 1, 1);


        gridLayout_2->addLayout(gridLayout, 0, 0, 1, 1);


        retranslateUi(LoggerMonitorWidget);

        QMetaObject::connectSlotsByName(LoggerMonitorWidget);
    } // setupUi

    void retranslateUi(QWidget *LoggerMonitorWidget)
    {
        LoggerMonitorWidget->setWindowTitle(QApplication::translate("LoggerMonitorWidget", "LoggerMonitorWidget", nullptr));
    } // retranslateUi

};

namespace Ui {
    class LoggerMonitorWidget: public Ui_LoggerMonitorWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOGGERMONITORWIDGET_H
