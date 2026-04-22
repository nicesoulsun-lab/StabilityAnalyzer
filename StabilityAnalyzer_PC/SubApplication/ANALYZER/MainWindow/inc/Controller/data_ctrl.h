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
    // 添加实验数据
    Q_INVOKABLE bool addProject(QString name,QString note);

    Q_INVOKABLE QVariantList getProjectName();

    // ==================== 实验管理 ====================
    // 更新实验状态
    Q_INVOKABLE bool updateExperimentStatus(int experimentId, int status);
    
    // 查询指定状态的实验
    Q_INVOKABLE QVariantList getExperimentsByStatus(int status);
    Q_INVOKABLE QVariantList getDeletedExperiments();
    
    // 删除实验
    Q_INVOKABLE bool deleteExperiment(int experimentId);
    Q_INVOKABLE bool restoreExperiment(int experimentId);
    Q_INVOKABLE bool hardDeleteExperiment(int experimentId);
    
    // 批量删除实验
    Q_INVOKABLE bool deleteExperiments(const QVariantList& experimentIds);
    Q_INVOKABLE bool hardDeleteExperiments(const QVariantList& experimentIds);
    
    // ==================== 实验数据管理 ====================
    // 添加实验数据
    Q_INVOKABLE bool addData(int experimentId, int timestamp, double height, 
                             double backscatterIntensity, double transmissionIntensity);
    
    // 批量添加实验数据
    Q_INVOKABLE bool batchAddData(const QVector<QVariantMap>& dataList);
    
    // 查询实验数据
    Q_INVOKABLE QVariantList getDataByExperiment(int experimentId);
    Q_INVOKABLE QVariantList getDataByRange(int experimentId, int startTimestamp, int endTimestamp);
    Q_INVOKABLE QVariantList getAllData();
    // 光强页使用预降采样后的整帧曲线数据，避免把全量点直接压到 QML。
    Q_INVOKABLE QVariantList getLightIntensityCurves(int experimentId, int pointsPerCurve);
    // 光强页参比/高度区间等分析参数走后端，前端只负责传参和显示。
    Q_INVOKABLE QVariantList getProcessedLightIntensityCurves(int experimentId, int pointsPerCurve, int referenceScanId,
                                                             double lowerMm, double upperMm, bool useReference);
    // 整体不稳定性曲线，优先读取缓存结果表。
    Q_INVOKABLE QVariantList getInstabilityCurveData(int experimentId);
    // 分区/自定义不稳定性曲线，segmentKey 用来区分底中顶和自定义缓存。
    Q_INVOKABLE QVariantList getInstabilityCurveDataByHeightRange(int experimentId, double lowerMm, double upperMm, const QString& segmentKey);
    Q_INVOKABLE QVariantList getUniformityIndices(int experimentId);
    Q_INVOKABLE QVariantList getLightIntensityAverages(int experimentId);
    // 返回澄清层/浓相层/沉淀层三条厚度曲线共用的原始结果行。
    Q_INVOKABLE QVariantList getSeparationLayerData(int experimentId);
    
    // 删除实验数据
    Q_INVOKABLE bool deleteData(int dataId);
    Q_INVOKABLE bool deleteDataByExperiment(int experimentId);

signals:
    // 实验数据添加成功信号
    // \param dataId 数据 ID（时间戳）
    // \param experimentId 所属实验 ID
    void dataAdded(int dataId, int experimentId);
    
    // 批量添加实验数据成功信号
    // \param count 添加的数据数量
    // \param experimentId 所属实验 ID
    void dataBatchAdded(int count, int experimentId);
    
    // 操作失败信号
    // \param message 错误信息
    void operationFailed(const QString& message);

    // 操作信息
    // \param message 错误信息
    void operationInfo(const QString& message);

private:
    SqlOrmManager* m_dbManager;
};

#endif // DATA_CTRL_H
