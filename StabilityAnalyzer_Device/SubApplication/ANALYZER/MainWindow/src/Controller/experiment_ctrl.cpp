
#include "inc/Controller/experiment_ctrl.h"
#include "inc/Common/experiment_comm_service.h"
#include "inc/Common/experiment_data_service.h"
#include "inc/Common/experiment_session_service.h"
#include "inc/Common/experiment_state_store.h"
#include "../../../SqlOrm/inc/SqlOrmManager.h"
#include "../../../TaskScheduler/inc/modbustaskscheduler.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QtMath>
#include <QDebug>
#include <algorithm>

namespace {
constexpr int kChannelCount = 4;
constexpr qint64 kDrainStopGraceMs = 15000;

/**
 * @brief 由于 addExperiment 当前返回 bool，这里通过“取最大ID”方式获取最新实验ID
 */
int findLatestExperimentId(SqlOrmManager* db)
{
    if (!db) {
        return 0;
    }

    // SqlOrm addExperiment currently returns bool, so we fallback to latest inserted id.
    const QVector<QVariantMap> experiments = db->getAllExperiments();
    int maxId = 0;
    for (const QVariantMap& item : experiments) {
        maxId = qMax(maxId, item.value("id", 0).toInt());
    }
    return maxId;
}

int ensureProjectIdForExperiment(SqlOrmManager *db,
                                 const ExperimentParams &params,
                                 int creatorId)
{
    if (!db) {
        return 0;
    }

    const QString normalizedProjectName = params.projectName.trimmed();
    if (params.projectId > 0) {
        const QVariantMap existingProject = db->getProjectById(params.projectId);
        if (!existingProject.isEmpty()) {
            return params.projectId;
        }
    }

    if (!normalizedProjectName.isEmpty()) {
        const QVector<QVariantMap> projects = db->getAllProjects();
        for (const QVariantMap &project : projects) {
            if (project.value(QStringLiteral("project_name")).toString().trimmed() == normalizedProjectName) {
                return project.value(QStringLiteral("id")).toInt();
            }
        }

        QVariantMap projectData;
        projectData.insert(QStringLiteral("project_name"), normalizedProjectName);
        projectData.insert(QStringLiteral("description"), QString());
        projectData.insert(QStringLiteral("creator_id"), creatorId);
        if (!db->addProject(projectData)) {
            return 0;
        }

        const QVector<QVariantMap> refreshedProjects = db->getAllProjects();
        for (const QVariantMap &project : refreshedProjects) {
            if (project.value(QStringLiteral("project_name")).toString().trimmed() == normalizedProjectName) {
                return project.value(QStringLiteral("id")).toInt();
            }
        }
    }

    return params.projectId;
}

int resolveScanIntervalSeconds(const ExperimentParams &params, int totalDurationSeconds)
{
    const int configuredIntervalSeconds =
            qMax(0, params.intervalHours) * 3600
            + qMax(0, params.intervalMinutes) * 60
            + qMax(0, params.intervalSeconds);
    if (configuredIntervalSeconds > 0) {
        return configuredIntervalSeconds;
    }

    if (params.scanCount > 0 && totalDurationSeconds > 0) {
        return qMax(1, totalDurationSeconds / params.scanCount);
    }

    return 0;
}

/**
 * @brief 获取默认配置目录
 *
 * 优先使用当前工作目录下的 `config/experiment_devices`（便于开发期直接改JSON联调）；
 * 若不存在则回退到应用目录下同名路径（兼容打包运行）。
 */
QString defaultConfigDirPath()
{
    // 统一使用“程序实际运行目录”下的配置目录：
    // Windows 示例：bin-mingw/config/experiment_devices
    // Linux 示例：/opt/<app>/config/experiment_devices
    return QCoreApplication::applicationDirPath() + "/config/experiment_devices";
}
}

/**
 * @brief 构造函数
 *
 * 初始化内容：
 * 1. 四通道默认串口与状态缓存；
 * 2. 调度器任务完成信号绑定；
 * 3. 串口配置加载；
 * 4. 启动每秒状态轮询定时器。
 */
