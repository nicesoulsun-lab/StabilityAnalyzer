#include "modbus_request.h"
#include <QElapsedTimer>
#include <QDebug>

// 静态成员初始化
std::atomic<qint64> ModbusRequest::s_nextRequestId(1);

ModbusRequest::ModbusRequest(int slaveId, ModbusFunctionCode functionCode, 
                           RequestPriority priority, QObject* parent)
    : QObject(parent)
    , m_requestId(s_nextRequestId.fetch_add(1))
    , m_slaveId(slaveId)
    , m_functionCode(functionCode)
    , m_priority(priority)
    , m_status(RequestStatus::Pending)
    , m_creationTime(QDateTime::currentMSecsSinceEpoch())
    , m_queueTime(0)
    , m_startTime(0)
    , m_completionTime(0)
    , m_responseTime(0)
    , m_timeout(1000)
    , m_maxRetries(3)
    , m_retryCount(0)
    , m_useCache(false)
    , m_cacheValidity(5000)
    , m_timeoutTimer(nullptr)
{
    qRegisterMetaType<QVector<quint16>>("QVector<quint16>");
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &ModbusRequest::onTimeout);
}

ModbusRequest::~ModbusRequest()
{
    stopTimeoutTimer();
}

void ModbusRequest::setStatus(RequestStatus status)
{
    if (m_status != status) {
        m_status = status;
        
        switch (status) {
        case RequestStatus::Queued:
            m_queueTime = QDateTime::currentMSecsSinceEpoch();
            break;
        case RequestStatus::Sending:
            m_startTime = QDateTime::currentMSecsSinceEpoch();
            startTimeoutTimer();
            break;
        case RequestStatus::Completed:
        case RequestStatus::Failed:
        case RequestStatus::Cancelled:
        case RequestStatus::Timeout:
            m_completionTime = QDateTime::currentMSecsSinceEpoch();
            m_responseTime = m_completionTime - m_startTime;
            stopTimeoutTimer();
            break;
        default:
            break;
        }
        
        emit statusChanged(m_requestId, status);
    }
}

void ModbusRequest::setResult(const ModbusResult& result)
{
    m_result = result;
    m_result.requestId = m_requestId;
    m_result.responseTime = m_responseTime;
    m_result.retryCount = m_retryCount;
    m_result.queueTime = m_queueTime > 0 ? m_startTime - m_queueTime : 0;
    m_result.processTime = m_responseTime;
}

void ModbusRequest::cancel()
{
    if (m_status == RequestStatus::Pending || 
        m_status == RequestStatus::Queued ||
        m_status == RequestStatus::Sending ||
        m_status == RequestStatus::WaitingResponse) {
        setStatus(RequestStatus::Cancelled);
        emit cancelled(m_requestId);
    }
}

void ModbusRequest::startTimeoutTimer()
{
    if (m_timeout > 0 && m_timeoutTimer) {
        m_timeoutTimer->start(m_timeout);
    }
}

void ModbusRequest::stopTimeoutTimer()
{
    if (m_timeoutTimer && m_timeoutTimer->isActive()) {
        m_timeoutTimer->stop();
    }
}

void ModbusRequest::onTimeout()
{
    if (m_status == RequestStatus::Sending || 
        m_status == RequestStatus::WaitingResponse) {
        setStatus(RequestStatus::Timeout);
        emit timeout(m_requestId);
    }
}

void ModbusRequest::setUseCache(bool newUseCache)
{
    m_useCache = newUseCache;
}

void ModbusRequest::setTimeout(int newTimeout)
{
    m_timeout = newTimeout;
}

