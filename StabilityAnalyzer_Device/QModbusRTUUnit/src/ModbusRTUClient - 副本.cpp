#include "ModbusRTUClient.h"
#include <QDebug>
#include <QThread>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QSerialPortInfo>
#include "cutelogger/logmanager.h"
ModbusRTUClient::ModbusRTUClient(QObject *parent)
    : QObject(parent)
    , m_serialPort(nullptr)
    , m_timeoutTimer(nullptr)
    , m_config(nullptr)
    , m_connectionState(Disconnected)
    , m_waitingForResponse(false)
    , m_expectedSlaveAddress(0)
    , m_expectedFunctionCode(0)
    , m_expectedByteCount(0)
    , m_asyncResponseReceived(false)
    , m_asyncError(NoError)
{
    qRegisterMetaType<QSerialPort::SerialPortError>("QSerialPort::SerialPortError");
    qRegisterMetaType<ConnectionState>("ConnectionState");
    qRegisterMetaType<ErrorCode>("ErrorCode");

//    connect(this,SIGNAL(sig_Startinit()),this,SLOT(startWork()));
//    QThread* thread = new QThread;
//    this->moveToThread(thread);
//    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
//    connect(thread, &QThread::started, this, &ModbusRTUClient::init);
//    thread->start();
//    LOG_INFO()<<"线程ID111............."<<QThread::currentThreadId();
}

ModbusRTUClient::~ModbusRTUClient()
{
    LOG_INFO() << "ModbusRTUClient析构函数开始执行...";
}

//清理资源
void ModbusRTUClient::clearUpResource()
{
    // 断开设备连接
    disconnectFromDevice();

    // 手动释放资源（这些对象都没有父对象）
    if (m_serialPort) {
        LOG_INFO() << "删除m_serialPort对象...";
        delete m_serialPort;
        m_serialPort = nullptr;
    }

    if (m_timeoutTimer) {
        LOG_INFO() << "删除m_timeoutTimer对象...";
        delete m_timeoutTimer;
        m_timeoutTimer = nullptr;
    }

    if (m_config) {
        LOG_INFO() << "删除m_config对象...";
        delete m_config;
        m_config = nullptr;
    }

}

//初始化，这个时候就在子线程里了
void ModbusRTUClient::init()
{
//    LOG_INFO()<<"线程ID333............."<<QThread::currentThreadId();
    // 创建串口对象
    m_serialPort = new QSerialPort();
    m_config = new ModbusConfig() ;

    // 创建超时定时器
    m_timeoutTimer = new QTimer();
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setInterval(m_config->responseTimeout());

    // 连接信号槽
    connect(m_serialPort, &QSerialPort::readyRead, this, &ModbusRTUClient::onReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &ModbusRTUClient::onErrorOccurred);
    connect(m_timeoutTimer, &QTimer::timeout, this, &ModbusRTUClient::handleTimeout);
}

//外部调用接口，多线程
//void ModbusRTUClient::start(QString portName)
//{
//    LOG_INFO()<<"线程ID222............."<<QThread::currentThreadId();
//    m_portName = portName;
//    emit sig_Startinit();
//}

//void ModbusRTUClient::startWork()
//{
//    LOG_INFO()<<"线程ID444............."<<QThread::currentThreadId();
//    //连接串口
//    connectToDevice(m_portName);
//}

ModbusConfig* ModbusRTUClient::config() const
{
    return m_config;
}

bool ModbusRTUClient::connectToDevice(const QString &portName)
{
    QMutexLocker locker(&m_mutex);

    if (m_connectionState == Connected || m_connectionState == Connecting) {
        setError(ConnectionError, "设备已连接或正在连接");
        return false;
    }

    setConnectionState(Connecting);

    // 使用配置或参数值
    QString actualPortName = portName.isEmpty() ? m_config->portName() : portName;

    // 配置串口参数
    m_serialPort->setPortName(actualPortName);
    m_serialPort->setBaudRate(static_cast<QSerialPort::BaudRate>(m_config->baudRate()));
    m_serialPort->setDataBits(static_cast<QSerialPort::DataBits>(m_config->dataBits()));
    m_serialPort->setParity(static_cast<QSerialPort::Parity>(m_config->parity()));
    m_serialPort->setStopBits(static_cast<QSerialPort::StopBits>(m_config->stopBits()));
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    // 尝试打开串口
    if (m_serialPort->open(QIODevice::ReadWrite)) {
        setConnectionState(Connected);
        m_receiveBuffer.clear();
        return true;
    } else {
        setConnectionState(Error);
        setError(ConnectionError, m_serialPort->errorString());
        return false;
    }
}

