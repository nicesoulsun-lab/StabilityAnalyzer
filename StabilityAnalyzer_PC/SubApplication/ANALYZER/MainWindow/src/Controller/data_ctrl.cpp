#include "inc/Controller/data_ctrl.h"
#include "SqlOrmManager.h"
#include "../../../SqlOrm/inc/SqlOrmManager.h"
#include <QDebug>
#include "Analysis/AdvancedCalculationEngine.h"
#include "Analysis/CurveChartAnalysisEngine.h"
#include "Analysis/LightCurveAnalysisEngine.h"
#include <QtMath>

namespace {

LightCurveAnalysisCacheKey makeLightCurveCacheKey(int experimentId, int pointsPerCurve, int referenceScanId,
                                                  double lowerMm, double upperMm, bool useReference)
{
    // 高度区间按微米级整数参与 key，避免浮点直接比较带来的缓存抖动。
    LightCurveAnalysisCacheKey key;
    key.experimentId = experimentId;
    key.pointsPerCurve = pointsPerCurve;
    key.referenceScanId = referenceScanId;
    key.lowerMilli = qRound64(lowerMm * 1000.0);
    key.upperMilli = qRound64(upperMm * 1000.0);
    key.useReference = useReference;
    return key;
}

}

dataCtrl::dataCtrl(QObject *parent)
    : QObject(parent)
    , m_dbManager(SqlOrmManager::instance())
{
    qDebug() << "[dataCtrl] 初始化完成";
}

dataCtrl::~dataCtrl()
{
    qDebug() << "[dataCtrl] 析构";
}

bool dataCtrl::addProject(QString name, QString note)
{
    if (name.isEmpty()) {
        qWarning() << "[dataCtrl] 工程名不能为空";
        emit operationFailed("工程名不能为空");
        return false;
    }

    QVariantMap projectData;
    projectData["project_name"] = name;
    projectData["description"] = note;

    bool success = m_dbManager->addProject(projectData);

    if(!success){
        qWarning() << "[dataCtrl]  创建工程失败";
        emit operationInfo(tr("创建工程失败"));
    }else{
        qWarning() << "[dataCtrl]  创建工程成功 "<<name;
        emit operationInfo(tr("已创建工程")+" : "+name);

        QVariantMap logData;
        logData["username"] = "";
        logData["user_id"] = -1;
        logData["operation"] = QString("创建了工程 %1").arg(name);
        m_dbManager->addOperationLog(logData);
    }

    return success;
}

QVariantList dataCtrl::getProjectName()
{
    auto project_list = m_dbManager->getAllProjects();

    QVariantList name_list;

    for(const QVariantMap& map : project_list){
        name_list.push_back(map["project_name"]);
    }

    return name_list;
}

bool dataCtrl::addData(int experimentId, int timestamp, double height,
                       double backscatterIntensity, double transmissionIntensity)
{
    if (experimentId <= 0) {
        qWarning() << "[dataCtrl] 实验 ID 无效";
        emit operationFailed("实验 ID 无效");
        return false;
    }
    
    QVariantMap data;
    data["experiment_id"] = experimentId;
    data["timestamp"] = timestamp;
    data["height"] = height;
    data["backscatter_intensity"] = backscatterIntensity;
    data["transmission_intensity"] = transmissionIntensity;
    
    bool success = m_dbManager->addExperimentData(data);
    
    if (success) {
        emit dataAdded(timestamp, experimentId);
        m_lightCurveAnalysisCache.clearExperiment(experimentId);
        qDebug() << "[dataCtrl] 添加实验数据成功，实验 ID:" << experimentId << "时间戳:" << timestamp;
    } else {
        emit operationFailed("添加实验数据失败");
        qWarning() << "[dataCtrl] 添加实验数据失败，实验 ID:" << experimentId;
    }
    
    return success;
}

bool dataCtrl::batchAddData(const QVector<QVariantMap>& dataList)
{
    if (dataList.isEmpty()) {
        qWarning() << "[dataCtrl] 数据列表为空";
        emit operationFailed("数据列表为空");
        return false;
    }
    
    bool success = m_dbManager->batchAddExperimentData(dataList);
    
    if (success) {
        int experimentId = dataList.first().value("experiment_id").toInt();
        m_lightCurveAnalysisCache.clearExperiment(experimentId);
        emit dataBatchAdded(dataList.size(), experimentId);
        qDebug() << "[dataCtrl] 批量添加实验数据成功，数量:" << dataList.size();
    } else {
        emit operationFailed("批量添加实验数据失败");
        qWarning() << "[dataCtrl] 批量添加实验数据失败，数量:" << dataList.size();
    }
    
    return success;
}

