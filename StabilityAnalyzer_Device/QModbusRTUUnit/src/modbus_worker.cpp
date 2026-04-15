#include "modbus_worker.h"
#include <QDebug>
#include <QCoreApplication>
#include <QMutexLocker>

ModbusWorker::ModbusWorker(QObject* parent)
    : QObject(parent)
    , m_connectionManager(nullptr)
    //    , m_workerThread(nullptr)
    , m_currentRequest(nullptr)
    , m_processTimer(nullptr)
    , m_responseTimer(nullptr)
    , m_isConnected(false)
    , m_isProcessing(false)
{
    qRegisterMetaType<QVector<quint16>>("QVector<quint16>");
    qRegisterMetaType<ModbusRequestPtr>("ModbusRequestPtr");
}

ModbusWorker::~ModbusWorker()
{
    shutdown();
}

bool ModbusWorker::initialize(const SerialPortConfig& serialConfig)
{
    // 创建连接管理器
    m_connectionManager = new ConnectionManager();
    
    // 连接信号槽
    connect(m_connectionManager, &ConnectionManager::dataReceived,
            this, &ModbusWorker::onDataReceived);
    connect(m_connectionManager, &ConnectionManager::connectionError,
            this, &ModbusWorker::onConnectionError);
    connect(m_connectionManager, &ConnectionManager::disconnected,
            this, &ModbusWorker::onDisconnected);
    connect(m_connectionManager, &ConnectionManager::connected,
            this, &ModbusWorker::onConnected);

    // 配置连接管理器
    ConnectionManager::ConnectionConfig config;
    config.serialConfig = serialConfig;
    m_connectionManager->setConfig(config);
    m_serialConfig = serialConfig;
    // 创建定时器
    m_processTimer = new QTimer(this);
    m_processTimer->setInterval(10); // 10ms处理间隔
    connect(m_processTimer, &QTimer::timeout, this, &ModbusWorker::processQueue);
    
    m_responseTimer = new QTimer(this);
    m_responseTimer->setSingleShot(true);
    connect(m_responseTimer, &QTimer::timeout, this, [this]() {
        if (m_currentRequest) {
            qWarning() << "请求超时:" << m_currentRequest->requestId();
            
            // 发送超时错误
            ModbusResult result;
            result.exception = TimeoutError;
            result.errorString = "请求超时";
            
            emit requestCompleted(m_currentRequest->requestId(), result);
            
            // 清理当前请求
            m_currentRequest.clear();
            m_responseBuffer.clear();
            m_isProcessing = false;
            
            updateCommStats(false);
        }
    });
    
    // 启动处理定时器
    m_processTimer->start();
    
    qDebug() << "ModbusWorker初始化成功";
    return true;
}

void ModbusWorker::shutdown()
{
    // 停止定时器
    if (m_processTimer) {
        m_processTimer->stop();
    }
    
    if (m_responseTimer) {
        m_responseTimer->stop();
    }
    
    // 断开连接
    disconnectDevice();
    
    // 清理队列
    clearQueue();
    
    // 清理连接管理器
    if (m_connectionManager) {
        delete m_connectionManager;
        m_connectionManager = nullptr;
    }
    
    qDebug() << "ModbusWorker 已关闭";
}

void ModbusWorker::doInitialize(const SerialPortConfig& serialConfig, bool* result)
{
    *result = initialize(serialConfig);
}

void ModbusWorker::doConnectDevice(bool* result)
{
    if (!result) {
        connectDevice();
        return;
    }

    if (!m_connectionManager) {
        m_isConnected = false;
        emit connectionStatusChanged(false);
        emit communicationError("连接管理器未初始化");
        *result = false;
        return;
    }

    ConnectionManager::ConnectionConfig config;
    config.serialConfig = m_serialConfig;

    qDebug() << "[ModbusWorker][Connect] port=" << m_serialConfig.portName
             << "baud=" << m_serialConfig.baudRate
             << "dataBits=" << m_serialConfig.dataBits
             << "stopBits=" << m_serialConfig.stopBits
             << "parity=" << m_serialConfig.parity;

    const bool ok = m_connectionManager->connect(config);
    m_isConnected = ok;
    emit connectionStatusChanged(ok);

    if (ok) {
        qDebug() << "Modbus设备连接成功";
        qDebug() << "[ModbusWorker][Connect] result=true"
                 << "cachedConnected=" << m_isConnected;
    } else {
        emit communicationError("设备连接失败");
        qWarning() << "Modbus设备连接失败";
        qWarning() << "[ModbusWorker][Connect] result=false"
                   << "cachedConnected=" << m_isConnected;
    }

    *result = ok;
}

