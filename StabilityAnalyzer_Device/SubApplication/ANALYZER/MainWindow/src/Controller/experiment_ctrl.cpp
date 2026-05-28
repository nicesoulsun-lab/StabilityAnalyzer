
#include "inc/Controller/experiment_ctrl.h"
#include "inc/Common/experiment_comm_service.h"
#include "inc/Common/experiment_data_service.h"
#include "inc/Common/experiment_session_service.h"
#include "inc/Common/experiment_state_store.h"
#include "../../../SqlOrm/inc/SqlOrmManager.h"
#include "../../../TaskScheduler/inc/modbustaskscheduler.h"
#include "deviceprofile.h"

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
constexpr qint64 kDrainStopGraceMs = 15000;

int configuredChannelCount()
{
    return deviceProfile().channelCount;
}

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
    const QString configDir = deviceProfile().resolvedConfigDir();
    if (QDir::isRelativePath(configDir)) {
        return QCoreApplication::applicationDirPath() + "/" + configDir;
    }

    return configDir;
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

    for (int i = 0; i < configuredChannelCount(); ++i) {
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
        serialCfg.portName = QStringLiteral("COM%1").arg(i + 1);
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

    for (int i = 0; i < configuredChannelCount(); ++i) {
        loadSerialConfig(i);
    }

    // 启动时对“历史持久化配置”做一次兜底修正：
    // 1) 如果 4 个通道里存在重复 slaveId，自动回退为 1/2/3/4（调度器要求唯一）。
    // 2) Linux 下若仍是 Windows 风格串口名（COMx），自动改成 ttyUSB0，避免端口不存在。
    QSet<int> usedSlaveIds;
    bool serialPatched = false;
    for (int i = 0; i < configuredChannelCount(); ++i) {
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
        for (int i = 0; i < configuredChannelCount(); ++i) {
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
    for (int i = 0; i < configuredChannelCount(); ++i) {
        const Channel channel = static_cast<Channel>(i);
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

    for (int i = 0; i < configuredChannelCount(); ++i) {
        const Channel channel = static_cast<Channel>(i);
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
    for (int i = 0; i < configuredChannelCount(); ++i) {
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

int ExperimentCtrl::channelCount() const
{
    return configuredChannelCount();
}

QString ExperimentCtrl::channelName(int channel) const
{
    if (channel < 0 || channel >= configuredChannelCount()) {
        return QString();
    }

    return deviceProfile().channelName(channel);
}

QString ExperimentCtrl::channelDisplayName(int channel) const
{
    const QString name = channelName(channel);
    return name.isEmpty() ? QString() : tr("%1通道").arg(name);
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

    // 校准扫描进行中时禁止启动实验
    if (m_calibrationModes.value(ch, false)) {
        emit operationFailed(tr("校准扫描进行中，请等待完成"));
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
    m_sessionService->loadCalibrationAvgTable(channel, m_dbManager);
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
    m_experimentIds[ch] = 0;
    m_startTimes[ch] = 0;
    QVariantMap mergedStatus;
    if (m_stateStore->updateChannelStatus(channel, {
        {"running", false},
        {"experiment_id", 0},
        {"remainingSeconds", 0}
    }, &mergedStatus)) {
        emit channelStatusUpdated(channel, mergedStatus);
    }

    emit experimentStopped(channel, experimentId);
    emit operationInfo(tr("实验已结束，请取出样品"));
    return true;
}

bool ExperimentCtrl::isExperimentRunning(int channel) const
{
    return m_runningFlags.value(static_cast<Channel>(channel), false);
}

bool ExperimentCtrl::requestStopExperiment(int channel)
{
    const Channel ch = static_cast<Channel>(channel);

    if (!m_runningFlags.value(ch, false)) {
        return false;
    }

    if (m_stopAfterDrainFlags.value(ch, false)) {
        emit operationInfo(tr("实验正在停止中，等待当前扫描完成"));
        return true;
    }

    const QVariantMap status = m_stateStore->channelStatus(channel);
    const int runStatus = status.value("runStatus", 0).toInt();

    if (runStatus == 0 || runStatus == 3) {
        return stopExperiment(channel);
    }

    m_scanTimers[ch]->stop();
    m_experimentTimers[ch]->stop();
    m_stopAfterDrainFlags[ch] = true;
    m_stopAfterDrainDeadlineMs[ch] = 0;

    emit experimentStopRequested(channel);
    emit operationInfo(tr("正在结束实验"));

    qDebug() << "[ExperimentCtrl][RequestStop] channel=" << channel
             << "runStatus=" << runStatus
             << "waiting for scan to complete";
    return true;
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
    return m_stateStore->channelStatus(channel);
}

void ExperimentCtrl::startCalibration(int channel, const QString& calibrationType)
{
    // 背射光和透射光校准扫描范围固定为 0~55mm，步长固定20μm，三次扫描取平均
    const Channel ch = static_cast<Channel>(channel);

    // 参数校验
    if (channel < 0 || channel >= configuredChannelCount()) {
        emit calibrationFailed(channel, tr("无效通道"));
        return;
    }
    if (calibrationType != QStringLiteral("transmission")
        && calibrationType != QStringLiteral("backscatter")) {
        emit calibrationFailed(channel, tr("无效校准类型"));
        return;
    }
    // 实验中通道禁止校准
    if (m_runningFlags.value(ch, false)) {
        emit calibrationFailed(channel, tr("该通道正在实验中，无法校准"));
        return;
    }
    // 防重复校准
    if (m_calibrationModes.value(ch, false)) {
        emit calibrationFailed(channel, tr("该通道已在校准中"));
        return;
    }
    if (!m_schedulerInitialized) {
        const QString configPath = defaultConfigDirPath();
        if (!initializeScheduler(configPath)) {
            emit calibrationFailed(channel, tr("通信模块未初始化"));
            return;
        }
    }

    // 初始化三次扫描状态，扫描范围固定 0~55mm，步长固定20μm
    CalibrationScanState state;
    state.scanRound = 0;
    state.totalRounds = 3;
    state.calibrationType = calibrationType;
    m_calibrationScanStates[ch] = state;
    m_calibrationModes[ch] = true;

    // 触发第一轮扫描
    triggerNextCalibrationScan(channel);

    qDebug() << "[ExperimentCtrl][Calibration] started, channel=" << channel
             << "type=" << calibrationType
             << "range=0~55 step=20 fixed, totalRounds=3";
}

void ExperimentCtrl::triggerNextCalibrationScan(int channel)
{
    const Channel ch = static_cast<Channel>(channel);
    CalibrationScanState& state = m_calibrationScanStates[ch];

    state.scanRound++;
    if (state.scanRound > state.totalRounds) {
        return;
    }

    // 下发扫描参数并启动扫描，步长固定20μm
    sendControlCommand(channel, "set_scan_range",
                       {{"start", static_cast<int>(state.scanRangeStartMm)},
                        {"end", static_cast<int>(state.scanEndMm)}});
    sendControlCommand(channel, "set_step", {{"step", 20}});
    sendControlCommand(channel, "start_scan", {{"value", 1}});

    emit calibrationProgress(channel, state.scanRound, state.totalRounds);

    qDebug() << "[ExperimentCtrl][Calibration] round" << state.scanRound << "/" << state.totalRounds
             << "triggered, channel=" << channel;
}

void ExperimentCtrl::tryFetchCalibrationData(int channel, int storageAReadableCount,
                                              int storageBReadableCount,
                                              int storageAState, int storageBState)
{
    const Channel ch = static_cast<Channel>(channel);
    const CalibrationScanState& state = m_calibrationScanStates.value(ch);

    QVector<QVariantMap> batchRows;

    // 从下位机 A/B 存储区读取校准原始数据
    m_dataService->tryFetchCalibrationData(
        channel,
        storageAReadableCount, storageBReadableCount,
        storageAState, storageBState,
        [this](int targetChannel) { return getDeviceId(targetChannel); },
        [this, state](int targetChannel, const QVector<quint16>& raw, bool areaA) {
            return m_sessionService->buildCalibrationRows(
                targetChannel, raw, areaA, state.scanRangeStartMm, state.scanStepUm);
        },
        [this](int targetChannel, const QString& command, const QVariantMap& params) {
            return sendControlCommand(targetChannel, command, params);
        },
        [&batchRows](int targetChannel, const QVector<QVariantMap>& rows) {
            Q_UNUSED(targetChannel)
            batchRows += rows;
        });

    // 按轮次累积数据，用于后续三次取平均
    if (!batchRows.isEmpty()) {
        CalibrationScanState& mutableState = m_calibrationScanStates[ch];
        if (mutableState.scanRound > 0 && mutableState.scanRound <= mutableState.totalRounds) {
            const int roundIndex = mutableState.scanRound - 1;
            if (roundIndex >= mutableState.scanRoundRows.size()) {
                mutableState.scanRoundRows.resize(roundIndex + 1);
            }
            mutableState.scanRoundRows[roundIndex] += batchRows;
        }
    }

    qDebug() << "[ExperimentCtrl][Calibration] data fetched, channel=" << channel
             << "batchRows=" << batchRows.size();
}

void ExperimentCtrl::computeCalibrationAverage(int channel)
{
    // 三次扫描完成后，按高度对齐求平均值，写入 calibration_avg_data 表
    const Channel ch = static_cast<Channel>(channel);
    const CalibrationScanState& state = m_calibrationScanStates.value(ch);
    const int rounds = state.scanRoundRows.size();

    if (rounds == 0) {
        emit calibrationFailed(channel, tr("无校准数据"));
        return;
    }

    const auto& baseRows = state.scanRoundRows[0];
    const int pointCount = baseRows.size();
    const QString calType = state.calibrationType;
    const bool isTransmission = (calType == QStringLiteral("transmission"));

    // 加载已有校准数据，合并另一类型的已有值
    // 用微米级整数做 key，避免浮点精度导致匹配失败
    const QVector<QVariantMap> existingRows = m_dbManager->getCalibrationAvgDataByChannel(channel);
    QMap<qint64, QVariantMap> existingByHeightUm;
    for (const QVariantMap& row : existingRows) {
        const qint64 heightUm = qRound64(row.value("height").toDouble() * 1000.0);
        existingByHeightUm[heightUm] = row;
    }

    QVector<QVariantMap> avgEntries;
    avgEntries.reserve(pointCount + existingRows.size());

    // 记录本次扫描覆盖的高度（微米级），用于后续补齐
    QSet<qint64> coveredHeightUm;
    coveredHeightUm.reserve(pointCount);

    double sumAllTrans = 0.0, sumAllBack = 0.0;
    double maxTrans = -1e9, minTrans = 1e9;
    double maxBack = -1e9, minBack = 1e9;

    for (int i = 0; i < pointCount; ++i) {
        const double height = baseRows[i]["height"].toDouble();
        const qint64 heightUm = qRound64(height * 1000.0);
        coveredHeightUm.insert(heightUm);
        double sumTrans = 0.0, sumBack = 0.0;
        int validCount = 0;

        for (int r = 0; r < rounds; ++r) {
            if (i < state.scanRoundRows[r].size()) {
                sumTrans += state.scanRoundRows[r][i]["transmission_intensity"].toDouble();
                sumBack += state.scanRoundRows[r][i]["backscatter_intensity"].toDouble();
                validCount++;
            }
        }

        if (validCount > 0) {
            const double avgTrans = sumTrans / validCount;
            const double avgBack = sumBack / validCount;

            QVariantMap entry;
            entry["channel"] = channel;
            entry["height"] = height;
            entry["scan_count"] = validCount;
            entry["created_at"] = QDateTime::currentDateTime().toString(Qt::ISODate);

            if (isTransmission) {
                // 透射光校准：存透射平均值，背射保留已有值
                entry["avg_transmission_intensity"] = avgTrans;
                entry["avg_backscatter_intensity"] = existingByHeightUm.value(heightUm).value("avg_backscatter_intensity", 0.0).toDouble();
                sumAllTrans += avgTrans;
                if (avgTrans > maxTrans) maxTrans = avgTrans;
                if (avgTrans < minTrans) minTrans = avgTrans;
            } else {
                // 背射光校准：存背射平均值，透射保留已有值
                entry["avg_backscatter_intensity"] = avgBack;
                entry["avg_transmission_intensity"] = existingByHeightUm.value(heightUm).value("avg_transmission_intensity", 0.0).toDouble();
                sumAllBack += avgBack;
                if (avgBack > maxBack) maxBack = avgBack;
                if (avgBack < minBack) minBack = avgBack;
            }

            avgEntries.append(entry);
        }
    }

    // 补齐本次扫描高度范围外的已有数据，防止另一类型校准数据丢失
    for (auto it = existingByHeightUm.constBegin(); it != existingByHeightUm.constEnd(); ++it) {
        if (!coveredHeightUm.contains(it.key())) {
            avgEntries.append(it.value());
        }
    }

    // 按高度升序排列，保证写入数据库的顺序一致
    std::sort(avgEntries.begin(), avgEntries.end(), [](const QVariantMap& a, const QVariantMap& b) {
        return a.value("height").toDouble() < b.value("height").toDouble();
    });

    m_dbManager->clearCalibrationAvgDataByChannel(channel);
    m_dbManager->batchAddCalibrationAvgData(avgEntries);

    CalibrationSummary summary;
    summary.channel = channel;
    summary.calibrationType = calType;
    summary.totalPoints = pointCount;
    summary.scanRounds = rounds;
    summary.overallAvgTransmission = pointCount > 0 ? sumAllTrans / pointCount : 0.0;
    summary.overallAvgBackscatter = pointCount > 0 ? sumAllBack / pointCount : 0.0;
    summary.maxTransmission = maxTrans;
    summary.minTransmission = minTrans;
    summary.maxBackscatter = maxBack;
    summary.minBackscatter = minBack;
    summary.calibratedAt = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));

    m_calibrationModes[ch] = false;
    m_calibrationScanStates.remove(ch);

    emit calibrationCompleted(channel, calibrationSummaryToVariantMap(summary));

    qDebug() << "[ExperimentCtrl][Calibration] completed, channel=" << channel
             << "type=" << calType
             << "points=" << pointCount << "rounds=" << rounds
             << "avgTrans=" << summary.overallAvgTransmission
             << "avgBack=" << summary.overallAvgBackscatter;
}

QVariantMap ExperimentCtrl::calibrationSummaryToVariantMap(const CalibrationSummary& summary) const
{
    QVariantMap m;
    m["channel"] = summary.channel;
    m["calibration_type"] = summary.calibrationType;
    m["total_points"] = summary.totalPoints;
    m["scan_rounds"] = summary.scanRounds;
    m["overall_avg_transmission"] = summary.overallAvgTransmission;
    m["overall_avg_backscatter"] = summary.overallAvgBackscatter;
    m["max_transmission"] = summary.maxTransmission;
    m["min_transmission"] = summary.minTransmission;
    m["max_backscatter"] = summary.maxBackscatter;
    m["min_backscatter"] = summary.minBackscatter;
    m["calibrated_at"] = summary.calibratedAt;
    return m;
}

QString ExperimentCtrl::getLastCalibrationTime(int channel) const
{
    const QVector<QVariantMap> rows = m_dbManager->getCalibrationAvgDataByChannel(channel);
    if (rows.isEmpty()) {
        return QString();
    }
    return rows.last().value("created_at").toString();
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
    for (int i = 0; i < configuredChannelCount(); ++i) {
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

    if (m_anyPollInProgress) {
        if (!m_pendingPollChannels.contains(channel)) {
            m_pendingPollChannels.enqueue(channel);
        }
        return;
    }
    m_anyPollInProgress = true;

    QVariantMap patch;
    patch["connected"] = isModbusConnected(channel);
    const bool connected = patch["connected"].toBool();

    if (!connected) {
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
        finishCurrentPoll();
        return;
    }

    QVariantMap status = readRealtimeStatus(channel);
    if (status.isEmpty()) {
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
        finishCurrentPoll();
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
        patch["stopping"] = m_stopAfterDrainFlags.value(ch, false);

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
            finishCurrentPoll();
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

    if (m_calibrationModes.value(ch, false)) {
        const int storageAReadableCount = status.value("storageAReadableCount", 0).toInt();
        const int storageBReadableCount = status.value("storageBReadableCount", 0).toInt();
        const int storageAState = status.value("storageAState", 0).toInt();
        const int storageBState = status.value("storageBState", 0).toInt();

        if (storageAReadableCount > 0 || storageBReadableCount > 0) {
            tryFetchCalibrationData(channel, storageAReadableCount, storageBReadableCount,
                                    storageAState, storageBState);
        }

        const CalibrationScanState& calState = m_calibrationScanStates.value(ch);
        const bool hasCurrentRoundData = (!calState.scanRoundRows.isEmpty()
                                          && !calState.scanRoundRows.last().isEmpty());
        const bool deviceIdle = (runStatus == 0);
        const bool noMoreData = (storageAReadableCount <= 0 && storageBReadableCount <= 0);

        if (hasCurrentRoundData && deviceIdle && noMoreData) {
            sendControlCommand(channel, "stop_scan", {{"value", 0}});

            // 本轮完成：若未达3次则触发下一轮，否则计算平均值
            if (calState.scanRound < calState.totalRounds) {
                triggerNextCalibrationScan(channel);
            } else {
                computeCalibrationAverage(channel);
            }
        }
    }

    QVariantMap mergedStatus;
    if (m_stateStore->updateChannelStatus(channel, patch, &mergedStatus)) {
//        qDebug() << "[ExperimentCtrl][UI] channel=" << channel
//                 << "running=" << mergedStatus.value("running").toBool()
//                 << "hasSample=" << mergedStatus.value("hasSample").toBool()
//                 << "isCovered=" << mergedStatus.value("isCovered").toBool()
//                 << "temp=" << mergedStatus.value("currentTemperature").toDouble()
//                 << "remain=" << mergedStatus.value("remainingSeconds").toInt()
//                 << "A=" << mergedStatus.value("storageAState").toInt()
//                 << "B=" << mergedStatus.value("storageBState").toInt();
        emit channelStatusUpdated(channel, mergedStatus);
    }
    finishCurrentPoll();
}
void ExperimentCtrl::finishCurrentPoll()
{
    m_anyPollInProgress = false;
    if (!m_pendingPollChannels.isEmpty()) {
        int nextCh = m_pendingPollChannels.dequeue();
        pollChannelStatus(nextCh);
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

            for (auto it = completedByScanId.constBegin(); it != completedByScanId.constEnd(); ++it) {
                if (it.value()) {
                    continue;
                }

                QVariantList partialRows;
                for (const QVariantMap &row : rows) {
                    if (row.value(QStringLiteral("scan_id"), -1).toInt() == it.key()) {
                        partialRows.append(row);
                    }
                }

                if (partialRows.isEmpty()) {
                    continue;
                }

                emit scanDataChunkReady(targetChannel,
                                        targetExperimentId,
                                        it.key(),
                                        false,
                                        partialRows);
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