QString ModbusRequest::exceptionToString(ModbusException exception)
{
    switch (exception) {
    case ModbusException::NoException: return "无异常";
    case ModbusException::IllegalFunction: return "非法功能码";
    case ModbusException::IllegalDataAddress: return "非法数据地址";
    case ModbusException::IllegalDataValue: return "非法数据值";
    case ModbusException::ServerDeviceFailure: return "服务器设备故障";
    case ModbusException::Acknowledge: return "确认";
    case ModbusException::ServerDeviceBusy: return "服务器设备忙";
    case ModbusException::NegativeAcknowledge: return "否认";
    case ModbusException::MemoryParityError: return "存储器奇偶错误";
    case ModbusException::GatewayPathUnavailable: return "网关路径不可用";
    case ModbusException::GatewayTargetDeviceFailedToRespond: return "网关目标设备响应失败";
    case ModbusException::TimeoutError: return "超时错误";
    case ModbusException::CRCCheckError: return "CRC校验错误";
    case ModbusException::FrameError: return "帧错误";
    case ModbusException::PortError: return "端口错误";
    case ModbusException::BufferOverflowError: return "缓冲区溢出";
    case ModbusException::RequestQueueFull: return "请求队列满";
    case ModbusException::ConnectionError: return "连接错误";
    case ModbusException::InvalidResponse: return "无效响应";
    default: return QString("未知错误(%1)").arg(static_cast<int>(exception));
    }
}

QString ModbusRequest::functionCodeToString(ModbusFunctionCode functionCode)
{
    switch (functionCode) {
    case ModbusFunctionCode::ReadCoils: return "读取线圈";
    case ModbusFunctionCode::ReadDiscreteInputs: return "读取离散输入";
    case ModbusFunctionCode::ReadHoldingRegisters: return "读取保持寄存器";
    case ModbusFunctionCode::ReadInputRegisters: return "读取输入寄存器";
    case ModbusFunctionCode::WriteSingleCoil: return "写入单个线圈";
    case ModbusFunctionCode::WriteSingleRegister: return "写入单个寄存器";
    case ModbusFunctionCode::WriteMultipleCoils: return "写入多个线圈";
    case ModbusFunctionCode::WriteMultipleRegisters: return "写入多个寄存器";
    case ModbusFunctionCode::MaskWriteRegister: return "掩码写寄存器";
    case ModbusFunctionCode::ReadWriteMultipleRegisters: return "读写多个寄存器";
    case ModbusFunctionCode::ReadFIFOQueue: return "读取FIFO队列";
    default: return QString("未知功能码(0x%1)").arg(static_cast<int>(functionCode), 2, 16, QChar('0'));
    }
}

QString ModbusRequest::priorityToString(RequestPriority priority)
{
    switch (priority) {
    case RequestPriority::Low: return "低";
    case RequestPriority::Normal: return "正常";
    case RequestPriority::High: return "高";
    case RequestPriority::Critical: return "紧急";
    case RequestPriority::System: return "系统";
    default: return QString("未知(%1)").arg(static_cast<int>(priority));
    }
}

// Builder实现
QSharedPointer<ModbusRequest> ModbusRequest::Builder::build()
{
    switch (m_functionCode) {
    case ModbusFunctionCode::ReadHoldingRegisters:
    case ModbusFunctionCode::ReadInputRegisters:
    {
        auto request = QSharedPointer<ReadHoldingRegistersRequest>::create(
            m_slaveId, m_startAddr, m_quantity, m_priority);
        request->setTimeout(m_timeout);
        request->setUseCache(m_useCache);
        return request;
    }
    case ModbusFunctionCode::WriteMultipleRegisters:
    {
        auto request = QSharedPointer<WriteMultipleRegistersRequest>::create(
            m_slaveId, m_startAddr, m_data, m_priority);
        request->setTimeout(m_timeout);
        return request;
    }
    case ModbusFunctionCode::ReadCoils:
    case ModbusFunctionCode::ReadDiscreteInputs:
    {
        auto request = QSharedPointer<ReadCoilsRequest>::create(
            m_slaveId, m_startAddr, m_quantity, m_priority);
        request->setTimeout(m_timeout);
        return request;
    }
    case ModbusFunctionCode::WriteSingleCoil:
    {
        bool value = m_data.size() > 0 ? (m_data[0] != 0) : false;
        auto request = QSharedPointer<WriteSingleCoilRequest>::create(
            m_slaveId, m_startAddr, value, m_priority);
        request->setTimeout(m_timeout);
        return request;
    }
    default:
        return nullptr;
    }
}

// ReadHoldingRegistersRequest实现
ReadHoldingRegistersRequest::ReadHoldingRegistersRequest(int slaveId, int startAddr, 
                                                       int quantity, RequestPriority priority)
    : ModbusRequest(slaveId, ModbusFunctionCode::ReadHoldingRegisters, priority)
    , m_startAddr(startAddr)
    , m_quantity(quantity)
{
}

