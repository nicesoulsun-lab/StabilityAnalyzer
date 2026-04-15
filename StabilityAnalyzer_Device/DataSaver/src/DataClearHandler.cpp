#include "DataClearHandler.h"
#include <QThread>
#include <QSqlResult>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include "logmanager.h"
#include <QStringList>
#include <QVector>
#include <QDir>
static constexpr int keepHistoryCount = 10;

DataClearHandler::DataClearHandler(QObject *parent)
    : QObject{parent}
{
    QThread* t = new QThread;
    this->moveToThread(t);
    connect(this, SIGNAL(sig_dataclear_init()), this, SLOT(slot_dataclear_init()));
    t->start();
}

void DataClearHandler::startClear()
{
    emit sig_dataclear_init();
}

DataClearHandler *DataClearHandler::intance()
{
    static DataClearHandler* d = nullptr;
    if(d == nullptr){
        d = new DataClearHandler;
    }
    return d;
}

void DataClearHandler::slot_dataclear_init()
{
    if(m_clearTimer == nullptr){
        m_clearTimer = new QTimer;
        connect(m_clearTimer, SIGNAL(timeout()), this, SLOT(slot_dataclear_clear()));
    }
    // 10s后执行清除操作
    m_clearTimer->start(10000);
}

void DataClearHandler::slot_dataclear_clear()
{
    m_clearTimer->stop();

    deleteDataByDate();

    // 暂定每小时执行一次
    m_clearTimer->start(6000 * 10 * 60);
}

bool DataClearHandler::checkHasCSGK(QSqlQuery query, const QString &sql)
{
    if(!query.exec(sql)){
        LOG_WARNING() << tr("清除历史数据时错误") << query.lastError().text() << sql;
        return false;
    }
    if(query.next() && query.value(0).toInt() > 0){
        return false;
    }
    return true;
}

void DataClearHandler::deleteData()
{
    // 创建 SQLite 数据库连接
    QSqlDatabase dbcon = QSqlDatabase::addDatabase("QSQLITE", "DataClearHandler-dbcon");
    dbcon.setDatabaseName("../data/detection_data.db");
    
    // 确保数据目录存在
    QDir dataDir("../data");
    if (!dataDir.exists()) {
        dataDir.mkpath(".");
    }
    
    if (!dbcon.open()) {
        LOG_ERROR() << "SQLite 数据库连接失败:" << dbcon.lastError().text();
        return;
    }
    
    QSqlQuery query(dbcon);
    QSqlQuery deleteQuery(dbcon);

    // 查询中间表，isupload值为1的任务数及任务id ，时间降序排序，并保存查询到的任务id保存到数组a中；
    QString sql=tr("select task_id, max(time) as createtime from upload_statusinfo where isupload = 1 group by task_id order by  createtime desc");
    if(!query.exec(sql))
    {
        LOG_WARNING() << tr("历史数据清理时错误") << query.lastError().text();
        return;
    }

    QVector<QString> needUploadTaskId;
    while(query.next())
    {
        needUploadTaskId.append(query.value(0).toString());
    }
    // 查询中间表，isupload值为0的任务数据，并保存查询到的任务id保存到数据b中；
    sql=tr("select task_id, max(time) as createtime from upload_statusinfo where isupload = 0 group by task_id");
    if(!query.exec(sql))
    {
        LOG_WARNING() << tr("历史数据清理时错误") << query.lastError().text();
        return;
    }

    QVector<QString> notNeedUploadTaskId;
    while(query.next())
    {
        notNeedUploadTaskId.append(query.value(0).toString());
    }

    // 在数组a中去除数组b中存在的任务id；
    for(QString tempStr : notNeedUploadTaskId)
    {
        if(needUploadTaskId.contains(tempStr))
        {
            needUploadTaskId.removeOne(tempStr);
        }
    }
    // 判断数组a中的数据个数大于10，进行下一步骤；
    if(needUploadTaskId.size()>keepHistoryCount)
    {
        // 因为数组a中的数据时按照时间降序排序，删除数组中索引值9之后的所有任务数据
        QStringList tempTaskIdList{};
        for (int i=keepHistoryCount;i<needUploadTaskId.size();i++)
        {
            tempTaskIdList.append(needUploadTaskId.at(i));
        }

        for (QString str : tempTaskIdList)
        {

            // 根据任务id 查询中间表，找到需要上传的数据表信息，删除数据表
            sql=tr("select table_name from upload_statusinfo where task_id = '%1'").arg(str);
            if(!query.exec(sql)){
                LOG_WARNING() << tr("历史数据清理时错误") << query.lastError().text();
                return;
            }
            while(query.next())
            {
                QString tempTableName=query.value(0).toString();
                sql=tr("drop table if exists %1 ").arg(tempTableName);
                if(!deleteQuery.exec(sql))
                {
                    LOG_WARNING() << tr("历史数据清理时错误") << query.lastError().text();
                    return;
                }
            }

            // 根据任务id ，查找工况信息，删除信息；
            sql=tr("delete from xtsz_rwgl_csgk where taskid = '%1'").arg(str);
            if(!deleteQuery.exec(sql))
            {
                LOG_WARNING() << tr("历史数据清理时错误") << query.lastError().text();
                return;
            }
            // 根据任务id，查找任务信息，删除信息；
            sql=tr("delete from xtsz_rwgl_rwxx where id = '%1'").arg(str);
            if(!deleteQuery.exec(sql))
            {
                LOG_WARNING() << tr("历史数据清理时错误") << query.lastError().text();
                return;
            }
            // 最后根据任务id，删除中间表信息；
            sql=tr("delete from upload_statusinfo where task_id = '%1'").arg(str);
            if(!deleteQuery.exec(sql))
            {
                LOG_WARNING() << tr("历史数据清理时错误") << query.lastError().text();
                return;
            }
        }
    }
    // SQLite 连接自动关闭
    dbcon.close();
}

