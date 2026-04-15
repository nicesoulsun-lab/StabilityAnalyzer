#include "connection_manager.h"
#include <QSerialPortInfo>
#include <QThread>
#include <QDebug>
#include <QElapsedTimer>
#include <QtMath>
#include "logmanager.h"
/**
 * @brief ConnectionManager构造函数
 * @param parent 父对象指针
 *
 * 初始化串口连接管理器的所有成员变量和定时器
 * 设置重连定时器为单次触发模式
 * 连接串口数据接收信号和错误信号到相应的槽函数
 */
ConnectionManager::ConnectionManager(QObject* parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
    , m_state(ConnectionState::Disconnected)
    , m_reconnectAttempts(0)
    , m_connectionTime(0)
    , m_bytesSent(0)
    , m_bytesReceived(0)
    , m_sendErrors(0)
    , m_receiveErrors(0)
{
    // 初始化重连定时器
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    QObject::connect(m_reconnectTimer, &QTimer::timeout, this, &ConnectionManager::onReconnectTimeout);
    
    // 初始化心跳包定时器
    m_keepAliveTimer = new QTimer(this);
    QObject::connect(m_keepAliveTimer, &QTimer::timeout, this, &ConnectionManager::onKeepAliveTimeout);
    
    // 初始化心跳包响应超时定时器
    m_heartbeatResponseTimer = new QTimer(this);
    m_heartbeatResponseTimer->setSingleShot(true);
    QObject::connect(m_heartbeatResponseTimer, &QTimer::timeout, this, &ConnectionManager::onHeartbeatResponseTimeout);
    
    // 初始化心跳包相关状态
    m_heartbeatTimeoutCount = 0;
    m_maxHeartbeatTimeouts = 3; // 默认最大3次超时
    m_heartbeatAcknowledged = true; // 初始状态为已确认
    
    // 连接串口信号到槽函数
    QObject::connect(m_serialPort, &QSerialPort::readyRead, this, &ConnectionManager::onReadyRead);
    QObject::connect(m_serialPort, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::error),
                     this, &ConnectionManager::onError);
}

/**
 * @brief ConnectionManager析构函数
 *
 * 自动断开连接并清理资源
 * 确保在对象销毁时正确关闭串口连接
 */
ConnectionManager::~ConnectionManager()
{
    disconnect();
}

/**
 * @brief 连接到Modbus设备
 * @param config 连接配置参数
 * @return 连接是否成功
 *
 * 线程安全的连接方法，使用互斥锁保护串口操作
 * 配置串口参数并尝试打开串口连接
 * 连接成功后会启动心跳包定时器，连接失败会启动重连机制
 */