void ModbusRTUClient::disconnectFromDevice()
{
    QMutexLocker locker(&m_mutex);

    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->close();
    }

    m_timeoutTimer->stop();
    m_waitingForResponse = false;
    m_receiveBuffer.clear();
    setConnectionState(Disconnected);
}

// 同步操作实现
QVector<bool> ModbusRTUClient::readCoilsSync(int slaveAddress, int startAddress, int count, int timeout)
{
    QMutexLocker locker(&m_asyncMutex);

    if (!sendRequest(buildRequestFrame(slaveAddress, 0x01, startAddress, count), slaveAddress, 0x01)) {
        return QVector<bool>();
    }

    // 等待响应
    int actualTimeout = timeout == -1 ? m_config->responseTimeout() : timeout;
    if (!m_asyncCondition.wait(&m_asyncMutex, actualTimeout)) {
        setError(TimeoutError, "读取线圈超时");
        return QVector<bool>();
    }

    if (m_asyncError != NoError) {
        return QVector<bool>();
    }

    // 解析响应数据
    if (m_asyncResponse.size() < 3) {
        setError(ProtocolError, "响应数据格式错误");
        return QVector<bool>();
    }

    int byteCount = static_cast<quint8>(m_asyncResponse[1]);
    QVector<bool> result;

    for (int i = 0; i < byteCount; i++) {
        quint8 byte = static_cast<quint8>(m_asyncResponse[2 + i]);
        for (int bit = 0; bit < 8 && (i * 8 + bit) < count; bit++) {
            result.append((byte >> bit) & 0x01);
        }
    }

    return result;
}

QVector<bool> ModbusRTUClient::readDiscreteInputsSync(int slaveAddress, int startAddress, int count, int timeout)
{
    QMutexLocker locker(&m_asyncMutex);

    if (!sendRequest(buildRequestFrame(slaveAddress, 0x02, startAddress, count), slaveAddress, 0x02)) {
        return QVector<bool>();
    }

    int actualTimeout = timeout == -1 ? m_config->responseTimeout() : timeout;
    if (!m_asyncCondition.wait(&m_asyncMutex, actualTimeout)) {
        setError(TimeoutError, "读取离散输入超时");
        return QVector<bool>();
    }

    if (m_asyncError != NoError) {
        return QVector<bool>();
    }

    if (m_asyncResponse.size() < 3) {
        setError(ProtocolError, "响应数据格式错误");
        return QVector<bool>();
    }

    int byteCount = static_cast<quint8>(m_asyncResponse[1]);
    QVector<bool> result;

    for (int i = 0; i < byteCount; i++) {
        quint8 byte = static_cast<quint8>(m_asyncResponse[2 + i]);
        for (int bit = 0; bit < 8 && (i * 8 + bit) < count; bit++) {
            result.append((byte >> bit) & 0x01);
        }
    }

    return result;
}

QVector<quint16> ModbusRTUClient::readHoldingRegistersSync(int slaveAddress, int startAddress, int count, int timeout)
{
    QMutexLocker locker(&m_asyncMutex);

    if (!sendRequest(buildRequestFrame(slaveAddress, 0x03, startAddress, count), slaveAddress, 0x03)) {
        return QVector<quint16>();
    }

    int actualTimeout = timeout == -1 ? m_config->responseTimeout() : timeout;
    if (!m_asyncCondition.wait(&m_asyncMutex, actualTimeout)) {
        setError(TimeoutError, "读取保持寄存器超时");
        return QVector<quint16>();
    }

    if (m_asyncError != NoError) {
        return QVector<quint16>();
    }

    if (m_asyncResponse.size() < 3) {
        setError(ProtocolError, "响应数据格式错误");
        return QVector<quint16>();
    }

    int byteCount = static_cast<quint8>(m_asyncResponse[1]);
    QVector<quint16> result;

    for (int i = 0; i < byteCount / 2; i++) {
        quint16 value = (static_cast<quint8>(m_asyncResponse[2 + i * 2]) << 8) | static_cast<quint8>(m_asyncResponse[3 + i * 2]);
        result.append(value);
    }

    return result;
}

