#ifndef DATA_CTRL_H
#define DATA_CTRL_H

#include <QObject>
#include <QVariantMap>
#include <QVector>
#include "Analysis/LightCurveAnalysisCache.h"
#include "mainwindow_global.h"

class SqlOrmManager;

class MAINWINDOW_EXPORT dataCtrl : public QObject
{
    Q_OBJECT

public:
    // 构造数据控制器，负责协调数据库访问和分析结果输出。
    explicit dataCtrl(QObject *parent = nullptr);
    // 析构数据控制器，释放运行期资源。
    ~dataCtrl();

    // ==================== 工程管理 ====================
    // 创建新工程，写入工程名称和备注信息。
    Q_INVOKABLE bool addProject(QString name,QString note);

    // 获取全部工程名称列表，供下拉框或新建实验页面使用。
    Q_INVOKABLE QVariantList getProjectName();

    // ==================== 实验管理 ====================
    // 更新实验导入状态或处理状态。
    Q_INVOKABLE bool updateExperimentStatus(int experimentId, int status);
    
    // 按状态查询实验记录列表。
    Q_INVOKABLE QVariantList getExperimentsByStatus(int status);
    // 查询当前位于回收站中的实验记录。
    Q_INVOKABLE QVariantList getDeletedExperiments();
    
    // 软删除单个实验，将其移入回收站。
    Q_INVOKABLE bool deleteExperiment(int experimentId);
    // 从回收站恢复单个实验。
    Q_INVOKABLE bool restoreExperiment(int experimentId);
    // 彻底删除单个实验及其关联数据。
    Q_INVOKABLE bool hardDeleteExperiment(int experimentId);
    
    // 批量软删除多个实验。
    Q_INVOKABLE bool deleteExperiments(const QVariantList& experimentIds);
    // 批量彻底删除多个实验。
    Q_INVOKABLE bool hardDeleteExperiments(const QVariantList& experimentIds);
    
    // ==================== 实验数据管理 ====================
    // 添加一条原始实验采样数据。
    Q_INVOKABLE bool addData(int experimentId, int timestamp, double height, 
                             double backscatterIntensity, double transmissionIntensity);
    
    // 批量写入原始实验采样数据。
    Q_INVOKABLE bool batchAddData(const QVector<QVariantMap>& dataList);
    
    // 查询指定实验的全部原始采样数据。
    Q_INVOKABLE QVariantList getDataByExperiment(int experimentId);
    // 按时间范围查询指定实验的原始采样数据。
    Q_INVOKABLE QVariantList getDataByRange(int experimentId, int startTimestamp, int endTimestamp);
    // 查询数据库中的全部原始实验数据。
    Q_INVOKABLE QVariantList getAllData();
    // 获取光强页使用的整帧曲线数据，后端已完成降采样。
    Q_INVOKABLE QVariantList getLightIntensityCurves(int experimentId, int pointsPerCurve);
    // 按参比帧和高度区间生成处理后的光强曲线数据。
    Q_INVOKABLE QVariantList getProcessedLightIntensityCurves(int experimentId, int pointsPerCurve, int referenceScanId,
                                                             double lowerMm, double upperMm, bool useReference);
    // 获取均匀度页面直接可绑定的图表数据结构。
    Q_INVOKABLE QVariantMap getUniformityChartData(int experimentId);
    // 获取光强平均值页面直接可绑定的图表数据结构。
    Q_INVOKABLE QVariantMap getLightIntensityAverageChartData(int experimentId);
    // 获取分层厚度页面直接可绑定的图表数据结构。
    Q_INVOKABLE QVariantMap getSeparationLayerChartData(int experimentId);
    // 获取不稳定性单图页面使用的趋势图数据结构。
    Q_INVOKABLE QVariantMap getInstabilitySeriesChartData(int experimentId, double lowerMm, double upperMm, const QString& segmentKey, const QString& title);
    // 获取不稳定性总览雷达图使用的数据结构。
    Q_INVOKABLE QVariantMap getInstabilityRadarChartData(int experimentId);
    // 获取峰厚度页面使用的趋势图数据结构。
    Q_INVOKABLE QVariantMap getPeakThicknessChartData(int experimentId, int intensityMode, double lowerMm, double upperMm, double thresholdValue);
    // 计算颗粒迁移速度，返回统一结果结构供高级计算页使用。
    Q_INVOKABLE QVariantMap calculateMigrationRate(const QVariantMap& params);
    // 执行流体力学高级计算，按目标字段反算对应结果。
    Q_INVOKABLE QVariantMap calculateHydrodynamic(const QVariantMap& params);
    // 执行光学高级计算，按目标字段反算粒径或体积浓度。
    Q_INVOKABLE QVariantMap calculateOptical(const QVariantMap& params);
    
    // 删除单条实验采样数据。
    Q_INVOKABLE bool deleteData(int dataId);
    // 删除指定实验下的全部采样数据。
    Q_INVOKABLE bool deleteDataByExperiment(int experimentId);

signals:
    // 实验数据添加成功信号
    //  dataId 数据 ID（时间戳）
    //  experimentId 所属实验 ID
    void dataAdded(int dataId, int experimentId);
    
    // 批量添加实验数据成功信号
    //  count 添加的数据数量
    //  experimentId 所属实验 ID
    void dataBatchAdded(int count, int experimentId);
    
    // 操作失败信号
    //  message 错误信息
    void operationFailed(const QString& message);

    // 操作信息
    //  message 错误信息
    void operationInfo(const QString& message);

private:
    SqlOrmManager* m_dbManager;
    // 光强处理结果的内存缓存，避免同一组参数反复触发后端分析。
    LightCurveAnalysisCache m_lightCurveAnalysisCache;
};

#endif // DATA_CTRL_H