ExperimentCtrl::ExperimentCtrl(QObject *parent)
    : QObject(parent)
    , m_dbManager(SqlOrmManager::instance())
    , m_scheduler(ModbusTaskScheduler::instance())
    , m_stateStore(new ExperimentStateStore())
    , m_sessionService(new ExperimentSessionService())
    , m_commService(new ExperimentCommService(m_scheduler))
    , m_dataService(new ExperimentDataService(m_dbManager, m_scheduler))
    , m_schedulerInitialized(false)
{
    qDebug() << "[ExperimentCtrl] initializing...";

    for (int i = 0; i < kChannelCount; ++i) {
        Channel channel = static_cast<Channel>(i);

        m_scanTimers[channel] = new QTimer(this);
        m_experimentTimers[channel] = new QTimer(this);
        m_statusPollTimers[channel] = new QTimer(this);
        m_runningFlags[channel] = false;
        m_plannedScanCounts[channel] = 0;
        m_startedScanCounts[channel] = 0;
        m_stopAfterDrainFlags[channel] = false;
        m_stopAfterDrainDeadlineMs[channel] = 0;
        // 默认给每个通道分配不同 slaveId，避免调度器加载 JSON 时因重复而跳过设备。
        m_stateStore->setSlaveId(i, i + 1);

        SerialConfig serialCfg;
        switch (channel) {
        case ChannelA: serialCfg.portName = "COM1"; break;
        case ChannelB: serialCfg.portName = "COM2"; break;
        case ChannelC: serialCfg.portName = "COM3"; break;
        case ChannelD: serialCfg.portName = "COM4"; break;
        }
        serialCfg.baudRate = 9600;
        serialCfg.dataBits = 8;
        serialCfg.parity = "NoParity";
        serialCfg.stopBits = 1;
        serialCfg.flowControl = "NoFlowControl";
        m_stateStore->setDefaultSerialConfig(i, serialCfg);

        m_stateStore->initializeChannelStatus(i, {
            {"channel", i},
            {"connected", false},
            {"startFlag", 0},
            {"runStatus", 0},
            {"running", false},
            {"experiment_id", 0},
            {"coverStatus", 0},
            {"isCovered", false},
            {"sampleStatus", 0},
            {"hasSample", false},
            {"currentTemperature", 0.0},
            {"transmission", 0.0},
            {"backscatter", 0.0},
            {"storageAReadableCount", 0},
            {"storageBReadableCount", 0},
            {"storageAState", 0},
            {"storageBState", 0},
            {"remainingSeconds", 0}
        });

        connect(m_scanTimers[channel], &QTimer::timeout, this, [this, channel]() {
            onScanTimer(channel);
        });

        connect(m_experimentTimers[channel], &QTimer::timeout, this, [this, channel]() {
            onExperimentTimeout(channel);
        });

        connect(m_statusPollTimers[channel], &QTimer::timeout, this, [this, channel]() {
            onChannelStatusPollTimer(static_cast<int>(channel));
        });
    }

    connect(m_scheduler, &ModbusTaskScheduler::taskCompleted,
            this, &ExperimentCtrl::onSchedulerTaskCompleted);

    for (int i = 0; i < kChannelCount; ++i) {
        loadSerialConfig(i);
    }

    // 启动时对“历史持久化配置”做一次兜底修正：
    // 1) 如果 4 个通道里存在重复 slaveId，自动回退为 1/2/3/4（调度器要求唯一）。
    // 2) Linux 下若仍是 Windows 风格串口名（COMx），自动改成 ttyUSB0，避免端口不存在。
    QSet<int> usedSlaveIds;
    bool serialPatched = false;
    for (int i = 0; i < kChannelCount; ++i) {
        const Channel ch = static_cast<Channel>(i);
        int sid = m_stateStore->slaveId(i, i + 1);
        if (sid <= 0 || usedSlaveIds.contains(sid)) {
            sid = i + 1;
            m_stateStore->setSlaveId(i, sid);
            serialPatched = true;
            qWarning() << "[ExperimentCtrl][Boot] duplicate/invalid slaveId fixed, channel=" << i
                       << "newSlaveId=" << sid;
        }
        usedSlaveIds.insert(sid);

#ifdef Q_OS_LINUX
        SerialConfig cfg = m_stateStore->serialConfig(i);
        if (cfg.portName.startsWith("COM", Qt::CaseInsensitive)) {
            cfg.portName = "ttyUSB0";
            m_stateStore->setSerialConfig(i, cfg);
            serialPatched = true;
            qWarning() << "[ExperimentCtrl][Boot] windows style port fixed for linux, channel=" << i
                       << "newPort=" << cfg.portName;
        }
#endif
    }

    // 如果进行了兜底修正，立即持久化，确保后续生成 JSON 与运行配置一致。
    if (serialPatched) {
        for (int i = 0; i < kChannelCount; ++i) {
            saveSerialConfig(i);
        }
    }

    // 启动阶段先让界面起来，再在事件循环空闲后初始化调度器与轮询，避免首帧卡顿。
    QTimer::singleShot(0, this, [this]() {
        initializeSchedulerAfterStartup();
        startDeferredStatusPolling();
    });

    qDebug() << "[ExperimentCtrl] initialized";
}

/**
 * @brief 析构函数
 *
 * 清理策略：
 * - 若实验仍在运行先停实验；
 * - 停止本类定时器；
 * - 最后停止调度器。
 */
