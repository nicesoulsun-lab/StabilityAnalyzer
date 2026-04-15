#ifndef DATASAVERWIDGET_H
#define DATASAVERWIDGET_H

#include <QWidget>
#include <QObject>
#include <atomic>
#include <memory>
#include <thread>
#include <QtCore>
#include "SubjectData.h"

QT_BEGIN_NAMESPACE
namespace Ui { class DataSaverWidget; }
QT_END_NAMESPACE

class DataSaverWidget : public QWidget
{
    Q_OBJECT

public:
    DataSaverWidget(QWidget *parent = nullptr);
    ~DataSaverWidget();


private:
    Ui::DataSaverWidget *ui;

};
#endif // DATASAVERWIDGET_H
