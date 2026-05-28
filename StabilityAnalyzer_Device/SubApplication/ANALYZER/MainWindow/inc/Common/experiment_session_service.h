#ifndef EXPERIMENT_SESSION_SERVICE_H
#define EXPERIMENT_SESSION_SERVICE_H

#include <QMap>
#include <QVariantMap>
#include <QVector>

#include "experiment_types.h"
#include "mainwindow_global.h"

class SqlOrmManager;

class MAINWINDOW_EXPORT ExperimentSessionService
{
public:
    ExperimentSessionService();

    int calculateTotalSeconds(int days, int hours, int minutes, int seconds) const;
    int calculateExpectedPointCount(const ExperimentParams& params) const;
    ExperimentScanProfile buildScanProfile(const ExperimentParams& params) const;

    void resetScanContexts(int channel);
    void setScanProfile(int channel, const ExperimentScanProfile& profile);
    void beginScanCycle(int channel, const ExperimentParams& params);

    int currentScanCount(int channel) const;
    int pendingContextCount(int channel) const;

    void loadCalibrationAvgTable(int channel, SqlOrmManager* dbManager);
    bool hasCalibrationAvgTable(int channel) const;
    double findCalibrationAvgTransmission(int channel, double heightUm) const;
    double findCalibrationAvgBackscatter(int channel, double heightUm) const;

    QVector<QVariantMap> buildRowsFromStorageData(int channel, const QVector<quint16>& raw, bool areaA);

    QVector<QVariantMap> buildCalibrationRows(int channel,
                                               const QVector<quint16>& raw,
                                               bool areaA,
                                               double scanRangeStartMm,
                                               double scanStepUm,
                                               int globalStartPairIndex) const;

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
    QMap<int, QVector<CalibrationAvgEntry>> m_calibrationAvgTables;
};

#endif