bool ConnectionManager::connect(const ConnectionConfig& config)
{
    QMutexLocker locker(&m_serialMutex);
    
    // 检查当前连接状态，避免重复连接
    if (m_state == ConnectionState::Connected ||
            m_state == ConnectionState::Connecting) {
        qWarning() << "连接已存在或正在连接";
        return false;
    }
    
    // 更新配置和状态
    m_config = config;
    m_state = ConnectionState::Connecting;
    emit stateChanged(m_state);
    
    // 配置串口参数
    m_serialPort->setPortName(config.serialConfig.portName);
    m_serialPort->setBaudRate(config.serialConfig.baudRate);
    m_serialPort->setDataBits(config.serialConfig.dataBits);
    m_serialPort->setParity(config.serialConfig.parity);
    m_serialPort->setStopBits(config.serialConfig.stopBits);
    m_serialPort->setFlowControl(config.serialConfig.flowControl);
    
    qDebug() << "正在连接串口:" << config.serialConfig.portName
             << "波特率:" << config.serialConfig.baudRate << "数据位：" <<config.serialConfig.dataBits
             <<"停止位：" << config.serialConfig.stopBits << "校验位：" << config.serialConfig.parity;
    
    // 尝试打开串口
    if (m_serialPort->open(QIODevice::ReadWrite)) {
        // 连接成功处理
        m_state = ConnectionState::Connected;
        m_connectionTime = QDateTime::currentMSecsSinceEpoch();
        m_reconnectAttempts = 0;
        
        // 清空缓冲区
        clearBuffers();
        
        // 启动心跳定时器（如果启用）
        if (config.keepAliveEnabled && config.keepAliveInterval > 0) {
            m_keepAliveTimer->start(config.keepAliveInterval);
        }
        
        qInfo() << "串口连接成功:" << config.serialConfig.portName;
        
        // 发射连接成功信号
        emit connected();
        emit stateChanged(m_state);
        
        return true;
    } else {
        // 连接失败处理
        m_state = ConnectionState::Error;
        m_errorString = QString("打开串口失败: %1").arg(m_serialPort->errorString());
        
        qCritical() << "串口连接失败:" << m_errorString;
        
        // 发射错误信号
        emit connectionError(m_errorString);
        emit stateChanged(m_state);
        
        // 启动重连定时器（如果启用重连策略）
        if (config.reconnectPolicy != NoReconnect &&
                config.reconnectInterval > 0) {
            m_reconnectTimer->start(config.reconnectInterval);
        }
        
        return false;
    }
}

/**
 * @brief 断开与Modbus设备的连接
 *
 * 线程安全的断开连接方法，使用互斥锁保护串口操作
 * 停止所有定时器，关闭串口，清空缓冲区，并重置连接状态
 * 发射相应的信号通知连接状态变化
 */
void ConnectionManager::disconnect()
{
    QMutexLocker locker(&m_serialMutex);
    
    // 检查是否已经断开连接
    if (m_state == ConnectionState::Disconnected) {
        return;
    }
    
    // 更新状态为正在断开
    m_state = ConnectionState::Disconnecting;
    emit stateChanged(m_state);
    
    // 停止所有定时器
    m_reconnectTimer->stop();
    m_keepAliveTimer->stop();
    
    // 关闭串口
    if (m_serialPort->isOpen()) {
        m_serialPort->clear(); // 增加：清空输入输出缓冲区，这个是解决出现软件退出之后还有任务或者串口还在执行
        m_serialPort->flush(); // 增加：等待所有数据位发送完毕
        m_serialPort->close();
        qInfo() << "串口已关闭:" << m_config.serialConfig.portName;
    }
    
    // 重置连接状态和统计信息
    m_state = ConnectionState::Disconnected;
    m_connectionTime = 0;
    m_reconnectAttempts = 0;
    
    // 清空缓冲区
    clearBuffers();
    
    // 发射断开连接信号
    emit disconnected(); //应用层关联这个信号断开调度器
    emit stateChanged(m_state);
}

/**
 * @brief 尝试重新连接
 * @return 重连是否成功
 *
 * 检查当前连接状态，如果已连接则直接返回成功
 * 检查是否超过最大重连次数限制
 * 增加重连尝试次数并发射重连尝试信号
 * 调用connect方法进行实际的重连操作
 */
bool ConnectionManager::reconnect()
{
    // 检查是否已经连接或正在连接
    if (m_state == ConnectionState::Connected ||
            m_state == ConnectionState::Connecting) {
        return true;
    }
    
    // 检查最大重连次数限制
    if (m_config.maxReconnectAttempts > 0 &&
            m_reconnectAttempts >= m_config.maxReconnectAttempts) {
        m_errorString = QString("超过最大重连次数: %1").arg(m_config.maxReconnectAttempts);
        emit connectionError(m_errorString);
        return false;
    }
    
    // 增加重连尝试次数并发射信号
    m_reconnectAttempts++;
    emit reconnectAttempt(m_reconnectAttempts, m_config.maxReconnectAttempts);
    
    qDebug() << "尝试重连，第" << m_reconnectAttempts << "次";
    
    // 调用connect方法进行实际的重连
    return connect(m_config);
}

