#include "DataSaverWidget.h"
#include "ui_DataSaverWidget.h"
#include "SubjectClassification.h"
#include "DataClearHandler.h"
DataSaverWidget::DataSaverWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DataSaverWidget)
{
    ui->setupUi(this);

    qDebug()<<"------------------存储:";

    // 启动存储
    SubjectClassification::subjectInstance()->start();
    // 启动清空
    DataClearHandler::intance()->startClear();
}

DataSaverWidget::~DataSaverWidget()
{
    delete ui;
}

//QMap<QPair<int,int>,QMap<int,QString>> map;
//QMap<int,QString> config;
//config.insert(Config::WheelForceParam::Dc,"dc");
//config.insert(Config::WheelForceParam::Dc,"dc");
//config.insert(Config::WheelForceParam::Dc,"dc");
//config.insert(Config::WheelForceParam::Dc,"dc");
//config.insert(Config::WheelForceParam::Dc,"dc");
//map.insert(QPair<int,int>(WheelForce,WheelForce_LGL),config);
//save();
void save(QString tableName,int subject, int childSubject,const QVector<QSharedPointer<SubjectData>> &obj,const QMap<QPair<int,int>,QMap<int,QString>> &map){


    //exec();
}