void DataClearHandler::deleteDataByDate()
{
    // 创建 SQLite 数据库连接
    QSqlDatabase dbcon = QSqlDatabase::addDatabase("QSQLITE", "DataClearHandler-dbcon");
    dbcon.setDatabaseName("../data/detection_data.db");
    
    // 确保数据目录存在
    QDir dataDir("../data");
    if (!dataDir.exists()) {
        dataDir.mkpath(".");
    }
    
    if (!dbcon.open()) {
        LOG_ERROR() << "SQLite 数据库连接失败:" << dbcon.lastError().text();
        return;
    }
    
    QSqlQuery query(dbcon);
    QStringList dateList=getDataTableNamesNewPg();
    LOG_INFO()<<"清除模块"<<dateList.size();
    QStringList tempTableNames{};
    for (QString str : dateList)
    {
         //LOG_INFO()<<"清除模块"<<str<<tempTableNames.size();
         deleteStaledata(query,str);
    }
    // 删除一个月前的上传完成的文件数据信息；
     // deleteFileInfosFromTable("",query);
    // SQLite 连接自动关闭
    dbcon.close();
}

void DataClearHandler::deleteDataFromStaledata()
{


}

QStringList DataClearHandler::getNeedDeleteDateInfo(QSqlQuery query)
{
    // 查询中间表，isupload值为1的，时间降序排序，并保存查询到的时间保存到数组a中；
    QStringList temp{};
    QString sql=tr("select time as createtime from upload_statusinfo where isupload = 1 group by createtime order by  createtime desc");
    if(!query.exec(sql))
    {
        LOG_WARNING() << tr("历史数据清除错误-查找已上传的日期信息") << query.lastError().text();
        return temp;
    }

    QVector<QString> needUploadTaskId;
    while(query.next())
    {
        needUploadTaskId.append(query.value(0).toString());
    }
    // 查询中间表，isupload值为0的任务数据，并保存查询到的时间保存到数据b中；
    sql=tr("select time as createtime from upload_statusinfo where isupload = 0 group by createtime");
    if(!query.exec(sql))
    {
        LOG_WARNING() << tr("历史数据清理时错误-查找未上传的日期信息") << query.lastError().text();
        return temp;
    }

    QVector<QString> notNeedUploadTaskId;
    while(query.next())
    {
        notNeedUploadTaskId.append(query.value(0).toString());
    }

    // 在数组a中去除数组b中存在的时间；
    for(QString tempStr : notNeedUploadTaskId)
    {
        if(needUploadTaskId.contains(tempStr))
        {
            needUploadTaskId.removeOne(tempStr);
        }
    }

    QStringList tempdateList{};
    if(needUploadTaskId.size()>keepHistoryCount)
    {
        // 因为数组a中的数据时按照时间降序排序，删除数组中索引值9之后的所有时间数据

        for (int i=keepHistoryCount;i<needUploadTaskId.size();i++)
        {
            tempdateList.append(needUploadTaskId.at(i));
        }
    }

    return tempdateList;
}

