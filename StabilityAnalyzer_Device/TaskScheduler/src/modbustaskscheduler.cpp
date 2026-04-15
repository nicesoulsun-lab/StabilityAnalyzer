#include "modbustaskscheduler.h"
#include <QJsonDocument>
#include <QDebug>
#include <QTextCodec>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
/**
 * @brief ModbusTaskScheduler 构造函数
 * @param parent 父对象指针
 */
ModbusTaskScheduler::ModbusTaskScheduler(QObject *parent)
    : QObject(parent)
    , m_isRunning(false)
    , m_isPaused(false)
    , m_totalDevices(0)
    , m_connectedDevices(0)
    , m_lastUsedDeviceId("")
    , m_portManager(new PortManager(this))
    , m_statusUpdateTimer(new QTimer(this))
    , m_taskQueueManager(new TaskQueueManager(this))
{
    qRegisterMetaType<TaskResult>("TaskResult");
    qRegisterMetaType<QVector<quint16>>("QVector<quint16>");

    m_configFile = QCoreApplication::applicationDirPath() + "/config/";

    // 连接信号
    connect(m_portManager, &PortManager::deviceTaskCompleted,
            this, &ModbusTaskScheduler::onDeviceTaskCompleted);
    connect(m_portManager, &PortManager::portConnectionChanged,
            this, &ModbusTaskScheduler::onPortConnectionChanged);

    // 连接任务队列管理器信号
    connect(m_taskQueueManager, &TaskQueueManager::taskStarted,
            this, &ModbusTaskScheduler::taskStarted);
    connect(m_taskQueueManager, &TaskQueueManager::taskCompleted,
            this, &ModbusTaskScheduler::onTaskCompleted);
    connect(m_taskQueueManager, &TaskQueueManager::queueStatusChanged,
            this, &ModbusTaskScheduler::onQueueStatusChanged);

    // 设置状态更新定时器
    m_statusUpdateTimer->setInterval(1000); // 每秒更新一次
    connect(m_statusUpdateTimer, &QTimer::timeout,
            this, &ModbusTaskScheduler::updateConnectionStatistics);
}

ModbusTaskScheduler *ModbusTaskScheduler::instance()
{
    // C++11 局部静态变量初始化是线程安全的
    static ModbusTaskScheduler instance;
    return &instance;
}

/**
 * @brief 设置配置文件路径
 * @param configFile 配置文件路径
 */
void ModbusTaskScheduler::setConfigFile(const QString &configFile)
{
    if (m_configFile != configFile) {
        m_configFile = configFile;
        emit configFileChanged(configFile);
    }
}

/**
 * @brief 加载配置文件
 * @return 加载成功返回true，失败返回false
 */
bool ModbusTaskScheduler::loadConfiguration()
{
    if (m_configFile.isEmpty()) {
        emit errorOccurred("Configuration file path is empty");
        return false;
    }

    // 读取文件夹下的所有设备的配置文件
    QFileInfo configFileInfo(m_configFile);
    if (!configFileInfo.isDir()) {
        emit errorOccurred("Configuration path must be a directory: " + m_configFile);
        return false;
    }

    // 加载文件夹下的所有配置文件
    return loadConfigurationFromDirectory(m_configFile);
}

/**
 * @brief 从配置文件夹加载所有配置文件
 * @param configDirPath 配置文件夹路径
 * @return 加载成功返回true，失败返回false
 */
bool ModbusTaskScheduler::loadConfigurationFromDirectory(const QString &configDirPath)
{
    QDir configDir(configDirPath);
    if (!configDir.exists()) {
        emit errorOccurred("Configuration directory does not exist: " + configDirPath);
        return false;
    }

    // 获取所有JSON配置文件
    QStringList filters;
    filters << "*.json";
    QFileInfoList configFiles = configDir.entryInfoList(filters, QDir::Files);

    if (configFiles.isEmpty()) {
        emit errorOccurred("No configuration files found in: " + configDirPath);
        return false;
    }

    qDebug() << "Found" << configFiles.size() << "configuration files in" << configDirPath;

    bool allLoaded = true;
    for (const QFileInfo &fileInfo : configFiles) {
        QString filePath = fileInfo.absoluteFilePath();
        qDebug() << "Loading configuration file:" << filePath;

        if (!loadConfigurationFromFile(filePath)) {
            qWarning() << "Failed to load configuration file:" << filePath;
            allLoaded = false;
        }
    }

    return allLoaded;
}

/**
 * @brief 从单个配置文件加载配置
 * @param configFilePath 配置文件路径
 * @return 加载成功返回true，失败返回false
 */
