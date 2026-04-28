#ifndef DATA_CTRL_H
#define DATA_CTRL_H

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QThread>
#include <QVariantMap>
#include <QVector>
#include "Analysis/LightCurveAnalysisCache.h"
#include "mainwindow_global.h"

class SqlOrmManager;
class DataTransmitController;

class MAINWINDOW_EXPORT dataCtrl : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool deviceImportRunning READ deviceImportRunning NOTIFY deviceImportRunningChanged)
    Q_PROPERTY(QVariantMap lastDeviceImportResult READ lastDeviceImportResult NOTIFY lastDeviceImportResultChanged)

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

    // 由 ControllerManager 注入通信控制器，供导入记录等跨端业务使用。
    void setDataTransmitController(DataTransmitController *controller);

    // ==================== 实验管理 ====================
    // 更新实验导入状态或处理状态。
    Q_INVOKABLE bool updateExperimentStatus(int experimentId, int status);
    
    // 按状态查询实验记录列表。
    Q_INVOKABLE QVariantList getExperimentsByStatus(int status);
    // 按 ID 查询单条实验记录，供详情页/实时页绑定基础实验信息。
    Q_INVOKABLE QVariantMap getExperimentById(int experimentId);
    // 从设备端查询可导入实验列表，供导入弹框直接绑定。
    Q_INVOKABLE QVariantList fetchDeviceImportExperiments();
    // 导入单条设备实验记录，供前端分批调度以避免界面长时间卡住。
    Q_INVOKABLE QVariantMap importSingleExperimentFromDevice(int deviceExperimentId);
    // 把选中的设备实验导入到 PC 本地数据库，并回写设备端导入状态。
    Q_INVOKABLE QVariantMap importExperimentsFromDevice(const QVariantList &deviceExperimentIds);
    // 后台导入设备实验，完成后通过 deviceImportFinished 通知 QML。
    Q_INVOKABLE bool startImportExperimentsFromDevice(const QVariantList &deviceExperimentIds);
    bool deviceImportRunning() const { return m_deviceImportRunning; }
    QVariantMap lastDeviceImportResult() const { return m_lastDeviceImportResult; }
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
    // 获取实验实际存在的 scan_id 列表，详情页按真实 scan_id 分批加载曲线。
    Q_INVOKABLE QVariantList getLightIntensityScanIds(int experimentId);
    // 获取光强页使用的整帧曲线数据，后端已完成降采样。
    Q_INVOKABLE QVariantList getLightIntensityCurves(int experimentId, int pointsPerCurve);
    // 获取单个完整 scan 的光强曲线数据，供实时页按 scan 增量刷新。
    Q_INVOKABLE QVariantList getLightIntensityCurve(int experimentId, int scanId, int pointsPerCurve);
    // 获取实时流在内存中的单条曲线，供实时页直接绘制，不走数据库回查。
    Q_INVOKABLE QVariantMap getRealtimeLightIntensityCurve(int experimentId, int scanId) const;
    // 获取当前实验已缓存的全部实时曲线，供实时页定时同步，不走数据库回查。
    Q_INVOKABLE QVariantList getRealtimeLightIntensityCurves(int experimentId) const;
    // 获取实时曲线缓存的轻量签名，供 QML 判断是否需要拉取完整点集。
    Q_INVOKABLE QString getRealtimeLightIntensityCurveSignature(int experimentId) const;
    // 实验结束后清理实时缓存和分析缓存，释放前台不再需要的运行期内存。
    Q_INVOKABLE void clearExperimentRuntimeResources(int experimentId);
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

    // 单个 scan_id 的数据写入完成后触发，实时页据此刷新曲线。
    void scanDataAdded(int experimentId, int scanId);
    // 单个 scan_id 有新数据片段写入后触发，实时页据此增量刷新正在采集的曲线。
    void scanDataChanged(int channel, int experimentId, int scanId, bool scanCompleted);
    // 实时流原始数据已转换为可直接绘制的曲线，前端无需再回查数据库。
    void realtimeLightCurveReady(int channel, int experimentId, QVariantMap curve, bool scanCompleted);
    
    // 操作失败信号
    //  message 错误信息
    void operationFailed(const QString& message);

    // 操作信息
    //  message 错误信息
    void operationInfo(const QString& message);
    void deviceImportRunningChanged();
    void lastDeviceImportResultChanged();
    void deviceImportFinished(QVariantMap result);
    void experimentRuntimeResourcesCleared(int experimentId);

private:
    QVariantMap importSingleExperimentFromDeviceInternal(int deviceExperimentId);
    void finishDeviceImportWorker(const QVariantMap &result);
    void handleStreamMessage(const QVariantMap &message);
    bool sendRequestAndWait(const QString &command,
                            const QVariantMap &payload,
                            QVariantMap *response = nullptr,
                            int timeoutMs = 10000);
    int ensureLocalProjectId(const QString &projectName, const QString &projectDescription = QString());
    int findLatestExperimentId() const;

    SqlOrmManager* m_dbManager;
    DataTransmitController *m_dataTransmitCtrl = nullptr;
    QPointer<QThread> m_deviceImportThread;
    bool m_deviceImportRunning = false;
    QVariantMap m_lastDeviceImportResult;
    QHash<int, QHash<int, QVariantMap>> m_realtimeLightCurveCache;
    // 光强处理结果的内存缓存，避免同一组参数反复触发后端分析。
    LightCurveAnalysisCache m_lightCurveAnalysisCache;
};

#endif // DATA_CTRL_H