QVariantList dataCtrl::getDataByExperiment(int experimentId)
{
    if (experimentId <= 0) {
        qWarning() << "[dataCtrl] 实验 ID 无效";
        return QVariantList();
    }

    const QVector<QVariantMap> rows = m_dbManager->getExperimentDataByExperiment(experimentId);
    QVariantList result;
    result.reserve(rows.size());
    for (const QVariantMap &row : rows) {
        result.append(row);
    }
    return result;
}

QVariantList dataCtrl::getDataByRange(int experimentId, int startTimestamp, int endTimestamp)
{
    if (experimentId <= 0) {
        qWarning() << "[dataCtrl] 实验 ID 无效";
        return QVariantList();
    }
    
    if (startTimestamp > endTimestamp) {
        qWarning() << "[dataCtrl] 时间范围无效";
        return QVariantList();
    }

    const QVector<QVariantMap> rows = m_dbManager->getExperimentDataByRange(experimentId, startTimestamp, endTimestamp);
    QVariantList result;
    result.reserve(rows.size());
    for (const QVariantMap &row : rows) {
        result.append(row);
    }
    return result;
}

QVariantList dataCtrl::getAllData()
{
    const QVector<QVariantMap> rows = m_dbManager->getAllExperimentData();
    QVariantList result;
    result.reserve(rows.size());
    for (const QVariantMap &row : rows) {
        result.append(row);
    }
    return result;
}

bool dataCtrl::deleteData(int dataId)
{
    if (dataId <= 0) {
        qWarning() << "[dataCtrl] 数据 ID 无效";
        emit operationFailed("数据 ID 无效");
        return false;
    }
    
    bool success = m_dbManager->deleteExperimentData(dataId);
    
    if (success) {
        m_lightCurveAnalysisCache.clear();
        qDebug() << "[dataCtrl] 删除实验数据成功，ID:" << dataId;
    } else {
        emit operationFailed("删除实验数据失败");
        qWarning() << "[dataCtrl] 删除实验数据失败，ID:" << dataId;
    }
    
    return success;
}

bool dataCtrl::deleteDataByExperiment(int experimentId)
{
    if (experimentId <= 0) {
        qWarning() << "[dataCtrl] 实验 ID 无效";
        emit operationFailed("实验 ID 无效");
        return false;
    }
    
    bool success = m_dbManager->deleteExperimentDataByExperiment(experimentId);
    
    if (success) {
        m_lightCurveAnalysisCache.clearExperiment(experimentId);
        qDebug() << "[dataCtrl] 删除实验数据成功，实验 ID:" << experimentId;
    } else {
        emit operationFailed("删除实验数据失败");
        qWarning() << "[dataCtrl] 删除实验数据失败，实验 ID:" << experimentId;
    }
    
    return success;
}

// ==================== 实验状态管理 ====================

bool dataCtrl::updateExperimentStatus(int experimentId, int status)
{
    if (experimentId <= 0) {
        qWarning() << "[dataCtrl] 实验 ID 无效";
        emit operationFailed("实验 ID 无效");
        return false;
    }
    
    if (status < 0 || status > 1) {
        qWarning() << "[dataCtrl] 实验状态无效，应为 0(未导入) 或 1(已导入)";
        emit operationFailed("实验状态无效");
        return false;
    }
    
    bool success = m_dbManager->updateExperimentStatus(experimentId, status);
    
    if (success) {
        qDebug() << "[dataCtrl] 更新实验状态成功，实验 ID:" << experimentId << "状态:" << status;
    } else {
        emit operationFailed("更新实验状态失败");
        qWarning() << "[dataCtrl] 更新实验状态失败，实验 ID:" << experimentId;
    }
    
    return success;
}

QVariantList dataCtrl::getExperimentsByStatus(int status)
{
    if (status < 0 || status > 1) {
        qWarning() << "[dataCtrl] 实验状态无效";
        return QVariantList();
    }
    
    const QVector<QVariantMap> rows = m_dbManager->getExperimentsByStatus(status);
    QVariantList result;
    result.reserve(rows.size());
    for (const QVariantMap &row : rows) {
        result.append(row);
    }
    qDebug() << "[dataCtrl] 查询实验状态成功，状态:" << status << "数量:" << result.size();
    return result;
}

