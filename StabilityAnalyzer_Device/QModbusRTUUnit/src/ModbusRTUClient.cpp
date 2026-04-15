#include "ModbusRTUClient.h"
#include <QCoreApplication>
#include <QThread>
#include <QDebug>

/**
 * @brief ModbusRtuClient构造函数
 * @param parent 父对象指针
 * 
 * 初始化串口对象、定时器对象和默认配置参数
 * 设置响应超时定时器和帧间延迟定时器为单次触发模式
 * 连接串口数据接收信号到onReadyRead槽函数
 */
ModbusRtuClient::ModbusRtuClient(QObject *parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
    , m_responseTimer(new QTimer(this))
    , m_interFrameTimer(new QTimer(this))
    , m_responseTimeout(1000)
    , m_interFrameDelay(5)
    , m_retryCount(3)
    , m_isConnected(false)
{
    // 设置定时器为单次触发模式
    m_responseTimer->setSingleShot(true);
    m_interFrameTimer->setSingleShot(true);

    // 连接串口数据接收信号
    QObject::connect(m_serialPort, &QSerialPort::readyRead, this, &ModbusRtuClient::onReadyRead);

    // 响应超时定时器超时处理
    QObject::connect(m_responseTimer, &QTimer::timeout, [this]() {
        if (m_responseTimer->isActive()) {
            m_responseTimer->stop();
        }
    });
}

/**
 * @brief ModbusRtuClient析构函数
 * 
 * 自动断开连接并清理资源
 */
ModbusRtuClient::~ModbusRtuClient()
{
    disconnect();
}

/**
 * @brief 连接到Modbus设备
 * @param config 串口配置参数
 * @return 连接是否成功
 * 
 * 使用互斥锁保护串口操作，确保线程安全
 * 如果已连接，先断开现有连接
 * 配置串口参数并尝试打开串口
 * 成功时清空串口缓冲区并发出connected信号
 */
bool ModbusRtuClient::connect(const SerialConfig &config)
{
    QMutexLocker locker(&m_serialMutex);

    // 如果已连接，先断开连接
    if (m_isConnected) {
        disconnect();
    }

    // 配置串口参数
    m_serialPort->setPortName(config.portName);
    m_serialPort->setBaudRate(config.baudRate);
    m_serialPort->setDataBits(config.dataBits);
    m_serialPort->setParity(config.parity);
    m_serialPort->setStopBits(config.stopBits);
    m_serialPort->setFlowControl(config.flowControl);

    // 尝试打开串口
    if (m_serialPort->open(QIODevice::ReadWrite)) {
        m_isConnected = true;
        m_serialPort->clear();  // 清空串口缓冲区
        emit connected();       // 发出连接成功信号
        return true;
    } else {
        // 连接失败，发出错误信号
        emit errorOccurred(QString("无法打开串口 %1: %2")
                          .arg(config.portName)
                          .arg(m_serialPort->errorString()));
        return false;
    }
}

/**
 * @brief 断开与Modbus设备的连接
 * 
 * 使用互斥锁保护串口操作，确保线程安全
 * 关闭串口，清空接收缓冲区，发出断开连接信号
 */
void ModbusRtuClient::disconnect()
{
    QMutexLocker locker(&m_serialMutex);

    // 关闭串口
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }

    // 重置状态
    m_isConnected = false;
    m_receiveBuffer.clear();  // 清空接收缓冲区
    emit disconnected();      // 发出断开连接信号
}

/**
 * @brief 检查是否已连接到设备
 * @return 连接状态
 */
bool ModbusRtuClient::isConnected() const
{
    return m_isConnected;
}

/**
 * @brief 串口数据接收槽函数
 * 
 * 当串口有数据可读时被调用
 * 使用互斥锁保护接收缓冲区操作
 * 将接收到的数据追加到接收缓冲区
 */
void ModbusRtuClient::onReadyRead()
{
    QMutexLocker locker(&m_serialMutex);
    m_receiveBuffer.append(m_serialPort->readAll());
}

/**
 * @brief 计算CRC校验码
 * @param data 要计算CRC的数据
 * @return CRC校验码
 * 
 * 使用Modbus RTU标准的CRC-16算法（多项式0xA001）
 * 算法步骤：
 * 1. 初始化CRC为0xFFFF
 * 2. 对每个字节进行异或操作
 * 3. 对每个位进行移位和异或操作
 */
