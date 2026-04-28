#ifndef EXPERIMENT_STATE_STORE_H
#define EXPERIMENT_STATE_STORE_H

/**
 * @file experiment_state_store.h
 * @brief 实验相关运行状态、参数与配置缓存。
 */

#include <QMap>
#include <QVariantMap>
#include <QVector>

#include "experiment_types.h"
#include "mainwindow_global.h"
#include "modbustaskscheduler.h"

// File note:
// ExperimentStateStore keeps UI-facing state, serial configuration, cached
// experiment params and in-memory data rows. It is intentionally logic-light.

/**
 * @brief 管理实验控制所需的本地状态与持久化配置。
 *
 * 该类只负责“存什么、从哪儿读”，不负责实验流程控制，也不负责通信。
 * 主要集中管理：
 * 1. 参数页输入缓存与 QSettings 持久化；
 * 2. 串口配置与从站地址；
 * 3. 首页状态快照；
 * 4. 内存中的实验数据缓存。
 */
class MAINWINDOW_EXPORT ExperimentStateStore
{
public:
    // Create an empty store. Callers populate defaults during controller boot.
    ExperimentStateStore();

    QString channelKey(int channel) const;

    void setParams(int channel, const ExperimentParams& params);
    bool hasParams(int channel) const;
    ExperimentParams params(int channel) const;
    QVariantMap loadParams(int channel);

    void setDefaultSerialConfig(int channel, const SerialConfig& cfg);
    SerialConfig serialConfig(int channel) const;
    void setSerialConfig(int channel, const SerialConfig& cfg);

    void setSlaveId(int channel, int slaveId);
    int slaveId(int channel, int fallback = 1) const;

    void saveSerialConfig(int channel) const;
    void loadSerialConfig(int channel);

    void initializeChannelStatus(int channel, const QVariantMap& initialStatus);
    QVariantMap channelStatus(int channel) const;
    bool updateChannelStatus(int channel, const QVariantMap& patch, QVariantMap* mergedStatus = nullptr);

    QVector<QVariantMap>* memoryCache(int channel);
    void clearMemoryCache(int channel);

private:
    QMap<int, int> m_slaveIds;
    QMap<int, SerialConfig> m_serialConfigs;
    QMap<int, ExperimentParams> m_params;
    QMap<int, QVariantMap> m_channelStatuses;
    QMap<int, QVector<QVariantMap>> m_memoryDataCache;
};

#endif