QVector<quint16> ModbusRTUClient::readInputRegistersSync(int slaveAddress, int startAddress, int count, int timeout)
{
    QMutexLocker locker(&m_asyncMutex);

    if (!sendRequest(buildRequestFrame(slaveAddress, 0x04, startAddress, count), slaveAddress, 0x04)) {
        return QVector<quint16>();
    }

    int actualTimeout = timeout == -1 ? m_config->responseTimeout() : timeout;
    if (!m_asyncCondition.wait(&m_asyncMutex, actualTimeout)) {
        setError(TimeoutError, "读取输入寄存器超时");
        return QVector<quint16>();
    }

    if (m_asyncError != NoError) {
        return QVector<quint16>();
    }

    if (m_asyncResponse.size() < 3) {
        setError(ProtocolError, "响应数据格式错误");
        return QVector<quint16>();
    }

    int byteCount = static_cast<quint8>(m_asyncResponse[1]);
    QVector<quint16> result;

    for (int i = 0; i < byteCount / 2; i++) {
        quint16 value = (static_cast<quint8>(m_asyncResponse[2 + i * 2]) << 8) | static_cast<quint8>(m_asyncResponse[3 + i * 2]);
        result.append(value);
    }

    return result;
}

bool ModbusRTUClient::writeSingleCoilSync(int slaveAddress, int address, bool value, int timeout)
{
    QMutexLocker locker(&m_asyncMutex);

    QByteArray data;
    data.append(static_cast<char>((address >> 8) & 0xFF));
    data.append(static_cast<char>(address & 0xFF));
    data.append(static_cast<char>(value ? 0xFF : 0x00));
    data.append(static_cast<char>(0x00));

    if (!sendRequest(buildRequestFrame(slaveAddress, 0x05, address, 1, data), slaveAddress, 0x05)) {
        return false;
    }

    int actualTimeout = timeout == -1 ? m_config->responseTimeout() : timeout;
    if (!m_asyncCondition.wait(&m_asyncMutex, actualTimeout)) {
        setError(TimeoutError, "写单个线圈超时");
        return false;
    }

    return m_asyncError == NoError;
}

bool ModbusRTUClient::writeSingleRegisterSync(int slaveAddress, int address, quint16 value, int timeout)
{
    QMutexLocker locker(&m_asyncMutex);

    QByteArray data;
    data.append(static_cast<char>((address >> 8) & 0xFF));
    data.append(static_cast<char>(address & 0xFF));
    data.append(static_cast<char>((value >> 8) & 0xFF));
    data.append(static_cast<char>(value & 0xFF));

    if (!sendRequest(buildRequestFrame(slaveAddress, 0x06, address, 1, data), slaveAddress, 0x06)) {
        return false;
    }

    int actualTimeout = timeout == -1 ? m_config->responseTimeout() : timeout;
    if (!m_asyncCondition.wait(&m_asyncMutex, actualTimeout)) {
        setError(TimeoutError, "写单个寄存器超时");
        return false;
    }

    return m_asyncError == NoError;
}