QVariantList dataCtrl::getDeletedExperiments()
{
    const QVector<QVariantMap> rows = m_dbManager->getDeletedExperiments();
    QVariantList result;
    result.reserve(rows.size());
    for (const QVariantMap &row : rows) {
        result.append(row);
    }
    qDebug() << "[dataCtrl] 查询已删除实验成功，数量:" << result.size();
    return result;
}

QVariantList dataCtrl::getLightIntensityCurves(int experimentId, int pointsPerCurve)
{
    if (experimentId <= 0) {
        qWarning() << "[dataCtrl] invalid experiment id for light intensity curves";
        return QVariantList();
    }

    // 这里直接透传数据库层的曲线聚合结果，QML 不再自己按 scan_id 分组。
    const QVector<QVariantMap> rows = m_dbManager->getLightIntensityCurvesByExperiment(experimentId, pointsPerCurve);
    QVariantList result;
    result.reserve(rows.size());
    for (const QVariantMap &row : rows) {
        result.append(row);
    }
    return result;
}

QVariantList dataCtrl::getProcessedLightIntensityCurves(int experimentId, int pointsPerCurve, int referenceScanId,
                                                        double lowerMm, double upperMm, bool useReference)
{
    if (experimentId <= 0) {
        qWarning() << "[dataCtrl] invalid experiment id for processed light intensity curves";
        return QVariantList();
    }

    const LightCurveAnalysisCacheKey cacheKey = makeLightCurveCacheKey(experimentId, pointsPerCurve, referenceScanId,
                                                                       lowerMm, upperMm, useReference);
    if (m_lightCurveAnalysisCache.contains(cacheKey)) {
        return m_lightCurveAnalysisCache.value(cacheKey);
    }

    // data_ctrl 现在只负责调度：
    // 先取原始曲线，再交给分析引擎处理，最后写回缓存。
    const QVector<QVariantMap> rawCurves = m_dbManager->getLightIntensityCurvesByExperiment(experimentId, pointsPerCurve);
    const QVector<LightCurveAnalysisCurve> parsedCurves = LightCurveAnalysisEngine::fromVariantMaps(rawCurves);
    // QML 传过来的是当前曲线列表索引，这里映射成真实 scan_id 再交给分析层。
    const int safeReferenceIndex = parsedCurves.isEmpty() ? 0 : qBound(0, referenceScanId, parsedCurves.size() - 1);
    const int actualReferenceScanId = parsedCurves.isEmpty() ? 0 : parsedCurves.at(safeReferenceIndex).scanId;
    const QVector<QVariantMap> rows = LightCurveAnalysisEngine::processCurves(parsedCurves, actualReferenceScanId,
                                                                              lowerMm, upperMm, useReference);
    QVariantList result;
    result.reserve(rows.size());
    for (const QVariantMap &row : rows) {
        result.append(row);
    }
    m_lightCurveAnalysisCache.insert(cacheKey, result);
    return result;
}

QVariantMap dataCtrl::getUniformityChartData(int experimentId)
{
    // 均匀度页入口：从结果表取数后，统一转成页面图表结构。
    if (experimentId <= 0) {
        qWarning() << "[dataCtrl] invalid experiment id for uniformity chart";
        return QVariantMap();
    }

    return CurveChartAnalysisEngine::buildUniformityChartData(
        m_dbManager->getUniformityIndicesByExperiment(experimentId));
}

QVariantMap dataCtrl::getLightIntensityAverageChartData(int experimentId)
{
    // 光强平均值页入口：页面不再自己整理时间轴和 BS/T 曲线。
    if (experimentId <= 0) {
        qWarning() << "[dataCtrl] invalid experiment id for light intensity average chart";
        return QVariantMap();
    }

    return CurveChartAnalysisEngine::buildLightIntensityAverageChartData(
        m_dbManager->getLightIntensityAveragesByExperiment(experimentId));
}

QVariantMap dataCtrl::getSeparationLayerChartData(int experimentId)
{
    // 分层厚度页入口：直接返回三层厚度的图表配置和曲线点列。
    if (experimentId <= 0) {
        qWarning() << "[dataCtrl] invalid experiment id for separation layer chart";
        return QVariantMap();
    }

    return CurveChartAnalysisEngine::buildSeparationLayerChartData(
        m_dbManager->getSeparationLayerDataByExperiment(experimentId));
}

