#include "inc/Controller/data_ctrl.h"
#include "SqlOrmManager.h"
#include "../../../SqlOrm/inc/SqlOrmManager.h"
#include "datatransmitcontroller.h"
#include <QDebug>
#include <QEventLoop>
#include <QMetaObject>
#include "Analysis/AdvancedCalculationEngine.h"
#include "Analysis/CurveChartAnalysisEngine.h"
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <QtMath>
#include <QStringList>
#include <new>

namespace {

constexpr int kImportBatchSize = 256;
constexpr int kImportScanPageSize = 256;

double effectiveScanEndMm(const QVariantMap &experiment)
{
    const double startMm = experiment.value(QStringLiteral("scan_range_start")).toDouble() / 1000.0;
    const double endMm = experiment.value(QStringLiteral("scan_range_end")).toDouble() / 1000.0;
    const double stepMm = experiment.value(QStringLiteral("scan_step"), 20).toDouble() / 1000.0;
    if (stepMm > 0.0 && endMm > startMm) {
        return qMax(startMm, endMm - stepMm);
    }
    return qMax(startMm, endMm);
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
    if (m_deviceImportThread && m_deviceImportThread->isRunning()) {
        m_deviceImportThread->requestInterruption();
        if (!m_deviceImportThread->wait(3000)) {
            qWarning() << "[dataCtrl] device import worker still running during destruction";
        }
    }
    qDebug() << "[dataCtrl] 析构";
}

void dataCtrl::setDataTransmitController(DataTransmitController *controller)
{
    if (m_dataTransmitCtrl == controller) {
        return;
    }

    m_dataTransmitCtrl = controller;
    qDebug() << "[dataCtrl] data transmit controller updated for request/response only"
             << "available=" << (m_dataTransmitCtrl != nullptr);
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

QVariantList dataCtrl::fetchDeviceImportExperiments()
{
    QVariantMap response;
    if (!sendRequestAndWait(QStringLiteral("list_importable_experiments"), QVariantMap(), &response)) {
        const QString message = response.value(QStringLiteral("message")).toString();
        emit operationFailed(message.isEmpty() ? tr("获取设备实验列表失败") : message);
        return QVariantList();
    }

    const QVariantList experiments = response.value(QStringLiteral("experiments")).toList();
    qDebug() << "[dataCtrl] 获取设备实验列表成功，数量:" << experiments.size();
    return experiments;
}

QVariantMap dataCtrl::importSingleExperimentFromDevice(int deviceExperimentId)
{
    QVariantMap result;
    result.insert(QStringLiteral("success"), false);
    result.insert(QStringLiteral("importedCount"), 0);
    result.insert(QStringLiteral("failedCount"), 1);

    if (deviceExperimentId <= 0) {
        result.insert(QStringLiteral("message"), tr("存在无效的实验记录 ID"));
        return result;
    }

    if (!m_dataTransmitCtrl || m_dataTransmitCtrl->deviceUiConnectionStateText() != QStringLiteral("Connected")) {
        result.insert(QStringLiteral("message"), tr("设备未连接，请检查连接状态"));
        return result;
    }

    try {
        return importSingleExperimentFromDeviceInternal(deviceExperimentId);
    } catch (const std::bad_alloc &) {
        m_dbManager->rollbackTransaction();
        result.insert(QStringLiteral("message"), tr("导入实验时内存不足，请减少单次导入数量或缩小单个实验数据量"));
        return result;
    }
}

QVariantMap dataCtrl::importSingleExperimentFromDeviceInternal(int deviceExperimentId)
{
    QVariantMap result;
    result.insert(QStringLiteral("success"), false);
    result.insert(QStringLiteral("importedCount"), 0);
    result.insert(QStringLiteral("failedCount"), 1);

    QVariantMap exportResponse;
    if (!sendRequestAndWait(QStringLiteral("get_experiment_export"),
                            QVariantMap{{QStringLiteral("experiment_id"), deviceExperimentId}},
                            &exportResponse)) {
        result.insert(QStringLiteral("message"), exportResponse.value(QStringLiteral("message")).toString());
        return result;
    }

    const QVariantMap deviceExperiment = exportResponse.value(QStringLiteral("experiment")).toMap();
    if (deviceExperiment.isEmpty()) {
        result.insert(QStringLiteral("message"), tr("设备实验 %1 不存在").arg(deviceExperimentId));
        return result;
    }

    QVariantList scanIdVariants = exportResponse.value(QStringLiteral("scan_ids")).toList();
    if (scanIdVariants.isEmpty()) {
        const int scanCount = qMax(0, deviceExperiment.value(QStringLiteral("count")).toInt());
        for (int scanId = 0; scanId < scanCount; ++scanId) {
            scanIdVariants.append(scanId);
        }
    }

    const QString sampleName = deviceExperiment.value(QStringLiteral("sample_name")).toString();
    const QString projectName = deviceExperiment.value(QStringLiteral("project_name")).toString().trimmed();
    const QString projectDescription = deviceExperiment.value(QStringLiteral("project_description")).toString();
    const int projectId = ensureLocalProjectId(projectName.isEmpty() ? tr("导入工程") : projectName,
                                               projectDescription);
    if (projectId <= 0) {
        result.insert(QStringLiteral("message"), tr("创建本地工程失败：%1").arg(projectName));
        return result;
    }

    if (!m_dbManager->beginTransaction()) {
        result.insert(QStringLiteral("message"), tr("开始本地导入事务失败"));
        return result;
    }

    const int localExperimentId = findLatestExperimentId() + 1;
    if (localExperimentId <= 0) {
        m_dbManager->rollbackTransaction();
        result.insert(QStringLiteral("message"), tr("无法分配本地实验 ID：%1").arg(sampleName));
        return result;
    }

    QVariantMap localExperiment;
    localExperiment.insert(QStringLiteral("id"), localExperimentId);
    localExperiment.insert(QStringLiteral("project_id"), projectId);
    localExperiment.insert(QStringLiteral("sample_name"), deviceExperiment.value(QStringLiteral("sample_name")));
    localExperiment.insert(QStringLiteral("operator_name"), deviceExperiment.value(QStringLiteral("operator_name")));
    localExperiment.insert(QStringLiteral("description"), deviceExperiment.value(QStringLiteral("description")));
    localExperiment.insert(QStringLiteral("creator_id"), deviceExperiment.value(QStringLiteral("creator_id"), -1));
    localExperiment.insert(QStringLiteral("duration"), deviceExperiment.value(QStringLiteral("duration")));
    localExperiment.insert(QStringLiteral("interval"), deviceExperiment.value(QStringLiteral("interval")));
    localExperiment.insert(QStringLiteral("count"), deviceExperiment.value(QStringLiteral("count")));
    localExperiment.insert(QStringLiteral("temperature_control"), deviceExperiment.value(QStringLiteral("temperature_control")));
    localExperiment.insert(QStringLiteral("target_temp"), deviceExperiment.value(QStringLiteral("target_temp")));
    localExperiment.insert(QStringLiteral("scan_range_start"), deviceExperiment.value(QStringLiteral("scan_range_start")));
    localExperiment.insert(QStringLiteral("scan_range_end"), deviceExperiment.value(QStringLiteral("scan_range_end")));
    localExperiment.insert(QStringLiteral("scan_step"), deviceExperiment.value(QStringLiteral("scan_step"), 20));
    localExperiment.insert(QStringLiteral("status"), 1);

    if (!m_dbManager->addExperiment(localExperiment)) {
        m_dbManager->rollbackTransaction();
        result.insert(QStringLiteral("message"), tr("写入本地实验失败：%1").arg(sampleName));
        return result;
    }

    int importedRows = 0;
    for (int scanIndex = 0; scanIndex < scanIdVariants.size(); ++scanIndex) {
        const int scanId = scanIdVariants.at(scanIndex).toInt();
        QVector<QVariantMap> importBatch;
        importBatch.reserve(kImportBatchSize);
        int pageOffset = 0;
        bool hasMore = true;
        bool receivedAnyRows = false;

        while (hasMore) {
            QVariantMap scanResponse;
            if (!sendRequestAndWait(QStringLiteral("get_experiment_scan_export"),
                                    QVariantMap{
            {QStringLiteral("experiment_id"), deviceExperimentId},
            {QStringLiteral("scan_id"), scanId},
            {QStringLiteral("offset"), pageOffset},
            {QStringLiteral("limit"), kImportScanPageSize}
        },
                                    &scanResponse,
                                    30000)) {
                m_dbManager->rollbackTransaction();
                result.insert(QStringLiteral("message"),
                              tr("读取设备扫描数据失败：%1 scanId=%2").arg(sampleName).arg(scanId));
                return result;
            }

            const QVariantList deviceDataRows = scanResponse.value(QStringLiteral("data")).toList();
            if (!deviceDataRows.isEmpty()) {
                receivedAnyRows = true;
            }

            qDebug() << "[Import][scan page]"
                     << "deviceExperimentId=" << deviceExperimentId
                     << "scanId=" << scanId
                     << "offset=" << pageOffset
                     << "rows=" << deviceDataRows.size()
                     << "hasMore=" << scanResponse.value(QStringLiteral("has_more")).toBool()
                     << "totalCount=" << scanResponse.value(QStringLiteral("total_count")).toInt();

            for (const QVariant &rowVariant : deviceDataRows) {
                QVariantMap row = rowVariant.toMap();
                row.remove(QStringLiteral("id"));
                row.insert(QStringLiteral("experiment_id"), localExperimentId);
                importBatch.append(std::move(row));

                if (importBatch.size() >= kImportBatchSize) {
                    if (!m_dbManager->batchAddExperimentData(importBatch)) {
                        m_dbManager->rollbackTransaction();
                        result.insert(QStringLiteral("message"), tr("写入本地实验数据失败：%1").arg(sampleName));
                        return result;
                    }
                    importedRows += importBatch.size();
                    importBatch.clear();
                }
            }

            hasMore = scanResponse.value(QStringLiteral("has_more")).toBool();
            pageOffset += deviceDataRows.size();

            if (deviceDataRows.isEmpty()) {
                if (hasMore) {
                    m_dbManager->rollbackTransaction();
                    result.insert(QStringLiteral("message"),
                                  tr("设备扫描数据分页异常：%1 scanId=%2").arg(sampleName).arg(scanId));
                    return result;
                }
                break;
            }
        }

        if (!receivedAnyRows) {
            m_dbManager->rollbackTransaction();
            result.insert(QStringLiteral("message"),
                          tr("设备扫描数据为空：%1 scanId=%2").arg(sampleName).arg(scanId));
            return result;
        }

        if (!importBatch.isEmpty()) {
            if (!m_dbManager->batchAddExperimentData(importBatch)) {
                m_dbManager->rollbackTransaction();
                result.insert(QStringLiteral("message"), tr("写入本地实验数据失败：%1").arg(sampleName));
                return result;
            }
            importedRows += importBatch.size();
        }
    }

    if (!m_dbManager->commitTransaction()) {
        m_dbManager->rollbackTransaction();
        result.insert(QStringLiteral("message"), tr("提交本地导入事务失败：%1").arg(sampleName));
        return result;
    }

    QVariantMap markResponse;
    QString warningMessage;
    if (!sendRequestAndWait(QStringLiteral("mark_experiment_imported"),
                            QVariantMap{
    {QStringLiteral("experiment_id"), deviceExperimentId},
    {QStringLiteral("status"), 1}
},
                            &markResponse)) {
        warningMessage = markResponse.value(QStringLiteral("message")).toString();
        if (warningMessage.isEmpty()) {
            warningMessage = tr("设备实验 %1 状态回写失败").arg(deviceExperimentId);
        }
    }

    result.insert(QStringLiteral("success"), true);
    result.insert(QStringLiteral("importedCount"), 1);
    result.insert(QStringLiteral("failedCount"), 0);
    result.insert(QStringLiteral("message"), tr("已导入实验：%1（%2 条数据）").arg(sampleName).arg(importedRows));
    if (!warningMessage.isEmpty()) {
        result.insert(QStringLiteral("warning"), warningMessage);
    }
    return result;
}

bool dataCtrl::startImportExperimentsFromDevice(const QVariantList &deviceExperimentIds)
{
    if (deviceExperimentIds.isEmpty()) {
        emit operationFailed(tr("请先选择需要导入的记录"));
        return false;
    }

    if (m_deviceImportThread && m_deviceImportThread->isRunning()) {
        emit operationFailed(tr("已有导入任务正在执行"));
        return false;
    }

    if (!m_dataTransmitCtrl || m_dataTransmitCtrl->deviceUiConnectionStateText() != QStringLiteral("Connected")) {
        emit operationFailed(tr("设备未连接，请检查连接状态"));
        return false;
    }

    if (!m_lastDeviceImportResult.isEmpty()) {
        m_lastDeviceImportResult.clear();
        emit lastDeviceImportResultChanged();
    }
    if (!m_deviceImportRunning) {
        m_deviceImportRunning = true;
        emit deviceImportRunningChanged();
    }

    QVariantList ids = deviceExperimentIds;
    QPointer<dataCtrl> self(this);
    m_deviceImportThread = QThread::create([self, ids]() {
        if (!self) {
            return;
        }

        QStringList failureMessages;
        QStringList warningMessages;
        int importedCount = 0;

        for (int i = 0; i < ids.size(); ++i) {
            if (!self || QThread::currentThread()->isInterruptionRequested()) {
                return;
            }

            const QVariantMap singleResult = self->importSingleExperimentFromDeviceInternal(ids.at(i).toInt());
            if (singleResult.value(QStringLiteral("success")).toBool()) {
                importedCount += singleResult.value(QStringLiteral("importedCount")).toInt();
                const QString warning = singleResult.value(QStringLiteral("warning")).toString();
                if (!warning.isEmpty()) {
                    warningMessages.append(warning);
                }
            } else {
                failureMessages.append(singleResult.value(QStringLiteral("message")).toString());
            }
        }

        QVariantMap result;
        result.insert(QStringLiteral("importedCount"), importedCount);
        result.insert(QStringLiteral("failedCount"), ids.size() - importedCount);
        result.insert(QStringLiteral("success"), importedCount > 0 && failureMessages.isEmpty());

        QStringList messageParts;
        if (importedCount > 0) {
            messageParts.append(self->tr("已导入 %1 条实验记录").arg(importedCount));
        }
        if (!failureMessages.isEmpty()) {
            messageParts.append(failureMessages.join(QStringLiteral("\n")));
        }
        if (!warningMessages.isEmpty()) {
            messageParts.append(self->tr("设备端状态回写提醒：%1").arg(warningMessages.join(QStringLiteral("；"))));
        }
        result.insert(QStringLiteral("message"), messageParts.join(QStringLiteral("\n")));

        qDebug() << "[dataCtrl] device import worker completed"
                 << "importedCount=" << result.value(QStringLiteral("importedCount")).toInt()
                 << "failedCount=" << result.value(QStringLiteral("failedCount")).toInt()
                 << "success=" << result.value(QStringLiteral("success")).toBool();

        QMetaObject::invokeMethod(self, [self, result]() {
            if (self) {
                self->finishDeviceImportWorker(result);
            }
        }, Qt::QueuedConnection);
    });

    connect(m_deviceImportThread, &QThread::finished, m_deviceImportThread, &QObject::deleteLater);
    m_deviceImportThread->start();
    return true;
}

void dataCtrl::finishDeviceImportWorker(const QVariantMap &result)
{
    qDebug() << "[dataCtrl] finishDeviceImportWorker"
             << "success=" << result.value(QStringLiteral("success")).toBool()
             << "importedCount=" << result.value(QStringLiteral("importedCount")).toInt()
             << "failedCount=" << result.value(QStringLiteral("failedCount")).toInt();
    m_deviceImportThread = nullptr;
    m_lastDeviceImportResult = result;
    qDebug() << "[dataCtrl] lastDeviceImportResult updated"
             << "message=" << result.value(QStringLiteral("message")).toString();
    emit lastDeviceImportResultChanged();
    if (m_deviceImportRunning) {
        m_deviceImportRunning = false;
        qDebug() << "[dataCtrl] deviceImportRunningChanged emit false";
        emit deviceImportRunningChanged();
    }
    emit deviceImportFinished(result);
}

QVariantMap dataCtrl::importExperimentsFromDevice(const QVariantList &deviceExperimentIds)
{
    QVariantMap result;
    result.insert(QStringLiteral("success"), false);
    result.insert(QStringLiteral("importedCount"), 0);
    result.insert(QStringLiteral("failedCount"), 0);

    if (deviceExperimentIds.isEmpty()) {
        result.insert(QStringLiteral("message"), tr("请先选择需要导入的记录"));
        return result;
    }

    if (!m_dataTransmitCtrl || m_dataTransmitCtrl->deviceUiConnectionStateText() != QStringLiteral("Connected")) {
        result.insert(QStringLiteral("message"), tr("设备未连接，请检查连接状态"));
        return result;
    }

    QStringList failureMessages;
    QStringList warningMessages;
    int importedCount = 0;

    for (const QVariant &idVariant : deviceExperimentIds) {
        const QVariantMap singleResult = importSingleExperimentFromDevice(idVariant.toInt());
        if (singleResult.value(QStringLiteral("success")).toBool()) {
            importedCount += singleResult.value(QStringLiteral("importedCount")).toInt();
            const QString warning = singleResult.value(QStringLiteral("warning")).toString();
            if (!warning.isEmpty()) {
                warningMessages.append(warning);
            }
        } else {
            failureMessages.append(singleResult.value(QStringLiteral("message")).toString());
        }
    }

    result.insert(QStringLiteral("importedCount"), importedCount);
    result.insert(QStringLiteral("failedCount"), deviceExperimentIds.size() - importedCount);

    if (importedCount > 0) {
        emit operationInfo(tr("已导入 %1 条实验记录").arg(importedCount));
    }

    const bool success = importedCount > 0 && failureMessages.isEmpty();
    result.insert(QStringLiteral("success"), success);

    QStringList messageParts;
    if (importedCount > 0) {
        messageParts.append(tr("已导入 %1 条实验记录").arg(importedCount));
    }
    if (!failureMessages.isEmpty()) {
        messageParts.append(failureMessages.join(QStringLiteral("\n")));
    }
    if (!warningMessages.isEmpty()) {
        messageParts.append(tr("设备端状态回写提醒：%1").arg(warningMessages.join(QStringLiteral("；"))));
    }
    result.insert(QStringLiteral("message"), messageParts.join(QStringLiteral("\n")));

    if (!failureMessages.isEmpty()) {
        qWarning() << "[dataCtrl] import failures:" << failureMessages;
    }
    if (!warningMessages.isEmpty()) {
        qWarning() << "[dataCtrl] import warnings:" << warningMessages;
    }

    return result;
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
        qDebug() << "[dataCtrl] 删除实验数据成功，实验 ID:" << experimentId;
    } else {
        emit operationFailed("删除实验数据失败");
        qWarning() << "[dataCtrl] 删除实验数据失败，实验 ID:" << experimentId;
    }
    
    return success;
}

bool dataCtrl::sendRequestAndWait(const QString &command,
                                  const QVariantMap &payload,
                                  QVariantMap *response,
                                  int timeoutMs)
{
    if (response) {
        response->clear();
    }

    if (!m_dataTransmitCtrl) {
        if (response) {
            response->insert(QStringLiteral("message"), tr("通信控制器未初始化"));
        }
        return false;
    }

    const QString requestId = QUuid::createUuid().toString().remove('{').remove('}');
    QVariantMap request = payload;
    request.insert(QStringLiteral("request_id"), requestId);

    QVariantMap replyPayload;
    bool finished = false;
    bool success = false;

    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);