bool ModbusRTUClient::writeMultipleCoilsSync(int slaveAddress, int startAddress, const QVector<bool> &values, int timeout)
{
    QMutexLocker locker(&m_asyncMutex);

    int count = values.size();
    if (count < 1 || count > 1968) {
        setError(InvalidParameter, "线圈数量必须在1-1968之间");
        return false;
    }

    int byteCount = (count + 7) / 8;
    QByteArray data;
    data.append(static_cast<char>((startAddress >> 8) & 0xFF));
    data.append(static_cast<char>(startAddress & 0xFF));
    data.append(static_cast<char>((count >> 8) & 0xFF));
    data.append(static_cast<char>(count & 0xFF));
    data.append(static_cast<char>(byteCount));

    quint8 currentByte = 0;
    for (int i = 0; i < count; i++) {
        if (values[i]) {
            currentByte |= (1 << (i % 8));
        }
        if ((i % 8 == 7) || (i == count - 1)) {
            data.append(static_cast<char>(currentByte));
            currentByte = 0;
        }
    }

    if (!sendRequest(buildRequestFrame(slaveAddress, 0x0F, startAddress, count, data), slaveAddress, 0x0F)) {
        return false;
    }

    int actualTimeout = timeout == -1 ? m_config->responseTimeout() : timeout;
    if (!m_asyncCondition.wait(&m_asyncMutex, actualTimeout)) {
        setError(TimeoutError, "写多个线圈超时");
        return false;
    }

    return m_asyncError == NoError;
}

bool ModbusRTUClient::writeMultipleRegistersSync(int slaveAddress, int startAddress, const QVector<quint16> &values, int timeout)
{
    QMutexLocker locker(&m_asyncMutex);

    int count = values.size();
    if (count < 1 || count > 123) {
        setError(InvalidParameter, "寄存器数量必须在1-123之间");
        return false;
    }

    QByteArray data;
    data.append(static_cast<char>((startAddress >> 8) & 0xFF));
    data.append(static_cast<char>(startAddress & 0xFF));
    data.append(static_cast<char>((count >> 8) & 0xFF));
    data.append(static_cast<char>(count & 0xFF));
    data.append(static_cast<char>(count * 2));

    for (quint16 value : values) {
        data.append(static_cast<char>((value >> 8) & 0xFF));
        data.append(static_cast<char>(value & 0xFF));
    }

    if (!sendRequest(buildRequestFrame(slaveAddress, 0x10, startAddress, count, data), slaveAddress, 0x10)) {
        return false;
    }

    int actualTimeout = timeout == -1 ? m_config->responseTimeout() : timeout;
    if (!m_asyncCondition.wait(&m_asyncMutex, actualTimeout)) {
        setError(TimeoutError, "写多个寄存器超时");
        return false;
    }

    return m_asyncError == NoError;
}

// 异步操作实现
bool ModbusRTUClient::readCoils(int slaveAddress, int startAddress, int count)
{
    LOG_INFO() << ">>> readCoils调用 <<<";
    LOG_INFO() << "线程ID:" << QThread::currentThreadId();
    LOG_INFO() << "参数: slave=" << slaveAddress << ", addr=" << startAddress << ", count=" << count;
    
    bool result = processAsyncOperation(slaveAddress, 0x01, startAddress, count);
    LOG_INFO() << "processAsyncOperation返回:" << result;
    LOG_INFO() << ">>> readCoils结束 <<<";
    return result;
}

bool ModbusRTUClient::readDiscreteInputs(int slaveAddress, int startAddress, int count)
{
    return processAsyncOperation(slaveAddress, 0x02, startAddress, count);
}

bool ModbusRTUClient::readHoldingRegisters(int slaveAddress, int startAddress, int count)
{
    return processAsyncOperation(slaveAddress, 0x03, startAddress, count);
}

bool ModbusRTUClient::readInputRegisters(int slaveAddress, int startAddress, int count)
{
    return processAsyncOperation(slaveAddress, 0x04, startAddress, count);
}

bool ModbusRTUClient::writeSingleCoil(int slaveAddress, int address, bool value)
{
    QByteArray data;
    data.append(static_cast<char>((address >> 8) & 0xFF));
    data.append(static_cast<char>(address & 0xFF));
    data.append(static_cast<char>(value ? 0xFF : 0x00));
    data.append(static_cast<char>(0x00));

    return processAsyncOperation(slaveAddress, 0x05, address, 1, data);
}