QByteArray ReadHoldingRegistersRequest::buildRequestData()
{
    QByteArray data;
    data.append(static_cast<char>(slaveId()));
    data.append(static_cast<char>(0x03));
    data.append(static_cast<char>((m_startAddr >> 8) & 0xFF));
    data.append(static_cast<char>(m_startAddr & 0xFF));
    data.append(static_cast<char>((m_quantity >> 8) & 0xFF));
    data.append(static_cast<char>(m_quantity & 0xFF));
    return data;
}

ModbusResult ReadHoldingRegistersRequest::parseResponse(const QByteArray& response)
{
    ModbusResult result;
    result.success = false;
    
    if (response.size() < 5) {
        result.exception = ModbusException::InvalidResponse;
        result.errorString = "响应长度不足";
        return result;
    }
    
    // 检查异常响应
    quint8 functionCode = static_cast<quint8>(response[1]);
    if (functionCode & 0x80) {
        if (response.size() >= 5) {
            quint8 errorCode = static_cast<quint8>(response[2]);
            result.exception = static_cast<ModbusException>(errorCode);
            result.errorString = exceptionToString(result.exception);
        }
        return result;
    }
    
    // 正常响应
    if (functionCode == static_cast<quint8>(ModbusFunctionCode::ReadHoldingRegisters)) {
        quint8 byteCount = static_cast<quint8>(response[2]);
        int expectedByteCount = m_quantity * 2;
        
        if (byteCount == expectedByteCount && response.size() >= 3 + byteCount + 2) {
            result.success = true;
            result.exception = ModbusException::NoException;
            
            for (int i = 0; i < m_quantity; i++) {
                int offset = 3 + i * 2;
                quint16 value = static_cast<quint8>(response[offset]) << 8 | 
                                static_cast<quint8>(response[offset + 1]);
                
//                ModbusValue modbusValue;
//                modbusValue.value = value;
//                modbusValue.scaledValue = static_cast<qreal>(value);
//                modbusValue.timestamp = QDateTime::currentDateTime();
//                modbusValue.isValid = true;
                
                result.values.append(value);
            }
            
            result.rawResponse = response;
        } else {
            result.exception = ModbusException::InvalidResponse;
            result.errorString = QString("字节数不匹配: 期望%1, 实际%2")
                                .arg(expectedByteCount).arg(byteCount);
        }
    }
    
    return result;
}

QString ReadHoldingRegistersRequest::description() const
{
    return QString("读取保持寄存器 [从站:%1, 地址:%2, 数量:%3]")
           .arg(slaveId())
           .arg(m_startAddr)
            .arg(m_quantity);
}

int ReadHoldingRegistersRequest::quantity() const
{
    return m_quantity;
}

// WriteMultipleRegistersRequest实现
WriteMultipleRegistersRequest::WriteMultipleRegistersRequest(int slaveId, int startAddr,
                                                           const QVector<quint16>& values,
                                                           RequestPriority priority)
    : ModbusRequest(slaveId, ModbusFunctionCode::WriteMultipleRegisters, priority)
    , m_startAddr(startAddr)
    , m_values(values)
{
}

QByteArray WriteMultipleRegistersRequest::buildRequestData()
{
    QByteArray data;

    data.append(static_cast<char>(slaveId()));
    data.append(static_cast<char>(0x10));
    // 起始地址
    data.append(static_cast<char>((m_startAddr >> 8) & 0xFF));
    data.append(static_cast<char>(m_startAddr & 0xFF));
    
    // 寄存器数量
    int quantity = m_values.size();
    data.append(static_cast<char>((quantity >> 8) & 0xFF));
    data.append(static_cast<char>(quantity & 0xFF));
    
    // 字节数
    data.append(static_cast<char>(quantity * 2));
    
    // 寄存器数据
    for (quint16 value : m_values) {
        data.append(static_cast<char>((value >> 8) & 0xFF));
        data.append(static_cast<char>(value & 0xFF));
    }
    
    return data;
}