QVariantMap dataCtrl::getInstabilitySeriesChartData(int experimentId, double lowerMm, double upperMm,
                                                    const QString& segmentKey, const QString& title)
{
    // 不稳定性单图入口：整体走默认缓存表，局部和自定义走按高度区间的缓存表。
    if (experimentId <= 0) {
        qWarning() << "[dataCtrl] invalid experiment id for instability series chart";
        return QVariantMap();
    }

    const QVector<QVariantMap> rows = segmentKey == QStringLiteral("overall")
        ? m_dbManager->getOrComputeInstabilityCurveDataByExperiment(experimentId)
        : m_dbManager->getOrComputeInstabilityCurveDataByHeightRange(experimentId, lowerMm, upperMm, segmentKey);
    return CurveChartAnalysisEngine::buildInstabilitySeriesChartData(rows, title, lowerMm, upperMm);
}

QVariantMap dataCtrl::getInstabilityRadarChartData(int experimentId)
{
    // 总览雷达图入口：先拆出整体、底部、中部、顶部四条曲线，再合成雷达图层。
    if (experimentId <= 0) {
        qWarning() << "[dataCtrl] invalid experiment id for instability radar chart";
        return QVariantMap();
    }

    const QVariantMap experiment = m_dbManager->getExperimentById(experimentId);
    const double minHeight = experiment.value("scan_range_start").toDouble() / 1000.0;
    const double maxHeight = experiment.value("scan_range_end").toDouble() / 1000.0;
    const double sectionHeight = qMax((maxHeight - minHeight) / 3.0, 0.0);
    const double firstSplit = minHeight + sectionHeight;
    const double secondSplit = minHeight + sectionHeight * 2.0;

    const QVariantMap overallSeries = getInstabilitySeriesChartData(experimentId, minHeight, maxHeight, QStringLiteral("overall"), QStringLiteral("整体"));
    const QVariantMap bottomSeries = getInstabilitySeriesChartData(experimentId, minHeight, firstSplit, QStringLiteral("bottom"), QStringLiteral("底部"));
    const QVariantMap middleSeries = getInstabilitySeriesChartData(experimentId, firstSplit, secondSplit, QStringLiteral("middle"), QStringLiteral("中部"));
    const QVariantMap topSeries = getInstabilitySeriesChartData(experimentId, secondSplit, maxHeight, QStringLiteral("top"), QStringLiteral("顶部"));

    return CurveChartAnalysisEngine::buildInstabilityRadarChartData(overallSeries, bottomSeries, middleSeries, topSeries);
}

QVariantMap dataCtrl::getPeakThicknessChartData(int experimentId, int intensityMode, double lowerMm, double upperMm, double thresholdValue)
{
    // 峰厚度页入口：直接使用原始扫描数据，由分析引擎完成阈值区段识别。
    if (experimentId <= 0) {
        qWarning() << "[dataCtrl] invalid experiment id for peak thickness chart";
        return QVariantMap();
    }

    return CurveChartAnalysisEngine::buildPeakThicknessChartData(
        m_dbManager->getExperimentDataByExperiment(experimentId),
        intensityMode,
        lowerMm,
        upperMm,
        thresholdValue);
}

QVariantMap dataCtrl::calculateMigrationRate(const QVariantMap& params)
{
    const int experimentId = params.value("experimentId").toInt();
    if (experimentId <= 0) {
        return AdvancedCalculationEngine::calculateMigrationRate(params);
    }

    return AdvancedCalculationEngine::calculateMigrationRate(
        params,
        m_dbManager->getSeparationLayerDataByExperiment(experimentId));
}

QVariantMap dataCtrl::calculateHydrodynamic(const QVariantMap& params)
{
    return AdvancedCalculationEngine::calculateHydrodynamic(params);
}

QVariantMap dataCtrl::calculateOptical(const QVariantMap& params)
{
    return AdvancedCalculationEngine::calculateOptical(params);
}

bool dataCtrl::deleteExperiment(int experimentId)
{
    if (experimentId <= 0) {
        qWarning() << "[dataCtrl] 实验 ID 无效";
        emit operationFailed("实验 ID 无效");
        return false;
    }
    
    bool success = m_dbManager->deleteExperiment(experimentId);
    
    if (success) {
        m_lightCurveAnalysisCache.clearExperiment(experimentId);
        qDebug() << "[dataCtrl] 删除实验成功，ID:" << experimentId;
        emit operationInfo(tr("已移入回收站，7天后自动清理"));

        QVariantMap logData;
        logData["username"] = "";
        logData["user_id"] = -1;
        logData["operation"] = QString("将实验 %1 移入回收站").arg(experimentId);
        m_dbManager->addOperationLog(logData);
    } else {
        emit operationFailed("删除实验失败");
        qWarning() << "[dataCtrl] 删除实验失败，ID:" << experimentId;
    }
    
    return success;
}