ExperimentCtrl::~ExperimentCtrl()
{
    for (auto channel : {ChannelA, ChannelB, ChannelC, ChannelD}) {
        if (m_runningFlags.value(channel, false)) {
            stopExperiment(channel);
        }

        if (m_scanTimers.value(channel)) {
            m_scanTimers[channel]->stop();
        }
        if (m_experimentTimers.value(channel)) {
            m_experimentTimers[channel]->stop();
        }
    }

    for (auto channel : {ChannelA, ChannelB, ChannelC, ChannelD}) {
        if (m_statusPollTimers.value(channel)) {
            m_statusPollTimers[channel]->stop();
        }
    }

    if (m_schedulerInitialized && m_scheduler->isRunning()) {
        m_scheduler->stopScheduler();
    }

    delete m_commService;
    m_commService = nullptr;
    delete m_dataService;
    m_dataService = nullptr;
    delete m_sessionService;
    m_sessionService = nullptr;
    delete m_stateStore;
    m_stateStore = nullptr;

    qDebug() << "[ExperimentCtrl] destroyed";
}

/**
 * @brief 初始化调度器并加载 JSON 配置目录
 *
 * 配置由 generateDefaultConfig 统一生成，保证任务名与业务层调用一致。
 */
bool ExperimentCtrl::initializeScheduler(const QString& configDirPath)
{
    const QString configPath = configDirPath.isEmpty() ? defaultConfigDirPath() : configDirPath;
    return m_commService->initializeScheduler(
        configPath,
        [this](int ch) { return m_stateStore->serialConfig(ch); },
        [this](int ch) { return m_stateStore->slaveId(ch, ch + 1); },
        &m_schedulerInitialized);
}

/**
 * @brief 生成四通道默认 JSON 任务配置
 *
 * 设计原则：
 * - 业务代码只调用“任务名”，不写硬编码寄存器地址；
 * - 地址/数量都放在 taskList 中维护；
 * - 后续改寄存器映射时优先改 JSON。
 */
void ExperimentCtrl::generateDefaultConfig(const QString& configDirPath)
{
    m_commService->generateDefaultConfig(
        configDirPath,
        [this](int ch) { return m_stateStore->serialConfig(ch); },
        [this](int ch) { return m_stateStore->slaveId(ch, ch + 1); });
}

void ExperimentCtrl::initializeSchedulerAfterStartup()
{
    const QString bootConfigPath = defaultConfigDirPath();
    if (!initializeScheduler(bootConfigPath)) {
        qWarning() << "[ExperimentCtrl][Boot] scheduler init failed after startup, configPath=" << bootConfigPath;
    } else {
        qDebug() << "[ExperimentCtrl][Boot] scheduler init success after startup";
    }
}

void ExperimentCtrl::startDeferredStatusPolling()
{
    for (int i = 0; i < kChannelCount; ++i) {
        const Channel ch = static_cast<Channel>(i);
        if (!m_statusPollTimers.value(ch)) {
            continue;
        }

        m_statusPollTimers[ch]->setInterval(1000);
        QTimer::singleShot(300 + i * 250, this, [this, ch]() {
            if (m_statusPollTimers.value(ch)) {
                m_statusPollTimers[ch]->start();
            }
        });
    }
}

QString ExperimentCtrl::getDeviceId(int channel) const
{
    return QString::number(m_stateStore->slaveId(channel, 1));
}

/**
 * @brief 保存实验参数
 *
 * - 写入状态存储，供运行态与重启恢复共同复用
 * - 同步写入 QSettings（应用重启后仍可恢复）
 */
void ExperimentCtrl::saveParams(int channel, const QVariantMap& params)
{
    ExperimentParams expParams;
    expParams.projectId = params.value("projectId", 0).toInt();
    expParams.sampleName = params.value("sampleName", "").toString();
    expParams.operatorName = params.value("operatorName", "").toString();
    expParams.description = params.value("description", "").toString();
    expParams.durationDays = params.value("durationDays", 0).toInt();
    expParams.durationHours = params.value("durationHours", 0).toInt();
    expParams.durationMinutes = params.value("durationMinutes", 0).toInt();
    expParams.durationSeconds = params.value("durationSeconds", 0).toInt();
    expParams.intervalHours = params.value("intervalHours", 0).toInt();
    expParams.intervalMinutes = params.value("intervalMinutes", 0).toInt();
    expParams.intervalSeconds = params.value("intervalSeconds", 0).toInt();
    expParams.scanCount = params.value("scanCount", 0).toInt();
    expParams.temperatureControl = params.value("temperatureControl", false).toBool();
    expParams.targetTemperature = params.value("targetTemperature", 0.0).toDouble();
    expParams.scanRangeStart = params.value("scanRangeStart", 0).toInt();
    expParams.scanRangeEnd = params.value("scanRangeEnd", 0).toInt();
    expParams.scanStep = params.value("scanStep", 20).toInt();
    m_stateStore->setParams(channel, expParams);

    emit operationInfo(tr("参数保存成功"));
}

/**
 * @brief 读取实验参数
 *
 * 优先从状态存储读取并按需回填内存缓存。
 */
QVariantMap ExperimentCtrl::loadParams(int channel)
{
    return m_stateStore->loadParams(channel);
}

