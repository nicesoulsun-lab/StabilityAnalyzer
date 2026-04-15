#ifndef EXPERIMENT_TYPES_H
#define EXPERIMENT_TYPES_H

/**
 * @file experiment_types.h
 * @brief 实验控制相关的共享数据结构定义。
 *
 * 这些结构体原先定义在 ExperimentCtrl 内部。拆分到 Common 后，
 * 便于状态存储、扫描会话和控制器之间复用，避免重复声明。
 */

#include <QString>

/**
 * @brief 单通道实验参数模型。
 *
 * 该结构体保存参数页输入，并在实验开始时用于：
 * 1. 写入数据库主表；
 * 2. 生成扫描快照；
 * 3. 下发扫描与温控指令。
 */
struct ExperimentParams
{
    int projectId = 0;
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

/**
 * @brief 单轮扫描上下文。
 *
 * 每次主机触发 `start_scan` 时创建一条上下文，用于把后续到达的
 * 采样点绑定到“所属扫描轮次”，避免因通信延迟把旧数据算进新一轮。
 */
struct ScanCycleContext
{
    int sequence = 0;
    int expectedPointCount = 0;
    int savedPointCount = 0;
    double startHeightUm = 0.0;
    double stepUm = 20.0;
    qint64 startedAtMs = 0;
};

/**
 * @brief 单次实验固定扫描参数快照。
 *
 * 实验开始后，单轮扫描上下文全部复用这份快照，避免运行中参数变动
 * 影响已经开始的实验。
 */
struct ExperimentScanProfile
{
    int expectedPointCount = 0;
    double startHeightUm = 0.0;
    double stepUm = 20.0;
};

#endif