bool ModbusTaskScheduler::loadConfigurationFromFile(const QString &configFilePath)
{
    QFile file(configFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred("Failed to open configuration file: " + configFilePath);
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    // 检测并转换编码，解决乱码的问题
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QString decodedData = codec->toUnicode(data);

    // 如果检测到乱码，尝试其他编码
    if (decodedData.contains(QChar(0xFFFD))) { // 0xFFFD 是 Unicode 替换字符
        // 尝试 GBK 编码（常见的中文编码）
        QTextCodec *gbkCodec = QTextCodec::codecForName("GBK");
        if (gbkCodec) {
            decodedData = gbkCodec->toUnicode(data);
        }
    }

    // 将字符串转换回 UTF-8 字节数组用于 JSON 解析
    QByteArray utf8Data = decodedData.toUtf8();
//    qDebug() << "读取到的配置文件内容：" << decodedData << "----" <<utf8Data;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(utf8Data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
//        emit errorOccurred("JSON parse error: " + parseError.errorString());
        qDebug()<<"Json parse error: " << parseError.errorString();
        return false;
    }

//    qDebug()<<"输出的doc是：" << doc;
    if (!doc.isObject()) {
//        emit errorOccurred("Configuration file is not a valid JSON object");
        qDebug()<<"Configuration file is not a valid JSON object";
        return false;
    }

    QJsonObject config = doc.object();
//    qDebug()<<"config："<<config;

    //判断下json文件是否正确，现在是判断了几个关键的键值
    if (!validateConfiguration(config)) {
//        emit errorOccurred("Configuration validation failed");
        qDebug()<<"Configuration validation failed";
        return false;
    }

    //解析配置文件数据
    return parseConfiguration(config);
}

/**
 * @brief 保存配置到文件
 * @param config 配置对象
 * @return 保存成功返回true，失败返回false
 */
bool ModbusTaskScheduler::saveConfiguration(const QJsonObject &config)
{
    if (m_configFile.isEmpty()) {
        emit errorOccurred("Configuration file path is empty");
        return false;
    }

    QFile file(m_configFile);
    if (!file.open(QIODevice::WriteOnly)) {
        emit errorOccurred("Failed to open configuration file for writing: " + m_configFile);
        return false;
    }

    QJsonDocument doc(config);
    file.write(doc.toJson());
    file.close();

    return true;
}

/**
 * @brief 启动任务调度器
 * @return 启动成功返回true，失败返回false
 */
bool ModbusTaskScheduler::startScheduler()
{
    if (m_isRunning) {
        qWarning() << "Scheduler is already running";
        return true;
    }

    if (m_configFile.isEmpty()) {
        emit errorOccurred("Configuration file not set");
        return false;
    }

    // 加载配置文件信息，创建对象
    if (m_deviceConfigMap.isEmpty()) {
        if (!loadConfiguration()) {
            emit errorOccurred("Failed to load configuration");
            return false;
        }
    }

    m_isRunning = true;
    m_isPaused = false;

    // 启动状态更新定时器
    m_statusUpdateTimer->start();

    // 启动任务队列管理器任务调度
    m_taskQueueManager->startScheduler();

    // 启动初始化的轮询任务调度，这个是启动所有的初始化轮询任务包括各个设备的
    startPollingTasks();

    emit runningStatusChanged(true);
    emit schedulerStarted();

    // 执行初始化任务
    executeInitTasks();

    return true;
}

/**
 * @brief 停止任务调度器
 * @return 停止成功返回true，失败返回false
 */
bool ModbusTaskScheduler::stopScheduler()
{
    if (!m_isRunning) {
        return true;
    }

    m_isRunning = false;
    m_isPaused = false;

    // 停止状态更新定时器
    m_statusUpdateTimer->stop();

    // 停止任务队列管理器
    m_taskQueueManager->stopScheduler();

    emit runningStatusChanged(false);
    emit schedulerStopped();

    return true;
}

/**
 * @brief 暂停任务调度器
 */
void ModbusTaskScheduler::pauseScheduler()
{
    if (!m_isRunning || m_isPaused) {
        return;
    }

    m_isPaused = true;

    // 暂停任务队列管理器
    if (m_taskQueueManager) {
        m_taskQueueManager->pauseScheduler();
    }

    // 暂停状态更新定时器
    if (m_statusUpdateTimer && m_statusUpdateTimer->isActive()) {
        m_statusUpdateTimer->stop();
    }

    qDebug() << "Task scheduler paused";
}

/**
 * @brief 恢复任务调度器
 */
void ModbusTaskScheduler::resumeScheduler()
{
    if (!m_isRunning || !m_isPaused) {
        return;
    }

    m_isPaused = false;

    // 恢复任务队列管理器
    if (m_taskQueueManager) {
        m_taskQueueManager->resumeScheduler();
    }

    // 重新启动状态更新定时器
    if (m_statusUpdateTimer && !m_statusUpdateTimer->isActive()) {
        m_statusUpdateTimer->start();
    }

    qDebug() << "Task scheduler resumed";
}

/**
 * @brief 执行初始化任务
 * 初始化任务已在 parseConfiguration -> initializeDeviceTasks 中完成入队，
 * 此处仅输出日志，避免重复入队。
 */
void ModbusTaskScheduler::executeInitTasks()
{
    if (!m_isRunning) {
        qWarning() << "Scheduler is not running";
        return;
    }

    // 初始化任务已在 parseConfiguration 中通过 initializeDeviceTasks 入队，
    // 此处不再重复入队，只记录日志。
    for (auto it = m_deviceConfigMap.begin(); it != m_deviceConfigMap.end(); ++it) {
        DeviceConfig *deviceConfig = it.value();
        QList<Task*> tasks = deviceConfig->tasks();
        for (Task *task : tasks) {
            if (task->taskType() == TaskType::INIT_TASK) {
                qDebug() << "Init task already queued via initializeDeviceTasks:"
                         << task->taskName() << "for device:" << it.key()
                         << "interval:" << task->interval() << "ms";
            }
        }
    }

    qDebug() << "executeInitTasks: init tasks are already in queue, no duplicate enqueue.";
}