/**
 * @brief 启动实验主流程
 *
 * 关键步骤：
 * 1. 校验运行状态、参数、调度器；
 * 2. 写入实验主表并拿到 experimentId；
 * 3. 下发扫描控制任务（温控/范围/步长/开始标志）；
 * 4. 更新首页状态并发 started 信号。
 */
bool ExperimentCtrl::startExperiment(int channel, int creatorId)
{
    const Channel ch = static_cast<Channel>(channel);

    if (m_runningFlags.value(ch, false)) {
        emit operationFailed(tr("实验已在运行中"));
        return false;
    }

    if (!m_stateStore->hasParams(channel)) {
        emit operationFailed(tr("请先设置实验参数"));
        return false;
    }

    if (!m_schedulerInitialized) {
        const QString configPath = defaultConfigDirPath();
        if (!initializeScheduler(configPath)) {
            emit operationFailed(tr("请先初始化通信模块"));
            return false;
        }
    }

    const ExperimentParams params = m_stateStore->params(channel);
    if (params.scanStep <= 0 || params.scanRangeEnd <= params.scanRangeStart) {
        emit operationFailed(tr("扫描区间或步长无效"));
        return false;
    }

    qDebug() << "[ExperimentCtrl][Start] channel=" << channel
             << "projectId=" << params.projectId
             << "projectName=" << params.projectName
             << "duration(d/h/m/s)=" << params.durationDays << params.durationHours << params.durationMinutes << params.durationSeconds
             << "interval(h/m/s)=" << params.intervalHours << params.intervalMinutes << params.intervalSeconds
             << "scanRange=" << params.scanRangeStart << "~" << params.scanRangeEnd
             << "step=" << params.scanStep
             << "tempCtrl=" << params.temperatureControl
             << "targetTemp=" << params.targetTemperature;

    const int deviceProjectId = ensureProjectIdForExperiment(m_dbManager, params, creatorId);
    if (deviceProjectId <= 0) {
        emit operationFailed(tr("创建设备端工程失败"));
        return false;
    }

    QVariantMap experimentData;
    experimentData["project_id"] = deviceProjectId;
    experimentData["sample_name"] = params.sampleName;
    experimentData["operator_name"] = params.operatorName;
    experimentData["description"] = params.description;
    experimentData["creator_id"] = creatorId;

    const int durationSeconds = m_sessionService->calculateTotalSeconds(
                params.durationDays, params.durationHours, params.durationMinutes, params.durationSeconds);
    const int intervalSeconds = resolveScanIntervalSeconds(params, durationSeconds);

    experimentData["duration"] = durationSeconds;
    experimentData["interval"] = intervalSeconds;
    experimentData["count"] = params.scanCount;
    experimentData["temperature_control"] = params.temperatureControl;
    experimentData["target_temp"] = params.targetTemperature;
    experimentData["scan_range_start"] = params.scanRangeStart;
    experimentData["scan_range_end"] = params.scanRangeEnd;
    experimentData["scan_step"] = params.scanStep;
    experimentData["status"] = 0;

    if (!m_dbManager->addExperiment(experimentData)) {
        emit operationFailed(tr("创建实验失败"));
        return false;
    }

    const int experimentId = findLatestExperimentId(m_dbManager);
    if (experimentId <= 0) {
        emit operationFailed(tr("创建实验失败"));
        return false;
    }
    qDebug() << "[ExperimentCtrl][Start] created experimentId=" << experimentId;

    m_experimentIds[ch] = experimentId;
    m_stateStore->clearMemoryCache(channel);
    m_sessionService->resetScanContexts(channel);
    ExperimentScanProfile scanProfile = m_sessionService->buildScanProfile(params);
    scanProfile.experimentStartMs = QDateTime::currentMSecsSinceEpoch();
    m_sessionService->setScanProfile(channel, scanProfile);
    m_startTimes[ch] = scanProfile.experimentStartMs;
    m_runningFlags[ch] = true;
    m_plannedScanCounts[ch] = qMax(1, params.scanCount);
    m_startedScanCounts[ch] = 0;
    m_stopAfterDrainFlags[ch] = false;
    m_stopAfterDrainDeadlineMs[ch] = 0;

    const bool multiScanByInterval = (intervalSeconds > 0 && m_plannedScanCounts.value(ch, 1) > 1);
    if (durationSeconds > 0 && !multiScanByInterval) {
        m_experimentTimers[ch]->start(durationSeconds * 1000);
    } else {
        m_experimentTimers[ch]->stop();
    }
    if (intervalSeconds > 0) {
        m_scanTimers[ch]->start(intervalSeconds * 1000);
    }
    qDebug() << "[ExperimentCtrl][Start] effective durationSeconds=" << durationSeconds
             << "effective intervalSeconds=" << intervalSeconds
             << "scanCount=" << params.scanCount;

    // 开始实验前，先将用户参数写入对应寄存器。
    sendControlCommand(channel, "set_scan_range", {{"start", params.scanRangeStart}, {"end", params.scanRangeEnd}});
    sendControlCommand(channel, "set_step", {{"step", params.scanStep}});
    sendControlCommand(channel, "set_temperature_control", {{"enabled", params.temperatureControl ? 1 : 0}});
    sendControlCommand(channel, "set_temperature", {{"temperature", params.targetTemperature}});
    // 立即触发首轮扫描；每次 start_scan 都先登记一条扫描上下文，
    // 后续到达的数据按上下文顺序分配，避免晚到数据串到下一轮高度。
    m_sessionService->beginScanCycle(channel, params);
    m_startedScanCounts[ch] = 1;
    if (m_startedScanCounts.value(ch, 0) >= m_plannedScanCounts.value(ch, 1)) {
        m_scanTimers[ch]->stop();
    }
    sendControlCommand(channel, "start_scan", {{"value", 1}});
    qDebug() << "[ExperimentCtrl][Start] control commands sent, first scan triggered";

    QVariantMap mergedStatus;
    if (m_stateStore->updateChannelStatus(channel, {
        {"running", true},
        {"experiment_id", experimentId},
        {"remainingSeconds", durationSeconds}
    }, &mergedStatus)) {
        emit channelStatusUpdated(channel, mergedStatus);
    }

    emit experimentStarted(channel, experimentId);
    emit operationInfo(tr("实验开始"));

    QVariantMap logData;
    logData["username"] = "";
    logData["user_id"] = creatorId;
    logData["operation"] = QString("开始了通道%1的实验").arg(channel);
    m_dbManager->addOperationLog(logData);

    return true;
}

