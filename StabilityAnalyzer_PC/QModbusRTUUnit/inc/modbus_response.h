#ifndef MODBUS_RESPONSE_H
#define MODBUS_RESPONSE_H

#include "modbus_types.h"
#include <QByteArray>
#include <QVector>
#include "qmodbusrtuunit_global.h"

/**
 * @brief Modbus响应基类
 */
class QMODBUSRTUUNIT_EXPORT ModbusResponse
{
public:
    ModbusResponse() = default;
    virtual ~ModbusResponse() = default;
    
    /**
     * @brief 从原始数据解析响应
     * @param data 原始响应数据
     * @return 解析是否成功
     */
    virtual bool parseFromRawData(const QByteArray& data) = 0;
    
    /**
     * @brief 获取异常代码
     */
    virtual ModbusException getExceptionCode() const = 0;
    
    /**
     * @brief 检查响应是否有效
     */
    virtual bool isValid() const = 0;
};

/**
 * @brief 读取保持寄存器响应
 */
class ReadHoldingRegistersResponse : public ModbusResponse
{
public:
    ReadHoldingRegistersResponse() = default;
    
    bool parseFromRawData(const QByteArray& data) override;
    ModbusException getExceptionCode() const override { return m_exceptionCode; }
    bool isValid() const override { return m_isValid; }
    
    QVector<quint16> getRegisters() const { return m_registers; }
    int getByteCount() const { return m_byteCount; }
    
private:
    QVector<quint16> m_registers;
    int m_byteCount = 0;
    ModbusException m_exceptionCode = NoException;
    bool m_isValid = false;
};

/**
 * @brief 读取输入寄存器响应
 */
class ReadInputRegistersResponse : public ModbusResponse
{
public:
    ReadInputRegistersResponse() = default;
    
    bool parseFromRawData(const QByteArray& data) override;
    ModbusException getExceptionCode() const override { return m_exceptionCode; }
    bool isValid() const override { return m_isValid; }
    
    QVector<quint16> getRegisters() const { return m_registers; }
    int getByteCount() const { return m_byteCount; }
    
private:
    QVector<quint16> m_registers;
    int m_byteCount = 0;
    ModbusException m_exceptionCode = NoException;
    bool m_isValid = false;
};

/**
 * @brief 读取线圈状态响应
 */
class ReadCoilsResponse : public ModbusResponse
{
public:
    ReadCoilsResponse() = default;
    
    bool parseFromRawData(const QByteArray& data) override;
    ModbusException getExceptionCode() const override { return m_exceptionCode; }
    bool isValid() const override { return m_isValid; }
    
    QVector<bool> getCoils() const { return m_coils; }
    int getByteCount() const { return m_byteCount; }
    
private:
    QVector<bool> m_coils;
    int m_byteCount = 0;
    ModbusException m_exceptionCode = NoException;
    bool m_isValid = false;
};

/**
 * @brief 读取离散输入响应
 */
class ReadDiscreteInputsResponse : public ModbusResponse
{
public:
    ReadDiscreteInputsResponse() = default;
    
    bool parseFromRawData(const QByteArray& data) override;
    ModbusException getExceptionCode() const override { return m_exceptionCode; }
    bool isValid() const override { return m_isValid; }
    
    QVector<bool> getInputs() const { return m_inputs; }
    int getByteCount() const { return m_byteCount; }
    
private:
    QVector<bool> m_inputs;
    int m_byteCount = 0;
    ModbusException m_exceptionCode = NoException;
    bool m_isValid = false;
};

/**
 * @brief 写入单个线圈响应
 */
class WriteSingleCoilResponse : public ModbusResponse
{
public:
    WriteSingleCoilResponse() = default;
    
    bool parseFromRawData(const QByteArray& data) override;
    ModbusException getExceptionCode() const override { return m_exceptionCode; }
    bool isValid() const override { return m_isValid; }
    
    int getAddress() const { return m_address; }
    bool getValue() const { return m_value; }
    
private:
    int m_address = 0;
    bool m_value = false;
    ModbusException m_exceptionCode = NoException;
    bool m_isValid = false;
};

/**
 * @brief 写入单个寄存器响应
 */
class WriteSingleRegisterResponse : public ModbusResponse
{
public:
    WriteSingleRegisterResponse() = default;
    
    bool parseFromRawData(const QByteArray& data) override;
    ModbusException getExceptionCode() const override { return m_exceptionCode; }
    bool isValid() const override { return m_isValid; }
    
    int getAddress() const { return m_address; }
    quint16 getValue() const { return m_value; }
    
private:
    int m_address = 0;
    quint16 m_value = 0;
    ModbusException m_exceptionCode = NoException;
    bool m_isValid = false;
};

/**
 * @brief 写入多个线圈响应
 */
class WriteMultipleCoilsResponse : public ModbusResponse
{
public:
    WriteMultipleCoilsResponse() = default;
    
    bool parseFromRawData(const QByteArray& data) override;
    ModbusException getExceptionCode() const override { return m_exceptionCode; }
    bool isValid() const override { return m_isValid; }
    
    int getAddress() const { return m_address; }
    int getQuantity() const { return m_quantity; }
    
private:
    int m_address = 0;
    int m_quantity = 0;
    ModbusException m_exceptionCode = NoException;
    bool m_isValid = false;
};

/**
 * @brief 写入多个寄存器响应
 */
class WriteMultipleRegistersResponse : public ModbusResponse
{
public:
    WriteMultipleRegistersResponse() = default;
    
    bool parseFromRawData(const QByteArray& data) override;
    ModbusException getExceptionCode() const override { return m_exceptionCode; }
    bool isValid() const override { return m_isValid; }
    
    int getAddress() const { return m_address; }
    int getQuantity() const { return m_quantity; }
    
private:
    int m_address = 0;
    int m_quantity = 0;
    ModbusException m_exceptionCode = NoException;
    bool m_isValid = false;
};

#endif // MODBUS_RESPONSE_H