bool ModbusRTUClient::writeSingleRegister(int slaveAddress, int address, quint16 value)
{
    QByteArray data;
    data.append(static_cast<char>((address >> 8) & 0xFF));
    data.append(static_cast<char>(address & 0xFF));
    data.append(static_cast<char>((value >> 8) & 0xFF));
    data.append(static_cast<char>(value & 0xFF));

    return processAsyncOperation(slaveAddress, 0x06, address, 1, data);
}

bool ModbusRTUClient::writeMultipleCoils(int slaveAddress, int startAddress, const QVector<bool> &values)
{
    int count = values.size();
    if (count < 1 || count > 1968) {
        setError(InvalidParameter, "线圈数量必须在1-1968之间");
        return false;
    }

    int byteCount = (count + 7) / 8;
    QByteArray data;
    data.append(static_cast<char>((startAddress >> 8) & 0xFF));
    data.append(static_cast<char>(startAddress & 0xFF));
    data.append(static_cast<char>((count >> 8) & 0xFF));
    data.append(static_cast<char>(count & 0xFF));
    data.append(static_cast<char>(byteCount));

    // 打包线圈状态
    quint8 currentByte = 0;
    for (int i = 0; i < count; i++) {
        if (values[i]) {
            currentByte |= (1 << (i % 8));
        }
        if ((i % 8 == 7) || (i == count - 1)) {
            data.append(static_cast<char>(currentByte));
            currentByte = 0;
        }
    }

    return processAsyncOperation(slaveAddress, 0x0F, startAddress, count, data);
}

//写多个寄存器
bool ModbusRTUClient::writeMultipleRegisters(int slaveAddress, int startAddress, const QVector<quint16> &values)
{
    int count = values.size();
    if (count < 1 || count > 123) {
        setError(InvalidParameter, "寄存器数量必须在1-123之间");
        return false;
    }

    QByteArray data;
    data.append(static_cast<char>((startAddress >> 8) & 0xFF));
    data.append(static_cast<char>(startAddress & 0xFF));
    data.append(static_cast<char>((count >> 8) & 0xFF));
    data.append(static_cast<char>(count & 0xFF));
    data.append(static_cast<char>(count * 2));

    // 添加寄存器值
    for (quint16 value : values) {
        data.append(static_cast<char>((value >> 8) & 0xFF));
        data.append(static_cast<char>(value & 0xFF));
    }

    return processAsyncOperation(slaveAddress, 0x10, startAddress, count, data);
}

// 属性访问器实现
ModbusRTUClient::ConnectionState ModbusRTUClient::connectionState() const
{
    return m_connectionState;
}

// 私有方法实现
void ModbusRTUClient::setConnectionState(ConnectionState state)
{
    if (m_connectionState != state) {
        m_connectionState = state;
        emit connectionStateChanged(m_connectionState);
    }
}

void ModbusRTUClient::setError(ErrorCode errorCode, const QString &errorString)
{
    emit errorOccurred(errorCode, errorString);
}

//队列管理
bool ModbusRTUClient::processAsyncOperation(int slaveAddress, int functionCode, int startAddress, int count, const QByteArray &data)
{
    LOG_INFO() << "=== processAsyncOperation开始 ===";
    LOG_INFO() << "线程ID:" << QThread::currentThreadId();
    LOG_INFO() << "参数: slave=" << slaveAddress << ", func=" << functionCode 
               << ", addr=" << startAddress << ", count=" << count;
    
    bool shouldProcessImmediately = false;
    
    //添加作用域，确保 QMutexLocker 在作用域结束时自动释放锁
    //分离锁的获取和释放，先完成队列操作并释放队列锁，然后再获取主锁检查状态
    //避免嵌套锁，确保在调用 processQueuedRequest 之前，队列锁已经完全释放
    {
        QMutexLocker queueLocker(&m_queueMutex);
        LOG_INFO() << "队列锁获取成功";
        
        // 创建请求并加入队列
        ModbusRequest request(slaveAddress, functionCode, startAddress, count, data);
        m_requestQueue.enqueue(request);
        LOG_INFO() << "请求加入队列，当前队列大小:" << m_requestQueue.size();
    }
    
    // 检查是否正在等待响应（需要主互斥锁）
    QMutexLocker mainLocker(&m_mutex);
    LOG_INFO() << "主锁获取成功，m_waitingForResponse:" << m_waitingForResponse;
    
    if (!m_waitingForResponse) {
        shouldProcessImmediately = true;
    } else {
        LOG_INFO() << "正在等待响应，跳过processQueuedRequest调用";
    }
    
    mainLocker.unlock();
    
    if (shouldProcessImmediately) {
        LOG_INFO() << "准备调用processQueuedRequest...";
        bool result = processQueuedRequest();
        LOG_INFO() << "processQueuedRequest返回:" << result;
        LOG_INFO() << "=== processAsyncOperation结束 ===";
        return result;
    }
    
    LOG_INFO() << "=== processAsyncOperation结束 ===";
    return true;
}

