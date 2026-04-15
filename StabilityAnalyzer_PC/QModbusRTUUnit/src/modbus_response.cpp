#include "modbus_response.h"
#include <QDebug>

// ReadHoldingRegistersResponse 实现
bool ReadHoldingRegistersResponse::parseFromRawData(const QByteArray& data)
{
    if (data.size() < 5) {
        m_isValid = false;
        return false;
    }
    
    // 检查异常响应
    if ((data[1] & 0x80) != 0) {
        m_exceptionCode = static_cast<ModbusException>(data[2]);
        m_isValid = true;
        return true;
    }
    
    // 正常响应格式：从站地址(1) + 功能码(1) + 字节数(1) + 寄存器值(N*2) + CRC(2)
    m_byteCount = static_cast<quint8>(data[2]);
    
    if (data.size() != 3 + m_byteCount + 2) {
        m_isValid = false;
        return false;
    }
    
    // 解析寄存器值
    m_registers.clear();
    for (int i = 0; i < m_byteCount; i += 2) {
        if (i + 3 >= data.size()) {
            break;
        }
        
        quint16 value = (static_cast<quint8>(data[3 + i]) << 8) | static_cast<quint8>(data[4 + i]);
        m_registers.append(value);
    }
    
    m_exceptionCode = NoException;
    m_isValid = true;
    return true;
}

// ReadInputRegistersResponse 实现
bool ReadInputRegistersResponse::parseFromRawData(const QByteArray& data)
{
    if (data.size() < 5) {
        m_isValid = false;
        return false;
    }
    
    // 检查异常响应
    if ((data[1] & 0x80) != 0) {
        m_exceptionCode = static_cast<ModbusException>(data[2]);
        m_isValid = true;
        return true;
    }
    
    // 正常响应格式：从站地址(1) + 功能码(1) + 字节数(1) + 寄存器值(N*2) + CRC(2)
    m_byteCount = static_cast<quint8>(data[2]);
    
    if (data.size() != 3 + m_byteCount + 2) {
        m_isValid = false;
        return false;
    }
    
    // 解析寄存器值
    m_registers.clear();
    for (int i = 0; i < m_byteCount; i += 2) {
        if (i + 3 >= data.size()) {
            break;
        }
        
        quint16 value = (static_cast<quint8>(data[3 + i]) << 8) | static_cast<quint8>(data[4 + i]);
        m_registers.append(value);
    }
    
    m_exceptionCode = NoException;
    m_isValid = true;
    return true;
}

// ReadCoilsResponse 实现
bool ReadCoilsResponse::parseFromRawData(const QByteArray& data)
{
    if (data.size() < 5) {
        m_isValid = false;
        return false;
    }
    
    // 检查异常响应
    if ((data[1] & 0x80) != 0) {
        m_exceptionCode = static_cast<ModbusException>(data[2]);
        m_isValid = true;
        return true;
    }
    
    // 正常响应格式：从站地址(1) + 功能码(1) + 字节数(1) + 线圈状态(N) + CRC(2)
    m_byteCount = static_cast<quint8>(data[2]);
    
    if (data.size() != 3 + m_byteCount + 2) {
        m_isValid = false;
        return false;
    }
    
    // 解析线圈状态
    m_coils.clear();
    for (int i = 0; i < m_byteCount; ++i) {
        if (i + 3 >= data.size()) {
            break;
        }
        
        quint8 byte = static_cast<quint8>(data[3 + i]);
        for (int j = 0; j < 8; ++j) {
            bool state = (byte & (1 << j)) != 0;
            m_coils.append(state);
        }
    }
    
    m_exceptionCode = NoException;
    m_isValid = true;
    return true;
}