//执行用户任务，执行用户任务的时候需要读取下串口信息，因为一个串口可能多个设备，每个设备串口信息不一样比如波特率不一样
void ModbusTaskScheduler::executeUserTask(const QString &slaveId, const QString &taskName
                                          , int isSync, QVector<quint16>& result, const QVector<quint16>& writeData
                                          , const QString& remark)
{
    if (!m_isRunning) {
        emit errorOccurred("Scheduler is not running");
        return;
    }

    result = QVector<quint16>(); //给个默认值
    // 查找对应slaveId的设备
    QString deviceId = slaveId;
    if (!m_deviceConfigMap.contains(deviceId)) {
        // 尝试查找包含该slaveId的设备
        for (auto it = m_deviceConfigMap.begin(); it != m_deviceConfigMap.end(); ++it) {
            if (it.key() == slaveId || it.value()->deviceId() == slaveId) {
                deviceId = it.key();
                break;
            }
        }
    }

    //获取对应的设备
    if (m_deviceConfigMap.contains(deviceId)) {
        DeviceConfig *deviceConfig = m_deviceConfigMap.value(deviceId);

        // 设备变了之后需要判断下这个设备的串口信息是否和之前的串口信息一致不一致的话需要重连
        if (deviceId != m_lastUsedDeviceId) {
            //qDebug() << "Device switched from" << m_lastUsedDeviceId << "to" << deviceId;

            // 获取当前设备的串口配置
            SerialConfig config = deviceConfig->serialConfig();

            // 检查串口配置是否发生变化
            if (isPortConfigChanged(config)) {
                qDebug() << "Port config changed, reconnecting - Port:" << "Device:" << deviceId;
                reconnectPortWithNewConfig(config);
            } else {
                // qDebug() << "Port config unchanged, Device:" << deviceId;
            }

            // 更新上一次使用的设备ID
            m_lastUsedDeviceId = deviceId;
        }

        //根据设备id和任务名称找到对应的任务执行
        Task *task = deviceConfig->findTask(taskName);

        if (task) {
            task->setIsSync(isSync); //赋值是否同步和写入值
            task->setWriteData(writeData);
            task->setTaskReamark(remark);

            //如果这个用户任务是轮询任务的话就放到低优先级队列，如果不是轮询任务就放到高优先级队列,轮询一般是异步
            //轮询任务放到了队列里面，每隔一个interval的时间间隔就需要如队列然后等待执行，然后轮询任务的开始和停止由外部调用者控制
            //如果需要停止这个轮询任务，不再往队列放请求了，就在外部调用暂停的方法，如果又要开始执行这个轮询任务了就再调用开始的方法
            if(task->interval()>0){
                m_taskQueueManager->addPollingTask(deviceId, task);
            }
            // 添加到高优先级队列（先进先出）
            else{
                m_taskQueueManager->addHighPriorityTask(deviceId, task);
            }
            //qDebug() << "User task added to queue:" << taskName
            //         << "for device (slaveId):" << slaveId;

            //如果是同步任务的话，等待任务执行完成
            if(isSync){
                // 等待任务完成
                const bool ok = waitForTaskCompletion(task, result, 5000);
                // qDebug() << "[Scheduler][ExecuteUserTask] sync task finished"
                //          << "device=" << deviceId
                //          << "task=" << taskName
                //          << "ok=" << ok
                //          << "resultSize=" << result.size();
            }
            //异步任务，直接返回空的qvector<quint16>()
            if (!isSync) {
                result = QVector<quint16>();
            }

        } else {
            emit errorOccurred("Task not found: " + taskName + " for device (slaveId): " + slaveId);
        }
    }else {
        emit errorOccurred("Device not found with slaveId: " + slaveId);

    }
}

//调度轮训任务
void ModbusTaskScheduler::schedulePollingTask(const QString &slaveId, const QString &taskName
                                          , int isSync, const QVector<quint16>& writeData)
{
    if (!m_isRunning) {
        emit errorOccurred("Scheduler is not running");
        return;
    }

    // 查找对应slaveId的设备
    QString deviceId = slaveId;
    if (!m_deviceConfigMap.contains(deviceId)) {
        // 尝试查找包含该slaveId的设备
        for (auto it = m_deviceConfigMap.begin(); it != m_deviceConfigMap.end(); ++it) {
            if (it.key() == slaveId || it.value()->deviceId() == slaveId) {
                deviceId = it.key();
                break;
            }
        }
    }

    if (m_deviceConfigMap.contains(deviceId)) {
        DeviceConfig *deviceConfig = m_deviceConfigMap.value(deviceId);
        Task *task = deviceConfig->findTask(taskName);

        if (task) {
            task->setIsSync(isSync);
            task->setWriteData(writeData);
            // 添加到轮询队列（低优先级队列）
            m_taskQueueManager->addPollingTask(deviceId, task);
        } else {
            emit errorOccurred("Task not found: " + taskName + " for device (slaveId): " + slaveId);
        }
    } else {
        emit errorOccurred("Device not found with slaveId: " + slaveId);
    }
}

QList<QString> ModbusTaskScheduler::getDeviceList() const
{
    QList<QString> deviceList;
    for (auto it = m_deviceConfigs.begin(); it != m_deviceConfigs.end(); ++it) {
        deviceList.append(it.key());
    }
    return deviceList;
}