void ModbusWorker::doGetIsConnected(bool* result)
{
    *result = isConnected();
}

void ModbusWorker::enqueueRequest(ModbusRequestPtr request)
{
    if (!request) {
        qWarning() << "尝试入队空请求";
        return;
    }
    
    QMutexLocker locker(&m_queueMutex);
    m_requestQueue.enqueue(request);
    // qDebug() << "[ModbusWorker][Enqueue] requestId=" << request->requestId()
    //          << "function=" << request->functionCode()
    //          << "slave=" << request->slaveId()
    //          << "queueSize=" << m_requestQueue.size()
    //          << "thread=" << QThread::currentThread();
    
    //qDebug() << "请求入队:" << request->requestId() << "功能码:" << request->functionCode();
}

void ModbusWorker::cancelRequest(qint64 requestId)
{
    QMutexLocker locker(&m_queueMutex);
    
    // 检查当前处理的请求
    if (m_currentRequest && m_currentRequest->requestId() == requestId) {
        m_responseTimer->stop();
        m_currentRequest.clear();
        m_responseBuffer.clear();
        m_isProcessing = false;
        
        qDebug() << "取消当前请求:" << requestId;
        return;
    }
    
    // 从队列中移除请求
    QQueue<QSharedPointer<ModbusRequest>> newQueue;
    while (!m_requestQueue.isEmpty()) {
        auto request = m_requestQueue.dequeue();
        if (request && request->requestId() != requestId) {
            newQueue.enqueue(request);
        }
    }
    m_requestQueue = newQueue;
    
    qDebug() << "取消队列请求:" << requestId;
}

void ModbusWorker::clearQueue()
{
    QMutexLocker locker(&m_queueMutex);
    m_requestQueue.clear();
    
    // 停止当前处理
    if (m_currentRequest) {
        m_responseTimer->stop();
        m_currentRequest.clear();
        m_responseBuffer.clear();
        m_isProcessing = false;
    }
    
    qDebug() << "清空请求队列";
}

bool ModbusWorker::isConnected() const
{
    return m_isConnected;
}

void ModbusWorker::connectDevice()
{
    bool ignored = false;
    doConnectDevice(&ignored);
}

void ModbusWorker::disconnectDevice()
{
    if (m_connectionManager) {
        m_connectionManager->disconnect();
    }
    
    m_isConnected = false;
    emit connectionStatusChanged(false);
    
    qDebug() << "Modbus设备已断开连接";
}

//队列处理，处理队列里的请求
void ModbusWorker::processQueue()
{
    if (m_isProcessing || !m_isConnected) {
        if (!m_isConnected) {
            // qDebug() << "[ModbusWorker][ProcessQueue] skip because disconnected"
            //          << "thread=" << QThread::currentThread();
        }
        return;
    }
    
    QMutexLocker locker(&m_queueMutex);
    if (m_requestQueue.isEmpty()) {
        return;
    }
    
    // 获取下一个请求
    m_currentRequest = m_requestQueue.dequeue();
    m_isProcessing = true;
    
    locker.unlock(); // 释放锁，避免阻塞

    if(!m_currentRequest){
        return;
    }

    // qDebug() << "[ModbusWorker][ProcessQueue] dequeued requestId=" << m_currentRequest->requestId()
    //          << "slave=" << m_currentRequest->slaveId()
    //          << "function=" << m_currentRequest->functionCode()
    //          << "remainingQueueSize=" << m_requestQueue.size()
    //          << "thread=" << QThread::currentThread();

    // 发送请求
    if (sendRequest(m_currentRequest)) {
        // 启动响应超时定时器
        m_responseTimer->start(m_requestTimeout);
        
        // Request frame details are already logged in sendRequest().
    } else {
        // 发送失败
        ModbusResult result;
        result.exception = SendError;
        result.errorString = "发送请求失败";
        
        emit requestCompleted(m_currentRequest->requestId(), result);
        
        m_currentRequest.clear();
        m_isProcessing = false;
        
        updateCommStats(false);
    }
}