/**
 * @brief 检查是否已连接到设备
 * @return 连接状态
 *
 * 检查当前连接状态是否为Connected，并且串口对象存在且已打开
 * 这是一个线程安全的方法，但需要外部同步保护
 */
bool ConnectionManager::isConnected() const
{
    return m_state == ConnectionState::Connected &&
            m_serialPort && m_serialPort->isOpen();
}

/**
 * @brief 发送数据到串口
 * @param data 要发送的数据字节数组
 * @return 发送是否成功
 *
 * 线程安全的数据发送方法，使用互斥锁保护串口操作
 * 检查连接状态和数据有效性，应用帧间延迟（如果需要）
 * 记录发送统计信息，发射数据发送信号
 */
bool ConnectionManager::sendData(const QByteArray& data)
{
    QMutexLocker locker(&m_serialMutex);
    
    // 检查连接状态
    if (!isConnected()) {
        m_sendErrors++;
        qWarning() << "发送数据失败: 串口未连接";
        return false;
    }
    
    // 检查数据有效性
    if (data.isEmpty()) {
        qWarning() << "发送数据失败: 数据为空";
        return false;
    }
    
    // 检查是否需要帧间延迟
    if (m_config.serialConfig.interFrameDelay > 0 &&
            !m_sendBuffer.isEmpty()) {
        QThread::msleep(m_config.serialConfig.interFrameDelay);
    }

    
    // 尝试发送数据
    qint64 bytesWritten = m_serialPort->write(data);
    
    // 检查发送结果
    if (bytesWritten == -1) {
        m_sendErrors++;
        qCritical() << "发送数据失败:" << m_serialPort->errorString();
        return false;
    }
    
    if (bytesWritten != data.size()) {
        m_sendErrors++;
        qWarning() << "发送数据不完整，期望:" << data.size()
                   << "实际:" << bytesWritten;
        return false;
    }
    
    // 刷新缓冲区确保数据发送完成
    if (!m_serialPort->flush()) {
        qWarning() << "刷新串口缓冲区失败";
    }
    
    // 更新发送统计信息
    m_bytesSent += bytesWritten;
    
    qDebug() << "发送数据:" << data.toHex(' ') << "大小:" << bytesWritten;
    
    // 发射数据发送信号
    emit dataSent(data);
    
    return true;
}

/**
 * @brief 心跳包响应超时处理
 *
 * 当心跳包响应超时定时器触发时调用
 * 增加心跳包超时计数，检查是否达到最大超时次数
 * 如果达到最大超时次数，认为连接已断开，触发重连机制
 */
void ConnectionManager::onHeartbeatResponseTimeout()
{
    QMutexLocker locker(&m_serialMutex);
    
    qWarning() << "心跳包响应超时，超时计数:" << m_heartbeatTimeoutCount + 1;
    
    // 增加超时计数
    m_heartbeatTimeoutCount++;
    
    // 检查是否达到最大超时次数
    if (m_heartbeatTimeoutCount >= m_maxHeartbeatTimeouts) {
        qCritical() << "心跳包连续超时" << m_heartbeatTimeoutCount << "次，认为连接已断开";
        
        // 重置超时计数
        m_heartbeatTimeoutCount = 0;
        
        // 更新连接状态为错误
        m_state = ConnectionState::Error;
        emit stateChanged(m_state);
        
        // 停止心跳包定时器
        m_keepAliveTimer->stop();
        
        // 关闭串口
        if (m_serialPort && m_serialPort->isOpen()) {
            m_serialPort->close();
        }
        
        // 根据重连策略启动重连
        if (m_config.reconnectPolicy != NoReconnect) {
            int delay = calculateReconnectDelay();
            qDebug() << "启动重连，延迟时间:" << delay << "毫秒";
            m_reconnectTimer->start(delay);
        }
        
        emit connectionError("心跳包响应超时，连接已断开");
    } else {
        qDebug() << "心跳包超时，当前计数:" << m_heartbeatTimeoutCount << "/" << m_maxHeartbeatTimeouts;
        
        // 重置心跳包确认状态，等待下一次心跳包
        m_heartbeatAcknowledged = false;
    }
}