// ReadDiscreteInputsResponse 实现
bool ReadDiscreteInputsResponse::parseFromRawData(const QByteArray& data)
{
    if (data.size() < 5) {
        m_isValid = false;
        return false;
    }
    
    // 检查异常响应
    if ((data[1] & 0x80) != 0) {
        m_exceptionCode = static_cast<ModbusException>(data[2]);
        m_isValid = true;
        return true;
    }
    
    // 正常响应格式：从站地址(1) + 功能码(1) + 字节数(1) + 离散输入状态(N) + CRC(2)
    m_byteCount = static_cast<quint8>(data[2]);
    
    if (data.size() != 3 + m_byteCount + 2) {
        m_isValid = false;
        return false;
    }
    
    // 解析离散输入状态
    m_inputs.clear();
    for (int i = 0; i < m_byteCount; ++i) {
        if (i + 3 >= data.size()) {
            break;
        }
        
        quint8 byte = static_cast<quint8>(data[3 + i]);
        for (int j = 0; j < 8; ++j) {
            bool state = (byte & (1 << j)) != 0;
            m_inputs.append(state);
        }
    }
    
    m_exceptionCode = NoException;
    m_isValid = true;
    return true;
}

// WriteSingleCoilResponse 实现
bool WriteSingleCoilResponse::parseFromRawData(const QByteArray& data)
{
    if (data.size() < 8) {
        m_isValid = false;
        return false;
    }
    
    // 检查异常响应
    if ((data[1] & 0x80) != 0) {
        m_exceptionCode = static_cast<ModbusException>(data[2]);
        m_isValid = true;
        return true;
    }
    
    // 正常响应格式：从站地址(1) + 功能码(1) + 地址(2) + 值(2) + CRC(2)
    m_address = (static_cast<quint8>(data[2]) << 8) | static_cast<quint8>(data[3]);
    quint16 value = (static_cast<quint8>(data[4]) << 8) | static_cast<quint8>(data[5]);
    m_value = (value == 0xFF00);
    
    m_exceptionCode = NoException;
    m_isValid = true;
    return true;
}

// WriteSingleRegisterResponse 实现
bool WriteSingleRegisterResponse::parseFromRawData(const QByteArray& data)
{
    if (data.size() < 8) {
        m_isValid = false;
        return false;
    }
    
    // 检查异常响应
    if ((data[1] & 0x80) != 0) {
        m_exceptionCode = static_cast<ModbusException>(data[2]);
        m_isValid = true;
        return true;
    }
    
    // 正常响应格式：从站地址(1) + 功能码(1) + 地址(2) + 值(2) + CRC(2)
    m_address = (static_cast<quint8>(data[2]) << 8) | static_cast<quint8>(data[3]);
    m_value = (static_cast<quint8>(data[4]) << 8) | static_cast<quint8>(data[5]);
    
    m_exceptionCode = NoException;
    m_isValid = true;
    return true;
}

// WriteMultipleCoilsResponse 实现
bool WriteMultipleCoilsResponse::parseFromRawData(const QByteArray& data)
{
    if (data.size() < 8) {
        m_isValid = false;
        return false;
    }
    
    // 检查异常响应
    if ((data[1] & 0x80) != 0) {
        m_exceptionCode = static_cast<ModbusException>(data[2]);
        m_isValid = true;
        return true;
    }
    
    // 正常响应格式：从站地址(1) + 功能码(1) + 地址(2) + 数量(2) + CRC(2)
    m_address = (static_cast<quint8>(data[2]) << 8) | static_cast<quint8>(data[3]);
    m_quantity = (static_cast<quint8>(data[4]) << 8) | static_cast<quint8>(data[5]);
    
    m_exceptionCode = NoException;
    m_isValid = true;
    return true;
}

// WriteMultipleRegistersResponse 实现
bool WriteMultipleRegistersResponse::parseFromRawData(const QByteArray& data)
{
    if (data.size() < 8) {
        m_isValid = false;
        return false;
    }
    
    // 检查异常响应
    if ((data[1] & 0x80) != 0) {
        m_exceptionCode = static_cast<ModbusException>(data[2]);
        m_isValid = true;
        return true;
    }
    
    // 正常响应格式：从站地址(1) + 功能码(1) + 地址(2) + 数量(2) + CRC(2)
    m_address = (static_cast<quint8>(data[2]) << 8) | static_cast<quint8>(data[3]);
    m_quantity = (static_cast<quint8>(data[4]) << 8) | static_cast<quint8>(data[5]);
    
    m_exceptionCode = NoException;
    m_isValid = true;
    return true;
}