/**
 * @brief 停止实验主流程
 *
 * 关键步骤：
 * 1. 停止本地计时器；
 * 2. 下发开始标志=0；
 * 3. 更新数据库实验状态；
 * 4. 更新首页状态并发 stopped 信号。
 */
bool ExperimentCtrl::stopExperiment(int channel)
{
    const Channel ch = static_cast<Channel>(channel);

    if (!m_runningFlags.value(ch, false)) {
        return false;
    }

    m_scanTimers[ch]->stop();
    m_experimentTimers[ch]->stop();
    m_plannedScanCounts[ch] = 0;
    m_startedScanCounts[ch] = 0;
    m_stopAfterDrainFlags[ch] = false;
    m_stopAfterDrainDeadlineMs[ch] = 0;

    // 停止实验时清除扫描触发标志。
    sendControlCommand(channel, "stop_scan", {{"value", 0}});
    qDebug() << "[ExperimentCtrl][Stop] channel=" << channel << "write startFlag=0";

    const int experimentId = m_experimentIds.value(ch, 0);

    m_stateStore->clearMemoryCache(channel);
    m_sessionService->resetScanContexts(channel);
    m_runningFlags[ch] = false;
    QVariantMap mergedStatus;
    if (m_stateStore->updateChannelStatus(channel, {
        {"running", false},
        {"experiment_id", 0},
        {"remainingSeconds", 0}
    }, &mergedStatus)) {
        emit channelStatusUpdated(channel, mergedStatus);
    }

    emit experimentStopped(channel, experimentId);
    emit operationInfo(tr("实验已停止"));
    return true;
}

bool ExperimentCtrl::isExperimentRunning(int channel) const
{
    return m_runningFlags.value(static_cast<Channel>(channel), false);
}

int ExperimentCtrl::getCurrentScanCount(int channel) const
{
    return m_sessionService->currentScanCount(channel);
}

int ExperimentCtrl::getCurrentExperimentId(int channel) const
{
    return m_experimentIds.value(static_cast<Channel>(channel), 0);
}

qint64 ExperimentCtrl::getElapsedTime(int channel) const
{
    const Channel ch = static_cast<Channel>(channel);
    if (!m_runningFlags.value(ch, false)) {
        return 0;
    }
    return (QDateTime::currentMSecsSinceEpoch() - m_startTimes.value(ch, 0)) / 1000;
}
void ExperimentCtrl::setSerialConfig(int channel, const QString& portName, int baudRate,
                                     int dataBits, int parity, int stopBits)
{
    // 将UI侧 parity 数值映射到调度器识别的字符串。
    SerialConfig cfg = m_stateStore->serialConfig(channel);
    cfg.portName = portName;
    cfg.baudRate = baudRate;
    cfg.dataBits = dataBits;
    cfg.parity = (parity == 0) ? "NoParity"
               : (parity == 1) ? "OddParity"
               : (parity == 2) ? "EvenParity"
               : (parity == 3) ? "SpaceParity"
                               : "MarkParity";
    cfg.stopBits = stopBits;

    m_stateStore->setSerialConfig(channel, cfg);
}

void ExperimentCtrl::setSlaveId(int channel, int slaveId)
{
    m_stateStore->setSlaveId(channel, slaveId);
}

bool ExperimentCtrl::connectModbusDevice(int channel)
{
    if (!m_commService->connectModbusDevice(
            channel,
            &m_schedulerInitialized,
            defaultConfigDirPath(),
            [this](int ch) { return m_stateStore->serialConfig(ch); },
            [this](int ch) { return m_stateStore->slaveId(ch, ch + 1); })) {
        emit experimentError(channel, tr("启动Modbus调度器失败"));
        return false;
    }
    return true;
}