ModbusResult WriteMultipleRegistersRequest::parseResponse(const QByteArray& response)
{
    ModbusResult result;
    result.success = false;
    
    if (response.size() < 8) {
        result.exception = ModbusException::InvalidResponse;
        result.errorString = "响应长度不足";
        return result;
    }
    
    // 检查异常响应
    quint8 functionCode = static_cast<quint8>(response[1]);
    if (functionCode & 0x80) {
        if (response.size() >= 5) {
            quint8 errorCode = static_cast<quint8>(response[2]);
            result.exception = static_cast<ModbusException>(errorCode);
            result.errorString = exceptionToString(result.exception);
        }
        return result;
    }
    
    // 正常响应
    if (functionCode == static_cast<quint8>(ModbusFunctionCode::WriteMultipleRegisters)) {
        if (response.size() == 8) {
            // 验证地址和数量是否匹配
            quint16 startAddr = static_cast<quint8>(response[2]) << 8 | 
                               static_cast<quint8>(response[3]);
            quint16 quantity = static_cast<quint8>(response[4]) << 8 | 
                              static_cast<quint8>(response[5]);
            
            if (startAddr == static_cast<quint16>(m_startAddr) && 
                quantity == static_cast<quint16>(m_values.size())) {
                result.success = true;
                result.exception = ModbusException::NoException;
                result.rawResponse = response;
                
                // 将写入的值添加到结果中
                for (quint16 value : m_values) {
//                    ModbusValue modbusValue;
//                    modbusValue.value = value;
//                    modbusValue.scaledValue = static_cast<qreal>(value);
//                    modbusValue.timestamp = QDateTime::currentDateTime();
//                    modbusValue.isValid = true;
                    result.values.append(value);
                }
            } else {
                result.exception = ModbusException::InvalidResponse;
                result.errorString = QString("响应地址或数量不匹配");
            }
        }
    }
    
    return result;
}

QString WriteMultipleRegistersRequest::description() const
{
    return QString("写入多个寄存器 [从站:%1, 起始地址:%2, 数量:%3]")
           .arg(slaveId())
           .arg(m_startAddr)
           .arg(m_values.size());
}

// ReadCoilsRequest实现
ReadCoilsRequest::ReadCoilsRequest(int slaveId, int startAddr, int quantity,
                                 RequestPriority priority)
    : ModbusRequest(slaveId, ModbusFunctionCode::ReadCoils, priority)
    , m_startAddr(startAddr)
    , m_quantity(quantity)
{
}

QByteArray ReadCoilsRequest::buildRequestData()
{
    QByteArray data;
    data.append(static_cast<char>(slaveId()));
    data.append(static_cast<char>(0x01));
    data.append(static_cast<char>((m_startAddr >> 8) & 0xFF));
    data.append(static_cast<char>(m_startAddr & 0xFF));
    data.append(static_cast<char>((m_quantity >> 8) & 0xFF));
    data.append(static_cast<char>(m_quantity & 0xFF));
    return data;
}

ModbusResult ReadCoilsRequest::parseResponse(const QByteArray& response)
{
    ModbusResult result;
    result.success = false;
    
    if (response.size() < 5) {
        result.exception = ModbusException::InvalidResponse;
        result.errorString = "响应长度不足";
        return result;
    }
    
    // 检查异常响应
    quint8 functionCode = static_cast<quint8>(response[1]);
    if (functionCode & 0x80) {
        if (response.size() >= 5) {
            quint8 errorCode = static_cast<quint8>(response[2]);
            result.exception = static_cast<ModbusException>(errorCode);
            result.errorString = exceptionToString(result.exception);
        }
        return result;
    }
    
    // 正常响应
    if (functionCode == static_cast<quint8>(ModbusFunctionCode::ReadCoils)) {
        quint8 byteCount = static_cast<quint8>(response[2]);
        int expectedByteCount = (m_quantity + 7) / 8;
        
        if (byteCount == expectedByteCount && response.size() >= 3 + byteCount + 2) {
            result.success = true;
            result.exception = ModbusException::NoException;
            
            // 解析线圈状态
            for (int i = 0; i < m_quantity; i++) {
                int byteIndex = i / 8;
                int bitIndex = i % 8;
                
                if (byteIndex < byteCount) {
                    quint8 byteValue = static_cast<quint8>(response[3 + byteIndex]);
                    bool coilState = (byteValue >> bitIndex) & 0x01;
                    
//                    ModbusValue modbusValue;
//                    modbusValue.value = coilState ? 1 : 0;
//                    modbusValue.scaledValue = coilState ? 1.0 : 0.0;
//                    modbusValue.timestamp = QDateTime::currentDateTime();
//                    modbusValue.isValid = true;
                    
                    result.values.append(coilState ? 1 : 0);
                }
            }
            
            result.rawResponse = response;
        } else {
            result.exception = ModbusException::InvalidResponse;
            result.errorString = QString("字节数不匹配: 期望%1, 实际%2")
                                .arg(expectedByteCount).arg(byteCount);
        }
    }
    
    return result;
}

