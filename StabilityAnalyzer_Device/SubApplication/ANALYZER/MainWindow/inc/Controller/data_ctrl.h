#ifndef DATA_CTRL_H
#define DATA_CTRL_H

#include <QObject>
#include <QVariantMap>
#include <QVector>
#include "mainwindow_global.h"

class SqlOrmManager;

class MAINWINDOW_EXPORT dataCtrl : public QObject
{
    Q_OBJECT

public:
    explicit dataCtrl(QObject *parent = nullptr);
    ~dataCtrl();

    // ==================== 工程管理 ====================
    Q_INVOKABLE bool addProject(QString name,QString note);

    Q_INVOKABLE QVariantList getProjectName();

    // ==================== 实验管理 ====================
    // 更新实验状态
    Q_INVOKABLE bool updateExperimentStatus(int experimentId, int status);
    
    // 返回设备端全部实验，供 PC 导入弹框列出候选记录。
    Q_INVOKABLE QVector<QVariantMap> getAllExperiments();

    // 返回单条实验的完整元数据，供导出时和原始数据一起打包。
    Q_INVOKABLE QVariantMap getExperimentById(int experimentId);

    // 查询指定状态的实验
    Q_INVOKABLE QVector<QVariantMap> getExperimentsByStatus(int status);
    
    // 删除实验
    Q_INVOKABLE bool deleteExperiment(int experimentId);
    
    // 批量删除实验
    Q_INVOKABLE bool deleteExperiments(const QVariantList& experimentIds);
    
    // ==================== 实验数据管理 ====================
    Q_INVOKABLE QVector<int> getScanIdsByExperiment(int experimentId);
    Q_INVOKABLE QVector<QVariantMap> getScanDataByExperimentAndScan(int experimentId, int scanId);

    Q_INVOKABLE bool deleteDataByExperiment(int experimentId);

signals:
    void operationFailed(const QString& message);

    // 操作信息
    // \param message 错误信息
    void operationInfo(const QString& message);

private:
    SqlOrmManager* m_dbManager;
};

#endif // DATA_CTRL_H
