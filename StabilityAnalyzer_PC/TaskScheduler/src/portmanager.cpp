#include "portmanager.h"
#include <QSerialPortInfo>
#include <QDebug>

/**
 * @brief PortManager 构造函数
 * @param parent 父对象指针
 */
PortManager::PortManager(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<TaskResult>("TaskResult");

}

PortManager::~PortManager()
{
    // 断开所有端口连接
    for (auto it = m_ports.begin(); it != m_ports.end(); ++it) {
        disconnectPort(it.key());
    }
    m_ports.clear();
    m_deviceMap.clear();
}

//添加串口到串口名称-串口信息字典
bool PortManager::addPort(const SerialConfig &portConfig)
{
    QString portName = portConfig.portName;
    if (m_ports.contains(portName)) {
        qWarning() << "Port already exists:" << portName;
       /* return false; *///在这个地方，如果这个串口已经存在了就返回true，
        return true; //如果一个串口有多个串口配置，就先默认使用读取到的第一个设备的串口配置，如果换了设备串口配置在执行任务的时候会更新重连
    }
    
    // 检查串口是否存在
    bool portExists = false;
    const auto availablePorts = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &portInfo : availablePorts) {
        if (portInfo.portName() == portName) {
            portExists = true;
            break;
        }
    }
    
    if (!portExists) {
        qWarning() << "Port does not exist:" << portName;
        return false;
    }
    
    PortInfo portInfo;
    portInfo.portName = portName;
    portInfo.baudRate = static_cast<QSerialPort::BaudRate>(portConfig.baudRate);
    portInfo.dataBits = static_cast<QSerialPort::DataBits>(portConfig.dataBits);
    portInfo.parity = static_cast<QSerialPort::Parity>(portConfig.parity.toInt());
    portInfo.stopBits = static_cast<QSerialPort::StopBits>(portConfig.stopBits);
    portInfo.flowControl = static_cast<QSerialPort::FlowControl>(portConfig.flowControl.toInt());
    portInfo.isConnected = false;
    portInfo.modbusClient = nullptr;
    
    //将这个串口信息放到字典中
    m_ports.insert(portName, portInfo);
    
    emit portCountChanged(m_ports.size());
    return true;
}

//移除串口
bool PortManager::removePort(const QString &portName)
{
    if (!m_ports.contains(portName)) {
        qWarning() << "Port not found:" << portName;
        return false;
    }
    
    // 断开连接
    disconnectPort(portName);
    
    // 移除该端口下的所有设备
    PortInfo &portInfo = m_ports[portName];
    for (Device *device : portInfo.devices) {
        m_deviceMap.remove(device->deviceId());
        device->deleteLater();
    }
    portInfo.devices.clear();
    
    m_ports.remove(portName);
    
    emit portCountChanged(m_ports.size());
    return true;
}

bool PortManager::isPortConnected(const QString &portName) const
{
    if (!m_ports.contains(portName)) {
        return false;
    }
    return m_ports.value(portName).isConnected;
}

//添加设备到串口，一个串口下面可能挂载多个设备，这个必须在添加串口信息到串口集合后面，要不在找对应的串口然后吧设备添加到这个串口的时候找不到
bool PortManager::addDeviceToPort(const QString &portName, Device *device)
{
    if (!m_ports.contains(portName)) {
        qWarning() << "Port not found:" << portName;
        return false;
    }
    
    if (m_deviceMap.contains(device->deviceId())) {
        qWarning() << "Device already exists:" << device->deviceId();
        return false;
    }

    PortInfo &portInfo = m_ports[portName];
    
    // 设置设备的Modbus客户端，判断设备对应的串口是否已经连接了，如果已经连接了就直接把这个客户端给设备
    // 设备连接应该在端口层面管理，不应该在这里单独连接设备
    if (portInfo.isConnected && portInfo.modbusClient) {
        device->setModbusClient(portInfo.modbusClient);
        // 不在这里调用device->connectToDevice()，因为连接已经在端口层面管理
    }
    
    //将这个设备添加到这个串口下面，这个时候可能已经挂载了设备，一个串口下面可以挂载一个或多个设备
    portInfo.devices.append(device);
    m_deviceMap.insert(device->deviceId(), device);
    qDebug()<<"插入设备id数值为，判断这个设备id是否为空：" << device->deviceId();

    // 连接设备信号，任务完成信号
    connect(device, &Device::taskCompleted, this, &PortManager::onDeviceTaskCompleted);
    
    return true;
}