QString ReadCoilsRequest::description() const
{
    return QString("读取线圈 [从站:%1, 地址:%2, 数量:%3]")
           .arg(slaveId())
           .arg(m_startAddr)
            .arg(m_quantity);
}

// WriteSingleCoilRequest实现
WriteSingleCoilRequest::WriteSingleCoilRequest(int slaveId, int addr, bool value,
                                             RequestPriority priority)
    : ModbusRequest(slaveId, ModbusFunctionCode::WriteSingleCoil, priority)
    , m_addr(addr)
    , m_value(value)
{
}

QByteArray WriteSingleCoilRequest::buildRequestData()
{
    QByteArray data;
    data.append(static_cast<char>(slaveId()));
    data.append(static_cast<char>(0x05));
    data.append(static_cast<char>((m_addr >> 8) & 0xFF));
    data.append(static_cast<char>(m_addr & 0xFF));
    data.append(m_value ? static_cast<char>(0xFF) : static_cast<char>(0x00));
    data.append(static_cast<char>(0x00));
    return data;
}

ModbusResult WriteSingleCoilRequest::parseResponse(const QByteArray& response)
{
    ModbusResult result;
    result.success = false;
    
    if (response.size() < 8) {
        result.exception = ModbusException::InvalidResponse;
        result.errorString = "响应长度不足";
        return result;
    }
    
    // 检查异常响应
    quint8 functionCode = static_cast<quint8>(response[1]);
    if (functionCode & 0x80) {
        if (response.size() >= 5) {
            quint8 errorCode = static_cast<quint8>(response[2]);
            result.exception = static_cast<ModbusException>(errorCode);
            result.errorString = exceptionToString(result.exception);
        }
        return result;
    }
    
    // 正常响应
    if (functionCode == static_cast<quint8>(ModbusFunctionCode::WriteSingleCoil)) {
        if (response.size() == 8) {
            // 验证地址和值是否匹配
            quint16 addr = static_cast<quint8>(response[2]) << 8 | 
                          static_cast<quint8>(response[3]);
            quint16 value = static_cast<quint8>(response[4]) << 8 | 
                           static_cast<quint8>(response[5]);
            
            bool expectedValue = m_value ? 0xFF00 : 0x0000;
            
            if (addr == static_cast<quint16>(m_addr) && value == expectedValue) {
                result.success = true;
                result.exception = ModbusException::NoException;
                result.rawResponse = response;
                
                // 添加写入的值到结果
//                ModbusValue modbusValue;
//                modbusValue.value = m_value ? 1 : 0;
//                modbusValue.scaledValue = m_value ? 1.0 : 0.0;
//                modbusValue.timestamp = QDateTime::currentDateTime();
//                modbusValue.isValid = true;
                result.values.append(m_value ? 1 : 0);
            } else {
                result.exception = ModbusException::InvalidResponse;
                result.errorString = QString("响应地址或值不匹配");
            }
        }
    }
    
    return result;
}

QString WriteSingleCoilRequest::description() const
{
    return QString("写入单个线圈 [从站:%1, 地址:%2, 值:%3]")
           .arg(slaveId())
           .arg(m_addr)
           .arg(m_value ? "ON" : "OFF");
}

// ReadInputRegistersRequest实现
ReadInputRegistersRequest::ReadInputRegistersRequest(int slaveId, int startAddr, int quantity,
                                                   RequestPriority priority)
    : ModbusRequest(slaveId, ModbusFunctionCode::ReadInputRegisters, priority)
    , m_startAddr(startAddr)
    , m_quantity(quantity)
{
}