QStringList DataClearHandler::getDataTableNamesNewPg()
{
    QStringList tempdateList{};
    // 轮轨力
    tempdateList.append("wrf_wrf_staledata");
    tempdateList.append("wrf_stability_staledata");
    tempdateList.append("wrf_noise_staledata");

    tempdateList.append("track_trackdata_staledata");

    tempdateList.append("signaldata_staledata");

    tempdateList.append("comm_ltemstren_staledata");
    tempdateList.append("comm_ltemswitch_staledata");
    tempdateList.append("comm_ltemfile_staledata");
    tempdateList.append("comm_wlanstren_staledata");
    tempdateList.append("comm_wlanfile_staledata");
    tempdateList.append("comm_ltemserviceconn_staledata");
    tempdateList.append("comm_ltemservicetrans_staledata");
    tempdateList.append("comm_wlanserviceconn_staledata");
    tempdateList.append("comm_wlanservicetrans_staledata");
    tempdateList.append("comm_wifiserviceconn_staledata");
    tempdateList.append("comm_wifiservicetrans_staledata");
    tempdateList.append("comm_5gserviceconn_staledata");
    tempdateList.append("comm_5gservicetrans_staledata");
    tempdateList.append("comm_euhtserviceconn_staledata");
    tempdateList.append("comm_euhtservicetrans_staledata");
    tempdateList.append("comm_paramconfiginfo_staledata");

    tempdateList.append("tightening_status_staledata");
    tempdateList.append("switch_closure_staledata");

    // tempdateList.append("run_gear_staledata");
    return tempdateList;
}

QStringList DataClearHandler::getTableNamesByDateInfo(QString date, QSqlQuery query)
{
    QStringList tempTableNames{};
    QString sql=tr("select table_name from upload_statusinfo where time = '%1'").arg(date);
    if(!query.exec(sql)){
        LOG_WARNING() << tr("历史数据清理时错误-查询检测日期对应的数据表信息") << query.lastError().text();
        return tempTableNames;
    }
    while(query.next())
    {
        QString tempTableName=query.value(0).toString();
        tempTableNames.append(tempTableName);
    }
    return tempTableNames;
}

bool DataClearHandler::deleteDataTable(QStringList tableNames, QSqlQuery query)
{
    for(QString tempTableName : tableNames)
    {
        QString sql=tr("drop table if exists %1 ").arg(tempTableName);
        if(!query.exec(sql))
        {
            LOG_WARNING() << tr("历史数据清理时错误-删除数据表") << query.lastError().text();
            return false;
        }
    }
    return true;
}

bool DataClearHandler::deleteDataTableInfoFromIntervalTable(QString date, QSqlQuery query)
{
    // 删除日期对应的数据记录；
    QString sql=tr("delete from upload_statusinfo where time = '%1'").arg(date);
    if(!query.exec(sql))
    {
        LOG_WARNING() << tr("历史数据清理时错误") << query.lastError().text();
        return false;
    }
    return true;
}

bool DataClearHandler::deleteFileInfosFromTable(QString date, QSqlQuery query)
{
    // 删除日期以前的数据记录；
    QString sql=tr("delete from http_uploadfile_statusinfo where isuploaded = 1 and time <= '%1'").arg(QDateTime::currentDateTime().addMonths(-1).toString("yyyy/MM/dd"));
    if(!query.exec(sql))
    {
        LOG_WARNING() << tr("历史数据清理时错误-删除文件存储信息") << query.lastError().text();
        return false;
    }
    return true;
}

int DataClearHandler::deleteStaledata(QSqlQuery query, QString tableName)
{
    // 数据清除重要代码：2、定时器自动删除一个月之前的数据
    QString sql=tr("delete from %1 where time <= '%2'").arg(tableName).arg(QDateTime::currentDateTime().addMonths(-1).toString("yyyy-MM-dd"));
    if(!query.exec(sql))
    {
        LOG_WARNING() << tr("历史数据清理时错误-删除历史数据失败信息") << query.lastError().text();
        return 0;
    }
    else {
        int tempInt=query.numRowsAffected();
        //LOG_INFO()<<"删除数据条数"<<tempInt<<tableName;
        return tempInt;

    }
}