    QMetaObject::Connection controlConn;
    QMetaObject::Connection timeoutConn;

    controlConn = connect(m_dataTransmitCtrl, &DataTransmitController::controlMessageReceived,
                          &loop, [&](const QVariantMap &message) {
        try {
            if (message.value(QStringLiteral("request_id")).toString() != requestId) {
                return;
            }

            if (message.value(QStringLiteral("type")).toString() != QStringLiteral("command_result")) {
                return;
            }

            replyPayload = message;
            success = message.value(QStringLiteral("success")).toBool();
            finished = true;
            loop.quit();
        } catch (const std::bad_alloc &) {
            replyPayload.clear();
            replyPayload.insert(QStringLiteral("message"), tr("设备响应数据过大，当前内存不足"));
            finished = true;
            success = false;
            loop.quit();
        }
    });

    timeoutConn = connect(&timeoutTimer, &QTimer::timeout, &loop, [&]() {
        replyPayload.insert(QStringLiteral("message"), tr("等待设备响应超时"));
        finished = true;
        success = false;
        loop.quit();
    });

    bool sendOk = false;
    if (QThread::currentThread() == m_dataTransmitCtrl->thread()) {
        sendOk = m_dataTransmitCtrl->sendControlCommand(command, request);
    } else {
        const bool invoked = QMetaObject::invokeMethod(m_dataTransmitCtrl, [&]() {
            sendOk = m_dataTransmitCtrl->sendControlCommand(command, request);
        }, Qt::BlockingQueuedConnection);
        if (!invoked) {
            disconnect(controlConn);
            disconnect(timeoutConn);
            replyPayload.insert(QStringLiteral("message"), tr("通信线程调用失败"));
            if (response) {
                *response = replyPayload;
            }
            return false;
        }
    }
    if (!sendOk) {
        disconnect(controlConn);
        disconnect(timeoutConn);
        replyPayload.insert(QStringLiteral("message"), m_dataTransmitCtrl->lastError());
        if (response) {
            *response = replyPayload;
        }
        return false;
    }

