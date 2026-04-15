#include "modbus_client.h"
#include <QDebug>
#include <QCoreApplication>

ModbusClient::ModbusClient(QObject* parent) 
    : QObject(parent)
    , m_worker(nullptr)
    , m_workerThread(nullptr)
    , m_requestIdCounter(0)
    , m_isInitialized(false)
{
    qRegisterMetaType<QVector<quint16>>("QVector<quint16>");
}

ModbusClient::~ModbusClient()
{
    shutdown();
}

//初始化客户端配置
bool ModbusClient::initialize(const ClientConfig& config)
{
    if (m_isInitialized) {
        qWarning() << "ModbusClient已经初始化";
        return true;
    }
    
    // 创建工作线程
    m_workerThread = new QThread(this);
    m_worker = new ModbusWorker();
    m_worker->moveToThread(m_workerThread);
    
    // 连接信号槽
    QObject::connect(m_worker, &ModbusWorker::requestCompleted,
                     this, &ModbusClient::onWorkerRequestCompleted);
    QObject::connect(m_worker, &ModbusWorker::connectionStatusChanged,
                     this, &ModbusClient::onWorkerConnectionStatusChanged);
    QObject::connect(m_worker, &ModbusWorker::communicationError,
                     this, &ModbusClient::onWorkerCommunicationError);
    QObject::connect(m_worker, &ModbusWorker::disconnected,
                     this, &ModbusClient::onDisconnected);
    QObject::connect(m_worker, &ModbusWorker::connected,
                     this, &ModbusClient::onConnected);

    
    // 启动工作线程
    m_workerThread->start();
    
    // 初始化工作线程
    bool success = false;
    QMetaObject::invokeMethod(m_worker, "doInitialize", Qt::BlockingQueuedConnection,
                              Q_ARG(SerialPortConfig, config.serialConfig),
                              Q_ARG(bool*, &success));
    
    if (!success) {
        qCritical() << "ModbusWorker初始化失败";
        shutdown();
        return false;
    }
    
    m_isInitialized = true;
    qDebug() << "ModbusClient初始化成功";
    return true;
}

void ModbusClient::shutdown()
{
    if (!m_isInitialized) {
        return;
    }
    
    // 断开连接
    disconnect();
    
    // 停止工作线程
    if (m_workerThread && m_workerThread->isRunning()) {
        QMetaObject::invokeMethod(m_worker, "shutdown", Qt::BlockingQueuedConnection);
        m_workerThread->quit();
        m_workerThread->wait(3000); // 等待3秒
        
        if (m_workerThread->isRunning()) {
            m_workerThread->terminate();
            m_workerThread->wait();
        }
    }
    
    // 清理资源
    if (m_worker) {
        delete m_worker;
        m_worker = nullptr;
    }
    
    if (m_workerThread) {
        delete m_workerThread;
        m_workerThread = nullptr;
    }
    
    // 清理同步操作相关资源
    {
        QMutexLocker locker(&m_syncMutex);
        for (auto eventLoop : m_syncEventLoops) {
            if (eventLoop) {
                eventLoop->quit();
                delete eventLoop;
            }
        }
        m_syncEventLoops.clear();
        m_syncResults.clear();
    }
    
    m_requestTagMap.clear();
    m_isInitialized = false;
    
    qDebug() << "ModbusClient已关闭";
}

bool ModbusClient::connect()
{
    if (!m_isInitialized) {
        qCritical() << "ModbusClient未初始化";
        return false;
    }
    
    QMetaObject::invokeMethod(m_worker, "connectDevice", Qt::BlockingQueuedConnection);
    
    bool success = m_cachedConnected.load();
    
    if (success) {
        qDebug() << "Modbus设备连接成功";
    } else {
        qWarning() << "Modbus设备连接失败";
    }
    
    return success;
}

void ModbusClient::disconnect()
{
    if (!m_isInitialized) {
        return;
    }
    
    QMetaObject::invokeMethod(m_worker, "disconnectDevice", Qt::BlockingQueuedConnection);
    qDebug() << "Modbus设备已断开连接";
}

bool ModbusClient::isConnected() const
{
    // 直接读取原子缓存状态，避免每次跨线程 BlockingQueuedConnection 查询造成的性能损耗和潜在死锁
    return m_isInitialized && m_cachedConnected.load();
}

// 同步操作实现
QVector<quint16> ModbusClient::readHoldingRegisters(int slaveId, int startAddr, int quantity)
{
    auto request = createRequest(ReadHoldingRegisters, slaveId, startAddr, quantity);
    auto result = executeSyncOperation(request);
    
    if (result.exception == NoException) {
        QVector<quint16> values;
        for (const auto& value : result.values) {
            values.append(value);
        }
        return values;
    }
    
    return QVector<quint16>();
}