QByteArray ReadInputRegistersRequest::buildRequestData()
{
    QByteArray data;
    data.append(static_cast<char>(slaveId()));
    data.append(static_cast<char>(0x04));
    data.append(static_cast<char>((m_startAddr >> 8) & 0xFF));
    data.append(static_cast<char>(m_startAddr & 0xFF));
    data.append(static_cast<char>((m_quantity >> 8) & 0xFF));
    data.append(static_cast<char>(m_quantity & 0xFF));
    return data;
}

ModbusResult ReadInputRegistersRequest::parseResponse(const QByteArray& response)
{
    ModbusResult result;
    result.success = false;
    
    if (response.size() < 5) {
        result.exception = ModbusException::InvalidResponse;
        result.errorString = "响应长度不足";
        return result;
    }
    
    // 检查异常响应
    quint8 functionCode = static_cast<quint8>(response[1]);
    if (functionCode & 0x80) {
        if (response.size() >= 5) {
            quint8 errorCode = static_cast<quint8>(response[2]);
            result.exception = static_cast<ModbusException>(errorCode);
            result.errorString = exceptionToString(result.exception);
        }
        return result;
    }
    
    // 正常响应
    if (functionCode == static_cast<quint8>(ModbusFunctionCode::ReadInputRegisters)) {
        quint8 byteCount = static_cast<quint8>(response[2]);
        int expectedByteCount = m_quantity * 2;
        
        if (byteCount == expectedByteCount && response.size() >= 3 + byteCount + 2) {
            result.success = true;
            result.exception = ModbusException::NoException;
            
            for (int i = 0; i < m_quantity; i++) {
                int offset = 3 + i * 2;
                quint16 value = static_cast<quint8>(response[offset]) << 8 | 
                                static_cast<quint8>(response[offset + 1]);
                
//                ModbusValue modbusValue;
//                modbusValue.value = value;
//                modbusValue.scaledValue = static_cast<qreal>(value);
//                modbusValue.timestamp = QDateTime::currentDateTime();
//                modbusValue.isValid = true;
                
                result.values.append(value);
            }
            
            result.rawResponse = response;
        } else {
            result.exception = ModbusException::InvalidResponse;
            result.errorString = QString("字节数不匹配: 期望%1, 实际%2")
                                .arg(expectedByteCount).arg(byteCount);
        }
    }
    
    return result;
}

QString ReadInputRegistersRequest::description() const
{
    return QString("读取输入寄存器 [从站:%1, 地址:%2, 数量:%3]")
           .arg(slaveId())
           .arg(m_startAddr)
           .arg(m_quantity);
}

int ReadInputRegistersRequest::quantity() const
{
    return m_quantity;
}

// WriteMultipleCoilsRequest实现
WriteMultipleCoilsRequest::WriteMultipleCoilsRequest(int slaveId, int startAddr,
                                                   const QVector<bool>& values,
                                                   RequestPriority priority)
    : ModbusRequest(slaveId, ModbusFunctionCode::WriteMultipleCoils, priority)
    , m_startAddr(startAddr)
    , m_values(values)
{
}

QByteArray WriteMultipleCoilsRequest::buildRequestData()
{
    QByteArray data;

    data.append(static_cast<char>(slaveId()));
    data.append(static_cast<char>(0x0F));

    // 起始地址
    data.append(static_cast<char>((m_startAddr >> 8) & 0xFF));
    data.append(static_cast<char>(m_startAddr & 0xFF));
    
    // 线圈数量
    int quantity = m_values.size();
    data.append(static_cast<char>((quantity >> 8) & 0xFF));
    data.append(static_cast<char>(quantity & 0xFF));
    
    // 字节数
    int byteCount = (quantity + 7) / 8;
    data.append(static_cast<char>(byteCount));
    
    // 线圈数据
    for (int byteIndex = 0; byteIndex < byteCount; byteIndex++) {
        quint8 byteValue = 0;
        for (int bitIndex = 0; bitIndex < 8; bitIndex++) {
            int coilIndex = byteIndex * 8 + bitIndex;
            if (coilIndex < quantity && m_values[coilIndex]) {
                byteValue |= (1 << bitIndex);
            }
        }
        data.append(static_cast<char>(byteValue));
    }
    
    return data;
}