//接收数据
void ModbusWorker::onDataReceived(const QByteArray& data)
{
    if (!m_isProcessing || !m_currentRequest) {
        return;
    }
    
    m_responseBuffer.append(data);
    
    // 检查是否接收到完整响应，包括帧的完整性判断，如果缓存区存在多个消息帧，则根据当前请求的信息去提取对应的帧消息
    if (detectAndProcessCompleteFrame()) {
        // 如果检测到完整帧并处理成功，清空缓冲区
        m_responseBuffer.clear();
    }
}

void ModbusWorker::onConnectionError(const QString& error)
{
    m_isConnected = false;
    emit connectionStatusChanged(false);
    emit communicationError(error);
    
    qWarning() << "连接错误:" << error;
}

//接收connectionmanager传递的断开信号断开连接
void ModbusWorker::onDisconnected()
{
    emit disconnected();
}

void ModbusWorker::onConnected()
{
    emit connected();
}

bool ModbusWorker::sendRequest(QSharedPointer<ModbusRequest> request)
{
    if (!m_connectionManager || !m_isConnected) {
        qWarning() << "[ModbusWorker][Send] skipped"
                   << "hasConnectionManager=" << (m_connectionManager != nullptr)
                   << "isConnected=" << m_isConnected;
        return false;
    }
    
    QByteArray requestData = request->buildRequestData();
    if (requestData.isEmpty()) {
        qWarning() << "构建请求数据失败";
        return false;
    }
    
    // 添加CRC校验
    // qDebug() << "[ModbusWorker][Send] requestId=" << request->requestId()
    //          << "slave=" << request->slaveId()
    //          << "function=" << request->functionCode()
    //          << "payloadNoCrc=" << requestData.toHex(' ');
    quint16 crc = calculateCRC(requestData);
    requestData.append(static_cast<char>(crc & 0xFF));
    requestData.append(static_cast<char>((crc >> 8) & 0xFF));
    
    // qDebug() << "[ModbusWorker][Send] requestId=" << request->requestId()
    //          << "frame=" << requestData.toHex(' ');
    return m_connectionManager->sendData(requestData);
}