    timeoutTimer.start(timeoutMs);
    loop.exec();

    disconnect(controlConn);
    disconnect(timeoutConn);

    if (!finished) {
        replyPayload.insert(QStringLiteral("message"), tr("等待设备响应超时"));
        success = false;
    }

    if (response) {
        *response = replyPayload;
    }
    return success;
}

int dataCtrl::ensureLocalProjectId(const QString &projectName, const QString &projectDescription)
{
    const QString normalizedProjectName = projectName.trimmed();
    const QVector<QVariantMap> projects = m_dbManager->getAllProjects();
    for (const QVariantMap &project : projects) {
        if (project.value(QStringLiteral("project_name")).toString().trimmed() == normalizedProjectName) {
            return project.value(QStringLiteral("id")).toInt();
        }
    }

    QVariantMap projectData;
    projectData.insert(QStringLiteral("project_name"), normalizedProjectName);
    projectData.insert(QStringLiteral("description"), projectDescription);
    if (!m_dbManager->addProject(projectData)) {
        return 0;
    }

    const QVector<QVariantMap> refreshedProjects = m_dbManager->getAllProjects();
    for (const QVariantMap &project : refreshedProjects) {
        if (project.value(QStringLiteral("project_name")).toString().trimmed() == normalizedProjectName) {
            return project.value(QStringLiteral("id")).toInt();
        }
    }

    return 0;
}

