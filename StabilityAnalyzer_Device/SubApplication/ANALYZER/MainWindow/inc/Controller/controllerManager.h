#ifndef CONTROLLERMANAGER_H
#define CONTROLLERMANAGER_H

#include <QObject>
#include <QDebug>
#include <QTimer>
#include "datatransmitcontroller.h"
#include "systemsetting_ctrl.h"
#include "user_ctrl.h"
#include "data_ctrl.h"
#include "experiment_ctrl.h"
#include "led_controller.h"
#include "realtime_curve_ctrl.h"
#include "DataModel/user_sql_listmodel.h"
#include "DataModel/experiment_listmodel.h"

class MAINWINDOW_EXPORT CtrllerManager : public QObject
{
    Q_OBJECT

public:
    explicit CtrllerManager(QObject *parent = nullptr)
        : QObject(parent)
        , m_dataTransmitCtrl(new DataTransmitController(this))
        , m_sysSettingCtrl(new systemSettingCtrl(this))
        , m_userCtrl(new userCtrl(this))
        , m_dataCtrl(new dataCtrl(this))
        , m_experimentCtrl(new ExperimentCtrl(this))
        , m_userListmodel(new user_sql_listmodel(this))
        , m_experimentListmodel(new experiment_listmodel(this))
        , m_ledController(new LedController(this))
        , m_realtimeCurveCtrl(new RealtimeCurveCtrl(m_experimentCtrl, m_dataCtrl, this))
    {
        QObject::connect(m_dataTransmitCtrl, &DataTransmitController::logMessage,
                         this,
                         [](const QString &message) {
            if (!message.contains(QStringLiteral("heartbeat"), Qt::CaseInsensitive))
                qDebug() << "[DataTransmit]" << message;
        });
        QObject::connect(m_dataTransmitCtrl, &DataTransmitController::lastErrorChanged,
                         this,
                         [this]() {
            const QString errorText = m_dataTransmitCtrl->lastError();
            if (!errorText.isEmpty()) {
                qWarning() << "[DataTransmit]" << errorText;
            }
        });

        QObject::connect(m_experimentCtrl, &ExperimentCtrl::channelStatusUpdated,
                         m_dataTransmitCtrl, &DataTransmitController::updateExperimentChannelStatus);

        QObject::connect(m_experimentCtrl, &ExperimentCtrl::experimentStarted,
                         m_ledController, &LedController::onExperimentStarted);
        QObject::connect(m_experimentCtrl, &ExperimentCtrl::experimentStopped,
                         m_ledController, &LedController::onExperimentStopped);

        // 轮询状态中的runStatus=3表示错误，传递给LED控制器
        QObject::connect(m_experimentCtrl, &ExperimentCtrl::channelStatusUpdated,
                         this,
                         [this](int channel, const QVariantMap &status) {
            // 实验中通道的错误状态由实验流程自行处理，不传播到LED错误逻辑
            if (m_experimentCtrl->isExperimentRunning(channel))
                return;
            const bool hasError = status.value(QStringLiteral("runStatus"), 0).toInt() == 3;
            m_ledController->applyChannelError(channel, hasError);
        });

        QObject::connect(m_experimentCtrl, &ExperimentCtrl::scanDataChunkReady,
                         this,
                         [this](int channel, int experimentId, int scanId, bool scanCompleted, const QVariantList &rows) {
            if (!scanCompleted) {
                return;
            }
            QVariantList streamRows;
            streamRows.reserve(rows.size());
            for (const QVariant &rowVariant : rows) {
                const QVariantMap row = rowVariant.toMap();
                streamRows.append(QVariantMap{
                    {QStringLiteral("timestamp"), row.value(QStringLiteral("timestamp"))},
                    {QStringLiteral("scan_id"), row.value(QStringLiteral("scan_id"), scanId)},
                    {QStringLiteral("scan_elapsed_ms"), row.value(QStringLiteral("scan_elapsed_ms"))},
                    {QStringLiteral("height"), row.value(QStringLiteral("height"))},
                    {QStringLiteral("backscatter_intensity"), row.value(QStringLiteral("backscatter_intensity"))},
                    {QStringLiteral("transmission_intensity"), row.value(QStringLiteral("transmission_intensity"))}
                });
            }

            const bool sent = m_dataTransmitCtrl->sendStreamMessage(QVariantMap{
                {QStringLiteral("type"), QStringLiteral("experiment_scan_data")},
                {QStringLiteral("channel"), channel},
                {QStringLiteral("experiment_id"), experimentId},
                {QStringLiteral("scan_id"), scanId},
                {QStringLiteral("scan_completed"), scanCompleted},
                {QStringLiteral("row_count"), streamRows.size()},
                {QStringLiteral("rows"), streamRows}
            });
            if (!sent) {
                qWarning() << "[ControllerManager] stream scan data send failed"
                           << "channel=" << channel
                           << "experimentId=" << experimentId
                           << "scanId=" << scanId
                           << "completed=" << scanCompleted
                           << "rows=" << rows.size();
            }
        });

        QObject::connect(m_dataTransmitCtrl, &DataTransmitController::startExperimentRequested,
                         this,
                         [this](int channel, int creatorId, const QVariantMap &params, const QString &requestId) {
            m_experimentCtrl->saveParams(channel, params);
            const bool started = m_experimentCtrl->startExperiment(channel, creatorId);
            m_dataTransmitCtrl->sendCommandResult(QStringLiteral("start_experiment"),
                                                 requestId,
                                                 started,
                                                 started ? QStringLiteral("Experiment started")
                                                         : QStringLiteral("Failed to start experiment"),
                                                 QVariantMap{
                                                     {QStringLiteral("channel"), channel},
                                                     {QStringLiteral("experiment_id"), m_experimentCtrl->getCurrentExperimentId(channel)}
                                                 });
        });

        QObject::connect(m_dataTransmitCtrl, &DataTransmitController::stopExperimentRequested,
                         this,
                         [this](int channel, const QString &requestId) {
            const bool accepted = m_experimentCtrl->requestStopExperiment(channel);
            m_dataTransmitCtrl->sendCommandResult(QStringLiteral("stop_experiment"),
                                                 requestId,
                                                 accepted,
                                                 accepted ? QStringLiteral("Stop request accepted")
                                                         : QStringLiteral("Failed to stop experiment"),
                                                 QVariantMap{{QStringLiteral("channel"), channel}});
        });

        QObject::connect(m_dataTransmitCtrl, &DataTransmitController::listImportExperimentsRequested,
                         this,
                         [this](const QString &requestId) {
            QVariantList experiments;
            const QVector<QVariantMap> rows = m_dataCtrl->getAllExperiments();
            for (const QVariantMap &row : rows) {
                experiments.append(row);
            }

            m_dataTransmitCtrl->sendCommandResult(QStringLiteral("list_importable_experiments"),
                                                  requestId,
                                                  true,
                                                  QString(),
                                                  QVariantMap{{QStringLiteral("experiments"), experiments}});
        });

        QObject::connect(m_dataTransmitCtrl, &DataTransmitController::exportExperimentRequested,
                         this,
                         [this](int experimentId, const QString &requestId) {
            const QVariantMap experiment = m_dataCtrl->getExperimentById(experimentId);
            if (experiment.isEmpty()) {
                m_dataTransmitCtrl->sendCommandResult(QStringLiteral("get_experiment_export"),
                                                      requestId,
                                                      false,
                                                      QStringLiteral("Experiment not found"),
                                                      QVariantMap{{QStringLiteral("experiment_id"), experimentId}});
                return;
            }

            QVariantList scanIds;
            const QVector<int> deviceScanIds = m_dataCtrl->getScanIdsByExperiment(experimentId);
            scanIds.reserve(deviceScanIds.size());
            for (int scanId : deviceScanIds) {
                scanIds.append(scanId);
            }
            const int dataCount = m_dataCtrl->getDataCountByExperiment(experimentId);

            m_dataTransmitCtrl->sendCommandResult(QStringLiteral("get_experiment_export"),
                                                  requestId,
                                                  true,
                                                  QString(),
                                                  QVariantMap{
                                                      {QStringLiteral("experiment_id"), experimentId},
                                                      {QStringLiteral("experiment"), experiment},
                                                      {QStringLiteral("scan_ids"), scanIds},
                                                      {QStringLiteral("data_count"), dataCount}
                                                  });
        });

        QObject::connect(m_dataTransmitCtrl, &DataTransmitController::exportExperimentScanRequested,
                         this,
                         [this](int experimentId, int scanId, int offset, int limit, const QString &requestId) {
            const QVariantMap experiment = m_dataCtrl->getExperimentById(experimentId);
            if (experiment.isEmpty()) {
                m_dataTransmitCtrl->sendCommandResult(QStringLiteral("get_experiment_scan_export"),
                                                      requestId,
                                                      false,
                                                      QStringLiteral("Experiment not found"),
                                                      QVariantMap{
                                                          {QStringLiteral("experiment_id"), experimentId},
                                                          {QStringLiteral("scan_id"), scanId},
                                                          {QStringLiteral("offset"), offset},
                                                          {QStringLiteral("limit"), limit}
                                                      });
                return;
            }

            QVariantList dataList;
            const int totalCount = m_dataCtrl->getDataCountByExperimentAndScan(experimentId, scanId);
            const QVector<QVariantMap> rows = m_dataCtrl->getDataByExperimentAndScan(experimentId, scanId, offset, limit);
            for (const QVariantMap &row : rows) {
                dataList.append(row);
            }
            const int pageLimit = limit > 0 ? limit : rows.size();
            const int nextOffset = offset + rows.size();
            const bool hasMore = pageLimit > 0 && nextOffset < totalCount;

            qDebug() << "[Import][device scan page]"
                     << "experimentId=" << experimentId
                     << "scanId=" << scanId
                     << "offset=" << offset
                     << "limit=" << pageLimit
                     << "rows=" << rows.size()
                     << "totalCount=" << totalCount
                     << "hasMore=" << hasMore;

            m_dataTransmitCtrl->sendCommandResult(QStringLiteral("get_experiment_scan_export"),
                                                  requestId,
                                                  totalCount > 0,
                                                  totalCount <= 0 ? QStringLiteral("Scan not found") : QString(),
                                                  QVariantMap{
                                                      {QStringLiteral("experiment_id"), experimentId},
                                                      {QStringLiteral("scan_id"), scanId},
                                                      {QStringLiteral("offset"), offset},
                                                      {QStringLiteral("limit"), pageLimit},
                                                      {QStringLiteral("total_count"), totalCount},
                                                      {QStringLiteral("has_more"), hasMore},
                                                      {QStringLiteral("data"), dataList}
                                                  });
        });

        QObject::connect(m_dataTransmitCtrl, &DataTransmitController::markExperimentImportedRequested,
                         this,
                         [this](int experimentId, int status, const QString &requestId) {
            const bool success = m_dataCtrl->updateExperimentStatus(experimentId, status);
            m_dataTransmitCtrl->sendCommandResult(QStringLiteral("mark_experiment_imported"),
                                                  requestId,
                                                  success,
                                                  success ? QStringLiteral("Experiment status updated")
                                                          : QStringLiteral("Failed to update experiment status"),
                                                  QVariantMap{
                                                      {QStringLiteral("experiment_id"), experimentId},
                                                      {QStringLiteral("status"), status}
                                                  });
        });

        QObject::connect(m_dataTransmitCtrl, &DataTransmitController::calibrationRequested,
                         this,
                         [this](int channel, const QString& calibrationType,
                                const QString &requestId) {
            m_experimentCtrl->startCalibration(channel, calibrationType);
            m_dataTransmitCtrl->sendCommandResult(
                QStringLiteral("start_calibration"), requestId, true,
                QString("Calibration started for channel %1").arg(channel),
                QVariantMap{{QStringLiteral("channel"), channel}});
        });

        QObject::connect(m_dataTransmitCtrl, &DataTransmitController::calibrationTimeRequested,
                         this,
                         [this](int channel, const QString& calibrationType, const QString &requestId) {
            const QString time = m_experimentCtrl->getLastCalibrationTime(channel, calibrationType);
            m_dataTransmitCtrl->sendCommandResult(
                QStringLiteral("get_calibration_time"), requestId, true,
                QStringLiteral(""),
                QVariantMap{{QStringLiteral("calibrated_at"), time}});
        });

        QObject::connect(m_experimentCtrl, &ExperimentCtrl::calibrationProgress,
                         this,
                         [this](int channel, int currentRound, int totalRounds) {
            m_dataTransmitCtrl->sendStreamMessage(QVariantMap{
                {QStringLiteral("type"), QStringLiteral("calibration_progress")},
                {QStringLiteral("channel"), channel},
                {QStringLiteral("current_round"), currentRound},
                {QStringLiteral("total_rounds"), totalRounds}
            });
        });

        QObject::connect(m_experimentCtrl, &ExperimentCtrl::calibrationCompleted,
                         this,
                         [this](int channel, const QVariantMap &summary) {
            m_dataTransmitCtrl->sendStreamMessage(QVariantMap{
                {QStringLiteral("type"), QStringLiteral("calibration_completed")},
                {QStringLiteral("channel"), channel},
                {QStringLiteral("summary"), summary}
            });
        });

        QObject::connect(m_experimentCtrl, &ExperimentCtrl::calibrationFailed,
                         this,
                         [this](int channel, const QString &reason) {
            m_dataTransmitCtrl->sendStreamMessage(QVariantMap{
                {QStringLiteral("type"), QStringLiteral("calibration_failed")},
                {QStringLiteral("channel"), channel},
                {QStringLiteral("reason"), reason}
            });
        });

        for (int channel = 0; channel < m_experimentCtrl->channelCount(); ++channel) {
            m_dataTransmitCtrl->updateExperimentChannelStatus(channel, m_experimentCtrl->getChannelStatus(channel));
        }

        QTimer::singleShot(0, m_dataTransmitCtrl, SLOT(startConnection()));
    }