QList<QString> ModbusTaskScheduler::getTaskList(const QString &deviceId) const
{
    QList<QString> taskList;

    if (!m_deviceConfigs.contains(deviceId)) {
        return taskList;
    }

    QJsonObject deviceConfig = m_deviceConfigs.value(deviceId);
    QJsonArray tasksArray = deviceConfig.value("tasks").toArray();

    for (const QJsonValue &taskValue : tasksArray) {
        QJsonObject taskConfig = taskValue.toObject();
        taskList.append(taskConfig.value("taskId").toString());
    }

    return taskList;
}

Device* ModbusTaskScheduler::getDevice(const QString &deviceId) const
{
    return m_portManager->findDevice(deviceId);
}

QString ModbusTaskScheduler::getDeviceStatus(const QString &deviceId) const
{
    Device *device = getDevice(deviceId);
    if (!device) {
        return "Not Found";
    }

    return device->isConnected() ? "Connected" : "Disconnected";
}

QString ModbusTaskScheduler::getTaskStatus(const QString &deviceId, const QString &taskName) const
{
    Device *device = getDevice(deviceId);
    if (!device) {
        return "Device Not Found";
    }

    Task *task = device->findTask(taskName);
    if (!task) {
        return "Task Not Found";
    }

    switch (task->status()) {
    case TaskStatus::PENDING: return "Pending";
    case TaskStatus::RUNNING: return "Running";
    case TaskStatus::COMPLETED: return "Completed";
    case TaskStatus::FAILED: return "Failed";
    default: return "Unknown";
    }
}

int ModbusTaskScheduler::getTaskInterval(const QString &deviceId, const QString &taskName) const
{
    if (m_deviceConfigMap.contains(deviceId)) {
        DeviceConfig *deviceConfig = m_deviceConfigMap.value(deviceId);
        Task *task = deviceConfig->findTask(taskName);
        if (task) {
            return task->interval();
        }
    }
    return -1; // 返回-1表示任务未找到或没有设置间隔
}

void ModbusTaskScheduler::onConfigurationChanged(const QString &newConfigFile)
{
    // 设置新的配置文件路径
    setConfigFile(newConfigFile);

    if (m_isRunning) {
        // 动态加载新的配置，支持多设备同时运行
        // 不停止调度器，直接加载新的设备配置
        if (!loadConfiguration()) {
            emit errorOccurred("Failed to load new configuration");
            return;
        }

        qDebug() << "New configuration loaded successfully while scheduler is running";
    }
}

//设备任务完成信号
void ModbusTaskScheduler::onDeviceTaskCompleted(TaskResult res, QVector<quint16>data)
{
    emit taskCompleted(res, data);
}

//端口连接改变
void ModbusTaskScheduler::onPortConnectionChanged(const QString &portName, bool connected)
{
    qDebug() << "Port" << portName << (connected ? "connected" : "disconnected");
    
    // 更新连接状态
    m_portConnectionStates[portName] = connected;
    
    // 根据连接状态控制任务调度
    if (connected) {
        // 串口连接成功，恢复任务调度
        resumeTaskSchedulingForPort(portName);
    } else {
        // 串口断开连接，暂停任务调度
        pauseTaskSchedulingForPort(portName);
    }
    
    updateConnectionStatistics();
}

//更新连接统计
void ModbusTaskScheduler::updateConnectionStatistics()
{
    int connectedCount = 0;
    int totalCount = m_deviceConfigMap.size();

    // 计算已连接的设备数量
    for (auto it = m_deviceConfigMap.begin(); it != m_deviceConfigMap.end(); ++it) {
        Device *device = getDevice(it.key());
        const QString portName = getPortNameForDevice(it.key());
        const bool deviceConnected = (device && device->isConnected());
        //qDebug() << "[Scheduler][ConnStats] deviceId=" << it.key()
        //         << "port=" << portName
        //         << "devicePtr=" << device
        //         << "deviceConnected=" << deviceConnected;
        if (device && device->isConnected()) {
            connectedCount++;
        }
    }

    qDebug() << "[Scheduler][ConnStats] summary total=" << totalCount
             << "connected=" << connectedCount
             << "previousConnected=" << m_connectedDevices;

    if (m_totalDevices != totalCount) {
        m_totalDevices = totalCount;
        emit deviceCountChanged(totalCount);
    }

    if (m_connectedDevices != connectedCount) {
        m_connectedDevices = connectedCount;
        emit connectionStatusChanged(connectedCount);
    }
}