/**
 * @brief 从串口读取数据
 * @param timeout 读取超时时间（毫秒），默认100ms
 * @return 读取到的数据字节数组
 *
 * 阻塞式读取方法，等待数据到达或超时
 * 使用定时器控制读取超时，避免无限等待
 * 读取到数据后更新统计信息并发射数据接收信号
 */
QByteArray ConnectionManager::readData(int timeout)
{
    QMutexLocker locker(&m_serialMutex);
    
    // 检查连接状态
    if (!isConnected()) {
        qWarning() << "读取数据失败: 串口未连接";
        return QByteArray();
    }
    
    // 启动超时定时器
    QElapsedTimer timer;
    timer.start();
    
    // 循环等待数据到达
    while (timer.elapsed() < timeout) {
        // 等待数据可读（10ms超时）
        if (m_serialPort->waitForReadyRead(10)) {
            QByteArray data = m_serialPort->readAll();
            //测试
            if (m_serialPort->error() != QSerialPort::NoError) {
                qDebug() << "Read error:" << m_serialPort->errorString();
            }
            if (!data.isEmpty()) {
                // 更新接收统计信息
                m_bytesReceived += data.size();
                m_receiveBuffer.append(data);
                
                qDebug() << "读取到数据:" << data.toHex(' ') << "大小:" << data.size();
                
                // 发射数据接收信号
                emit dataReceived(data);
                
                return data;
            }
        }
        
        // 短暂休眠避免CPU占用过高
        QThread::msleep(1);
    }
    
    // 超时处理
    qWarning() << "读取数据超时，超时时间:" << timeout << "ms";
    return QByteArray();
}

/**
 * @brief 刷新串口缓冲区
 *
 * 线程安全的缓冲区刷新方法
 * 清空串口的发送和接收硬件缓冲区
 * 用于确保所有待发送数据都已发送完成
 */
void ConnectionManager::flush()
{
    QMutexLocker locker(&m_serialMutex);
    
    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->flush();
    }
}

/**
 * @brief 清空内部缓冲区
 *
 * 清空内部的数据缓冲区，但不影响串口硬件缓冲区
 * 用于重新开始数据采集或处理异常情况
 * 注意：此方法不需要互斥锁保护，因为调用者应确保线程安全
 */
void ConnectionManager::clearBuffers()
{
    //    QMutexLocker locker(&m_serialMutex);
    
    // 清空串口硬件缓冲区
    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->clear(QSerialPort::Input | QSerialPort::Output);
    }
    
    // 清空内部软件缓冲区
    m_receiveBuffer.clear();
    m_sendBuffer.clear();
}

/**
 * @brief 设置连接配置
 * @param config 新的连接配置参数
 *
 * 动态更新连接配置，如果当前已连接且配置有变化，会先断开连接再应用新配置
 * 配置更改后需要重新连接才能生效
 * 线程安全的方法，使用互斥锁保护配置更新操作
 */
void ConnectionManager::setConfig(const ConnectionConfig& config)
{
    QMutexLocker locker(&m_serialMutex);
    
    bool needReconnect = false;
    
    // 检查串口配置是否改变
    if (m_config.serialConfig!= config.serialConfig) {
        needReconnect = true;
    }
    
    // 更新配置
    m_config = config;
    
    // 如果配置改变且当前已连接，需要重新连接
    if (needReconnect && isConnected()) {
        // 重新连接
        disconnect();
        connect(m_config);
    }
}