bool PortManager::removeDeviceFromPort(const QString &portName, const QString &deviceId)
{
    if (!m_ports.contains(portName)) {
        return false;
    }
    
    if (!m_deviceMap.contains(deviceId)) {
        return false;
    }
    
    PortInfo &portInfo = m_ports[portName];
    Device *device = m_deviceMap.value(deviceId);
    
    portInfo.devices.removeAll(device);
    m_deviceMap.remove(deviceId);
    device->deleteLater();
    
    return true;
}

QList<Device*> PortManager::getDevicesOnPort(const QString &portName) const
{
    if (!m_ports.contains(portName)) {
        return QList<Device*>();
    }
    return m_ports.value(portName).devices;
}

//连接串口
bool PortManager::connectPort(const QString &portName)
{
    if (!m_ports.contains(portName)) {
        qWarning() << "Port not found:" << portName;
        return false;
    }
    
    //从集合里面找到这个串口对应的串口信息
    PortInfo &portInfo = m_ports[portName];
    
    if (portInfo.isConnected) {
        qWarning() << "Port already connected:" << portName;
        return true;
    }
    
    // 如果Modbus客户端已存在，则重用；否则创建新的
    if (!portInfo.modbusClient) {
        portInfo.modbusClient = createModbusClient(portName, portInfo);
        if (!portInfo.modbusClient) {
            qWarning() << "Failed to create Modbus client for port:" << portName;
            return false;
        }
    }
    
    // 检查Modbus客户端是否真正连接成功
    ModbusClient *client = qobject_cast<ModbusClient*>(portInfo.modbusClient);
    if (!client) {
        qWarning() << "Invalid Modbus client object for port:" << portName;
        return false;
    }
    
    // 尝试连接设备
    connect(client, &ModbusClient::disconnected, this, &PortManager::onModbusClientDisconnected, Qt::DirectConnection);
    connect(client, &ModbusClient::connected, this, &PortManager::onModbusClientConnected, Qt::DirectConnection);

    bool connected = client->connect();
    if (!connected) {
        qWarning() << "Failed to connect Modbus client to port:" << portName;
        // 连接失败，清理资源
        destroyModbusClient(portName);
        return false;
    }
    
    // 连接成功，更新状态
    portInfo.isConnected = true;
    
    // 为端口下的所有设备设置Modbus客户端
    // 设备连接在端口层面管理，这里只更新设备连接状态
    for (Device *device : portInfo.devices) {
        device->setModbusClient(portInfo.modbusClient);
        // 更新设备连接状态（不重新连接，只检查状态）
        device->connectToDevice();
    }
    
    emit portConnectionChanged(portName, true);
    return true;
}

bool PortManager::disconnectPort(const QString &portName)
{
    if (!m_ports.contains(portName)) {
        return false;
    }
    
    PortInfo &portInfo = m_ports[portName];
    
    if (!portInfo.isConnected) {
        return true;
    }
    
    // 断开端口下的所有设备
    for (Device *device : portInfo.devices) {
        device->disconnectFromDevice();
        device->setModbusClient(nullptr);
    }
    
    // 销毁Modbus客户端
    destroyModbusClient(portName);
    
    portInfo.isConnected = false;
    portInfo.modbusClient = nullptr;
    
    emit portConnectionChanged(portName, false);
    return true;
}

//暂时没用到
void PortManager::executeInitTasks()
{
    for (auto it = m_ports.begin(); it != m_ports.end(); ++it) {
        PortInfo &portInfo = it.value();
        if (portInfo.isConnected) {
            for (Device *device : portInfo.devices) {
                device->executeInitTasks();
            }
        }
    }
}

//暂时没用到
void PortManager::executeUserTask(const QString &portName, const QString &deviceId, const QString &taskId)
{
    if (!m_ports.contains(portName)) {
        emit errorOccurred("Port not found: " + portName);
        return;
    }
    
    Device *device = findDevice(deviceId);
    if (!device) {
        emit errorOccurred("Device not found: " + deviceId);
        return;
    }
    
    device->executeUserTask(taskId);
}

