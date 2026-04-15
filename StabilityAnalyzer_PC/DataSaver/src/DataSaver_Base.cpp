#include "DataSaver_Base.h"
#include "SubjectData.h"
#include <QDebug>
#include "logmanager.h"
#include "BasePackData.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileInfo>
#include <QDir>

using namespace SubjectType;

DataSaver_Base::DataSaver_Base(int subject, QObject *parent)
    : QObject{parent}
{
    m_subject = subject;
    QThread* t = new QThread;
    this->moveToThread(t);
    connect(this, SIGNAL(sig_datasaver_start()), this, SLOT(slot_datasaver_start()));
    t->start();
    //qDebug() << "datasave construct ++++ "<< this << QThread::currentThread();
}

DataSaver_Base::~DataSaver_Base()
{

}

void DataSaver_Base::start()
{
    emit sig_datasaver_start();
}

QString DataSaver_Base::GetTableName(const QString &tableName)
{
    QString taskId=m_subTaskInfo.taskId;
//    QString sql = QString("select * from upload_statusinfo where task_id = '%1'").arg(taskId);
//    QSqlQuery tmpquery(m_db);
//    if(tmpquery.exec(sql))
//    {
//        while(tmpquery.next())
//        {
//            QString name=tmpquery.value(2).toString();
//            if(name.contains(tableName))
//            {
//                return name;
//            }
//        }
//        return "";
//    }
    return tableName+"_"+taskId;
    //return "";
}

QString DataSaver_Base::GetCarryTableName(const QString& tableName)
{
    //每个专业的数据表，以专业名+时间命名
    QDateTime current_date_time =QDateTime::currentDateTime();
    QString current_date =current_date_time.toString("yyyy_MM_dd");
    return tableName+"_"+current_date;
}

void DataSaver_Base::executeSql()
{
    if(m_tmpSqlTable.isEmpty()){
        return;
    }
    QSqlQuery query(m_db);
    QString str = "";
    QSqlQuery insquery(m_db);
    // 修改 SQL 语法：反引号改为双引号
    QString sql = tr("select \"id\", \"time\", \"sql\" from %1 order by \"time\" limit 10").arg(m_tmpSqlTable);
    if(query.exec(sql)){
        if(query.size() <= 0){
            return;
        }
        LOG_INFO_TIME(Logger::TimingMs,tr("%1_Execute,query size %2").arg(m_tmpSqlTable).arg(query.size()));

        if(m_db.transaction()){
            //int cnt = 0;
            while(query.next()){
                //LOG_INFO() << tr("执行sql语句数量") << ++cnt << m_subject;
                str.append(tr("'%1',").arg(query.value(0).toString()));
                if(!insquery.exec(query.value(2).toString())){
                    LOG_ERROR() << tr("执行存储sql失败") << insquery.lastError().text() << insquery.lastQuery();
                }
            }
            if(!str.isEmpty()){
                str.remove(str.size()-1,1);
                if(!insquery.exec(tr("delete from %1 where \"id\" in(%2)").arg(m_tmpSqlTable).arg(str))){
                    LOG_ERROR() << tr("删除已执行sql失败") << insquery.lastError().text() << insquery.lastQuery();
                }
            }
            if(m_db.commit()){
                //LOG_INFO() <<tr("执行插入sql完成并删除记录");
            }else{
                LOG_ERROR() << tr("执行存储sql事务提交失败");
                m_db.rollback();
            }
        }else{
            LOG_ERROR() << tr("执行存储sql时开启事务失败");
            m_db.rollback();
        }
    }else{
        LOG_ERROR() <<tr("查询记录失败") << sql << query.lastError().text();
    }
}