int dataCtrl::findLatestExperimentId() const
{
    int maxId = 0;
    const QVector<QVariantMap> experiments = m_dbManager->getAllExperiments();
    for (const QVariantMap &experiment : experiments) {
        maxId = qMax(maxId, experiment.value(QStringLiteral("id")).toInt());
    }
    return maxId;
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

QVariantMap dataCtrl::getExperimentById(int experimentId)
{
    if (experimentId <= 0) {
        qWarning() << "[dataCtrl] 实验 ID 无效";
        return QVariantMap();
    }

    return m_dbManager->getExperimentById(experimentId);
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

QVariantMap dataCtrl::getPeakThicknessChartData(int experimentId, int intensityMode, double lowerMm, double upperMm, double thresholdValue)
{
    // 峰厚度页入口：直接使用原始扫描数据，由分析引擎完成阈值区段识别。
    if (experimentId <= 0) {
        qWarning() << "[dataCtrl] invalid experiment id for peak thickness chart";
        return QVariantMap();
    }

    try {
        return m_dbManager->getPeakThicknessChartDataByExperiment(
                    experimentId,
                    intensityMode,
                    lowerMm,
                    upperMm,
                    thresholdValue);
    } catch (const std::bad_alloc &) {
        qWarning() << "[dataCtrl] insufficient memory while loading peak thickness chart"
                   << "experimentId=" << experimentId
                   << "intensityMode=" << intensityMode
                   << "lowerMm=" << lowerMm
                   << "upperMm=" << upperMm
                   << "thresholdValue=" << thresholdValue;
        return QVariantMap();
    }
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

    // 回收站的“彻底删除”需要同时清掉实验表和原始 experiment_data，
    // 这里先显式删除实验原始数据，再执行实验记录物理删除，避免残留孤儿数据。
    const bool dataDeleted = m_dbManager->deleteExperimentDataByExperiment(experimentId);
    if (!dataDeleted) {
        emit operationFailed("清理实验数据失败");
        qWarning() << "[dataCtrl] 清理实验数据失败，实验 ID:" << experimentId;
        return false;
    }

    bool success = m_dbManager->hardDeleteExperiment(experimentId);

    if (success) {
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
        if (expId <= 0) {
            continue;
        }

        const bool dataDeleted = m_dbManager->deleteExperimentDataByExperiment(expId);
        if (!dataDeleted) {
            qWarning() << "[dataCtrl] 批量彻底删除时清理实验数据失败，实验 ID:" << expId;
            continue;
        }

        if (m_dbManager->hardDeleteExperiment(expId)) {
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