ModbusResult WriteMultipleCoilsRequest::parseResponse(const QByteArray& response)
{
    ModbusResult result;
    result.success = false;
    
    if (response.size() < 8) {
        result.exception = ModbusException::InvalidResponse;
        result.errorString = "响应长度不足";
        return result;
    }
    
    // 检查异常响应
    quint8 functionCode = static_cast<quint8>(response[1]);
    if (functionCode & 0x80) {
        if (response.size() >= 5) {
            quint8 errorCode = static_cast<quint8>(response[2]);
            result.exception = static_cast<ModbusException>(errorCode);
            result.errorString = exceptionToString(result.exception);
        }
        return result;
    }
    
    // 正常响应
    if (functionCode == static_cast<quint8>(ModbusFunctionCode::WriteMultipleCoils)) {
        if (response.size() == 8) {
            // 验证地址和数量是否匹配
            quint16 startAddr = static_cast<quint8>(response[2]) << 8 | 
                               static_cast<quint8>(response[3]);
            quint16 quantity = static_cast<quint8>(response[4]) << 8 | 
                              static_cast<quint8>(response[5]);
            
            if (startAddr == static_cast<quint16>(m_startAddr) && 
                quantity == static_cast<quint16>(m_values.size())) {
                result.success = true;
                result.exception = ModbusException::NoException;
                result.rawResponse = response;
                
                // 将写入的线圈值添加到结果中
                for (bool value : m_values) {
//                    ModbusValue modbusValue;
//                    modbusValue.value = value ? 1 : 0;
//                    modbusValue.scaledValue = value ? 1.0 : 0.0;
//                    modbusValue.timestamp = QDateTime::currentDateTime();
//                    modbusValue.isValid = true;
                    result.values.append(value ? 1 : 0);
                }
            } else {
                result.exception = ModbusException::InvalidResponse;
                result.errorString = QString("响应地址或数量不匹配");
            }
        }
    }
    
    return result;
}

QString WriteMultipleCoilsRequest::description() const
{
    return QString("写入多个线圈 [从站:%1, 起始地址:%2, 数量:%3]")
           .arg(slaveId())
           .arg(m_startAddr)
           .arg(m_values.size());
}

// ReadDiscreteInputsRequest实现
ReadDiscreteInputsRequest::ReadDiscreteInputsRequest(int slaveId, int startAddr, int quantity,
                                                   RequestPriority priority)
    : ModbusRequest(slaveId, ModbusFunctionCode::ReadDiscreteInputs, priority)
    , m_startAddr(startAddr)
    , m_quantity(quantity)
{
}

QByteArray ReadDiscreteInputsRequest::buildRequestData()
{
    QByteArray data;
    data.append(static_cast<char>(slaveId()));
    data.append(static_cast<char>(0x02));
    data.append(static_cast<char>((m_startAddr >> 8) & 0xFF));
    data.append(static_cast<char>(m_startAddr & 0xFF));
    data.append(static_cast<char>((m_quantity >> 8) & 0xFF));
    data.append(static_cast<char>(m_quantity & 0xFF));
    return data;
}