QStringList ConnectionManager::availablePorts() const
{
    QStringList ports;
    
    for (const QSerialPortInfo& info : QSerialPortInfo::availablePorts()) {
        ports.append(info.portName());
    }
    
    return ports;
}

bool ConnectionManager::detectOptimalSettings(const QString& portName, SerialPortConfig& detectedConfig)
{
    // 简单的波特率检测
    QList<qint32> baudRates = {9600, 19200, 38400, 57600, 115200};
    QList<QSerialPort::Parity> parities = {
        QSerialPort::NoParity,
        QSerialPort::EvenParity,
        QSerialPort::OddParity
    };
    
    for (qint32 baudRate : baudRates) {
        for (QSerialPort::Parity parity : parities) {
            QSerialPort testPort;
            testPort.setPortName(portName);
            testPort.setBaudRate(baudRate);
            testPort.setDataBits(QSerialPort::Data8);
            testPort.setParity(parity);
            testPort.setStopBits(QSerialPort::OneStop);
            testPort.setFlowControl(QSerialPort::NoFlowControl);
            
            if (testPort.open(QIODevice::ReadWrite)) {
                // 发送一个简单的Modbus查询（从站1，读取保持寄存器0，数量1）
                QByteArray testRequest;
                testRequest.append(static_cast<char>(0x01)); // 从站地址
                testRequest.append(static_cast<char>(0x03)); // 功能码：读取保持寄存器
                testRequest.append(static_cast<char>(0x00)); // 起始地址高字节
                testRequest.append(static_cast<char>(0x00)); // 起始地址低字节
                testRequest.append(static_cast<char>(0x00)); // 寄存器数量高字节
                testRequest.append(static_cast<char>(0x01)); // 寄存器数量低字节
                
                // 计算CRC
                quint16 crc = calculateCRC(testRequest);
                testRequest.append(static_cast<char>(crc & 0xFF));
                testRequest.append(static_cast<char>((crc >> 8) & 0xFF));
                
                testPort.write(testRequest);
                testPort.flush();
                testPort.waitForBytesWritten(100);
                
                // 等待响应
                if (testPort.waitForReadyRead(200)) {
                    QByteArray response = testPort.readAll();
                    
                    if (response.size() >= 5) {
                        // 验证响应
                        if (validateModbusFrame(response)) {
                            detectedConfig.portName = portName;
                            detectedConfig.baudRate = baudRate;
                            detectedConfig.dataBits = QSerialPort::Data8;
                            detectedConfig.parity = parity;
                            detectedConfig.stopBits = QSerialPort::OneStop;
                            detectedConfig.flowControl = QSerialPort::NoFlowControl;
                            detectedConfig.interFrameDelay = 5;
                            detectedConfig.responseTimeout = 1000;
                            detectedConfig.maxRetries = 3;
                            
                            testPort.close();
                            return true;
                        }
                    }
                }
                
                testPort.close();
            }
        }
    }
    
    return false;
}

bool ConnectionManager::performHealthCheck()
{
    if (!isConnected()) {
        return false;
    }
    
    // 发送一个简单的测试请求
    QByteArray testRequest;
    testRequest.append(static_cast<char>(0x01)); // 从站地址
    testRequest.append(static_cast<char>(0x03)); // 功能码：读取保持寄存器
    testRequest.append(static_cast<char>(0x00)); // 起始地址高字节
    testRequest.append(static_cast<char>(0x00)); // 起始地址低字节
    testRequest.append(static_cast<char>(0x00)); // 寄存器数量高字节
    testRequest.append(static_cast<char>(0x01)); // 寄存器数量低字节
    
    quint16 crc = calculateCRC(testRequest);
    testRequest.append(static_cast<char>(crc & 0xFF));
    testRequest.append(static_cast<char>((crc >> 8) & 0xFF));
    
    // 发送测试请求
    if (!sendData(testRequest)) {
        return false;
    }
    
    // 等待响应
    QElapsedTimer timer;
    timer.start();
    
    while (timer.elapsed() < 1000) {
        if (m_serialPort->waitForReadyRead(10)) {
            QByteArray response = m_serialPort->readAll();
            if (!response.isEmpty() && validateModbusFrame(response)) {
                return true;
            }
        }
    }
    
    return false;
}