Device* PortManager::findDevice(const QString &deviceId) const
{
    return m_deviceMap.value(deviceId, nullptr);
}

QObject* PortManager::getModbusClient(const QString &portName) const
{
    if (!m_ports.contains(portName)) {
        return nullptr;
    }
    return m_ports.value(portName).modbusClient;
}

void PortManager::onDeviceTaskCompleted(TaskResult res, QVector<quint16>data)
{
    Device *device = qobject_cast<Device*>(sender());
    if (device) {
        emit deviceTaskCompleted(res, data);
    }
}

//创建modbusrtuclient客户端，一个串口对应一个modbusrtuclient，任务串行执行
QObject* PortManager::createModbusClient(const QString &portName, const PortInfo &portInfo)
{
    // 创建ModbusClient实例
    ModbusClient *client = new ModbusClient(this);
    
    // 配置串口参数
    ModbusClient::ClientConfig clientConfig;
    clientConfig.serialConfig.portName = portName;
    clientConfig.serialConfig.baudRate = portInfo.baudRate;
    clientConfig.serialConfig.dataBits = portInfo.dataBits;
    clientConfig.serialConfig.parity = portInfo.parity;
    clientConfig.serialConfig.stopBits = portInfo.stopBits;
    clientConfig.serialConfig.flowControl = portInfo.flowControl;
    
    // 初始化客户端配置
    if (!client->initialize(clientConfig)) {
        qWarning() << "Failed to initialize Modbus client for port:" << portName;
        client->deleteLater();
        return nullptr;
    }
    
    qDebug() << "Modbus client created successfully for port:" << portName;
    return client;
}

//接收modbusclient传递过来的断开连接信号
void PortManager::onModbusClientDisconnected()
{
    // 查找是哪个ModbusClient发出的断开信号
    ModbusClient *disconnectedClient = qobject_cast<ModbusClient*>(sender());
    if (!disconnectedClient) {
        qWarning() << "Invalid sender in onModbusClientDisconnected";
        return;
    }
    
    // 查找对应的端口
    QString portName;
    for (auto it = m_ports.begin(); it != m_ports.end(); ++it) {
        if (it.value().modbusClient == disconnectedClient) {
            portName = it.key();
            break;
        }
    }
    
    if (portName.isEmpty()) {
        qWarning() << "Could not find port for disconnected ModbusClient";
        return;
    }
    
    // 更新连接状态
    if (m_ports.contains(portName)) {
        PortInfo &portInfo = m_ports[portName];
        portInfo.isConnected = false;
        
        qDebug() << "检测到串口意外断开:" << portName;
        
        // 发出端口连接状态变化信号，通知ModbusTaskScheduler停止任务调度
        emit portConnectionChanged(portName, false);
    }
}

void PortManager::onModbusClientConnected()
{
    // 查找是哪个ModbusClient发出的断开信号
    ModbusClient *connectedClient = qobject_cast<ModbusClient*>(sender());
    if (!connectedClient) {
        qWarning() << "Invalid sender in onModbusClientDisconnected";
        return;
    }

    // 查找对应的端口
    QString portName;
    for (auto it = m_ports.begin(); it != m_ports.end(); ++it) {
        if (it.value().modbusClient == connectedClient) {
            portName = it.key();
            break;
        }
    }

    if (portName.isEmpty()) {
        qWarning() << "Could not find port for disconnected ModbusClient";
        return;
    }

    // 更新连接状态
    if (m_ports.contains(portName)) {
        PortInfo &portInfo = m_ports[portName];
        portInfo.isConnected = true;

        qDebug() << "检测到串口连接成功:" << portName;

        // 发出端口连接状态变化信号，通知ModbusTaskScheduler重新启动任务调度
        emit portConnectionChanged(portName, true);
    }
}

void PortManager::destroyModbusClient(const QString &portName)
{
    if (!m_ports.contains(portName)) {
        return;
    }
    
    PortInfo &portInfo = m_ports[portName];
    if (portInfo.modbusClient) {
        // 断开信号连接
        ModbusClient *client = qobject_cast<ModbusClient*>(portInfo.modbusClient);
        if (client) {
            disconnect(client, &ModbusClient::disconnected, this, &PortManager::onModbusClientDisconnected);
        }
        
        portInfo.modbusClient->deleteLater();
    }
}

