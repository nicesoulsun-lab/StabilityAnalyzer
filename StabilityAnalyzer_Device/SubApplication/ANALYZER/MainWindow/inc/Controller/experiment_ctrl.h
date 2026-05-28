#ifndef EXPERIMENT_CTRL_H
#define EXPERIMENT_CTRL_H

#include <QObject>
#include <QVariantMap>
#include <QVector>
#include <QMap>
#include <QQueue>
#include <QTimer>
#include "Common/experiment_types.h"
#include "mainwindow_global.h"
#include "modbustaskscheduler.h"

class SqlOrmManager;
class ExperimentCommService;
class ExperimentDataService;
class ExperimentStateStore;
class ExperimentSessionService;

class MAINWINDOW_EXPORT ExperimentCtrl : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int channelCount READ channelCount CONSTANT)

public:
    enum Channel {
        ChannelA = 0,
        ChannelB,
        ChannelC,
        ChannelD
    };
    Q_ENUM(Channel)

    explicit ExperimentCtrl(QObject *parent = nullptr);
    ~ExperimentCtrl();

    Q_INVOKABLE void saveParams(int channel, const QVariantMap& params);
    Q_INVOKABLE QVariantMap loadParams(int channel);

    Q_INVOKABLE bool startExperiment(int channel, int creatorId);
    Q_INVOKABLE bool stopExperiment(int channel);
    Q_INVOKABLE bool requestStopExperiment(int channel);
    Q_INVOKABLE bool isExperimentRunning(int channel) const;

    Q_INVOKABLE int getCurrentScanCount(int channel) const;
    Q_INVOKABLE int getCurrentExperimentId(int channel) const;
    Q_INVOKABLE qint64 getElapsedTime(int channel) const;

    Q_INVOKABLE void setSerialConfig(int channel, const QString& portName, int baudRate, int dataBits, int parity, int stopBits);
    Q_INVOKABLE void setSlaveId(int channel, int slaveId);
    Q_INVOKABLE bool initializeScheduler(const QString& configDirPath = "");
    Q_INVOKABLE bool connectModbusDevice(int channel);
    Q_INVOKABLE void disconnectModbusDevice(int channel);
    Q_INVOKABLE bool isModbusConnected(int channel) const;
    Q_INVOKABLE void saveSerialConfig(int channel);
    Q_INVOKABLE void loadSerialConfig(int channel);
    Q_INVOKABLE int channelCount() const;
    Q_INVOKABLE QString channelName(int channel) const;
    Q_INVOKABLE QString channelDisplayName(int channel) const;

    Q_INVOKABLE QVariantMap getChannelStatus(int channel) const;

    Q_INVOKABLE void startCalibration(int channel, const QString& calibrationType);

    Q_INVOKABLE QString getLastCalibrationTime(int channel, const QString& calibrationType) const;

signals:
    void experimentStarted(int channel, int experimentId);
    void experimentStopped(int channel, int experimentId);
    void experimentStopRequested(int channel);

    void scanCompleted(int channel, int scanCount, const QVariantMap& data);

    void experimentError(int channel, const QString& error);
    void operationInfo(const QString& message);
    void operationFailed(const QString& message);

    void channelStatusUpdated(int channel, const QVariantMap& status);
    void scanDataChunkReady(int channel, int experimentId, int scanId, bool scanCompleted, const QVariantList& rows);

    void calibrationProgress(int channel, int currentRound, int totalRounds);
    void calibrationCompleted(int channel, const QVariantMap& summary);
    void calibrationFailed(int channel, const QString& reason);

private slots:
    void onScanTimer(int channel);
    void onExperimentTimeout(int channel);
    void onSchedulerTaskCompleted(TaskResult res, QVector<quint16> data);
    void onStatusPollTimer();
    void onChannelStatusPollTimer(int channel);

private:
    QString getDeviceId(int channel) const;
    bool sendControlCommand(int channel, const QString& command, const QVariantMap& params);
    QVariantMap readSensorData(int channel);
    QVariantMap readRealtimeStatus(int channel);
    void pollChannelStatus(int channel);
    void finishCurrentPoll();
    void initializeSchedulerAfterStartup();
    void startDeferredStatusPolling();

    void tryFetchStoredData(int channel, int storageAReadableCount, int storageBReadableCount,
                            int storageAState, int storageBState);
    void tryFetchCalibrationData(int channel, int storageAReadableCount, int storageBReadableCount,
                                  int storageAState, int storageBState);
    void computeCalibrationAverage(int channel);
    void triggerNextCalibrationScan(int channel);
    QVariantMap calibrationSummaryToVariantMap(const CalibrationSummary& summary) const;

    void generateDefaultConfig(const QString& configDirPath);

private:
    SqlOrmManager* m_dbManager;
    ModbusTaskScheduler* m_scheduler;
    ExperimentStateStore* m_stateStore;
    ExperimentSessionService* m_sessionService;
    ExperimentCommService* m_commService;
    ExperimentDataService* m_dataService;

    QMap<Channel, QTimer*> m_scanTimers;
    QMap<Channel, QTimer*> m_experimentTimers;

    QMap<Channel, int> m_experimentIds;
    QMap<Channel, qint64> m_startTimes;
    QMap<Channel, bool> m_runningFlags;
    QMap<Channel, int> m_plannedScanCounts;
    QMap<Channel, int> m_startedScanCounts;
    QMap<Channel, bool> m_stopAfterDrainFlags;
    QMap<Channel, qint64> m_stopAfterDrainDeadlineMs;
    bool m_anyPollInProgress = false;
    QQueue<int> m_pendingPollChannels;

    QMap<Channel, bool> m_calibrationModes;
    QMap<Channel, CalibrationScanState> m_calibrationScanStates;

    bool m_schedulerInitialized;

    QMap<Channel, QTimer*> m_statusPollTimers;
};

#endif
