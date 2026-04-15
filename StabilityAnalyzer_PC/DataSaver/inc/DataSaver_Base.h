#ifndef DATASAVER_BASE_H
#define DATASAVER_BASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSharedPointer>
#include <QVector>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QTimer>
#include "TaskInfo.h"


class SubjectData;
/**
 * @brief The DataSaver_Base class
 * 数据存储 - 专业独立存储基类
 * 每个专业一个独立的存储线程
 * 存储策略: 一组原始数据 --> 拼成一条插入sql语句 --> 缓存多条sql语句 --> 使用事务将多条插入操作sql语句保存到临时表中
 * --> 开启一个线程定时从临时表中取出多条sql --> 使用事务执行sql
 */
class DataSaver_Base : public QObject
{
    Q_OBJECT
public:
    explicit DataSaver_Base(int subject = 0, QObject *parent = nullptr);
    virtual ~DataSaver_Base();
    // 启动存储
    void start();

    /**
     * @brief GetTableName 根据旧的数据表名，找到新的数据表名；
     * @param tableName 旧的数据表名
     * @return
     */
    QString GetTableName(const QString& tableName);

    /**
     * @brief GetCarryTableName  广州地铁搭载车-根据旧的数据表名，找到新的数据表名；
     * @param tableName 旧的数据表名
     * @return
     */
    QString GetCarryTableName(const QString& tableName);

private:
    // 执行入库操作 不同的专业表结构不同 插入语句也不同, 派生类中实现
    void saveSql();
    void executeSql();
    void startExecuteSql();

    void startExcuteSqlUseCopy();
protected:

    // 创建sql串
    virtual void createDataSqlList(QVector<QSharedPointer<SubjectData>>& dataList);
private slots:
    //void slot_timeSpaceSyncController_subjectData(int subject, int childSubject, QSharedPointer<SubjectData> obj);
    // 开始执行初始化 在子线程中创建资源
    void slot_datasaver_start();
    // 执行存储
    void slot_datasaver_saveData();
    // 接收数据 存入队列中
    void slot_datasaver_subjectData(SubTaskInfo info,QVector<QSharedPointer<SubjectData>> dataList);

signals:
    // 初始化信号
    void sig_datasaver_start();
protected:
    QMutex m_mutex;
    // 数据存储长连接
    QSqlDatabase m_db;
    // 存储缓存队列
    QVector<QSharedPointer<SubjectData>> m_dataList;
    QVector<QString> m_dataSqlList;
    // 当前工况信息
    SubTaskInfo m_subTaskInfo;
    int m_subject = 0;
    QString m_tmpSqlTable = "";
    // 当前趟数信息
    int m_timeIndex=0;

private:
    QTimer* m_saveTimer = nullptr;
    // 定时存储时间间隔 默认2s
    int m_interval = 100;

};

#endif // DATASAVER_BASE_H