//解析数据
void ModbusWorker::processResponse(const QByteArray& data)
{
    if (!m_currentRequest) {
        m_isProcessing = false;  //重置处理状态
        return;
    }
    
    qDebug() << "[ModbusWorker][Reply] requestId=" << m_currentRequest->requestId()
             << "slave=" << m_currentRequest->slaveId()
             << "function=" << m_currentRequest->functionCode()
             << "frame=" << data.toHex(' ');

    // 验证CRC
    if (!validateModbusCRC(data)) {
        qWarning() << "CRC校验失败，原始数据:" << data.toHex(' ');

        ModbusResult result;
        result.exception = CRCCheckError;
        result.errorString = "CRC校验失败";

        emit requestCompleted(m_currentRequest->requestId(), result);

        m_responseTimer->stop();
        m_currentRequest.clear();
        m_responseBuffer.clear();
        m_isProcessing = false;

        updateCommStats(false);
        return;
    }
    
    // 检查异常响应
    if (data.size() >= 5 && (data[1] & 0x80) != 0) {
        // 异常响应
        quint8 slaveId = static_cast<quint8>(data[0]);
        quint8 functionCode = static_cast<quint8>(data[1] & 0x7F);
        quint8 exceptionCode = static_cast<quint8>(data[2]);
        
        handleExceptionResponse(slaveId, functionCode, exceptionCode);
        //m_isProcessing = false;  //重置处理状态
        return;
    }
    
    // 创建响应对象并解析
    auto response = createResponse(m_currentRequest->functionCode());
    if (!response) {
        qWarning() << "创建响应对象失败";
        
        ModbusResult result;
        result.exception = InvalidResponse;
        result.errorString = "创建响应对象失败";
        
        emit requestCompleted(m_currentRequest->requestId(), result);
        
        m_responseTimer->stop();
        m_currentRequest.clear();
        m_responseBuffer.clear();
        m_isProcessing = false;
        
        updateCommStats(false);
        //m_isProcessing = false;  //重置处理状态
        return;
    }
    
    if (!response->parseFromRawData(data)) {
        qWarning() << "解析响应数据失败";
        
        ModbusResult result;
        result.exception = InvalidResponse;
        result.errorString = "解析响应数据失败";
        
        emit requestCompleted(m_currentRequest->requestId(), result);
        
        m_responseTimer->stop();
        m_currentRequest.clear();
        m_responseBuffer.clear();
        m_isProcessing = false;
        
        updateCommStats(false);
        m_isProcessing = false;  //重置处理状态
        return;
    }
    
    // 处理成功响应
    ModbusResult result;
    result.exception = response->getExceptionCode();
    result.success = true;
    
    // 根据功能码设置数据
    switch (m_currentRequest->functionCode()) {
    case ReadHoldingRegisters: {
        auto readResponse = dynamic_cast<ReadHoldingRegistersResponse*>(response.data());
        if (readResponse) {
            // 将寄存器值转换为ModbusValue
            QVector<quint16> registers = readResponse->getRegisters();
            for (quint16 value : registers) {
                //                ModbusValue modbusValue;
                //                modbusValue.value = value;
                //                modbusValue.scaledValue = value;
                //                modbusValue.timestamp = QDateTime::currentDateTime();
                //                modbusValue.isValid = true;
                result.values.append(value);
            }
            qDebug() << "[ModbusWorker][ReplyParsed] requestId=" << m_currentRequest->requestId()
                     << "registerCount=" << registers.size()
                     << "values=" << registers;
        }
        break;
    }
    case ReadInputRegisters: {
        auto readResponse = dynamic_cast<ReadInputRegistersResponse*>(response.data());
        if (readResponse) {
            QVector<quint16> registers = readResponse->getRegisters();
            for (quint16 value : registers) {
                result.values.append(value);
            }
            qDebug() << "[ModbusWorker][ReplyParsed] requestId=" << m_currentRequest->requestId()
                     << "registerCount=" << registers.size()
                     << "values=" << registers;
        }
        break;
    }
    case ReadCoils:
    case ReadDiscreteInputs: {
        auto readResponse = dynamic_cast<ReadCoilsResponse*>(response.data());
        if (readResponse) {
            // 将线圈值转换为ModbusValue
            QVector<bool> coils = readResponse->getCoils();
            for (bool value : coils) {
                //                ModbusValue modbusValue;
                //                modbusValue.value = value ? 1 : 0;
                //                modbusValue.scaledValue = value ? 1.0 : 0.0;
                //                modbusValue.timestamp = QDateTime::currentDateTime();
                //                modbusValue.isValid = true;
                result.values.append(value ? 1 : 0);
            }
        }
        break;
    }
    case WriteSingleCoil:
    case WriteSingleRegister:
    case WriteMultipleCoils:
    case WriteMultipleRegisters: {
        qDebug() << "[ModbusWorker][ReplyParsed] requestId=" << m_currentRequest->requestId()
                 << "writeAckFrame=" << data.toHex(' ');
        break;
    }
    default:
        break;
    }
    
    result.errorString = "";
    
    emit requestCompleted(m_currentRequest->requestId(), result);

    //在m_currentRequest.clear之前输出日志，否则会出现空指针的情况
    // qDebug() << "[ModbusWorker][Done] requestId=" << m_currentRequest->requestId()
    //          << "success=" << (result.exception == NoException)
    //          << "valueCount=" << result.values.size();

    m_responseTimer->stop();
    m_currentRequest.clear();
    m_responseBuffer.clear();
    m_isProcessing = false;
    
    updateCommStats(result.exception == NoException);
}

