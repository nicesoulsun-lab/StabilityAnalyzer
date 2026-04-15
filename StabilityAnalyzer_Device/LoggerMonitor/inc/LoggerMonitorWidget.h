#ifndef LOGGERMONITORWIDGET_H
#define LOGGERMONITORWIDGET_H

#include <QWidget>
#include <QVector>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class LoggerMonitorWidget; }
QT_END_NAMESPACE

class LoggerMonitorWidget : public QWidget
{
    Q_OBJECT

public:
    LoggerMonitorWidget(QWidget *parent = nullptr);
    ~LoggerMonitorWidget();
    static LoggerMonitorWidget* monitorWidget();
protected:
     void showEvent(QShowEvent *event);
     void closeEvent(QCloseEvent *event);
public slots:
     void slot_monitorWidget_appendMsg(QString msg);
signals:
     void sig_close();
     void sig_show();
private:
    Ui::LoggerMonitorWidget *ui;
};
#endif // LOGGERMONITORWIDGET_H
