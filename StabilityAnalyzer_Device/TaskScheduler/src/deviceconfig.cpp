#include "deviceconfig.h"
#include "task.h"
#include <QDebug>
#include <QJsonArray>
#include <QSerialPortInfo>
/**
 * @brief DeviceConfig 构造函数
 * @param parent 父对象指针
 * 这个类主要是提供配置文件数据的读取接口，还要读取配置文件获取配置信息都在这个类处理
 */
DeviceConfig::DeviceConfig(QObject *portManager, QObject *parent)
    : QObject(parent)
    , m_isConnected(false)
    , m_interFrameDelay(5)
    , m_responseTimeout(1000)
    , m_maxRetries(3)
    , m_portManager(portManager)
{
}

/**
 * @brief 从JSON配置加载设备配置
 * @param config JSON配置对象
 * @return 加载成功返回true，失败返回false
 */
bool DeviceConfig::loadFromJson(const QJsonObject &config)
{
    try {
        // 解析通用信息
        parseGeneralInfo(config);
        
        // 解析串口配置
        QJsonObject serialConfig = config.value("serialConfig").toObject();
        if (!serialConfig.isEmpty()) {
            parseSerialConfig(serialConfig);
        }
        
        // 解析任务列表
        QJsonArray taskList = config.value("taskList").toArray();
        if (!taskList.isEmpty()) {
            parseTaskList(taskList);
        }
        
        qDebug() << "Device configuration loaded - ID:" << m_deviceId
                 << "Name:" << m_deviceName
                 << "Tasks:" << m_tasks.size();
        
        emit configurationLoaded();
        return true;
        
    } catch (const std::exception &e) {
        emit errorOccurred(QString("Failed to load device configuration: %1").arg(e.what()));
        return false;
    }
}

/**
 * @brief 获取初始化任务（轮询任务）
 * @return 初始化任务列表
 */
QList<Task*> DeviceConfig::initTasks() const
{
    QList<Task*> initTasks;
    for (Task *task : m_tasks) {
        if (task && task->taskType() == TaskType::INIT_TASK) {
            initTasks.append(task);
        }
    }
    return initTasks;
}

/**
 * @brief 获取用户任务
 * @return 用户任务列表
 */
QList<Task*> DeviceConfig::userTasks() const
{
    QList<Task*> userTasks;
    for (Task *task : m_tasks) {
        if (task && task->taskType() == TaskType::USER_TASK) {
            userTasks.append(task);
        }
    }
    return userTasks;
}

/**
 * @brief 根据任务名称查找任务
 * @param taskName 任务名称
 * @return 任务指针，未找到返回nullptr
 */
Task* DeviceConfig::findTask(const QString &taskName) const
{
    for (Task *task : m_tasks) {
        if (task->taskName() == taskName) {
            return task;
        }
    }
    return nullptr;
}

/**
 * @brief 设置设备连接状态
 * @param connected 连接状态
 */
void DeviceConfig::setConnected(bool connected)
{
    if (m_isConnected != connected) {
        m_isConnected = connected;
        emit connectionStatusChanged(connected);
    }
}

/**
 * @brief 解析设备通用信息
 * @param config JSON配置对象
 */
void DeviceConfig::parseGeneralInfo(const QJsonObject &config)
{
//    m_deviceId = config.value("deviceId").toString();
//    if (m_deviceId.isEmpty()) {
//        m_deviceId = QString::number(config.value("slaveId").toInt(1));
//    }
    m_deviceId = QString::number(config.value("slaveId").toInt(1));
    m_deviceName = config.value("name").toString();
    m_description = config.value("description").toString();
    m_manufacturer = config.value("manufacturer").toString();
    m_model = config.value("model").toString();
}

/**
 * @brief 解析串口配置
 * @param config 串口配置对象
 */
void DeviceConfig::parseSerialConfig(const QJsonObject &config)
{
    //判断串口是否为空，不为空的则使用串口，为空的话则根据pid，vid去找对应的串口
    QString port = config.value("portName").toString();
    QString usedPort = port;
    QString pid = config.value("pid").toString();
    QString vid  = config.value("vid").toString();

    //判断端口字段是否为空
    if(port.isEmpty() || port == ""){
        QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
        for (const QSerialPortInfo &info : ports) {
            if(info.hasProductIdentifier()&&info.hasVendorIdentifier()){
                qDebug()<<"---"<<static_cast<int>(info.productIdentifier());
                qDebug()<<"---"<<static_cast<int>(info.vendorIdentifier());
                if (info.productIdentifier() == pid && info.vendorIdentifier() == vid){
                    usedPort = info.portName();
                    qDebug()<<"-----------------"<<usedPort;
                    break;
                }
            }
        }
    }
    m_serialConfig.portName = usedPort;
    m_serialConfig.baudRate = config.value("baudRate").toInt(9600);
    m_serialConfig.dataBits = config.value("dataBits").toInt(8);
    m_serialConfig.parity = config.value("parity").toString("NoParity");
    m_serialConfig.stopBits = config.value("stopBits").toInt(1);
    m_serialConfig.flowControl = config.value("flowControl").toString("NoFlowControl");
    m_interFrameDelay = config.value("interFrameDelay").toInt(5);
    m_responseTimeout = config.value("responseTimeout").toInt(1000);
    m_maxRetries = config.value("maxRetries").toInt(3);
}

/**
 * @brief 解析任务列表
 * @param tasksArray 任务数组
 */
void DeviceConfig::parseTaskList(const QJsonArray &tasksArray)
{
    // 清理现有任务
    for (Task *task : m_tasks) {
        task->deleteLater();
    }
    m_tasks.clear();
    m_taskMap.clear();
    
    // 遍历解析每个任务信息
    for (const QJsonValue &taskValue : tasksArray) {
        QJsonObject taskConfig = taskValue.toObject();
        
        // 添加设备ID到任务配置
//        taskConfig["deviceId"] = m_deviceId;
        
        //创建task任务类，使用json文件数据创建，给任务类传递json数据和串口对象
        Task *task = Task::createFromJson(taskConfig, m_portManager);
        //给任务关联任务id，这个任务id就是读取的配置文件的slaveId
        task->setDeviceId(m_deviceId);
//        qDebug()<< __FUNCTION__ << "task信息：" << task->deviceId()<<m_deviceId;
        if (task) {
            m_tasks.append(task);
            m_taskMap.insert(task->taskName(), task);
            
//            qDebug() << "Task created - Device:" << m_deviceId
//                     << "Task Name:" << task->taskName()
//                     << "Name:" << task->taskName();
        }
    }
}

QObject *DeviceConfig::portManager() const
{
    return m_portManager;
}

const SerialConfig &DeviceConfig::serialConfig() const
{
    return m_serialConfig;
}