bool ModbusWorker::validateModbusCRC(const QByteArray& data)
{
    if (data.size() < 2) {
        return false;
    }

    // 计算CRC（排除最后两个字节的CRC值）
    QByteArray dataWithoutCRC = data.left(data.size() - 2);
    quint16 calculatedCRC = calculateCRC(dataWithoutCRC);
    
    // 获取接收到的CRC  低字节在前
    //    quint16 receivedCRC = static_cast<quint8>(data[data.size() - 2]) |
    //                         (static_cast<quint8>(data[data.size() - 1]) << 8);
    quint16 receivedCRC = (static_cast<quint8>(data[data.size() - 1]) << 8) |
            static_cast<quint8>(data[data.size() - 2]);
    return calculatedCRC == receivedCRC;
}

QSharedPointer<ModbusResponse> ModbusWorker::createResponse(ModbusFunctionCode functionCode)
{
    switch (functionCode) {
    case ReadHoldingRegisters:
        return QSharedPointer<ModbusResponse>(new ReadHoldingRegistersResponse());
    case ReadInputRegisters:
        return QSharedPointer<ModbusResponse>(new ReadInputRegistersResponse());
    case ReadCoils:
        return QSharedPointer<ModbusResponse>(new ReadCoilsResponse());
    case ReadDiscreteInputs:
        return QSharedPointer<ModbusResponse>(new ReadDiscreteInputsResponse());
    case WriteSingleCoil:
        return QSharedPointer<ModbusResponse>(new WriteSingleCoilResponse());
    case WriteSingleRegister:
        return QSharedPointer<ModbusResponse>(new WriteSingleRegisterResponse());
    case WriteMultipleCoils:
        return QSharedPointer<ModbusResponse>(new WriteMultipleCoilsResponse());
    case WriteMultipleRegisters:
        return QSharedPointer<ModbusResponse>(new WriteMultipleRegistersResponse());
    default:
        return nullptr;
    }
}

void ModbusWorker::handleExceptionResponse(quint8 slaveId, quint8 functionCode, quint8 exceptionCode)
{
    ModbusResult result;
    result.exception = static_cast<ModbusException>(exceptionCode);
    
    switch (exceptionCode) {
    case IllegalFunction:
        result.errorString = "非法功能码";
        break;
    case IllegalDataAddress:
        result.errorString = "非法数据地址";
        break;
    case IllegalDataValue:
        result.errorString = "非法数据值";
        break;
    case ServerDeviceFailure:
        result.errorString = "从站设备故障";
        break;
    default:
        result.errorString = "未知异常";
        break;
    }
    
    if (!m_currentRequest) {
        qWarning() << "handleExceptionResponse: m_currentRequest is null, ignore exception response";
        m_isProcessing = false;
        return;
    }

    emit requestCompleted(m_currentRequest->requestId(), result);
    
    m_responseTimer->stop();
    m_currentRequest.clear();
    m_responseBuffer.clear();
    m_isProcessing = false;
    
    updateCommStats(false);
    
    qWarning() << "异常响应:" << result.errorString;
}

void ModbusWorker::updateCommStats(bool success)
{
    m_commStats.totalRequests++;
    
    if (success) {
        m_commStats.successfulRequests++;
    } else {
        m_commStats.failedRequests++;
    }
}


//quint16 ModbusWorker::calculateCRC(const QByteArray& data)
//{
//    quint16 crc = 0xFFFF;
//    for (int i = 0; i < data.size(); ++i) {
//        crc ^= static_cast<quint8>(data.at(i));
//        for (int k = 0; k < 8; ++k)
//            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
//    }
//    // 只要跑的是这段代码，下面一行必定打印 b685
//    qDebug() << "CRC calc for" << data.toHex(' ') << '=' << crc;
//    return crc;
//}

