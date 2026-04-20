#ifndef EXPERIMENT_SESSION_SERVICE_H
#define EXPERIMENT_SESSION_SERVICE_H

/**
 * @file experiment_session_service.h
 * @brief 实验扫描会话与高度分配逻辑。
 */

#include <QMap>
#include <QVariantMap>
#include <QVector>

#include "experiment_types.h"
#include "mainwindow_global.h"

// File note:
// ExperimentSessionService owns scan profiles and pending scan contexts. It
// converts raw storage pairs into rows with stable height and point indexes.

/**
 * @brief 管理实验运行期间的扫描上下文与高度计算。
 *
 * 该类专注于“本批数据属于哪一轮扫描、该落在哪个高度”，
 * 不直接读写 Modbus，也不直接操作数据库。
 */
class MAINWINDOW_EXPORT ExperimentSessionService
{
public:
    // Create an empty session manager for all channels.
    ExperimentSessionService();

    int calculateTotalSeconds(int days, int hours, int minutes, int seconds) const;
    int calculateExpectedPointCount(const ExperimentParams& params) const;
    ExperimentScanProfile buildScanProfile(const ExperimentParams& params) const;

    void resetScanContexts(int channel);
    void setScanProfile(int channel, const ExperimentScanProfile& profile);
    void beginScanCycle(int channel, const ExperimentParams& params);

    int currentScanCount(int channel) const;
    int pendingContextCount(int channel) const;

    QVector<QVariantMap> buildRowsFromStorageData(int channel, const QVector<quint16>& raw, bool areaA);

private:
    void refreshCurrentScanCount(int channel);
    QVector<QVariantMap> parseStoragePairs(int channel, const QVector<quint16>& raw, bool areaA,
                                          double startHeightUm, double stepUm,
                                          int startPointIndex, qint64 scanStartedAtMs,
                                          qint64 elapsedSinceExperimentStartMs, int scanId) const;

    QMap<int, QVector<ScanCycleContext>> m_scanContexts;
    QMap<int, ExperimentScanProfile> m_scanProfiles;
    QMap<int, int> m_nextScanSequences;
    QMap<int, int> m_currentScanCounts;
};

#endif