QVector<quint16> ModbusClient::readInputRegisters(int slaveId, int startAddr, int quantity)
{
    auto request = createRequest(ReadInputRegisters, slaveId, startAddr, quantity);
    auto result = executeSyncOperation(request);
    
    if (result.exception == NoException) {
        QVector<quint16> values;
        for (const auto& value : result.values) {
            values.append(value);
        }
        return values;
    }
    
    return QVector<quint16>();
}

QVector<quint16> ModbusClient::readCoils(int slaveId, int startAddr, int quantity)
{
    auto request = createRequest(ReadCoils, slaveId, startAddr, quantity);
    auto result = executeSyncOperation(request);
    
    if (result.exception == NoException) {
        QVector<quint16> values;
        for (const auto& value : result.values) {
            values.append(value);
        }
        return values;
    }
    
    return QVector<quint16>();
}

QVector<quint16> ModbusClient::readDiscreteInputs(int slaveId, int startAddr, int quantity)
{
    auto request = createRequest(ReadDiscreteInputs, slaveId, startAddr, quantity);
    auto result = executeSyncOperation(request);
    
    if (result.exception == NoException) {
        QVector<quint16> values;
        for (const auto& value : result.values) {
            values.append(value);
        }
        return values;
    }
    
    return QVector<quint16>();
}

bool ModbusClient::writeSingleCoil(int slaveId, int address, const QVector<quint16>& values)
{
    if (values.isEmpty()) {
        qWarning() << "Write single coil: values vector is empty";
        return false;
    }
    
    quint16 value = values.first();
    QByteArray data;
    data.append(static_cast<char>((address >> 8) & 0xFF));
    data.append(static_cast<char>(address & 0xFF));
    // 将uint16值转换为bool：非0为true，0为false
    bool coilValue = (value != 0);
    data.append(static_cast<char>(coilValue ? 0xFF : 0x00));
    data.append(static_cast<char>(0x00));
    
    auto request = createRequest(WriteSingleCoil, slaveId, address, 1, data);
    auto result = executeSyncOperation(request);
    
    return result.exception == NoException;
}

bool ModbusClient::writeSingleRegister(int slaveId, int address, const QVector<quint16>& values)
{
    if (values.isEmpty()) {
        qWarning() << "Write single register: values vector is empty";
        return false;
    }
    
    quint16 value = values.first();
    QByteArray data;
    data.append(static_cast<char>((address >> 8) & 0xFF));
    data.append(static_cast<char>(address & 0xFF));
    data.append(static_cast<char>((value >> 8) & 0xFF));
    data.append(static_cast<char>(value & 0xFF));
    
    auto request = createRequest(WriteSingleRegister, slaveId, address, 1, data);
    auto result = executeSyncOperation(request);
    
    return result.exception == NoException;
}

bool ModbusClient::writeMultipleCoils(int slaveId, int startAddr, const QVector<quint16>& values)
{
    int quantity = values.size();
    int byteCount = (quantity + 7) / 8;
    
    QByteArray data;
    data.append(static_cast<char>((startAddr >> 8) & 0xFF));
    data.append(static_cast<char>(startAddr & 0xFF));
    data.append(static_cast<char>((quantity >> 8) & 0xFF));
    data.append(static_cast<char>(quantity & 0xFF));
    data.append(static_cast<char>(byteCount));
    
    // 打包线圈状态 - 将uint16转换为bool
    for (int i = 0; i < byteCount; ++i) {
        quint8 byte = 0;
        for (int j = 0; j < 8; ++j) {
            int index = i * 8 + j;
            if (index < quantity) {
                // 将uint16值转换为bool：非0为true，0为false
                bool coilState = (values[index] != 0);
                if (coilState) {
                    byte |= (1 << j);
                }
            }
        }
        data.append(static_cast<char>(byte));
    }
    
    auto request = createRequest(WriteMultipleCoils, slaveId, startAddr, quantity, data);
    auto result = executeSyncOperation(request);
    
    return result.exception == NoException;
}