//队列处理
bool ModbusRTUClient::processQueuedRequest()
{
    LOG_INFO() << "*** processQueuedRequest开始 ***";
    LOG_INFO() << "线程ID:" << QThread::currentThreadId();
    
    //这个方法也获取队列锁，这个一定要保证在前面的方法已经释放了这个队列锁了，否则会出现死锁、锁嵌套什么的
    QMutexLocker locker(&m_queueMutex);
    LOG_INFO() << "队列锁获取成功，队列大小:" << m_requestQueue.size();
    
    if (m_requestQueue.isEmpty()) {
        LOG_INFO() << "队列为空，返回false";
        LOG_INFO() << "*** processQueuedRequest结束 ***";
        return false;
    }
    
    ModbusRequest request = m_requestQueue.dequeue();
    LOG_INFO() << "取出请求: slave=" << request.slaveAddress << ", func=" << request.functionCode
               << ", addr=" << request.startAddress << ", count=" << request.count;
    
    QByteArray frame = buildRequestFrame(request.slaveAddress, request.functionCode, 
                                        request.startAddress, request.count, request.data);
    LOG_INFO() << "构建请求帧完成，帧长度:" << frame.length();
    
    bool result = sendRequest(frame, request.slaveAddress, request.functionCode);
    LOG_INFO() << "sendRequest返回:" << result;
    LOG_INFO() << "*** processQueuedRequest结束 ***";
    return result;
}

//发送结果信号
void ModbusRTUClient::emitAsyncResult(int slaveAddress, int functionCode, int startAddress, int count, const QByteArray &response)
{
    // 根据功能码发射相应的信号
    switch (functionCode) {
    case 0x01: // 读取线圈
        if (response.size() >= 3) {
            int byteCount = static_cast<quint8>(response[1]);
            QVector<bool> values;
            for (int i = 0; i < byteCount; i++) {
                quint8 byte = static_cast<quint8>(response[2 + i]);
                for (int bit = 0; bit < 8 && (i * 8 + bit) < count; bit++) {
                    values.append((byte >> bit) & 0x01);
                }
            }
            emit coilsRead(slaveAddress, startAddress, values);
        }
        break;
        
    case 0x02: // 读取离散输入
        if (response.size() >= 3) {
            int byteCount = static_cast<quint8>(response[1]);
            QVector<bool> values;
            for (int i = 0; i < byteCount; i++) {
                quint8 byte = static_cast<quint8>(response[2 + i]);
                for (int bit = 0; bit < 8 && (i * 8 + bit) < count; bit++) {
                    values.append((byte >> bit) & 0x01);
                }
            }
            emit discreteInputsRead(slaveAddress, startAddress, values);
        }
        break;
        
    case 0x03: // 读取保持寄存器
        if (response.size() >= 3) {
            int byteCount = static_cast<quint8>(response[1]);
            QVector<quint16> values;
            for (int i = 0; i < byteCount / 2; i++) {
                quint16 value = (static_cast<quint8>(response[2 + i * 2]) << 8) | static_cast<quint8>(response[3 + i * 2]);
                values.append(value);
            }
            emit holdingRegistersRead(slaveAddress, startAddress, values);
        }
        break;
        
    case 0x04: // 读取输入寄存器
        if (response.size() >= 3) {
            int byteCount = static_cast<quint8>(response[1]);
            QVector<quint16> values;
            for (int i = 0; i < byteCount / 2; i++) {
                quint16 value = (static_cast<quint8>(response[2 + i * 2]) << 8) | static_cast<quint8>(response[3 + i * 2]);
                values.append(value);
            }
            emit inputRegistersRead(slaveAddress, startAddress, values);
        }
        break;
        
    case 0x05: // 写单个线圈
    case 0x06: // 写单个寄存器
    case 0x0F: // 写多个线圈
    case 0x10: // 写多个寄存器
        emit writeCompleted(slaveAddress, functionCode, startAddress, count);
        break;
        
    default:
        break;
    }
}