float ConnectionManager::signalQuality() const
{
    if (m_bytesSent + m_bytesReceived == 0) {
        return 100.0f; // 没有数据传输，认为质量好
    }
    
    float errorRate = static_cast<float>(m_sendErrors + m_receiveErrors) /
            (m_bytesSent + m_bytesReceived);
    
    // 错误率越低，信号质量越好
    return 100.0f * (1.0f - qMin(errorRate, 1.0f));
}

void ConnectionManager::onReadyRead()
{
    QMutexLocker locker(&m_serialMutex);
    
    if (!m_serialPort || !m_serialPort->isOpen()) {
        return;
    }
    
    QByteArray data = m_serialPort->readAll();
    //测试
    if (m_serialPort->error() != QSerialPort::NoError) {
        qDebug() << "Read error2:" << m_serialPort->errorString();
    }
    if (!data.isEmpty()) {
        m_bytesReceived += data.size();
        m_receiveBuffer.append(data);
        
        LOG_INFO() << __FUNCTION__ << "接收到数据:" << data.toHex(' ') << "大小:" << data.size();
        
        // 检测是否为心跳包响应
        if (m_heartbeatResponseTimer->isActive() && isHeartbeatResponse(data)) {
            qDebug() << "收到心跳包响应，重置心跳包状态";
            m_heartbeatResponseTimer->stop();
            m_heartbeatAcknowledged = true;
            m_heartbeatTimeoutCount = 0; // 重置超时计数
        }
        
        emit dataReceived(data);
    }
}

//串口serialport报错槽函数
void ConnectionManager::onError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }
    
    QString errorStr;
    
    switch (error) {
    case QSerialPort::DeviceNotFoundError:
        errorStr = "设备未找到";
        break;
    case QSerialPort::PermissionError:
        errorStr = "权限不足";
        break;
    case QSerialPort::OpenError:
        errorStr = "打开设备失败";
        break;
    case QSerialPort::ParityError:
        errorStr = "奇偶校验错误";
        break;
    case QSerialPort::FramingError:
        errorStr = "帧错误";
        break;
    case QSerialPort::BreakConditionError:
        errorStr = "中断条件错误";
        break;
    case QSerialPort::WriteError:
        errorStr = "写入错误";
        m_sendErrors++;
        break;
    case QSerialPort::ReadError:
        errorStr = "读取错误";
        m_receiveErrors++;
        break;
    case QSerialPort::ResourceError:
        errorStr = "资源错误（设备被拔出）";
        break;
    case QSerialPort::UnsupportedOperationError:
        errorStr = "不支持的操作";
        break;
    case QSerialPort::UnknownError:
        errorStr = "未知错误";
        break;
    case QSerialPort::TimeoutError:
        errorStr = "超时错误";
        break;
    case QSerialPort::NotOpenError:
        errorStr = "设备未打开";
        break;
    default:
        errorStr = QString("错误代码: %1").arg(error);
        break;
    }
    
    m_errorString = errorStr;
    qCritical() << "串口错误:" << errorStr << m_serialPort->errorString();
    
    // 如果是严重错误，断开连接
    if (error == QSerialPort::ResourceError ||
            error == QSerialPort::DeviceNotFoundError ||
            error == QSerialPort::PermissionError) {
        
        m_state = ConnectionState::Error;
        emit stateChanged(m_state);
        
        // 启动重连
        if (m_config.reconnectPolicy != NoReconnect) {
            int delay = calculateReconnectDelay();
            m_reconnectTimer->start(delay);
        }
    }
    
    emit connectionError(errorStr);
}