quint16 ModbusWorker::calculateCRC(const QByteArray& data)
{
    quint16 crc = 0xFFFF;

    for (int i = 0; i < data.size(); ++i) {
        quint8 byte = static_cast<quint8>(data.at(i));

        crc ^= byte;

        for (int j = 0; j < 8; ++j) {
            bool lsb = crc & 0x0001;
            crc >>= 1;
            if (lsb) {
                crc ^= 0xA001;
            }
        }
    }

    return crc;
}

// 检测并处理完整的Modbus RTU帧
bool ModbusWorker::detectAndProcessCompleteFrame()
{
    if (m_responseBuffer.size() < 5) {
        // 最小帧长度不足
        return false;
    }
    
    // 遍历缓冲区，寻找完整的帧
    for (int i = 0; i <= m_responseBuffer.size() - 5; ++i) {
        // 检查从位置i开始的帧
        quint8 slaveId = static_cast<quint8>(m_responseBuffer.at(i));
        quint8 functionCode = static_cast<quint8>(m_responseBuffer.at(i + 1));
        
        // 检查从站ID是否匹配当前请求
        if (slaveId != m_currentRequest->slaveId()) {
            continue; // 从站ID不匹配，继续查找
        }
        
        // 检查功能码是否匹配当前请求
        if (functionCode != m_currentRequest->functionCode() &&
                functionCode != (m_currentRequest->functionCode() | 0x80)) {
            continue; // 功能码不匹配，继续查找
        }
        
        // 根据功能码确定帧长度
        int expectedLength = getExpectedFrameLength(functionCode, m_responseBuffer, i);
        if (expectedLength == -1) {
            continue; // 无法确定帧长度，继续查找
        }
        
        // 检查是否有足够的数据
        if (i + expectedLength > m_responseBuffer.size()) {
            // 数据不足，等待更多数据
            return false;
        }
        
        // 提取完整帧
        QByteArray frame = m_responseBuffer.mid(i, expectedLength);
        
        // 验证CRC
        if (!validateModbusCRC(frame)) {
            qWarning() << "CRC校验失败，跳过此帧:" << frame.toHex(' ');
            continue; // CRC校验失败，继续查找
        }
        
        // 处理找到的完整帧
        processResponse(frame);
        
        // 移除已处理的帧数据
        m_responseBuffer.remove(0, i + expectedLength);
        
        return true;
    }
    
    // 如果没有找到完整帧，检查是否需要清空缓冲区
    if (m_responseBuffer.size() > 1000) {
        qWarning() << "缓冲区过大，清空缓冲区以避免内存问题";
        m_responseBuffer.clear();
    }
    
    return false;
}

// 根据功能码获取预期的帧长度
int ModbusWorker::getExpectedFrameLength(quint8 functionCode, const QByteArray& buffer, int startPos)
{
    if (buffer.size() < startPos + 2) {
        return -1;
    }
    
    // 异常响应帧长度固定为5字节
    if (functionCode & 0x80) {
        return 5; // 从站ID + 异常功能码 + 异常码 + CRC(2字节)
    }
    
    switch (functionCode) {
    case ReadCoils:
    case ReadDiscreteInputs:
    case ReadHoldingRegisters:
    case ReadInputRegisters: {
        // 读取类功能码：从站ID + 功能码 + 字节数 + 数据 + CRC
        if (buffer.size() < startPos + 3) {
            return -1;
        }
        quint8 byteCount = static_cast<quint8>(buffer.at(startPos + 2));
        return 3 + byteCount + 2; // 3字节头 + 数据字节数 + 2字节CRC
    }
        
    case WriteSingleCoil:
    case WriteSingleRegister:
        // 写入单个：从站ID + 功能码 + 地址(2) + 值(2) + CRC
        return 8;
        
    case WriteMultipleCoils:
    case WriteMultipleRegisters:
        // 写入多个：从站ID + 功能码 + 地址(2) + 数量(2) + CRC
        return 8;
        
    default:
        qWarning() << "未知功能码:" << functionCode;
        return -1;
    }
}