// 构建请求数据，以下为原有协议的实现方法（需要根据项目设计进行调整）
QByteArray ModbusRTUClient::buildRequestFrame(int slaveAddress, int functionCode, int startAddress, int count, const QByteArray &data)
{
    QByteArray frame;
    frame.append(static_cast<char>(slaveAddress));
    frame.append(static_cast<char>(functionCode));
    frame.append(static_cast<char>((startAddress >> 8) & 0xFF));
    frame.append(static_cast<char>(startAddress & 0xFF));
    frame.append(static_cast<char>((count >> 8) & 0xFF));
    frame.append(static_cast<char>(count & 0xFF));

    if (!data.isEmpty()) {
        frame.append(data);
    }

    quint16 crc = calculateCRC(frame);
    frame.append(static_cast<char>(crc & 0xFF));
    frame.append(static_cast<char>((crc >> 8) & 0xFF));

    return frame;
}

//crc校验
quint16 ModbusRTUClient::calculateCRC(const QByteArray &data)
{
    quint16 crc = 0xFFFF;

    for (int i = 0; i < data.size(); i++) {
        crc ^= static_cast<quint8>(data[i]);

        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

//解析
bool ModbusRTUClient::parseResponse(const QByteArray &response, int slaveAddress, int functionCode)
{
    // 简化的响应解析实现
    if (response.size() < 5) {
        LOG_INFO() << "response数据大小不正确....";
        return false;
    }

    // 检查CRC
    QByteArray dataWithoutCRC = response.left(response.size() - 2);
    quint16 calculatedCRC = calculateCRC(dataWithoutCRC);
    quint16 receivedCRC = static_cast<quint8>(response[response.size() - 2]) |
                         (static_cast<quint8>(response[response.size() - 1]) << 8);

    if (calculatedCRC != receivedCRC) {
        setError(CRCError, "CRC校验失败");
        LOG_INFO() << "response数据CRC校验失败...."<<calculatedCRC<<receivedCRC;
        return false;
    }

    // 检查地址和功能码
    if (static_cast<quint8>(response[0]) != slaveAddress) {
        setError(ProtocolError, "从站地址不匹配");
        return false;
    }

    if (static_cast<quint8>(response[1]) != functionCode) {
        setError(ProtocolError, "功能码不匹配");
        return false;
    }

    return true;
}

//发送请求
bool ModbusRTUClient::sendRequest(const QByteArray &request, int slaveAddress, int functionCode)
{
    QMutexLocker locker(&m_mutex);
    LOG_INFO()<<"发送请求数据："<<request;

    if (m_connectionState != Connected) {
        setError(ConnectionError, QString("设备未连接，当前状态: %1").arg(m_connectionState));
        return false;
    }

    if (m_waitingForResponse) {
        setError(ProtocolError, QString("正在等待从站 %1 的响应，无法发送新请求").arg(m_expectedSlaveAddress));
        return false;
    }

    // 发送请求
    qint64 bytesWritten = m_serialPort->write(request);
    if (bytesWritten != request.size()) {
        setError(ConnectionError, "发送数据失败");
        return false;
    }
    m_serialPort->flush();

    // 设置等待响应状态
    m_waitingForResponse = true;
    m_expectedSlaveAddress = slaveAddress;
    m_expectedFunctionCode = functionCode;
    m_receiveBuffer.clear();

    // 启动超时定时器
    m_timeoutTimer->start();

    return true;
}

//接收消息，需要根据项目设计进行调整
void ModbusRTUClient::onReadyRead()
{
    LOG_INFO() << "=== onReadyRead开始 ===";
    LOG_INFO() << "线程ID:" << QThread::currentThreadId();
    
    QMutexLocker locker(&m_mutex);
    LOG_INFO() << "主锁获取成功，m_waitingForResponse:" << m_waitingForResponse;

    if (!m_waitingForResponse) {
        LOG_INFO() << "不在等待响应状态，直接返回";
        LOG_INFO() << "=== onReadyRead结束 ===";
        return;
    }

    QByteArray newData = m_serialPort->readAll();
    LOG_INFO() << "读取到新数据，长度:" << newData.length() << newData;
    m_receiveBuffer.append(newData);
    LOG_INFO() << "接收缓冲区总长度:" << m_receiveBuffer.size();

    // 检查是否收到完整帧
    if (m_receiveBuffer.size() >= 5) {
        LOG_INFO() << "开始解析响应帧...";
        // 帧检测逻辑
        if (parseResponse(m_receiveBuffer, m_expectedSlaveAddress, m_expectedFunctionCode)) {
            LOG_INFO() << "响应帧解析成功，停止超时定时器";
            m_timeoutTimer->stop();
            m_waitingForResponse = false;

            // 处理异步响应
            {
                QMutexLocker asyncLocker(&m_asyncMutex);
                m_asyncResponse = m_receiveBuffer;
                m_asyncResponseReceived = true;
                m_asyncError = NoError;
                m_asyncCondition.wakeAll();
                LOG_INFO() << "异步响应处理完成";
            }
            
            // 发射异步结果信号
            LOG_INFO() << "发射异步结果信号";
            emitAsyncResult(m_expectedSlaveAddress, m_expectedFunctionCode, 0, 0, m_receiveBuffer);
            
            // 处理队列中的下一个请求
            LOG_INFO() << "准备调用processQueuedRequest处理下一个请求";
            processQueuedRequest();

            // 发出相应的信号
            emit writeCompleted(m_expectedSlaveAddress, m_expectedFunctionCode, 0, 0);
            LOG_INFO() << "writeCompleted信号已发射";
        } else {
            LOG_INFO() << "响应帧解析失败，继续等待数据";
        }
    } else {
        LOG_INFO() << "数据长度不足，继续等待";
    }
    
    LOG_INFO() << "=== onReadyRead结束 ===";
}

//处理异步错误
void ModbusRTUClient::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
        setError(ConnectionError, m_serialPort->errorString());

        // 处理异步错误
        {
            QMutexLocker asyncLocker(&m_asyncMutex);
            m_asyncResponseReceived = true;
            m_asyncError = ConnectionError;
            m_asyncCondition.wakeAll();
        }
    }
}

//处理异步超时
void ModbusRTUClient::handleTimeout()
{
    LOG_INFO() << "=== handleTimeout开始 ===";
    LOG_INFO() << "线程ID:" << QThread::currentThreadId();
    
    QMutexLocker locker(&m_mutex);
    LOG_INFO() << "主锁获取成功，m_waitingForResponse:" << m_waitingForResponse;

    if (m_waitingForResponse) {
        LOG_INFO() << "检测到超时，停止等待响应";
        m_waitingForResponse = false;
        setError(TimeoutError, QString("从站 %1 的功能码 %2 操作超时").arg(m_expectedSlaveAddress).arg(m_expectedFunctionCode));

        // 处理异步超时
        {
            QMutexLocker asyncLocker(&m_asyncMutex);
            m_asyncResponseReceived = true;
            m_asyncError = TimeoutError;
            m_asyncCondition.wakeAll();
            LOG_INFO() << "异步超时处理完成";
        }
        
        // 处理队列中的下一个请求
        LOG_INFO() << "准备调用processQueuedRequest处理下一个请求";
        processQueuedRequest();
    } else {
        LOG_INFO() << "不在等待响应状态，忽略超时";
    }
    
    LOG_INFO() << "=== handleTimeout结束 ===";
}