void ConnectionManager::onReconnectTimeout()
{
    if (m_state != ConnectionState::Connected) {
        reconnect();
    }
}

void ConnectionManager::onKeepAliveTimeout()
{
    // 心跳包不再直接调用 sendData 发送原始数据，避免绕过 ModbusWorker 请求队列
    // 污染 m_responseBuffer 导致正在等待响应的请求解析失败。
    // 由上层业务（TaskScheduler）通过正常任务队列发送心跳请求。
    // 此处仅发出信号通知上层心跳定时器触发。
    if (isConnected()) {
        emit keepAliveReceived();
    }
}

quint16 ConnectionManager::calculateCRC(const QByteArray& data)
{
    quint16 crc = 0xFFFF;
    
    for (int i = 0; i < data.length(); ++i) {
        crc ^= static_cast<quint8>(data.at(i));
        
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc;
}

bool ConnectionManager::verifyCRC(const QByteArray& data)
{
    if (data.length() < 2) {
        return false;
    }
    
    QByteArray dataWithoutCRC = data.left(data.length() - 2);
    quint16 receivedCRC = static_cast<quint8>(data.at(data.length() - 2)) |
            (static_cast<quint8>(data.at(data.length() - 1)) << 8);
    quint16 calculatedCRC = calculateCRC(dataWithoutCRC);
    
    return receivedCRC == calculatedCRC;
}

int ConnectionManager::calculateReconnectDelay()
{
    switch (m_config.reconnectPolicy) {
    case ImmediateReconnect:
        return 0;
    case IncrementalReconnect:
        return qMin(m_reconnectAttempts * 1000, 10000); // 最大10秒
    case ExponentialBackoff:
        return qMin(static_cast<int>(qPow(2, m_reconnectAttempts)) * 1000, 30000); // 最大30秒
    default:
        return m_config.reconnectInterval;
    }
}

bool ConnectionManager::validateModbusFrame(const QByteArray& data)
{
    // 检查数据帧长度
    if (data.size() < 4) {
        return false;
    }
    
    // 验证CRC校验码
    if (!verifyCRC(data)) {
        return false;
    }
    
    // 检查从站地址（0-247）
    quint8 slaveAddress = static_cast<quint8>(data[0]);
    if (slaveAddress > 247) {
        return false;
    }
    
    // 检查功能码（1-127）
    quint8 functionCode = static_cast<quint8>(data[1]);
    if (functionCode < 1 || functionCode > 127) {
        return false;
    }
    
    return true;
}

/**
 * @brief 检测是否为心跳包响应
 * @param data 接收到的数据帧
 * @return 是否为心跳包响应
 *
 * 检测接收到的数据是否为心跳包查询的响应
 * 心跳包查询：从站1，功能码0x03，读取保持寄存器0，数量1，TODO这个后续可以根据实际开发和需求再定具体要发什么消息就行
 * 响应格式：从站地址，功能码，字节数，寄存器值，CRC
 */
bool ConnectionManager::isHeartbeatResponse(const QByteArray& data)
{
    // 检查数据帧长度（心跳包响应应为7字节：地址+功能码+字节数+2字节寄存器值+2字节CRC）
    if (data.size() != 7) {
        return false;
    }
    
    // 验证CRC校验码
    if (!verifyCRC(data)) {
        return false;
    }
    
    // 检查从站地址（心跳包查询使用从站1）
    quint8 slaveAddress = static_cast<quint8>(data[0]);
    if (slaveAddress != 1) {
        return false;
    }
    
    // 检查功能码（0x03为读取保持寄存器）
    quint8 functionCode = static_cast<quint8>(data[1]);
    if (functionCode != 0x03) {
        return false;
    }
    
    // 检查字节数（应为2字节）
    quint8 byteCount = static_cast<quint8>(data[2]);
    if (byteCount != 2) {
        return false;
    }
    
    return true;
}
