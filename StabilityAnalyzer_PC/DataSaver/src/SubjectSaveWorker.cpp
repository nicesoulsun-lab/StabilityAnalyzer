#include "SubjectSaveWorker.h"
#include <QSharedPointer>
#include <QSqlQuery>
#include <QDateTime>
#include <QMetaEnum>
#include <QUuid>
//#include "qdbpool.h"
#include "BasePackData.h"
#include "SubjectData.h"
#include "Logger.h"
#include "qsqlerror.h"
#include <QJsonObject>
#include <QJsonDocument>
using namespace SubjectType;
SubjectSaveWorker::SubjectSaveWorker(QObject *parent)
    : QObject{parent}
{

}

// 数据存储重要代码：将数据拼接成sql插入m_dataSqlList。
void WheelGeometry_Save::createDataSqlList(QVector<QSharedPointer<SubjectData> > &dataList)
{
    //增加宏判断，区分广州地铁
    QString tableName = "";
    QString csgkId = "";
    tableName = GetCarryTableName("track_trackdata");
    QString timeStr=QDateTime::currentDateTime().toString("yyyyMMdd");
    csgkId = timeStr.append("01");
    if(tableName.isEmpty()){
        LOG_ERROR() << "获取到的专业数据表为空,之前的表名是track_trackdata！";
        return;
    }

    //QString tableName=GetTableName("track_trackdata");
    QString sql = "insert into %1(id,time,post,correct_post,lprf,rprf,laln,raln,gage,cant,xlvl,warp,hacc,vacc,cvtr,csgkid,isupload) values ";
    sql=sql.arg(tableName);
    for(QSharedPointer<SubjectData>& data : dataList){
        // 拼串 执行批量插入
//        QString time = QDateTime::fromMSecsSinceEpoch(data.data()->time()).toString("yyyy-MM-dd hh:mm:ss.zzz");
//        qreal kilometer = data.data()->kilometer();
//        sql.append("(\"").append(data.data()->uuid()).append("\",\"").append(time).append("\",");
//        sql.append(QString::number(data.data()->originKilometer())).append(",").append(QString::number(kilometer)).append(",");
//        sql.append(QString::number(data.data()->get(Config::WheelGeometryParam::LeftHL))).append(",");
//        sql.append(QString::number(data.data()->get(Config::WheelGeometryParam::RightHL))).append(",");
//        sql.append(QString::number(data.data()->get(Config::WheelGeometryParam::LeftDire))).append(",");
//        sql.append(QString::number(data.data()->get(Config::WheelGeometryParam::RightDire))).append(",");
//        sql.append(QString::number(data.data()->get(Config::WheelGeometryParam::Dis))).append(",");
//        sql.append(QString::number(data.data()->get(Config::WheelGeometryParam::SuperHigh))).append(",");
//        sql.append(QString::number(data.data()->get(Config::WheelGeometryParam::Level))).append(",");
//        sql.append(QString::number(data.data()->get(Config::WheelGeometryParam::TrianglePit))).append(",");
//        sql.append(QString::number(data.data()->get(Config::WheelGeometryParam::HorAcc))).append(",");
//        sql.append(QString::number(data.data()->get(Config::WheelGeometryParam::VerAcc))).append(",");
//        sql.append(QString::number(data.data()->get(Config::WheelGeometryParam::DisChange))).append(",\"");
        sql.append(csgkId).append("\",0)");
        sql.append(",");
    }
    sql.remove(sql.size()-1,1);
    sql.append(";");
    m_dataSqlList.append(sql);
}