void DataSaver_Base::saveSql()
{
    if(m_db.transaction()){
        QSqlQuery query(m_db);
        LOG_INFO() << tr("保存sql语句数量") << m_dataSqlList.size() << m_subject;
        while(m_dataSqlList.size() > 0){
            QString sql = m_dataSqlList.takeFirst();
            // 使用临时表作为缓存 将sql语句存储在临时表中 前提是临时表必须存在
            if(!m_tmpSqlTable.isEmpty()){
                QString insert = tr("insert into %4(\"id\", \"time\", \"sql\", \"isexecute\") values('%1', '%2', '%3', 0)")
                        .arg(QUuid::createUuid().toString().remove('{').remove('}').remove('-'))
                        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                        .arg(sql)
                        .arg(m_tmpSqlTable);
                if(!query.exec(insert))
                {
                    LOG_WARNING() << tr("数据存储错误") << insert << query.lastError().text();
                }
            }else{
                // 不使用临时表 直接执行sql (相当于使用内存作为缓存 数据频率高 数据量大时内存会有压力)
                if(!query.exec(sql))
                {
                    LOG_WARNING() << tr("数据存储错误") << sql << query.lastError().text();
                }
            }

        }
        if(!m_db.commit()){
            LOG_ERROR() << tr("存储数据时事务提交失败");
            m_db.rollback();
        }
    }else{
        LOG_ERROR()<< tr("开启存储事务失败")<<m_db.lastError().text();
    }
}

void DataSaver_Base::startExecuteSql()
{
    if(m_tmpSqlTable.isEmpty()){
        return;
    }
    // 启动一个独立线程 执行插入 sql (注意控制频率 有可能导致 mysql 占用较高的 cpu)

    QString table = m_tmpSqlTable;
    QThread* executeThread = new QThread();
    executeThread->setObjectName("DataSaver_ExecuteThread_" + table);
    
    QObject::connect(executeThread, &QThread::started, executeThread, [table, executeThread](){
        // 创建 SQLite 数据库连接
        QSqlDatabase executeDb = QSqlDatabase::addDatabase("QSQLITE", QUuid::createUuid().toString()+ "-" + "datasaver-execute");
        executeDb.setDatabaseName("../data/detection_data.db");
        if (!executeDb.open()) {
            LOG_ERROR() << "SQLite 数据库连接失败:" << executeDb.lastError().text();
            executeThread->quit();
            executeThread->deleteLater();
            return;
        }
        
        QSqlQuery query(executeDb);
        while(true){
            QThread::msleep(100);
            QString str = "";
            QSqlQuery insquery(executeDb);
            // 修改 SQL 语法：反引号改为双引号
            QString sql = tr("select \"id\", \"time\", \"sql\" from %1 order by \"time\" limit 5").arg(table);
            if(query.exec(sql)){
                if(query.size() <= 0){
                    continue;
                }
                //LOG_INFO_TIME(Logger::TimingMs,tr("%1_Execute,query size %2").arg(table).arg(query.size()));

                if(executeDb.transaction()){
                    while(query.next()){
                        str.append(tr("'%1',").arg(query.value(0).toString()));
                        QString sqlstr = query.value(2).toString();
                        //LOG_INFO() << "inquery------------------------------" << sqlstr;
                        sqlstr.replace("\"{", "'{"); //通信需要存储 json 数据，存储 json 数据的时候双引号变成单引号
                        sqlstr.replace("}\"", "}'");
                        //LOG_INFO() << "inquery2222---------------------------" << sqlstr;

                        if(!insquery.exec(sqlstr)){
                            LOG_ERROR() << tr("执行存储 sql 失败") << insquery.lastError().text() << insquery.lastQuery()<<"m_tmptablesql:"<<table;
                        }
                    }
                    if(!str.isEmpty()){
                        str.remove(str.size()-1,1);
                        if(!insquery.exec(tr("delete from %1 where \"id\" in(%2)").arg(table).arg(str))){
                            LOG_ERROR() << tr("删除已执行 sql 失败") << insquery.lastError().text() << insquery.lastQuery();
                        }
                    }
                    if(executeDb.commit()){
                        //LOG_INFO() <<tr("执行插入 sql 完成并删除记录");
                    }else{
                        LOG_ERROR() << tr("执行存储 sql 事务提交失败");
                        executeDb.rollback();
                    }
                }else{
                    LOG_ERROR() << tr("执行存储 sql 时开启事务失败");
                }
            }else{
                LOG_ERROR() <<tr("查询记录失败") << sql << query.lastError().text();
            }
        }
    });
    
    // 线程结束时自动删除
    QObject::connect(executeThread, &QThread::finished, executeThread, &QThread::deleteLater);
    
    executeThread->start();
}