void ExperimentCtrl::disconnectModbusDevice(int channel)
{
    Q_UNUSED(channel)
    m_commService->disconnectModbusDevice(m_schedulerInitialized, [this]() {
        for (auto it = m_runningFlags.constBegin(); it != m_runningFlags.constEnd(); ++it) {
            if (it.value()) {
                return true;
            }
        }
        return false;
    });
}

bool ExperimentCtrl::isModbusConnected(int channel) const
{
    return m_commService->isModbusConnected(channel, m_schedulerInitialized,
                                            [this](int ch) { return getDeviceId(ch); });
}

void ExperimentCtrl::saveSerialConfig(int channel)
{
    m_stateStore->saveSerialConfig(channel);
}

void ExperimentCtrl::loadSerialConfig(int channel)
{
    m_stateStore->loadSerialConfig(channel);
}

QVariantMap ExperimentCtrl::getChannelStatus(int channel) const
{
    // 给QML首帧渲染使用：返回当前缓存快照。
    return m_stateStore->channelStatus(channel);
}

void ExperimentCtrl::onScanTimer(int channel)
{
    // 扫描间隔定义为“本次扫描开始到下次扫描开始的时间”：
    // 到点后由上位机写地址0(40001)=1，触发下位机开始新一轮扫描。
    // 注意：这里不再清零全局计数，而是新增一条扫描上下文。
    const Channel ch = static_cast<Channel>(channel);
    if (!m_runningFlags.value(ch, false)) {
        return;
    }

    const int plannedScanCount = qMax(1, m_plannedScanCounts.value(ch, 1));
    const int startedScanCount = m_startedScanCounts.value(ch, 0);
    if (startedScanCount >= plannedScanCount) {
        m_scanTimers[ch]->stop();
        qDebug() << "[ExperimentCtrl][ScanTimer] channel=" << channel
                 << "planned scans reached, stop scheduling new scans"
                 << "started=" << startedScanCount
                 << "planned=" << plannedScanCount;
        return;
    }

    m_sessionService->beginScanCycle(channel, m_stateStore->params(channel));
    m_startedScanCounts[ch] = startedScanCount + 1;
    if (m_startedScanCounts.value(ch, 0) >= plannedScanCount) {
        m_scanTimers[ch]->stop();
    }
    sendControlCommand(channel, "start_scan", {{"value", 1}});
    qDebug() << "[ExperimentCtrl][ScanTimer] channel=" << channel
             << "started=" << m_startedScanCounts.value(ch, 0)
             << "planned=" << plannedScanCount
             << "begin new scan cycle, pendingContexts=" << m_sessionService->pendingContextCount(channel)
             << "trigger scan by write addr 0(40001)=1";
}

void ExperimentCtrl::onExperimentTimeout(int channel)
{
    // 总时长到达后不直接停机，先停止发新扫描并等待剩余数据排空，避免最后一轮被截断。
    const Channel ch = static_cast<Channel>(channel);
    if (!m_runningFlags.value(ch, false)) {
        return;
    }

    m_scanTimers[ch]->stop();
    m_stopAfterDrainFlags[ch] = true;
    m_stopAfterDrainDeadlineMs[ch] = 0;
    qDebug() << "[ExperimentCtrl][Timeout] channel=" << channel
             << "stop scheduling new scans and wait for drain"
             << "pendingContexts=" << m_sessionService->pendingContextCount(channel)
             << "deadlineMs=" << m_stopAfterDrainDeadlineMs.value(ch, 0);
}

void ExperimentCtrl::onSchedulerTaskCompleted(TaskResult res, QVector<quint16> data)
{
    // 调度器统一回调日志：便于定位“任务名-返回值-异常状态”。
    // qDebug() << "[ExperimentCtrl] task completed:" << res.deviceId << res.taskName
    //          << "remark:" << res.remark << "exception:" << res.isException
    //          << "data size:" << data.size();
}

void ExperimentCtrl::onStatusPollTimer()
{
    // 兼容入口：保留旧接口，内部改为逐通道执行。
    for (int i = 0; i < kChannelCount; ++i) {
        pollChannelStatus(i);
    }
}

void ExperimentCtrl::onChannelStatusPollTimer(int channel)
{
    pollChannelStatus(channel);
}

