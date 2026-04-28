#ifndef EXPERIMENT_CTRL_H
#define EXPERIMENT_CTRL_H

#include <QObject>
#include <QTimer>
#include <QVariantMap>
#include <QVector>

#include "mainwindow_global.h"
#include "modbustaskscheduler.h"

class DataTransmitController;
class SqlOrmManager;

/**
 * @brief PC 侧实验控制器
 *
 * 当前职责分两层：
 * 1. 本地保存“新建实验”弹窗填写的参数，供 QML 回显；
 * 2. 通过 DataTransmit 控制通道把开始/停止实验请求转发给 Device 侧。
 *
 * 这样可以尽量保持 QML 层调用方式不变，同时把真正的实验执行落到 Device 侧。
 */
class MAINWINDOW_EXPORT ExperimentCtrl : public QObject
{
    Q_OBJECT

public:
    enum Channel {
        ChannelA = 0,
        ChannelB,
        ChannelC,
        ChannelD
    };
    Q_ENUM(Channel)

    struct ExperimentParams {
        int projectId = 0;
        QString projectName;
        QString sampleName;
        QString operatorName;
        QString description;

        int durationDays = 0;
        int durationHours = 0;
        int durationMinutes = 0;
        int durationSeconds = 0;

        int intervalHours = 0;
        int intervalMinutes = 0;
        int intervalSeconds = 0;

        int scanCount = 0;

        bool temperatureControl = false;
        double targetTemperature = 0.0;

        int scanRangeStart = 0;
        int scanRangeEnd = 0;
        int scanStep = 20;
    };

    explicit ExperimentCtrl(QObject *parent = nullptr);
    ~ExperimentCtrl() override;

    Q_INVOKABLE void saveParams(int channel, const QVariantMap &params);
    Q_INVOKABLE QVariantMap loadParams(int channel);

    Q_INVOKABLE bool startExperiment(int channel, int creatorId);
    Q_INVOKABLE bool stopExperiment(int channel);
    Q_INVOKABLE bool isExperimentRunning(int channel) const;

    Q_INVOKABLE int getCurrentScanCount(int channel) const;
    Q_INVOKABLE int getCurrentExperimentId(int channel) const;
    Q_INVOKABLE qint64 getElapsedTime(int channel) const;

    Q_INVOKABLE void setSerialConfig(int channel, const QString &portName, int baudRate, int dataBits, int parity, int stopBits);
    Q_INVOKABLE void setSlaveId(int channel, int slaveId);
    Q_INVOKABLE bool initializeScheduler(const QString &configDirPath = QString());
    Q_INVOKABLE bool connectModbusDevice(int channel);
    Q_INVOKABLE void disconnectModbusDevice(int channel);
    Q_INVOKABLE bool isModbusConnected(int channel) const;
    Q_INVOKABLE void saveSerialConfig(int channel);
    Q_INVOKABLE void loadSerialConfig(int channel);

    /**
     * @brief 由 ControllerManager 注入通信控制器
     *
     * ExperimentCtrl 本身不负责创建 DataTransmitController，避免和启动入口耦合。
     */
    void setDataTransmitController(DataTransmitController *controller);

signals:
    void experimentStarted(int channel, int experimentId);
    void experimentStopped(int channel, int experimentId);
    void scanCompleted(int channel, int scanCount, const QVariantMap &data);
    void experimentError(int channel, const QString &error);
    void operationInfo(const QString &message);
    void operationFailed(const QString &message);

private slots:
    void onScanTimer(int channel);
    void onExperimentTimeout(int channel);
    void onSchedulerTaskCompleted(TaskResult res, QVector<quint16> data);

private:
    int calculateTotalSeconds(int days, int hours, int minutes, int seconds) const;
    QString getChannelKey(int channel) const;
    QString getDeviceId(int channel) const;
    bool sendControlCommand(int channel, const QString &command, const QVariantMap &params);
    QVariantMap readSensorData(int channel);
    void generateDefaultConfig(const QString &configDirPath);
    void syncExperimentChannelsFromDevice();
    void finalizeStoppedChannel(Channel channel, int channelIndex, int experimentId, bool emitSignal);

    // 同步等待 Device 侧控制命令响应，供 start/stop 这类 QML 立即返回 bool 的接口使用。
    bool sendRequestAndWait(const QString &command,
                            const QVariantMap &payload,
                            QVariantMap *response = nullptr,
                            int timeoutMs = 5000);
    QVariantMap buildExperimentData(const ExperimentParams &params, int creatorId) const;
    ExperimentParams paramsFromVariantMap(const QVariantMap &params) const;

private:
    SqlOrmManager *m_dbManager = nullptr;
    ModbusTaskScheduler *m_scheduler = nullptr;
    DataTransmitController *m_dataTransmitCtrl = nullptr;
    QMap<Channel, int> m_slaveIds;
    QMap<Channel, SerialConfig> m_serialConfigs;
    QMap<Channel, ExperimentParams> m_params;
    QMap<Channel, QTimer *> m_scanTimers;
    QMap<Channel, QTimer *> m_experimentTimers;
    QMap<Channel, int> m_experimentIds;
    QMap<Channel, int> m_currentScanCounts;
    QMap<Channel, qint64> m_startTimes;
    QMap<Channel, bool> m_runningFlags;
    bool m_schedulerInitialized = false;
};

#endif
