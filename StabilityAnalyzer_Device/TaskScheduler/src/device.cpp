#include "device.h"
#include <QDebug>
#include <QJsonArray>
#include "modbus_client.h"

/**
 * @brief Device 构造函数
 * @param config 设备配置信息
 * @param parent 父对象指针
 * Device不再自己创建任务，而是使用DeviceConfig中已经创建的任务,要不然这样会出现两个任务实例
   任务将在DeviceConfig中创建，它负责解读json文件然后创建任务，然后通过addTasks方法添加到Device中，
   device直接使用deviceconfig里面的任务
 */
Device::Device(const QString &deviceId, QObject *portManager, QObject *parent)
    : QObject(parent)
    , m_isConnected(false)
    , m_portManager(portManager)
{
    qRegisterMetaType<TaskResult>("TaskResult");
    // 从配置中获取设备ID，使用slaveId作为设备标识
//    int slaveId = config.value("slaveId").toInt();
//    m_deviceId = QString::number(slaveId);
//    m_deviceName = config.value("name").toString();
    m_deviceId = deviceId;
}

void Device::addTask(Task *task)
{
    if (!task) {
        qWarning() << "Cannot add null task to device:" << m_deviceId;
        return;
    }
    
    if (m_taskMap.contains(task->taskName())) {
        qWarning() << "Task already exists:" << task->taskName();
        return;
    }
    
    qDebug() << "Device::addTask - Adding task:" << task->taskName() << "to device:" << m_deviceId;
    
    // 设置设备实例给任务
    task->setDevice(this);
    
    m_tasks.append(task);
    m_taskMap.insert(task->taskName(), task);
    
    connect(task, &Task::taskCompleted, this, &Device::onTaskCompleted);
    
    qDebug() << "Device::addTask - Task added successfully, total tasks:" << m_tasks.size();
}

void Device::addTasks(const QList<Task*> &tasks)
{
    for (Task *task : tasks) {
        addTask(task);
    }
    qDebug() << "Device::addTasks - Added" << tasks.size() << "tasks to device:" << m_deviceId;
}

void Device::removeTask(const QString &taskName)
{
    if (!m_taskMap.contains(taskName)) {
        return;
    }
    
    Task *task = m_taskMap.value(taskName);
    m_tasks.removeAll(task);
    m_taskMap.remove(taskName);
    task->deleteLater();
}

Task* Device::findTask(const QString &taskName) const
{
    return m_taskMap.value(taskName, nullptr);
}

//执行初始化任务
void Device::executeInitTasks()
{
    if (!m_isConnected) {
        qWarning() << "Device not connected, cannot execute init tasks:" << m_deviceId;
        return;
    }
    
    for (Task *task : m_tasks) {
        if (task->taskType() == TaskType::INIT_TASK) {
            task->execute();
        }
    }
}

//执行用户任务
void Device::executeUserTask(const QString &taskName)
{
    if (!m_isConnected) {
        emit errorOccurred("Device not connected: " + m_deviceId);
        return;
    }
    
    Task *task = findTask(taskName);
    if (!task) {
        emit errorOccurred("Task not found: " + taskName);
        return;
    }
    
    if (task->taskType() != TaskType::USER_TASK) {
        emit errorOccurred("Task is not user task: " + taskName);
        return;
    }
    
    task->execute();
}

//连接设备
void Device::connectToDevice()
{
    if (!m_modbusClient) {
        emit errorOccurred("No Modbus client set for device: " + m_deviceId);
        qWarning() << "No Modbus client set for device:" << m_deviceId;
        return;
    }
    
    if (m_isConnected) {
        qDebug() << "Device already connected:" << m_deviceId;
        return;
    }
    
    // 调用Modbus客户端的方法来检查设备连接状态
    // 设备连接在端口层面管理，这里只检查连接状态
    ModbusClient *client = qobject_cast<ModbusClient*>(m_modbusClient);
    if (client) {
        // 检查Modbus客户端是否已连接
        if (client->isConnected()) {
            m_isConnected = true;
            emit connectionStatusChanged(true);
            qDebug() << "Device connected successfully:" << m_deviceId;
        } else {
            emit errorOccurred("Modbus client not connected for device: " + m_deviceId);
            qWarning() << "Modbus client not connected for device:" << m_deviceId;
        }
    } else {
        emit errorOccurred("Invalid Modbus client for device: " + m_deviceId);
        qWarning() << "Invalid Modbus client for device:" << m_deviceId;
    }
}

//断开连接
void Device::disconnectFromDevice()
{
    if (!m_isConnected) {
        return;
    }
    
    // 调用Modbus客户端的方法来断开设备连接
    ModbusClient *client = qobject_cast<ModbusClient*>(m_modbusClient);
    if (client) {
        client->disconnect();
        m_isConnected = false;
        emit connectionStatusChanged(false);
        qDebug() << "Device disconnected:" << m_deviceId;
    } else {
        emit errorOccurred("Invalid Modbus client for device: " + m_deviceId);
        qWarning() << "Invalid Modbus client for device:" << m_deviceId;
    }
}

//接收task传递过来的任务完成信号
void Device::onTaskCompleted(TaskResult res, QVector<quint16>data)
{
    Task *task = qobject_cast<Task*>(sender());
    if (task) {
        emit taskCompleted(res, data);
    }
}
