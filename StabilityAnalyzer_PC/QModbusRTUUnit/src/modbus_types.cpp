#include "modbus_types.h"
#include <QDataStream>
#include <QDebug>

// 静态成员初始化
QDataStream &operator<<(QDataStream &out, const ModbusValue &value)
{
    out << value.value << value.scaledValue << value.timestamp << value.isValid;
    return out;
}

QDataStream &operator>>(QDataStream &in, ModbusValue &value)
{
    in >> value.value >> value.scaledValue >> value.timestamp >> value.isValid;
    return in;
}

// 注册元类型
static bool registerMetaTypes()
{
    qRegisterMetaType<ModbusException>("ModbusException");
    qRegisterMetaType<ModbusFunctionCode>("ModbusFunctionCode");
    qRegisterMetaType<RequestPriority>("RequestPriority");
    qRegisterMetaType<RequestStatus>("RequestStatus");
    qRegisterMetaType<ConnectionState>("ConnectionState");
    qRegisterMetaType<ModbusValue>("ModbusValue");
    qRegisterMetaType<RegisterInfo>("RegisterInfo");
    qRegisterMetaType<ModbusResult>("ModbusResult");
    qRegisterMetaType<CommunicationStats>("CommunicationStats");
    qRegisterMetaType<SerialPortConfig>("SerialPortConfig");
    return true;
}

static bool registered = registerMetaTypes();

// 枚举转字符串
QString modbusExceptionToString(ModbusException exception)
{
    switch (exception) {
    case ModbusException::NoException: return "NoException";
    case ModbusException::IllegalFunction: return "IllegalFunction";
    case ModbusException::IllegalDataAddress: return "IllegalDataAddress";
    case ModbusException::IllegalDataValue: return "IllegalDataValue";
    case ModbusException::ServerDeviceFailure: return "ServerDeviceFailure";
    case ModbusException::Acknowledge: return "Acknowledge";
    case ModbusException::ServerDeviceBusy: return "ServerDeviceBusy";
    case ModbusException::NegativeAcknowledge: return "NegativeAcknowledge";
    case ModbusException::MemoryParityError: return "MemoryParityError";
    case ModbusException::GatewayPathUnavailable: return "GatewayPathUnavailable";
    case ModbusException::GatewayTargetDeviceFailedToRespond: return "GatewayTargetDeviceFailedToRespond";
    case ModbusException::TimeoutError: return "TimeoutError";
    case ModbusException::CRCCheckError: return "CRCCheckError";
    case ModbusException::FrameError: return "FrameError";
    case ModbusException::PortError: return "PortError";
    case ModbusException::BufferOverflowError: return "BufferOverflowError";
    case ModbusException::RequestQueueFull: return "RequestQueueFull";
    case ModbusException::ConnectionError: return "ConnectionError";
    case ModbusException::InvalidResponse: return "InvalidResponse";
    default: return QString("Unknown(%1)").arg(static_cast<int>(exception));
    }
}

QString modbusFunctionCodeToString(ModbusFunctionCode functionCode)
{
    switch (functionCode) {
    case ModbusFunctionCode::ReadCoils: return "ReadCoils";
    case ModbusFunctionCode::ReadDiscreteInputs: return "ReadDiscreteInputs";
    case ModbusFunctionCode::ReadHoldingRegisters: return "ReadHoldingRegisters";
    case ModbusFunctionCode::ReadInputRegisters: return "ReadInputRegisters";
    case ModbusFunctionCode::WriteSingleCoil: return "WriteSingleCoil";
    case ModbusFunctionCode::WriteSingleRegister: return "WriteSingleRegister";
    case ModbusFunctionCode::WriteMultipleCoils: return "WriteMultipleCoils";
    case ModbusFunctionCode::WriteMultipleRegisters: return "WriteMultipleRegisters";
    case ModbusFunctionCode::MaskWriteRegister: return "MaskWriteRegister";
    case ModbusFunctionCode::ReadWriteMultipleRegisters: return "ReadWriteMultipleRegisters";
    case ModbusFunctionCode::ReadFIFOQueue: return "ReadFIFOQueue";
    default: return QString("Unknown(0x%1)").arg(static_cast<int>(functionCode), 2, 16, QChar('0'));
    }
}