#ifndef DATACLEARHANDLER_H
#define DATACLEARHANDLER_H

#include <QObject>
#include <QTimer>
#include "datasaver_global.h"
#include <QSqlQuery>

/**
 * @brief The DataClearHandler class
 * 历史数据清除 独立线程中运行
 * 应用每次启动时执行
 * 10次任务之前的 已上传的数据执行清除操作
 * TODO 考虑将该模块放在车地传输应用中执行
 */
class DATASAVER_EXPORT DataClearHandler : public QObject
{
    Q_OBJECT
public:
    explicit DataClearHandler(QObject *parent = nullptr);
    // 启动数据清除 只需调用一次即可
    void startClear();
    static DataClearHandler* intance();
private slots:
    void slot_dataclear_init();
    // 执行清除操作
    void slot_dataclear_clear();
private:
    bool checkHasCSGK(QSqlQuery query, const QString& sql);

    /**
     * @brief DeleteData 删除超过10次的数据
     */
    void deleteData();

    /**
     * @brief DeleteDataByDate 根据时间查询历史数据表进行清除历史数据
     */
    void deleteDataByDate();

    /**
     * @brief deleteDataFromStaledata 查询上传完成的数据进行删除
     */
    void deleteDataFromStaledata();

    /**
     * @brief getNeedDeleteDateInfo 获取需要删除的日期信息
     * @param query 数据库操作对象
     * @return 查询到的需要删除的日期信息数组
     */
    QStringList getNeedDeleteDateInfo(QSqlQuery query);
    // 获取需要删除的数据表名
    QStringList getDataTableNamesNewPg();

    /**
     * @brief getTableNameByDateInfo 根据日期，获取需要删除的数据表名
     * @param date 日期
     * @param query 数据库操作对象
     * @return 查询出的数据表名数组
     */
    QStringList getTableNamesByDateInfo(QString date,QSqlQuery query);
    /**
     * @brief deleteDataTable 删除数据存储表
     * @param tableNames 需要删除的数据表名
     * @param query 数据库操作对象
     * @return 是否操作成功
     */
    bool deleteDataTable(QStringList tableNames,QSqlQuery query);

    /**
     * @brief deleteDataTableInfoFromIntervalTable 删除数据表信息，从数据传输中间概况表中
     * @param date 需要删除的日期信息
     * @param query 数据库操作对象
     * @return 是否操作成功
     */
    bool deleteDataTableInfoFromIntervalTable(QString date,QSqlQuery query);

    /**
     * @brief deleteFileInfosFromTable  删除文件信息概况表的记录
     * @param date 需要删除的日期信息
     * @param query 数据库操作对象
     * @return 是否操作成功
     */
    bool deleteFileInfosFromTable(QString date,QSqlQuery query);
    /**
     * @brief deleteStaledata 从上传完成数据表删除数据
     * @param tableName 数据表名
     * @return 删除数据个数
     */
    int deleteStaledata(QSqlQuery query,QString tableName);

signals:
    void sig_dataclear_init();
private:
    QTimer* m_clearTimer = nullptr;
};

#endif // DATACLEARHANDLER_H