bool dataCtrl::restoreExperiment(int experimentId)
{
    if (experimentId <= 0) {
        qWarning() << "[dataCtrl] 实验 ID 无效";
        emit operationFailed("实验 ID 无效");
        return false;
    }

    bool success = m_dbManager->restoreExperiment(experimentId);

    if (success) {
        m_lightCurveAnalysisCache.clearExperiment(experimentId);
        qDebug() << "[dataCtrl] 恢复实验成功，ID:" << experimentId;
        emit operationInfo(tr("已恢复实验"));

        QVariantMap logData;
        logData["username"] = "";
        logData["user_id"] = -1;
        logData["operation"] = QString("恢复了实验 %1").arg(experimentId);
        m_dbManager->addOperationLog(logData);
    } else {
        emit operationFailed("恢复实验失败");
        qWarning() << "[dataCtrl] 恢复实验失败，ID:" << experimentId;
    }

    return success;
}

bool dataCtrl::hardDeleteExperiment(int experimentId)
{
    if (experimentId <= 0) {
        qWarning() << "[dataCtrl] 实验 ID 无效";
        emit operationFailed("实验 ID 无效");
        return false;
    }

    bool success = m_dbManager->hardDeleteExperiment(experimentId);

    if (success) {
        m_lightCurveAnalysisCache.clearExperiment(experimentId);
        qDebug() << "[dataCtrl] 彻底删除实验成功，ID:" << experimentId;
        emit operationInfo(tr("已彻底删除实验"));

        QVariantMap logData;
        logData["username"] = "";
        logData["user_id"] = -1;
        logData["operation"] = QString("彻底删除了实验 %1").arg(experimentId);
        m_dbManager->addOperationLog(logData);
    } else {
        emit operationFailed("彻底删除实验失败");
        qWarning() << "[dataCtrl] 彻底删除实验失败，ID:" << experimentId;
    }

    return success;
}

bool dataCtrl::deleteExperiments(const QVariantList& experimentIds)
{
    if (experimentIds.isEmpty()) {
        qWarning() << "[dataCtrl] 没有选择要删除的实验";
        emit operationFailed("请先选择要删除的实验");
        return false;
    }
    
    int successCount = 0;
    for (const auto& idVariant : experimentIds) {
        int expId = idVariant.toInt();
        if (expId > 0 && m_dbManager->deleteExperiment(expId)) {
            m_lightCurveAnalysisCache.clearExperiment(expId);
            successCount++;
        }
    }
    
    if (successCount > 0) {
        qDebug() << "[dataCtrl] 批量删除实验成功，数量:" << successCount;
        emit operationInfo(tr("已移入回收站 %1 个实验，7天后自动清理").arg(successCount));

        QVariantMap logData;
        logData["username"] = "";
        logData["user_id"] = -1;
        logData["operation"] = QString("批量将 %1 条实验移入回收站").arg(successCount);
        m_dbManager->addOperationLog(logData);

        return true;
    } else {
        emit operationFailed("删除实验失败");
        qWarning() << "[dataCtrl] 批量删除实验失败";
        return false;
    }
}

bool dataCtrl::hardDeleteExperiments(const QVariantList& experimentIds)
{
    if (experimentIds.isEmpty()) {
        qWarning() << "[dataCtrl] 没有选择要彻底删除的实验";
        emit operationFailed("请先选择要删除的实验");
        return false;
    }

    int successCount = 0;
    for (const auto& idVariant : experimentIds) {
        int expId = idVariant.toInt();
        if (expId > 0 && m_dbManager->hardDeleteExperiment(expId)) {
            m_lightCurveAnalysisCache.clearExperiment(expId);
            successCount++;
        }
    }

    if (successCount > 0) {
        qDebug() << "[dataCtrl] 批量彻底删除实验成功，数量:" << successCount;
        emit operationInfo(tr("已彻底删除 %1 个实验").arg(successCount));

        QVariantMap logData;
        logData["username"] = "";
        logData["user_id"] = -1;
        logData["operation"] = QString("批量彻底删除了 %1 条实验").arg(successCount);
        m_dbManager->addOperationLog(logData);

        return true;
    } else {
        emit operationFailed("彻底删除实验失败");
        qWarning() << "[dataCtrl] 批量彻底删除实验失败";
        return false;
    }
}