    // 获取各个 Controller 实例
    DataTransmitController *getDataTransmitCtrl() const { return m_dataTransmitCtrl; }
    systemSettingCtrl *getSystemSettingCtrl() const { return m_sysSettingCtrl; }
    userCtrl *getUserCtrl() const { return m_userCtrl; }
    dataCtrl *getDataCtrl() const { return m_dataCtrl; }
    ExperimentCtrl *getExperimentCtrl() const { return m_experimentCtrl; }
    LedController *getLedController() const { return m_ledController; }
    RealtimeCurveCtrl *getRealtimeCurveCtrl() const { return m_realtimeCurveCtrl; }

    user_sql_listmodel *getUserListmodel() const { return m_userListmodel; }
    experiment_listmodel *getExperimentListmodel() const { return m_experimentListmodel; }

private:
    DataTransmitController *m_dataTransmitCtrl = nullptr;
    systemSettingCtrl *m_sysSettingCtrl = nullptr;
    userCtrl *m_userCtrl = nullptr;
    dataCtrl *m_dataCtrl = nullptr;
    ExperimentCtrl *m_experimentCtrl = nullptr;
    user_sql_listmodel *m_userListmodel = nullptr;
    experiment_listmodel *m_experimentListmodel = nullptr;
    LedController *m_ledController = nullptr;
    RealtimeCurveCtrl *m_realtimeCurveCtrl = nullptr;
};

#endif // CONTROLLERMANAGER_H