void ExperimentCtrl::pollChannelStatus(int channel)
{
    const Channel ch = static_cast<Channel>(channel);

    QVariantMap patch;
    patch["connected"] = isModbusConnected(channel);
    const bool connected = patch["connected"].toBool();

    if (!connected) {
        // 下位机未连接时不做读任务，避免每秒刷屏告警。
        qDebug() << "[ExperimentCtrl][Poll] channel=" << channel
                 << "skip read because connected=false";
        patch["running"] = false;
        patch["remainingSeconds"] = 0;
        QVariantMap mergedStatus;
        if (m_stateStore->updateChannelStatus(channel, patch, &mergedStatus)) {
            qDebug() << "[ExperimentCtrl][UI] channel=" << channel
                     << "running=" << mergedStatus.value("running").toBool()
                     << "hasSample=" << mergedStatus.value("hasSample").toBool()
                     << "isCovered=" << mergedStatus.value("isCovered").toBool()
                     << "temp=" << mergedStatus.value("currentTemperature").toDouble()
                     << "remain=" << mergedStatus.value("remainingSeconds").toInt()
                     << "A=" << mergedStatus.value("storageAState").toInt()
                     << "B=" << mergedStatus.value("storageBState").toInt();
            emit channelStatusUpdated(channel, mergedStatus);
        }
        return;
    }

    // 通过 JSON 任务名分项读取状态。
    QVariantMap status = readRealtimeStatus(channel);
    if (status.isEmpty()) {
        // 已连接但读不到数据，才打印告警（用于定位通讯异常）。
        qWarning() << "[ExperimentCtrl][Poll] channel=" << channel
                   << "connected but readRealtimeStatus empty";
        QVariantMap mergedStatus;
        if (m_stateStore->updateChannelStatus(channel, patch, &mergedStatus)) {
            qDebug() << "[ExperimentCtrl][UI] channel=" << channel
                     << "running=" << mergedStatus.value("running").toBool()
                     << "hasSample=" << mergedStatus.value("hasSample").toBool()
                     << "isCovered=" << mergedStatus.value("isCovered").toBool()
                     << "temp=" << mergedStatus.value("currentTemperature").toDouble()
                     << "remain=" << mergedStatus.value("remainingSeconds").toInt()
                     << "A=" << mergedStatus.value("storageAState").toInt()
                     << "B=" << mergedStatus.value("storageBState").toInt();
            emit channelStatusUpdated(channel, mergedStatus);
        }
        return;
    }

    patch.unite(status);

    // 按最新协议：地址1（runStatus）反映设备运行态，1=回零、2=扫描。
    const int runStatus = status.value("runStatus", 0).toInt();
    const bool runningByDevice = (runStatus == 1 || runStatus == 2);
    const bool runningByHost = m_runningFlags.value(ch, false);
//    qDebug() << "[ExperimentCtrl][Poll] channel=" << channel
//             << "startFlag=" << status.value("startFlag").toInt()
//             << "sample=" << status.value("sampleStatus").toInt()
//             << "temp=" << status.value("currentTemperature").toDouble()
//             << "A=" << status.value("storageAState").toInt()
//             << "B=" << status.value("storageBState").toInt()
//             << "runningByDevice=" << runningByDevice
//             << "runningByHost=" << runningByHost;

    if (runningByHost) {
        // 主机侧实验进行中：实时计算剩余时间并更新首页。
        const ExperimentParams params = m_stateStore->params(channel);
        const int total = m_sessionService->calculateTotalSeconds(
                    params.durationDays, params.durationHours, params.durationMinutes, params.durationSeconds);
        const int elapsed = static_cast<int>(getElapsedTime(channel));
        const int remaining = qMax(0, total - elapsed);
        patch["experiment_id"] = m_experimentIds.value(ch, 0);
        patch["remainingSeconds"] = remaining;
        patch["running"] = true;

        // 实验过程中：依据采集/存储状态决定是否读取A/B区数据。
        tryFetchStoredData(channel,
                           status.value("storageAReadableCount", 0).toInt(),
                           status.value("storageBReadableCount", 0).toInt(),
                           status.value("storageAState", 0).toInt(),
                           status.value("storageBState", 0).toInt());

        if (remaining <= 0) {
            m_stopAfterDrainFlags[ch] = true;
        }

        const int storageAReadableCount = status.value("storageAReadableCount", 0).toInt();
        const int storageBReadableCount = status.value("storageBReadableCount", 0).toInt();
        const bool hasReadableData = (storageAReadableCount > 0) || (storageBReadableCount > 0);
        if (m_stopAfterDrainFlags.value(ch, false)
                && m_stopAfterDrainDeadlineMs.value(ch, 0) <= 0
                && !runningByDevice) {
            m_stopAfterDrainDeadlineMs[ch] = QDateTime::currentMSecsSinceEpoch() + kDrainStopGraceMs;
            qDebug() << "[ExperimentCtrl][StopAfterDrain] channel=" << channel
                     << "device idle, start drain deadline"
                     << "pendingContexts=" << m_sessionService->pendingContextCount(channel)
                     << "hasReadableData=" << hasReadableData
                     << "deadlineMs=" << m_stopAfterDrainDeadlineMs.value(ch, 0);
        }
        const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        const bool drainDeadlineReached = m_stopAfterDrainDeadlineMs.value(ch, 0) > 0
                && nowMs >= m_stopAfterDrainDeadlineMs.value(ch, 0);
        const int pendingContexts = m_sessionService->pendingContextCount(channel);
        if (m_stopAfterDrainFlags.value(ch, false)
                && ((pendingContexts <= 0 && !hasReadableData) || drainDeadlineReached)) {
            qDebug() << "[ExperimentCtrl][StopAfterDrain] channel=" << channel
                     << "hasReadableData=" << hasReadableData
                     << "drainDeadlineReached=" << drainDeadlineReached
                     << "pendingContexts=" << pendingContexts;
            stopExperiment(channel);
            return;
        }
    } else {
        // 实验未开始时：仅刷新基础状态（样品、运行、温度等）。
        const int currentExperimentId = m_experimentIds.value(ch, 0);
        patch["experiment_id"] = currentExperimentId;
        if (currentExperimentId <= 0) {
            patch["running"] = false;
            patch["remainingSeconds"] = 0;
        } else {
            patch["running"] = runningByDevice;
            if (!runningByDevice) {
                patch["remainingSeconds"] = 0;
            }
        }
    }

    QVariantMap mergedStatus;
    if (m_stateStore->updateChannelStatus(channel, patch, &mergedStatus)) {
        qDebug() << "[ExperimentCtrl][UI] channel=" << channel
                 << "running=" << mergedStatus.value("running").toBool()
                 << "hasSample=" << mergedStatus.value("hasSample").toBool()
                 << "isCovered=" << mergedStatus.value("isCovered").toBool()
                 << "temp=" << mergedStatus.value("currentTemperature").toDouble()
                 << "remain=" << mergedStatus.value("remainingSeconds").toInt()
                 << "A=" << mergedStatus.value("storageAState").toInt()
                 << "B=" << mergedStatus.value("storageBState").toInt();
        emit channelStatusUpdated(channel, mergedStatus);
    }
}
bool ExperimentCtrl::sendControlCommand(int channel, const QString& command, const QVariantMap& params)
{
    return m_commService->sendControlCommand(channel, command, params, m_schedulerInitialized,
                                             [this](int ch) { return getDeviceId(ch); });
}