void DataSaver_Base::createDataSqlList(QVector<QSharedPointer<SubjectData> > &dataList)
{

}

//线程开始
void DataSaver_Base::slot_datasaver_start()
{
    /// 子线程中初始化资源
    if(m_saveTimer == nullptr){
        // 初始化定时器
        m_saveTimer = new QTimer;
        connect(m_saveTimer, SIGNAL(timeout()), this, SLOT(slot_datasaver_saveData()));
        
        // 初始化 SQLite 数据库连接
        m_db = QSqlDatabase::addDatabase("QSQLITE", QUuid::createUuid().toString()+ "-" + "datasaver");
        m_db.setDatabaseName("../data/detection_data.db");
        
        // 确保数据目录存在
        QDir dataDir("../data");
        if (!dataDir.exists()) {
            dataDir.mkpath(".");
        }
        
        if (!m_db.open()) {
            LOG_ERROR() << "SQLite 数据库连接失败:" << m_db.lastError().text();
            return;
        }
        
        LOG_INFO() << "SQLite 数据库连接成功:" << m_db.databaseName();
    }
    // 启动定时执行一次存储
    m_saveTimer->start(m_interval);

    // 开启线程 执行插入的sql语句
    startExecuteSql();
}

//定时执行保存操作
void DataSaver_Base::slot_datasaver_saveData()
{
    /// 此处打印出的线程id应该与构造函数中的线程id不是同一个
    //qDebug() << "datasave savedata ++++ " << QThread::currentThread();
    QMutexLocker lock(&m_mutex);
    // 先停止定时器
    //m_saveTimer->stop();
    // 执行插入操作
    /// saveSql中执行的操作: vector中取出sql语句 -> 拼批量保存sql语句到数据库中
    if(m_dataSqlList.size() > 0){
        // sql语句缓存到数据库中 或者直接将数据保存到库中
        saveSql();
    }
    // 执行数据库中的sql语句
    //executeSql();
    // 重新启动定时器
    //m_saveTimer->start(m_interval);
}

//接收到时空同步发送的数据就开始sql语句拼接
void DataSaver_Base::slot_datasaver_subjectData(SubTaskInfo info,QVector<QSharedPointer<SubjectData>> dataList)
{
    QMutexLocker lock(&m_mutex);
    m_subTaskInfo = info;
    //static qint64 cnt = 0;
    //cnt += dataList.size();
    LOG_INFO() << tr("存储专业数据saver") <<dataList.size();
    // 遍历vecctor数据 拼串 放入m_dataSqlList缓存
    if(dataList.size() <= 0){
        return;
    }
    createDataSqlList(dataList);
    auto size = m_dataSqlList.size();
    //QMetaEnum metaEnumt = QMetaEnum::fromType<EM_Subject>();
    //LOG_INFO() << tr("专业[%1]存储sql串缓存队列[%2], 数据数量[%3]").arg(metaEnumt.valueToKey(EM_Subject(m_subject))).arg(size).arg(dataList.size());
    if(size > 1000){
        LOG_WARNING() << tr("专业[%1]存储sql串缓存队列数据过大,[%2]").arg(m_subject).arg(size);
    }

    // 不使用下面直接缓存数据的方式
    //    m_dataList.append(dataList);
    //    auto size = m_dataList.size();
    //    // qDebug()<<"------------------------------test:"<<size;
    //    if(size > 2000){
    //        QMetaEnum metaEnumt = QMetaEnum::fromType<EM_Subject>();
    //        LOG_WARNING() << tr("专业[%1]存储缓存队列数据过大,[%2]").arg(metaEnumt.valueToKey(EM_Subject(m_subject))).arg(size);
    //    }
}