//解析配置文件，传递的参数是整个配置文件信息
bool ModbusTaskScheduler::parseConfiguration(const QJsonObject &config)
{
    // 检查配置结构
    QJsonObject deviceConfig = config.value("device").toObject();
    if (deviceConfig.isEmpty()) {
//        emit errorOccurred("Invalid configuration structure: missing 'device' field");
        return false;
    }

    // 获取设备ID（使用slaveId作为设备标识）
    int slaveId = deviceConfig.value("slaveId").toInt();
    QString deviceId = QString::number(slaveId);
    if (deviceId.isEmpty()) {
//        emit errorOccurred("deviceId is empty");
        return false;
    }

    // 检查是否已存在相同slaveId的设备
    if (m_deviceConfigMap.contains(deviceId)) {
        qWarning() << "Device with slaveId" << deviceId << "already exists, skipping...";
        return false; // 跳过错误的重复设备
    }

    // 创建新的设备配置，每个设备信息是一个deviceconfig，然后使用设备id作为键区分
    //每个设备对应一个串口，一个串口下可能挂载多个设备，在创建每个设备类的时候传递串口对象进去
    DeviceConfig *newDeviceConfig = new DeviceConfig(m_portManager);
    if (newDeviceConfig->loadFromJson(deviceConfig)) { //传递全部json数据，解析json数据全部在deviceconfig类执行，外部调用
        m_deviceConfigs.insert(deviceId, deviceConfig);
        m_deviceConfigMap.insert(deviceId, newDeviceConfig); //设备id-设备信息存到字典中
        m_deviceToPortMap.insert(deviceId, newDeviceConfig->serialConfig().portName);
        updatePortConfigCache(newDeviceConfig->serialConfig());

        qDebug() << "[Scheduler][Parse] deviceId=" << deviceId
                 << "port=" << newDeviceConfig->serialConfig().portName
                 << "taskCount=" << newDeviceConfig->tasks().size();

        // 为这个设备创建串口连接
//        initializePortConnection(deviceConfig, deviceId);
        initializePortConnection(newDeviceConfig->serialConfig(), deviceId);

        // 从Device对象获取任务（这些任务已经设置了设备）
        Device *device = m_portManager->findDevice(deviceId);
        if (device) {
            // 将DeviceConfig中的任务添加到Device中，确保使用的任务都是一个，使用的都是deviceconfig的任务对象
            device->addTasks(newDeviceConfig->tasks());

            // 初始化设备轮询任务（使用已经设置设备的任务），用户触发任务不执行
            m_taskQueueManager->initializeDeviceTasks(deviceId, device->tasks());
        } else {
            qWarning() << "Device not found for deviceId:" << deviceId << "- cannot initialize tasks";
        }

        qDebug() << "Device configuration loaded successfully - Device ID:" << deviceId
                 << "Device Name:" << newDeviceConfig->deviceName();
    } else {
        emit errorOccurred("Failed to load device configuration for device: " + deviceId);
        newDeviceConfig->deleteLater();
        return false;
    }

    return true;
}

//校验配置文件，判断配置文件的有效性
bool ModbusTaskScheduler::validateConfiguration(const QJsonObject &config) const
{
//    qDebug() << "VALIDATE CONFIG KEYS:" << config.keys();

    // 基本验证
    if (!config.contains("device") || !config.value("device").isObject()) {
        return false;
    }
    QJsonObject device = config.value("device").toObject();
//    qDebug() << "VALIDATE CONFIG KEYS:" << device.keys();

    if(!device.contains("serialConfig")){
        return false;
    }
    if (!device.contains("taskList")) {
        return false;
    }

    return true;
}

//初始化串口连接
void ModbusTaskScheduler::initializePortConnection(const SerialConfig &deviceConfig, const QString& deviceId)
{
    // 获取串口配置
    QString portName = deviceConfig.portName;
    if (portName.isEmpty()) {
        qWarning() << "No port name specified for device:" << deviceId;
        return;
    }

    // 添加串口到串口字典(串口名称-串口信息),串口下的多个设备多个串口配置，先默认使用读取到的第一个设备的串口配置，后续在执行任务的时候修改重连
    if (!m_portManager->addPort(deviceConfig)) {
        qWarning() << "Failed to add port:" << portName << "for device:" << deviceId;
        return;
    }
    
    // 初始化串口连接状态和任务调度状态
    if (!m_portConnectionStates.contains(portName)) {
        m_portConnectionStates[portName] = false; // 初始状态为未连接
        m_portTaskSchedulingStates[portName] = false; // 初始状态为任务调度暂停
    }

    // 创建设备，并把这个设备添加到串口管理类
    Device *device = new Device(deviceId, this);
    if (!m_portManager->addDeviceToPort(portName, device)) {
        qWarning() << "Failed to add device to port:" << portName << "device:" << deviceId;
        device->deleteLater();
        return;
    }

    // 如果端口尚未连接，则连接串口,创建modbusclent客户端
    if (!m_portManager->isPortConnected(portName)) {
        if (!m_portManager->connectPort(portName)) {
            qWarning() << "Failed to connect port:" << portName << "for device:" << deviceId;
            return;
        }
    } else {
        qDebug() << "Port already connected, device added successfully - Port:" << portName
                 << "Device:" << deviceId;
        
        // 如果端口已经连接，确保任务调度状态正确
        if (m_portConnectionStates.contains(portName) && !m_portTaskSchedulingStates[portName]) {
            resumeTaskSchedulingForPort(portName);
        }
    }

    qDebug() << "Port connection initialized successfully - Port:" << portName
             << "Device:" << deviceId;
    qDebug() << "[Scheduler][InitPort] deviceId=" << deviceId
             << "mappedPort=" << m_deviceToPortMap.value(deviceId)
             << "portConnected=" << m_portManager->isPortConnected(portName);
}