bool ModbusClient::writeMultipleRegisters(int slaveId, int startAddr, const QVector<quint16>& values)
{
    int quantity = values.size();
    
    QByteArray data;
    data.append(static_cast<char>((startAddr >> 8) & 0xFF));
    data.append(static_cast<char>(startAddr & 0xFF));
    data.append(static_cast<char>((quantity >> 8) & 0xFF));
    data.append(static_cast<char>(quantity & 0xFF));
    data.append(static_cast<char>(quantity * 2)); // 字节数
    
    // 添加寄存器值
    for (quint16 value : values) {
        data.append(static_cast<char>((value >> 8) & 0xFF));
        data.append(static_cast<char>(value & 0xFF));
    }
    
    auto request = createRequest(WriteMultipleRegisters, slaveId, startAddr, quantity, data);
    auto result = executeSyncOperation(request);
    
    return result.exception == NoException;
}

// 异步操作实现
void ModbusClient::readHoldingRegistersAsync(int slaveId, int startAddr, int quantity, const QString& tag)
{
    auto request = createRequest(ReadHoldingRegisters, slaveId, startAddr, quantity);
    executeAsyncOperation(request, tag.isEmpty() ? QString("read_holding_%1_%2_%3").arg(slaveId).arg(startAddr).arg(quantity) : tag);
}

void ModbusClient::readInputRegistersAsync(int slaveId, int startAddr, int quantity, const QString& tag)
{
    auto request = createRequest(ReadInputRegisters, slaveId, startAddr, quantity);
    executeAsyncOperation(request, tag.isEmpty() ? QString("read_input_%1_%2_%3").arg(slaveId).arg(startAddr).arg(quantity) : tag);
}

void ModbusClient::readCoilsAsync(int slaveId, int startAddr, int quantity, const QString& tag)
{
    auto request = createRequest(ReadCoils, slaveId, startAddr, quantity);
    executeAsyncOperation(request, tag.isEmpty() ? QString("read_coils_%1_%2_%3").arg(slaveId).arg(startAddr).arg(quantity) : tag);
}

void ModbusClient::readDiscreteInputsAsync(int slaveId, int startAddr, int quantity, const QString& tag)
{
    auto request = createRequest(ReadDiscreteInputs, slaveId, startAddr, quantity);
    executeAsyncOperation(request, tag.isEmpty() ? QString("read_discrete_%1_%2_%3").arg(slaveId).arg(startAddr).arg(quantity) : tag);
}

void ModbusClient::writeSingleCoilAsync(int slaveId, int address, const QVector<quint16>& values, const QString& tag)
{
    if (values.isEmpty()) {
        qWarning() << "Write single coil async: values vector is empty";
        return;
    }
    
    quint16 value = values.first();
    QByteArray data;
    data.append(static_cast<char>((address >> 8) & 0xFF));
    data.append(static_cast<char>(address & 0xFF));
    // 将uint16值转换为bool：非0为true，0为false
    bool coilValue = (value != 0);
    data.append(static_cast<char>(coilValue ? 0xFF : 0x00));
    data.append(static_cast<char>(0x00));
    
    auto request = createRequest(WriteSingleCoil, slaveId, address, 1, data);
    executeAsyncOperation(request, tag.isEmpty() ? QString("write_coil_%1_%2").arg(slaveId).arg(address) : tag);
}

void ModbusClient::writeSingleRegisterAsync(int slaveId, int address, const QVector<quint16>& values, const QString& tag)
{
    if (values.isEmpty()) {
        qWarning() << "Write single register async: values vector is empty";
        return;
    }
    
    quint16 value = values.first();
    QByteArray data;
    data.append(static_cast<char>((address >> 8) & 0xFF));
    data.append(static_cast<char>(address & 0xFF));
    data.append(static_cast<char>((value >> 8) & 0xFF));
    data.append(static_cast<char>(value & 0xFF));
    
    auto request = createRequest(WriteSingleRegister, slaveId, address, 1, data);
    executeAsyncOperation(request, tag.isEmpty() ? QString("write_register_%1_%2").arg(slaveId).arg(address) : tag);
}

void ModbusClient::writeMultipleCoilsAsync(int slaveId, int startAddr, const QVector<quint16>& values, const QString& tag)
{
    int quantity = values.size();
    int byteCount = (quantity + 7) / 8;
    
    QByteArray data;
    data.append(static_cast<char>((startAddr >> 8) & 0xFF));
    data.append(static_cast<char>(startAddr & 0xFF));
    data.append(static_cast<char>((quantity >> 8) & 0xFF));
    data.append(static_cast<char>(quantity & 0xFF));
    data.append(static_cast<char>(byteCount));
    
    for (int i = 0; i < byteCount; ++i) {
        quint8 byte = 0;
        for (int j = 0; j < 8; ++j) {
            int index = i * 8 + j;
            if (index < quantity) {
                // 将uint16值转换为bool：非0为true，0为false
                bool coilState = (values[index] != 0);
                if (coilState) {
                    byte |= (1 << j);
                }
            }
        }
        data.append(static_cast<char>(byte));
    }
    
    auto request = createRequest(WriteMultipleCoils, slaveId, startAddr, quantity, data);
    executeAsyncOperation(request, tag.isEmpty() ? QString("write_coils_%1_%2").arg(slaveId).arg(startAddr) : tag);
}