QVariantMap ExperimentCtrl::readSensorData(int channel)
{
    return m_commService->readSensorData(channel, m_schedulerInitialized,
                                         [this](int ch) { return getDeviceId(ch); });
}

QVariantMap ExperimentCtrl::readRealtimeStatus(int channel)
{
    return m_commService->readRealtimeStatus(channel, m_schedulerInitialized,
                                             [this](int ch) { return getDeviceId(ch); });
}

void ExperimentCtrl::tryFetchStoredData(int channel, int storageAReadableCount, int storageBReadableCount,
                                        int storageAState, int storageBState)
{
    const int experimentId = m_experimentIds.value(static_cast<Channel>(channel), 0);
    m_dataService->tryFetchStoredData(
        channel,
        experimentId,
        storageAReadableCount,
        storageBReadableCount,
        storageAState,
        storageBState,
        m_stateStore->memoryCache(channel),
        [this](int targetChannel) { return getDeviceId(targetChannel); },
        [this](int targetChannel, const QVector<quint16>& raw, bool areaA) {
            return m_sessionService->buildRowsFromStorageData(targetChannel, raw, areaA);
        },
        [this](int targetChannel, const QString& command, const QVariantMap& params) {
            return sendControlCommand(targetChannel, command, params);
        },
        [this, channel]() { return m_sessionService->currentScanCount(channel); },
        [this](int targetChannel, int targetExperimentId, const QVector<QVariantMap>& rows) {
            if (rows.isEmpty()) {
                return;
            }

            QMap<int, bool> completedByScanId;
            for (const QVariantMap &row : rows) {
                const int scanId = row.value(QStringLiteral("scan_id"), -1).toInt();
                completedByScanId[scanId] = completedByScanId.value(scanId, false)
                        || row.value(QStringLiteral("scan_completed"), false).toBool();
            }

            QVector<QVariantMap> *cache = m_stateStore->memoryCache(targetChannel);
            if (!cache) {
                return;
            }

            for (auto it = completedByScanId.constBegin(); it != completedByScanId.constEnd(); ++it) {
                if (!it.value()) {
                    continue;
                }

                QVariantList fullScanRows;
                for (const QVariantMap &cachedRow : *cache) {
                    if (cachedRow.value(QStringLiteral("scan_id"), -1).toInt() == it.key()) {
                        fullScanRows.append(cachedRow);
                    }
                }

                if (fullScanRows.isEmpty()) {
                    continue;
                }

                emit scanDataChunkReady(targetChannel,
                                        targetExperimentId,
                                        it.key(),
                                        true,
                                        fullScanRows);

                const int completedScanId = it.key();
                cache->erase(std::remove_if(cache->begin(), cache->end(),
                                            [completedScanId](const QVariantMap &cachedRow) {
                    return cachedRow.value(QStringLiteral("scan_id"), -1).toInt() == completedScanId;
                }), cache->end());
            }
        });
}