quint16 ModbusRtuClient::calculateCRC(const QByteArray &data)
{
    quint16 crc = 0xFFFF;

    // 遍历每个字节
    for (int i = 0; i < data.length(); ++i) {
        crc ^= (quint8)data.at(i);

        // 对每个位进行处理
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;  // 多项式0xA001
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

/**
 * @brief 验证CRC校验码
 * @param data 包含CRC的数据帧
 * @return CRC验证是否通过
 * 
 * 验证步骤：
 * 1. 检查数据长度是否足够（至少2字节）
 * 2. 提取接收到的CRC校验码
 * 3. 计算数据的CRC校验码
 * 4. 比较两个CRC是否一致
 */
bool ModbusRtuClient::verifyCRC(const QByteArray &data)
{
    // 检查数据长度
    if (data.length() < 2) return false;

    // 提取数据部分（去除CRC）
    QByteArray dataWithoutCRC = data.left(data.length() - 2);
    
    // 提取接收到的CRC（小端字节序）
    quint16 receivedCRC = (quint8)data.at(data.length() - 2) |
                         ((quint8)data.at(data.length() - 1) << 8);
    
    // 计算CRC
    quint16 calculatedCRC = calculateCRC(dataWithoutCRC);

    // 比较CRC
    return receivedCRC == calculatedCRC;
}

/**
 * @brief 构建Modbus请求帧
 * @param slaveId 从站地址
 * @param functionCode 功能码
 * @param data 请求数据
 * @return 完整的Modbus请求帧（包含CRC）
 * 
 * 帧结构：
 * [从站地址][功能码][数据][CRC低字节][CRC高字节]
 */
QByteArray ModbusRtuClient::buildRequest(int slaveId, ModbusFunctionCode functionCode,
                                        const QByteArray &data)
{
    QByteArray request;
    
    // 构建基本帧：从站地址 + 功能码 + 数据
    request.append((char)slaveId);
    request.append((char)functionCode);
    request.append(data);

    // 计算并添加CRC校验码
    quint16 crc = calculateCRC(request);
    request.append((char)(crc & 0xFF));   // CRC低字节
    request.append((char)((crc >> 8) & 0xFF));  // CRC高字节

    return request;
}

/**
 * @brief 发送Modbus请求
 * @param slaveId 从站地址
 * @param functionCode 功能码
 * @param data 请求数据
 * @param exception 异常信息输出参数（可选）
 * @return 发送的请求帧数据
 * 
 * 发送步骤：
 * 1. 检查连接状态
 * 2. 构建完整的Modbus请求帧
 * 3. 等待帧间延迟（如果需要）
 * 4. 清空接收缓冲区
 * 5. 发送请求数据
 * 6. 刷新串口缓冲区
 * 7. 发出请求发送信号
 */
QByteArray ModbusRtuClient::sendRequest(int slaveId, ModbusFunctionCode functionCode,
                                       const QByteArray &data, ModbusException *exception)
{
    // 检查连接状态
    if (!m_isConnected) {
        if (exception) *exception = PORT_ERROR;
        return QByteArray();
    }

    // 构建请求帧
    QByteArray request = buildRequest(slaveId, functionCode, data);

    // 确保帧间延迟（避免连续发送导致数据冲突）
    if (m_interFrameDelay > 0) {
        m_interFrameTimer->start(m_interFrameDelay);
        while (m_interFrameTimer->isActive()) {
            QCoreApplication::processEvents();  // 处理事件循环
        }
    }

    // 清空接收缓冲区，准备接收新响应
    m_receiveBuffer.clear();

    // 发送请求
    qint64 bytesWritten = m_serialPort->write(request);
    if (bytesWritten != request.size()) {
        if (exception) *exception = PORT_ERROR;
        return QByteArray();
    }

    // 刷新串口缓冲区，确保数据发送完成
    m_serialPort->flush();
    
    // 发出请求发送信号
    emit requestSent(slaveId, functionCode);

    return request;
}

QByteArray ModbusRtuClient::readResponse(int slaveId, ModbusFunctionCode functionCode,
                                        ModbusException *exception)
{
    // 等待响应
    m_responseTimer->start(m_responseTimeout);
    while (m_responseTimer->isActive()) {
        QCoreApplication::processEvents();

        if (m_receiveBuffer.length() >= 5) { // 最小响应长度
            // 检查地址和功能码是否匹配
            if ((quint8)m_receiveBuffer.at(0) != (quint8)slaveId) {
                continue;
            }

            quint8 receivedFunctionCode = (quint8)m_receiveBuffer.at(1);

            // 检查是否异常响应
            if (receivedFunctionCode & 0x80) {
                if (m_receiveBuffer.length() >= 5) {
                    if (verifyCRC(m_receiveBuffer)) {
                        quint8 errorCode = (quint8)m_receiveBuffer.at(2);
                        if (exception) *exception = (ModbusException)errorCode;
                        return QByteArray();
                    }
                }
                continue;
            }

            // 正常响应
            if (receivedFunctionCode == functionCode) {
                // 根据功能码确定完整响应长度
                int expectedLength = 0;
                switch (functionCode) {
                case ReadCoils:
                case ReadDiscreteInputs:
                case ReadHoldingRegisters:
                case ReadInputRegisters:
                    if (m_receiveBuffer.length() >= 3) {
                        quint8 byteCount = (quint8)m_receiveBuffer.at(2);
                        expectedLength = 3 + byteCount + 2; // 地址+功能码+字节数+数据+CRC
                    }
                    break;
                case WriteSingleCoil:
                case WriteSingleRegister:
                    expectedLength = 8;
                    break;
                case WriteMultipleCoils:
                case WriteMultipleRegisters:
                    expectedLength = 8;
                    break;
                }

                if (expectedLength > 0 && m_receiveBuffer.length() >= expectedLength) {
                    if (verifyCRC(m_receiveBuffer.left(expectedLength))) {
                        m_responseTimer->stop();
                        QByteArray response = m_receiveBuffer.left(expectedLength);
                        m_receiveBuffer.remove(0, expectedLength);
                        emit responseReceived(slaveId, functionCode);
                        if (exception) *exception = NoException;
                        return response;
                    }
                }
            }
        }
    }

    if (exception) *exception = TIMEOUT_ERROR;
    return QByteArray();
}

QVector<quint16> ModbusRtuClient::readHoldingRegisters(int slaveId, int startAddr,
                                                      int quantity, ModbusException *exception)
{
    QMutexLocker locker(&m_serialMutex);

    if (quantity < 1 || quantity > 125) {
        if (exception) *exception = ILLEGAL_DATA_VALUE;
        return QVector<quint16>();
    }

    QByteArray data;
    data.append((char)((startAddr >> 8) & 0xFF));
    data.append((char)(startAddr & 0xFF));
    data.append((char)((quantity >> 8) & 0xFF));
    data.append((char)(quantity & 0xFF));

    ModbusException localException = NoException;
    QByteArray response;

    for (int retry = 0; retry < m_retryCount; ++retry) {
        sendRequest(slaveId, ReadHoldingRegisters, data, &localException);
        if (localException != NoException) continue;

        response = readResponse(slaveId, ReadHoldingRegisters, &localException);
        if (localException == NoException && response.length() > 0) {
            break;
        }
    }

    if (exception) *exception = localException;

    if (localException == NoException && response.length() >= 5) {
        quint8 byteCount = (quint8)response.at(2);
        if (byteCount == quantity * 2) {
            QVector<quint16> registers;
            for (int i = 0; i < quantity; ++i) {
                quint16 value = ((quint8)response.at(3 + i * 2) << 8) |
                               (quint8)response.at(4 + i * 2);
                registers.append(value);
            }
            return registers;
        }
    }

    return QVector<quint16>();
}

QVector<quint16> ModbusRtuClient::readInputRegisters(int slaveId, int startAddr,
                                                    int quantity, ModbusException *exception)
{
    QMutexLocker locker(&m_serialMutex);

    if (quantity < 1 || quantity > 125) {
        if (exception) *exception = ILLEGAL_DATA_VALUE;
        return QVector<quint16>();
    }

    QByteArray data;
    data.append((char)((startAddr >> 8) & 0xFF));
    data.append((char)(startAddr & 0xFF));
    data.append((char)((quantity >> 8) & 0xFF));
    data.append((char)(quantity & 0xFF));

    ModbusException localException = NO_EXCEPTION;
    QByteArray response;

    for (int retry = 0; retry < m_retryCount; ++retry) {
        sendRequest(slaveId, ReadInputRegisters, data, &localException);
        if (localException != NoException) continue;

        response = readResponse(slaveId, ReadInputRegisters, &localException);
        if (localException == NoException && response.length() > 0) {
            break;
        }
    }

    if (exception) *exception = localException;

    if (localException == NoException && response.length() >= 5) {
        quint8 byteCount = (quint8)response.at(2);
        if (byteCount == quantity * 2) {
            QVector<quint16> registers;
            for (int i = 0; i < quantity; ++i) {
                quint16 value = ((quint8)response.at(3 + i * 2) << 8) |
                               (quint8)response.at(4 + i * 2);
                registers.append(value);
            }
            return registers;
        }
    }

    return QVector<quint16>();
}

QVector<bool> ModbusRtuClient::readCoils(int slaveId, int startAddr,
                                        int quantity, ModbusException *exception)
{
    QMutexLocker locker(&m_serialMutex);

    if (quantity < 1 || quantity > 2000) {
        if (exception) *exception = ILLEGAL_DATA_VALUE;
        return QVector<bool>();
    }

    QByteArray data;
    data.append((char)((startAddr >> 8) & 0xFF));
    data.append((char)(startAddr & 0xFF));
    data.append((char)((quantity >> 8) & 0xFF));
    data.append((char)(quantity & 0xFF));

    ModbusException localException = NO_EXCEPTION;
    QByteArray response;

    for (int retry = 0; retry < m_retryCount; ++retry) {
        sendRequest(slaveId, ReadCoils, data, &localException);
        if (localException != NoException) continue;

        response = readResponse(slaveId, ReadCoils, &localException);
        if (localException == NoException && response.length() > 0) {
            break;
        }
    }

    if (exception) *exception = localException;

    if (localException == NoException && response.length() >= 5) {
        quint8 byteCount = (quint8)response.at(2);
        if (byteCount == (quantity + 7) / 8) {
            return bytesToCoils(response.mid(3, byteCount), quantity);
        }
    }

    return QVector<bool>();
}

QVector<bool> ModbusRtuClient::readDiscreteInputs(int slaveId, int startAddr,
                                                 int quantity, ModbusException *exception)
{
    QMutexLocker locker(&m_serialMutex);

    if (quantity < 1 || quantity > 2000) {
        if (exception) *exception = ILLEGAL_DATA_VALUE;
        return QVector<bool>();
    }

    QByteArray data;
    data.append((char)((startAddr >> 8) & 0xFF));
    data.append((char)(startAddr & 0xFF));
    data.append((char)((quantity >> 8) & 0xFF));
    data.append((char)(quantity & 0xFF));

    ModbusException localException = NO_EXCEPTION;
    QByteArray response;

    for (int retry = 0; retry < m_retryCount; ++retry) {
        sendRequest(slaveId, ReadDiscreteInputs, data, &localException);
        if (localException != NoException) continue;

        response = readResponse(slaveId, ReadDiscreteInputs, &localException);
        if (localException == NoException && response.length() > 0) {
            break;
        }
    }

    if (exception) *exception = localException;

    if (localException == NO_EXCEPTION && response.length() >= 5) {
        quint8 byteCount = (quint8)response.at(2);
        if (byteCount == (quantity + 7) / 8) {
            return bytesToCoils(response.mid(3, byteCount), quantity);
        }
    }

    return QVector<bool>();
}

bool ModbusRtuClient::writeSingleRegister(int slaveId, int addr, quint16 value,
                                         ModbusException *exception)
{
    QMutexLocker locker(&m_serialMutex);

    QByteArray data;
    data.append((char)((addr >> 8) & 0xFF));
    data.append((char)(addr & 0xFF));
    data.append((char)((value >> 8) & 0xFF));
    data.append((char)(value & 0xFF));

    ModbusException localException = NO_EXCEPTION;
    QByteArray response;

    for (int retry = 0; retry < m_retryCount; ++retry) {
        sendRequest(slaveId, WriteSingleRegister, data, &localException);
        if (localException != NoException) continue;

        response = readResponse(slaveId, WriteSingleRegister, &localException);
        if (localException == NoException && response.length() > 0) {
            break;
        }
    }

    if (exception) *exception = localException;
    return localException == NoException;
}

bool ModbusRtuClient::writeMultipleRegisters(int slaveId, int startAddr,
                                            const QVector<quint16> &values,
                                            ModbusException *exception)
{
    QMutexLocker locker(&m_serialMutex);

    int quantity = values.size();
    if (quantity < 1 || quantity > 123) {
        if (exception) *exception = ILLEGAL_DATA_VALUE;
        return false;
    }

    QByteArray data;
    data.append((char)((startAddr >> 8) & 0xFF));
    data.append((char)(startAddr & 0xFF));
    data.append((char)((quantity >> 8) & 0xFF));
    data.append((char)(quantity & 0xFF));
    data.append((char)(quantity * 2));
    data.append(registersToBytes(values));

    ModbusException localException = NO_EXCEPTION;
    QByteArray response;

    for (int retry = 0; retry < m_retryCount; ++retry) {
        sendRequest(slaveId, WriteMultipleRegisters, data, &localException);
        if (localException != NoException) continue;

        response = readResponse(slaveId, WriteMultipleRegisters, &localException);
        if (localException == NoException && response.length() > 0) {
            break;
        }
    }

    if (exception) *exception = localException;
    return localException == NO_EXCEPTION;
}

bool ModbusRtuClient::writeSingleCoil(int slaveId, int addr, bool value,
                                     ModbusException *exception)
{
    QMutexLocker locker(&m_serialMutex);

    QByteArray data;
    data.append((char)((addr >> 8) & 0xFF));
    data.append((char)(addr & 0xFF));
    data.append(value ? (char)0xFF : (char)0x00);
    data.append((char)0x00);

    ModbusException localException = NO_EXCEPTION;
    QByteArray response;

    for (int retry = 0; retry < m_retryCount; ++retry) {
        sendRequest(slaveId, WriteSingleCoil, data, &localException);
        if (localException != NoException) continue;

        response = readResponse(slaveId, WriteSingleCoil, &localException);
        if (localException == NoException && response.length() > 0) {
            break;
        }
    }

    if (exception) *exception = localException;
    return localException == NO_EXCEPTION;
}

bool ModbusRtuClient::writeMultipleCoils(int slaveId, int startAddr,
                                        const QVector<bool> &values,
                                        ModbusException *exception)
{
    QMutexLocker locker(&m_serialMutex);

    int quantity = values.size();
    if (quantity < 1 || quantity > 1968) {
        if (exception) *exception = ILLEGAL_DATA_VALUE;
        return false;
    }

    QByteArray coilBytes = coilsToBytes(values);

    QByteArray data;
    data.append((char)((startAddr >> 8) & 0xFF));
    data.append((char)(startAddr & 0xFF));
    data.append((char)((quantity >> 8) & 0xFF));
    data.append((char)(quantity & 0xFF));
    data.append((char)coilBytes.size());
    data.append(coilBytes);

    ModbusException localException = NO_EXCEPTION;
    QByteArray response;

    for (int retry = 0; retry < m_retryCount; ++retry) {
        sendRequest(slaveId, WriteMultipleCoils, data, &localException);
        if (localException != NoException) continue;

        response = readResponse(slaveId, WriteMultipleCoils, &localException);
        if (localException == NoException && response.length() > 0) {
            break;
        }
    }

    if (exception) *exception = localException;
    return localException == NO_EXCEPTION;
}

QByteArray ModbusRtuClient::coilsToBytes(const QVector<bool> &coils)
{
    int byteCount = (coils.size() + 7) / 8;
    QByteArray bytes(byteCount, 0);

    for (int i = 0; i < coils.size(); ++i) {
        if (coils[i]) {
            int byteIndex = i / 8;
            int bitPosition = i % 8;
            bytes[byteIndex] = static_cast<char>(bytes[byteIndex] | (1 << bitPosition));        }
    }

    return bytes;
}

QVector<bool> ModbusRtuClient::bytesToCoils(const QByteArray &bytes, int count)
{
    QVector<bool> coils;
    coils.reserve(count);

    for (int i = 0; i < count; ++i) {
        int byteIndex = i / 8;
        int bitIndex = i % 8;

        if (byteIndex < bytes.size()) {
            coils.append((bytes[byteIndex] >> bitIndex) & 0x01);
        } else {
            coils.append(false);
        }
    }

    return coils;
}

QByteArray ModbusRtuClient::registersToBytes(const QVector<quint16> &registers)
{
    QByteArray bytes;
    bytes.reserve(registers.size() * 2);

    for (quint16 value : registers) {
        bytes.append((char)((value >> 8) & 0xFF));
        bytes.append((char)(value & 0xFF));
    }

    return bytes;
}

QVector<quint16> ModbusRtuClient::bytesToRegisters(const QByteArray &bytes)
{
    QVector<quint16> registers;

    if (bytes.size() % 2 != 0) {
        return registers;
    }

    for (int i = 0; i < bytes.size(); i += 2) {
        quint16 value = ((quint8)bytes.at(i) << 8) | (quint8)bytes.at(i + 1);
        registers.append(value);
    }

    return registers;
}

void ModbusRtuClient::setResponseTimeout(int milliseconds)
{
    m_responseTimeout = milliseconds;
}

void ModbusRtuClient::setInterFrameDelay(int milliseconds)
{
    m_interFrameDelay = milliseconds;
}

void ModbusRtuClient::setRetryCount(int count)
{
    m_retryCount = qMax(1, count);
}

// 异步操作实现
AsyncResult ModbusRtuClient::executeAsyncRead(int slaveId, ModbusFunctionCode functionCode,
                                             int startAddr, int quantity)
{
    AsyncResult result;
    result.success = false;

    switch (functionCode) {
    case ReadHoldingRegisters:
        result.data = readHoldingRegisters(slaveId, startAddr, quantity, &result.exception);
        result.success = (result.exception == NoException);
        break;
    case ReadInputRegisters:
        result.data = readInputRegisters(slaveId, startAddr, quantity, &result.exception);
        result.success = (result.exception == NoException);
        break;
    case ReadCoils: {
        QVector<bool> coils = readCoils(slaveId, startAddr, quantity, &result.exception);
        result.success = (result.exception == NoException);
        if (result.success) {
            // 将bool转换为quint16存储
            result.data.reserve(coils.size());
            for (bool coil : coils) {
                result.data.append(coil ? 1 : 0);
            }
        }
        break;
    }
    case ReadDiscreteInputs: {
        QVector<bool> inputs = readDiscreteInputs(slaveId, startAddr, quantity, &result.exception);
        result.success = (result.exception == NoException);
        if (result.success) {
            result.data.reserve(inputs.size());
            for (bool input : inputs) {
                result.data.append(input ? 1 : 0);
            }
        }
        break;
    }
    default:
        result.exception = ILLEGAL_FUNCTION;
        break;
    }

    if (!result.success) {
        switch (result.exception) {
        case TIMEOUT_ERROR:
            result.errorString = "响应超时";
            break;
        case CRC_ERROR:
            result.errorString = "CRC校验错误";
            break;
        case ILLEGAL_FUNCTION:
            result.errorString = "非法功能码";
            break;
        case ILLEGAL_DATA_ADDRESS:
            result.errorString = "非法数据地址";
            break;
        case ILLEGAL_DATA_VALUE:
            result.errorString = "非法数据值";
            break;
        default:
            result.errorString = "未知错误";
            break;
        }
    }

    return result;
}

AsyncResult ModbusRtuClient::executeAsyncWrite(int slaveId, ModbusFunctionCode functionCode,
                                              int startAddr, const QByteArray &data)
{
    AsyncResult result;
    result.success = false;

    switch (functionCode) {
    case WriteSingleRegister: {
        if (data.size() >= 2) {
            quint16 value = ((quint8)data.at(0) << 8) | (quint8)data.at(1);
            result.success = writeSingleRegister(slaveId, startAddr, value, &result.exception);
            if (result.success) {
                result.data.append(value);
            }
        }
        break;
    }
    case WriteMultipleRegisters: {
        QVector<quint16> registers = bytesToRegisters(data);
        result.success = writeMultipleRegisters(slaveId, startAddr, registers, &result.exception);
        if (result.success) {
            result.data = registers;
        }
        break;
    }
    case WriteSingleCoil: {
        bool value = (data.size() > 0 && data.at(0) != 0);
        result.success = writeSingleCoil(slaveId, startAddr, value, &result.exception);
        if (result.success) {
            result.data.append(value ? 1 : 0);
        }
        break;
    }
    case WriteMultipleCoils: {
        // 注意：这里data是字节数组，需要转换为bool数组
        // 实际调用时应该传入正确的参数
        break;
    }
    default:
        result.exception = ILLEGAL_FUNCTION;
        break;
    }

    if (!result.success) {
        switch (result.exception) {
        case TIMEOUT_ERROR:
            result.errorString = "响应超时";
            break;
        case CRC_ERROR:
            result.errorString = "CRC校验错误";
            break;
        default:
            result.errorString = "写操作失败";
            break;
        }
    }

    return result;
}

QFuture<AsyncResult> ModbusRtuClient::readHoldingRegistersAsync(int slaveId, int startAddr, int quantity)
{
    return QtConcurrent::run([this, slaveId, startAddr, quantity]() {
        return executeAsyncRead(slaveId, READ_HOLDING_REGISTERS, startAddr, quantity);
    });
}

QFuture<AsyncResult> ModbusRtuClient::readInputRegistersAsync(int slaveId, int startAddr, int quantity)
{
    return QtConcurrent::run([this, slaveId, startAddr, quantity]() {
        return executeAsyncRead(slaveId, READ_INPUT_REGISTERS, startAddr, quantity);
    });
}

QFuture<AsyncResult> ModbusRtuClient::readCoilsAsync(int slaveId, int startAddr, int quantity)
{
    return QtConcurrent::run([this, slaveId, startAddr, quantity]() {
        return executeAsyncRead(slaveId, READ_COILS, startAddr, quantity);
    });
}

QFuture<AsyncResult> ModbusRtuClient::readDiscreteInputsAsync(int slaveId, int startAddr, int quantity)
{
    return QtConcurrent::run([this, slaveId, startAddr, quantity]() {
        return executeAsyncRead(slaveId, READ_DISCRETE_INPUTS, startAddr, quantity);
    });
}

QFuture<AsyncResult> ModbusRtuClient::writeSingleRegisterAsync(int slaveId, int addr, quint16 value)
{
    QByteArray data;
    data.append((char)((value >> 8) & 0xFF));
    data.append((char)(value & 0xFF));

    return QtConcurrent::run([this, slaveId, addr, data]() {
        return executeAsyncWrite(slaveId, WRITE_SINGLE_REGISTER, addr, data);
    });
}

QFuture<AsyncResult> ModbusRtuClient::writeMultipleRegistersAsync(int slaveId, int startAddr,
                                                                 const QVector<quint16> &values)
{
    QByteArray data = registersToBytes(values);

    return QtConcurrent::run([this, slaveId, startAddr, data]() {
        return executeAsyncWrite(slaveId, WRITE_MULTIPLE_REGISTERS, startAddr, data);
    });
}

QFuture<AsyncResult> ModbusRtuClient::writeSingleCoilAsync(int slaveId, int addr, bool value)
{
    QByteArray data;
    data.append(value ? (char)0xFF : (char)0x00);

    return QtConcurrent::run([this, slaveId, addr, data]() {
        return executeAsyncWrite(slaveId, WRITE_SINGLE_COIL, addr, data);
    });
}

QFuture<AsyncResult> ModbusRtuClient::writeMultipleCoilsAsync(int slaveId, int startAddr,
                                                             const QVector<bool> &values)
{
    QByteArray data = coilsToBytes(values);

    return QtConcurrent::run([this, slaveId, startAddr, data]() {
        // 注意：这里需要传递线圈数量，但executeAsyncWrite目前不支持
        // 可以扩展executeAsyncWrite来处理这种情况
        AsyncResult result;
        result.success = false;
        result.exception = ILLEGAL_FUNCTION;
        result.errorString = "该功能暂未实现";
        return result;
    });
}