void ModbusClient::writeMultipleRegistersAsync(int slaveId, int startAddr, const QVector<quint16>& values, const QString& tag)
{
    int quantity = values.size();
    
    QByteArray data;
    data.append(static_cast<char>((startAddr >> 8) & 0xFF));
    data.append(static_cast<char>(startAddr & 0xFF));
    data.append(static_cast<char>((quantity >> 8) & 0xFF));
    data.append(static_cast<char>(quantity & 0xFF));
    data.append(static_cast<char>(quantity * 2));
    
    for (quint16 value : values) {
        data.append(static_cast<char>((value >> 8) & 0xFF));
        data.append(static_cast<char>(value & 0xFF));
    }
    
    auto request = createRequest(WriteMultipleRegisters, slaveId, startAddr, quantity, data);
    executeAsyncOperation(request, tag.isEmpty() ? QString("write_registers_%1_%2").arg(slaveId).arg(startAddr) : tag);
}

void ModbusClient::cancelRequest(qint64 requestId)
{
    if (!m_isInitialized) {
        return;
    }
    
    QMetaObject::invokeMethod(m_worker, "cancelRequest", Qt::QueuedConnection,
                              Q_ARG(qint64, requestId));
    
    // 清理同步操作相关资源
    {
        QMutexLocker locker(&m_syncMutex);
        if (m_syncEventLoops.contains(requestId)) {
            auto eventLoop = m_syncEventLoops.take(requestId);
            if (eventLoop) {
                eventLoop->quit();
                delete eventLoop;
            }
            m_syncResults.remove(requestId);
        }
    }
    
    m_requestTagMap.remove(requestId);
}

void ModbusClient::clearQueue()
{
    if (!m_isInitialized) {
        return;
    }
    
    QMetaObject::invokeMethod(m_worker, "clearQueue", Qt::QueuedConnection);
    
    // 清理所有同步操作相关资源
    {
        QMutexLocker locker(&m_syncMutex);
        for (auto eventLoop : m_syncEventLoops) {
            if (eventLoop) {
                eventLoop->quit();
                delete eventLoop;
            }
        }
        m_syncEventLoops.clear();
        m_syncResults.clear();
    }
    
    m_requestTagMap.clear();
}

// 私有方法实现
qint64 ModbusClient::generateRequestId()
{
    QMutexLocker locker(&m_idMutex);
    return ++m_requestIdCounter;
}

QSharedPointer<ModbusRequest> ModbusClient::createRequest(ModbusFunctionCode functionCode, int slaveId, 
                                                          int startAddr, int quantity, const QByteArray& data)
{
    QSharedPointer<ModbusRequest> request;
    
    switch (functionCode) {
    case ReadHoldingRegisters:
        request = QSharedPointer<ReadHoldingRegistersRequest>::create(slaveId, startAddr, quantity);
        break;
    case ReadInputRegisters:
        request = QSharedPointer<ReadInputRegistersRequest>::create(slaveId, startAddr, quantity);
        break;
    case ReadCoils:
        request = QSharedPointer<ReadCoilsRequest>::create(slaveId, startAddr, quantity);
        break;
    case ReadDiscreteInputs:
        request = QSharedPointer<ReadDiscreteInputsRequest>::create(slaveId, startAddr, quantity);
        break;
    case WriteSingleCoil:
        // For WriteSingleCoil, we need to handle the data parameter
        if (data.size() >= 2) {
            bool value = data[0] != 0;
            request = QSharedPointer<WriteSingleCoilRequest>::create(slaveId, startAddr, value);
        }
        break;
    case WriteSingleRegister:
        // For WriteSingleRegister, we need to extract the value from data
        if (data.size() >= 4) {
            quint16 value = static_cast<quint8>(data[2]) << 8 | static_cast<quint8>(data[3]);
            request = QSharedPointer<WriteSingleRegisterRequest>::create(slaveId, startAddr, value);
        }
        break;
    case WriteMultipleRegisters:
        // For WriteMultipleRegisters, we need to convert QByteArray to QVector<quint16>
        if (data.size() >= quantity * 2) {
            QVector<quint16> values;
            for (int i = 0; i < quantity; i++) {
                quint16 value = static_cast<quint8>(data[i*2]) << 8 | static_cast<quint8>(data[i*2+1]);
                values.append(value);
            }
            request = QSharedPointer<WriteMultipleRegistersRequest>::create(slaveId, startAddr, values);
        }
        break;
    case WriteMultipleCoils:
        // For WriteMultipleCoils, we need to convert QByteArray to QVector<bool>
        if (data.size() >= (quantity + 7) / 8) {
            QVector<bool> values;
            for (int i = 0; i < quantity; i++) {
                int byteIndex = i / 8;
                int bitIndex = i % 8;
                bool value = (data[byteIndex] >> bitIndex) & 1;
                values.append(value);
            }
            request = QSharedPointer<WriteMultipleCoilsRequest>::create(slaveId, startAddr, values);
        }
        break;
    default:
        qWarning() << "Unsupported function code:" << functionCode;
        break;
    }
    
    return request;
}