ModbusResult ReadDiscreteInputsRequest::parseResponse(const QByteArray& response)
{
    ModbusResult result;
    result.success = false;
    
    if (response.size() < 5) {
        result.exception = ModbusException::InvalidResponse;
        result.errorString = "响应长度不足";
        return result;
    }
    
    // 检查异常响应
    quint8 functionCode = static_cast<quint8>(response[1]);
    if (functionCode & 0x80) {
        if (response.size() >= 5) {
            quint8 errorCode = static_cast<quint8>(response[2]);
            result.exception = static_cast<ModbusException>(errorCode);
            result.errorString = exceptionToString(result.exception);
        }
        return result;
    }
    
    // 正常响应
    if (functionCode == static_cast<quint8>(ModbusFunctionCode::ReadDiscreteInputs)) {
        quint8 byteCount = static_cast<quint8>(response[2]);
        int expectedByteCount = (m_quantity + 7) / 8;
        
        if (byteCount == expectedByteCount && response.size() >= 3 + byteCount + 2) {
            result.success = true;
            result.exception = ModbusException::NoException;
            
            // 解析离散输入状态
            for (int i = 0; i < m_quantity; i++) {
                int byteIndex = i / 8;
                int bitIndex = i % 8;
                
                if (byteIndex < byteCount) {
                    quint8 byteValue = static_cast<quint8>(response[3 + byteIndex]);
                    bool inputState = (byteValue >> bitIndex) & 0x01;
                    
//                    ModbusValue modbusValue;
//                    modbusValue.value = inputState ? 1 : 0;
//                    modbusValue.scaledValue = inputState ? 1.0 : 0.0;
//                    modbusValue.timestamp = QDateTime::currentDateTime();
//                    modbusValue.isValid = true;
                    
                    result.values.append(inputState ? 1 : 0);
                }
            }
            
            result.rawResponse = response;
        } else {
            result.exception = ModbusException::InvalidResponse;
            result.errorString = QString("字节数不匹配: 期望%1, 实际%2")
                                .arg(expectedByteCount).arg(byteCount);
        }
    }
    
    return result;
}

QString ReadDiscreteInputsRequest::description() const
{
    return QString("读取离散输入 [从站:%1, 地址:%2, 数量:%3]")
           .arg(slaveId())
           .arg(m_startAddr)
           .arg(m_quantity);
}

int ReadDiscreteInputsRequest::quantity() const
{
    return m_quantity;
}

// WriteSingleRegisterRequest实现
WriteSingleRegisterRequest::WriteSingleRegisterRequest(int slaveId, int addr, quint16 value,
                                                     RequestPriority priority)
    : ModbusRequest(slaveId, ModbusFunctionCode::WriteSingleRegister, priority)
    , m_addr(addr)
    , m_value(value)
{
}

QByteArray WriteSingleRegisterRequest::buildRequestData()
{
    QByteArray data;
    data.append(static_cast<char>(slaveId()));
    data.append(static_cast<char>(0x06));
    data.append(static_cast<char>((m_addr >> 8) & 0xFF));
    data.append(static_cast<char>(m_addr & 0xFF));
    data.append(static_cast<char>((m_value >> 8) & 0xFF));
    data.append(static_cast<char>(m_value & 0xFF));
    return data;
}

ModbusResult WriteSingleRegisterRequest::parseResponse(const QByteArray& response)
{
    ModbusResult result;
    result.success = false;
    
    if (response.size() < 8) {
        result.exception = ModbusException::InvalidResponse;
        result.errorString = "响应长度不足";
        return result;
    }
    
    // 检查异常响应
    quint8 functionCode = static_cast<quint8>(response[1]);
    if (functionCode & 0x80) {
        if (response.size() >= 5) {
            quint8 errorCode = static_cast<quint8>(response[2]);
            result.exception = static_cast<ModbusException>(errorCode);
            result.errorString = exceptionToString(result.exception);
        }
        return result;
    }
    
    // 正常响应
    if (functionCode == static_cast<quint8>(ModbusFunctionCode::WriteSingleRegister)) {
        if (response.size() == 8) {
            // 验证地址和值是否匹配
            quint16 addr = static_cast<quint8>(response[2]) << 8 | 
                          static_cast<quint8>(response[3]);
            quint16 value = static_cast<quint8>(response[4]) << 8 | 
                           static_cast<quint8>(response[5]);
            
            if (addr == static_cast<quint16>(m_addr) && value == m_value) {
                result.success = true;
                result.exception = ModbusException::NoException;
                result.rawResponse = response;
                
                // 添加写入的值到结果
//                ModbusValue modbusValue;
//                modbusValue.value = m_value;
//                modbusValue.scaledValue = static_cast<qreal>(m_value);
//                modbusValue.timestamp = QDateTime::currentDateTime();
//                modbusValue.isValid = true;
                result.values.append(m_value);
            } else {
                result.exception = ModbusException::InvalidResponse;
                result.errorString = QString("响应地址或值不匹配");
            }
        }
    }
    
    return result;
}

QString WriteSingleRegisterRequest::description() const
{
    return QString("写入单个寄存器 [从站:%1, 地址:%2, 值:%3]")
           .arg(slaveId())
           .arg(m_addr)
           .arg(m_value);
}