//等待同步任务执行完成
bool ModbusTaskScheduler::waitForTaskCompletion(Task* task, QVector<quint16>& result, int timeoutMs)
{
    if (!task) {
        return false;
    }
    
    QEventLoop eventLoop;
    QTimer timeoutTimer;
    bool completed = false;
    bool success = false;
    QVector<quint16> completedData;
    
    // 设置超时时间
    timeoutTimer.setSingleShot(true);
    timeoutTimer.start(timeoutMs);
    
    // 连接任务完成信号
    QObject::connect(task, &Task::taskCompleted, &eventLoop,
                     [&](const TaskResult &taskRes, const QVector<quint16> &data) {
        completed = true;
        success = !taskRes.isException;
        completedData = data;
        // qDebug() << "[Scheduler][WaitTask] task=" << task->taskName()
        //          << "device=" << task->deviceId()
        //          << "completed=" << completed
        //          << "success=" << success
        //          << "dataSize=" << data.size();
        eventLoop.quit();
    });
    
    // 连接超时信号
    QObject::connect(&timeoutTimer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    
    // 阻塞并等待任务完成或超时
    eventLoop.exec();
    
    if (timeoutTimer.isActive()) {
        // 任务正常完成
        timeoutTimer.stop();
        // qDebug() << "Sync task finished:" << task->taskName()
        //          << "completed=" << completed
        //          << "success=" << success
        //          << "dataSize=" << completedData.size();
        
        // 获取任务结果
        result = completedData;
        return completed && success;
    } else {
        // 任务超时
        qWarning() << "Sync task timeout:" << task->taskName();
        
        // 取消任务
        task->cancel();
        return false;
    }
}

/**
 * @brief 处理任务开始信号
 * @param deviceId 设备ID
 * @param taskName 任务名称
 */
void ModbusTaskScheduler::onTaskStarted(const QString &deviceId, const QString &taskName)
{
    qDebug() << "Task started - Device:" << deviceId << "Task:" << taskName;

    // 这里可以添加任务开始时的处理逻辑，比如更新UI状态、记录日志什么的
}

/**
 * @brief 处理任务完成信号
 * @param deviceId 设备ID
 * @param taskName 任务名称
 * @param success 执行是否成功
 * @param result 执行结果
 * TaskQueueManager传递过来的信号
 */
void ModbusTaskScheduler::onTaskCompleted(TaskResult res, QVector<quint16>data)
{
    //把通信的数据使用信号的形式发送出去
    emit taskCompleted(res, data);
}

/**
 * @brief 处理队列状态变化信号
 * @param highPriorityCount 高优先级队列任务数
 * @param pollingCount 轮询队列任务数
 */
void ModbusTaskScheduler::onQueueStatusChanged(int highPriorityCount, int pollingCount)
{
    // 这里可以添加队列状态变化的处理逻辑
    // 例如：更新UI显示、调整调度策略等
}

/**
 * @brief 启动所有轮询任务调度
 * 这个是只执行初始化的轮询任务
 * 如果是用户触发的这种，根据判断interval这个字段是否大于0来判断是否是轮询任务
 * 如果是轮询任务就调用开始轮询
 */
void ModbusTaskScheduler::startPollingTasks()
{
    if (!m_isRunning) {
        qWarning() << "Cannot start polling tasks: scheduler is not running";
        return;
    }

    // 遍历所有设备的所有任务，为每个轮询任务创建独立的定时器
    for (auto it = m_deviceConfigMap.begin(); it != m_deviceConfigMap.end(); ++it) {
        const QString &deviceId = it.key();
        DeviceConfig *deviceConfig = it.value();

        // 获取设备的所有任务
        QList<Task*> tasks = deviceConfig->tasks();
        for (Task *task : tasks) {
            // 通过interval()>0并且是初始化任务判断是否为初始化轮询任务
            if (task->taskType() == TaskType::INIT_TASK && task->interval() > 0) {
                QString taskId = deviceId + "_" + task->taskName();

                // 如果定时器已存在，检查是否需要重新启动
                if (m_pollingTimers.contains(taskId)) {
                    QTimer *existingTimer = m_pollingTimers[taskId];
                    if (!existingTimer->isActive()) {
                        existingTimer->start();
                        qDebug() << "Resumed polling task timer - Device:" << deviceId
                                 << "Task:" << task->taskName() << "Interval:" << task->interval() << "ms";
                    }
                } else {
                    // 创建新的轮询任务定时器
                    QTimer *pollingTimer = new QTimer(this);
                    pollingTimer->setInterval(task->interval());

                    // 连接定时器信号
                    connect(pollingTimer, &QTimer::timeout, this, [this, deviceId, task]() {
                        // 调度轮询任务到低优先级队列
                        schedulePollingTask(deviceId, task->taskName(), 0, QVector<quint16>());
                        qDebug() << "Polling timer triggered - Device:" << deviceId
                                 << "Task:" << task->taskName() << "Interval:" << task->interval() << "ms";
                    });

                    // 启动定时器
                    pollingTimer->start();
                    m_pollingTimers[taskId] = pollingTimer;

                    qDebug() << "Started polling task timer - Device:" << deviceId
                             << "Task:" << task->taskName() << "Interval:" << task->interval() << "ms";
                }
            }
        }
    }
}

/**
 * @brief 停止所有轮询任务调度
 */
void ModbusTaskScheduler::stopPollingTasks()
{
    // 停止所有轮询任务定时器，但不销毁
    for (auto it = m_pollingTimers.begin(); it != m_pollingTimers.end(); ++it) {
        QTimer *timer = it.value();
        if (timer->isActive()) {
            timer->stop();
        }
    }
    qDebug() << "Stopped all polling task timers";
}

/**
 * @brief 暂停指定的轮询任务
 * @param deviceId 设备ID
 * @param taskName 任务名称
 */
void ModbusTaskScheduler::pausePollingTask(const QString &deviceId, const QString &taskName)
{
    QString taskId = deviceId + "_" + taskName;

    if (m_pollingTimers.contains(taskId)) {
        QTimer *timer = m_pollingTimers[taskId];
        if (timer->isActive()) {
            timer->stop();
            qDebug() << "Paused polling task - Device:" << deviceId << "Task:" << taskName;
        } else {
            qDebug() << "Polling task already paused - Device:" << deviceId << "Task:" << taskName;
        }
    } else {
        qWarning() << "Polling task not found - Device:" << deviceId << "Task:" << taskName;
    }
}

/**
 * @brief 恢复指定的轮询任务
 * @param deviceId 设备ID
 * @param taskName 任务名称
 */
void ModbusTaskScheduler::resumePollingTask(const QString &deviceId, const QString &taskName)
{
    QString taskId = deviceId + "_" + taskName;

    if (m_pollingTimers.contains(taskId)) {
        QTimer *timer = m_pollingTimers[taskId];
        if (!timer->isActive()) {
            timer->start();
            qDebug() << "Resumed polling task - Device:" << deviceId << "Task:" << taskName;
        } else {
            qDebug() << "Polling task already running - Device:" << deviceId << "Task:" << taskName;
        }
    } else {
        //如果开启任务的时候发现定时器不存在就新建这个任务的定时器
        DeviceConfig *deviceConfig = nullptr;
        if (m_deviceConfigMap.contains(deviceId)) {
            deviceConfig = m_deviceConfigMap.value(deviceId);
        }
        if (!deviceConfig) {
            qWarning() << "resumePollingTask: deviceConfig not found for deviceId:" << deviceId;
            return;
        }
        Task* task = deviceConfig->findTask(taskName); //找到对应的任务
        if(!task){
            qDebug()<<"task is invalid";
            return;
        }
        QTimer *pollingTimer = new QTimer(this);
        pollingTimer->setInterval(task->interval());

        // 连接定时器信号
        connect(pollingTimer, &QTimer::timeout, this, [this, deviceId, task]() {
            // 调度轮询任务到低优先级队列
            schedulePollingTask(deviceId, task->taskName(), 0, QVector<quint16>());
            qDebug() << "Polling timer triggered - Device:" << deviceId
                     << "Task:" << task->taskName() << "Interval:" << task->interval() << "ms";
        });

        // 启动定时器
        pollingTimer->start();
        m_pollingTimers[taskId] = pollingTimer;

        qDebug() << "Started polling task timer - Device:" << deviceId
                 << "Task:" << task->taskName() << "Interval:" << task->interval() << "ms";
    }
}

/**
 * @brief 轮询任务定时器触发时的处理
 */
void ModbusTaskScheduler::onPollingTimer()
{
    if (!m_isRunning) {
        return;
    }

    // 遍历所有设备，调度轮询任务
    for (auto it = m_deviceConfigMap.begin(); it != m_deviceConfigMap.end(); ++it) {
        const QString &deviceId = it.key();
        DeviceConfig *deviceConfig = it.value();

        // 获取设备的所有任务
        QList<Task*> tasks = deviceConfig->tasks();
        for (Task *task : tasks) {
            if (task->taskName().contains("poll", Qt::CaseInsensitive) ||
                task->taskName().contains("read", Qt::CaseInsensitive)) {

                // 调度轮询任务到低优先级队列
                schedulePollingTask(deviceId, task->taskName(), 0, QVector<quint16>());
                qDebug() << "Scheduled polling task - Device:" << deviceId
                         << "Task:" << task->taskName() << "Interval:" << task->interval() << "ms";

                // 每个设备只调度一个轮询任务
                break;
            }
        }
    }
}

// 串口配置缓存和设备-串口映射相关方法实现

/**
 * @brief 更新串口配置缓存
 * @param portName 串口名称
 * @param serialConfig 串口配置
 */
void ModbusTaskScheduler::updatePortConfigCache(const SerialConfig &serialConfig)
{
    QString portName = serialConfig.portName;
    m_portConfigCache[portName] = serialConfig;
    qDebug() << "Updated port config cache - Port:" << portName
             << "BaudRate:" << serialConfig.baudRate
             << "DataBits:" << serialConfig.dataBits
             << "Parity:" << serialConfig.parity;
}

/**
 * @brief 检查串口配置是否发生变化
 * @param portName 串口名称
 * @param serialConfig 新的串口配置
 * @return 配置是否发生变化
 */
bool ModbusTaskScheduler::isPortConfigChanged(const SerialConfig &serialConfig) const
{
    QString portName = serialConfig.portName;
    if (!m_portConfigCache.contains(portName)) {
        // 缓存中没有该串口的配置，说明是第一次使用
        qDebug()<<"第一次使用这个串口" << portName;
        return true;
    }
    
    SerialConfig cachedConfig = m_portConfigCache.value(portName);
    
    // 比较关键配置参数
    const bool changed =
        cachedConfig.baudRate != serialConfig.baudRate ||
        cachedConfig.dataBits != serialConfig.dataBits ||
        cachedConfig.parity != serialConfig.parity ||
        cachedConfig.stopBits != serialConfig.stopBits ||
        cachedConfig.flowControl != serialConfig.flowControl;

    if (changed) {
        qDebug() << "Port config changed - Port:" << portName
                 << "cached baud/data/parity/stop/flow="
                 << cachedConfig.baudRate
                 << cachedConfig.dataBits
                 << cachedConfig.parity
                 << cachedConfig.stopBits
                 << cachedConfig.flowControl
                 << "new="
                 << serialConfig.baudRate
                 << serialConfig.dataBits
                 << serialConfig.parity
                 << serialConfig.stopBits
                 << serialConfig.flowControl;
        return true;
    }
    
    return false;
}

/**
 * @brief 使用新配置重新连接串口
 * @param portName 串口名称
 * @param serialConfig 新的串口配置
 */
void ModbusTaskScheduler::reconnectPortWithNewConfig(const SerialConfig &serialConfig)
{
    QString portName = serialConfig.portName;
    qDebug() << "Reconnecting port with new config - Port:" << portName;
    
    // 先断开当前连接
    if (m_portManager->isPortConnected(portName)) {
        m_portManager->disconnectPort(portName);
        qDebug() << "Disconnected port:" << portName;
    }
    
    // 更新串口配置
    if (m_portManager->addPort(serialConfig)) {
        // 重新连接串口
        if (m_portManager->connectPort(portName)) {
            qDebug() << "Successfully reconnected port with new config - Port:" << portName;
            // 更新缓存
            updatePortConfigCache(serialConfig);
        } else {
            qWarning() << "Failed to reconnect port:" << portName;
        }
    } else {
        qWarning() << "Failed to update port config:" << portName;
    }
}

/**
 * @brief 获取设备对应的串口名称
 * @param deviceId 设备ID
 * @return 串口名称
 */
QString ModbusTaskScheduler::getPortNameForDevice(const QString &deviceId) const
{
    if (m_deviceToPortMap.contains(deviceId)) {
        return m_deviceToPortMap.value(deviceId);
    }
    
    // 从设备配置中查找
    if (m_deviceConfigMap.contains(deviceId)) {
        DeviceConfig *deviceConfig = m_deviceConfigMap.value(deviceId);
        QJsonObject deviceConfigObj = m_deviceConfigs.value(deviceId);
        QJsonObject serialConfig = deviceConfigObj.value("serialConfig").toObject();
        return serialConfig.value("portName").toString();
    }
    
    return QString();
}

/**
 * @brief 获取设备对应的串口配置
 * @param deviceId 设备ID
 * @return 串口配置
 */
QJsonObject ModbusTaskScheduler::getSerialConfigForDevice(const QString &deviceId) const
{
    if (m_deviceConfigMap.contains(deviceId)) {
        QJsonObject deviceConfigObj = m_deviceConfigs.value(deviceId);
        return deviceConfigObj.value("serialConfig").toObject();
    }
    
    return QJsonObject();
}

/**
 * @brief 暂停指定串口的任务调度
 * @param portName 串口名称
 */
void ModbusTaskScheduler::pauseTaskSchedulingForPort(const QString &portName)
{
    if (!m_portTaskSchedulingStates.contains(portName) || m_portTaskSchedulingStates[portName]) {
        qDebug() << "Pausing task scheduling for port:" << portName;
        
        // 暂停该串口下所有设备的轮询任务
        QList<Device*> devices = m_portManager->getDevicesOnPort(portName);
        for (Device *device : devices) {
            QString deviceId = device->deviceId();
            
            // 暂停该设备的所有轮询任务
            if (m_deviceConfigMap.contains(deviceId)) {
                DeviceConfig *deviceConfig = m_deviceConfigMap.value(deviceId);
                QList<Task*> tasks = deviceConfig->tasks();
                
                for (Task *task : tasks) {
                    if (task->interval() > 0) { // 轮询任务
                        pausePollingTask(deviceId, task->taskName());
                        qDebug() << "Paused polling task:" << task->taskName() << "for device:" << deviceId;
                    }
                }
            }
        }
        
        // 更新任务调度状态
        m_portTaskSchedulingStates[portName] = false;
        qDebug() << "Task scheduling paused for port:" << portName;
    }
}

/**
 * @brief 恢复指定串口的任务调度
 * @param portName 串口名称
 */
void ModbusTaskScheduler::resumeTaskSchedulingForPort(const QString &portName)
{
    if (!m_portTaskSchedulingStates.contains(portName) || !m_portTaskSchedulingStates[portName]) {
        qDebug() << "Resuming task scheduling for port:" << portName;
        
        // 恢复该串口下所有设备的轮询任务
        QList<Device*> devices = m_portManager->getDevicesOnPort(portName);
        for (Device *device : devices) {
            QString deviceId = device->deviceId();
            
            // 恢复该设备的所有轮询任务
            if (m_deviceConfigMap.contains(deviceId)) {
                DeviceConfig *deviceConfig = m_deviceConfigMap.value(deviceId);
                QList<Task*> tasks = deviceConfig->tasks();
                
                for (Task *task : tasks) {
                    if (task->interval() > 0) { // 轮询任务
                        resumePollingTask(deviceId, task->taskName());
                        qDebug() << "Resumed polling task:" << task->taskName() << "for device:" << deviceId;
                    }
                }
            }
        }
        
        // 更新任务调度状态
        m_portTaskSchedulingStates[portName] = true;
        qDebug() << "Task scheduling resumed for port:" << portName;
    }
}

/**
 * @brief 检查指定串口的任务调度是否活跃
 * @param portName 串口名称
 * @return 任务调度状态
 */
bool ModbusTaskScheduler::isPortTaskSchedulingActive(const QString &portName) const
{
    return m_portTaskSchedulingStates.value(portName, false);
}