//执行同步操作
ModbusResult ModbusClient::executeSyncOperation(QSharedPointer<ModbusRequest> request)
{
    if (!m_isInitialized) {
        ModbusResult result;
        result.exception = TimeoutError;
        result.errorString = "客户端未初始化";
        return result;
    }

    if (!request) {
        ModbusResult result;
        result.exception = IllegalDataValue;
        result.errorString = "request为空";
        return result;
    }

    qint64 requestId = request->requestId();
    
    // 创建事件循环
    QEventLoop* eventLoop = new QEventLoop();
    
    {
        QMutexLocker locker(&m_syncMutex);
        m_syncEventLoops[requestId] = eventLoop;
        ModbusResult timeoutResult;
        timeoutResult.exception = TimeoutError;
        timeoutResult.errorString = "请求超时";
        m_syncResults[requestId] = timeoutResult;
    }
    
    // 设置超时
    QTimer::singleShot(5000, eventLoop, &QEventLoop::quit);
    
    // 发送请求
    QMetaObject::invokeMethod(m_worker, "enqueueRequest", Qt::QueuedConnection,
                              Q_ARG(QSharedPointer<ModbusRequest>, request));
    
    // 等待响应
    eventLoop->exec();
    
    // 获取结果
    ModbusResult result;
    {
        QMutexLocker locker(&m_syncMutex);
        if (m_syncResults.contains(requestId)) {
            result = m_syncResults.value(requestId);
        } else {
            result.exception = TimeoutError;
            result.errorString = "请求超时";
        }
        m_syncEventLoops.remove(requestId);
        m_syncResults.remove(requestId);
    }
    
    delete eventLoop;
    return result;
}

void ModbusClient::executeAsyncOperation(QSharedPointer<ModbusRequest> request, const QString& tag)
{
    if (!m_isInitialized) {
        ModbusResult result;
        result.exception = ConnectionError;
        result.errorString = "客户端未初始化";
        emit requestCompleted(tag, result);
        return;
    }
    
    if(!request){
        ModbusResult result;
        result.exception = IllegalDataValue;
        result.errorString = "request 为空";
        emit requestCompleted(tag, result);
        return;
    }

    qint64 requestId = request->requestId();
    m_requestTagMap[requestId] = tag;
    
    QMetaObject::invokeMethod(m_worker, "enqueueRequest", Qt::QueuedConnection,
                              Q_ARG(QSharedPointer<ModbusRequest>, request));
}

// 私有槽函数实现
void ModbusClient::onWorkerRequestCompleted(qint64 requestId, const ModbusResult& result)
{
    // 处理同步操作
    {
        QMutexLocker locker(&m_syncMutex);
        if (m_syncEventLoops.contains(requestId)) {
            m_syncResults[requestId] = result;
            auto eventLoop = m_syncEventLoops.value(requestId);
            if (eventLoop && eventLoop->isRunning()) {
                eventLoop->quit();
            }
            return;
        }
    }
    
    // 处理异步操作
    QString tag = m_requestTagMap.value(requestId, QString());
    m_requestTagMap.remove(requestId);
    
    qDebug()<< __FUNCTION__ << "发送接收的数据....";
    emit requestCompleted(tag, result);
}

void ModbusClient::onWorkerConnectionStatusChanged(bool connected)
{
    // 同步更新缓存状态，供 isConnected() 直接读取，避免跨线程查询
    m_cachedConnected.store(connected);
    emit connectionStatusChanged(connected);
}

void ModbusClient::onWorkerCommunicationError(const QString& error)
{
    emit communicationError(error);
}

//接受modbusworker发送过来的断开连接信息
void ModbusClient::onDisconnected()
{
    emit disconnected();
}

void ModbusClient::onConnected()
{
    emit connected();
}
