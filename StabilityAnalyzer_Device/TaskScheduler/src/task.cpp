#include "task.h"
#include <QDebug>
#include <QMetaObject>
#include <QMetaMethod>
#include <QThread>
#include "device.h"

Task::Task(const QJsonObject &config, QObject *parent)
    : QObject(parent)
    , m_taskStatus(TaskStatus::PENDING)
    , m_device(nullptr)
    , m_scaleFactor(1.0)
    , m_offset(0.0)
    , m_isSync(0)
{
    qRegisterMetaType<QVector<quint16>>("QVector<quint16>");

    // 只使用任务名称，不再使用taskId
    m_taskName = config.value("name").toString();

    // 判断任务类型：根据operate字段判断（0=初始化任务，1=事件触发任务）
    int operate = config.value("operate").toInt();
    m_taskType = TaskType::USER_TASK;
    if (operate == 0) {
        m_taskType = TaskType::INIT_TASK;  // 初始化任务，初始化任务一般都是轮询任务
    } else if (operate == 1) {
        m_taskType = TaskType::USER_TASK;  // 事件触发任务
    }

    m_taskStatus = TaskStatus::PENDING; //设置任务状态

    // 解析Modbus功能码和地址
    QString type = config.value("type").toString();
    m_functionCode = parseFunctionCode(type);
    m_startAddress = static_cast<quint16>(config.value("address").toInt());
    m_quantity = static_cast<quint16>(config.value("count").toInt());
    if(config.contains("pollingTime"))
        m_interval = config.value("pollingTime").toInt();

    // 从配置中读取设备ID
    m_deviceId = config.value("deviceId").toString();

    // 生成唯一任务标识符，用于数据隔离
    m_taskId = TaskResult::generateTaskId(m_deviceId, m_taskName);

    //    m_dataType = config.value("dataType").toString("uint16");
    //    m_scaleFactor = config.value("scaleFactor").toDouble(1.0);
    //    if (m_scaleFactor == 1.0) {
    //        m_scaleFactor = config.value("scale_factor").toDouble(1.0);
    //    }
    //    m_offset = config.value("offset").toDouble(0.0);

//    qDebug()<< "任务Name:" << m_taskName
//            << "Type:" << (m_taskType == TaskType::INIT_TASK ? "INIT" : "USER")
//            << "Device:" << m_deviceId;
}

//执行任务
QVector<quint16> Task::execute()
{
    if (m_taskStatus == TaskStatus::RUNNING) {
        qWarning() << "Task is already running:" << m_taskName;
        return QVector<quint16>();
    }
    
    setStatus(TaskStatus::RUNNING);
    
    // 任务执行
    // qDebug() << "Executing task:" << m_taskName
    //          << "Device:" << m_deviceId
    //          << "Address:" << m_startAddress
    //          << "Quantity:" << m_quantity;
    
    //创建返回结果枚举
    TaskResult res;
    res.deviceId = m_deviceId;
    res.taskName = m_taskName;
    res.isException = true;
    res.remark = m_taskReamark;

    // 检查设备实例是否可用
    if (!m_device) {
        qWarning() << "Device not available for task:" << m_taskName << "m_device pointer:" << m_device;
        qWarning() << "Task deviceId:" << m_deviceId;
        setStatus(TaskStatus::FAILED);
        emit taskCompleted(res, QVector<quint16>()); //在接收返回结果的信号槽和方法里面一定要记得判断下集合的长度再做处理
        return QVector<quint16>();
    }
    
    // 从Device实例获取ModbusClient实例
    Device *device = qobject_cast<Device*>(m_device);
    if (!device) {
        qWarning() << "Device object is not a Device instance for task:" << m_taskName;
        setStatus(TaskStatus::FAILED);
        emit taskCompleted(res, QVector<quint16>());
        return QVector<quint16>();
    }
    
    QObject *modbusClient = device->modbusClient();
    
    if (!modbusClient) {
        qWarning() << "ModbusClient not available for device:" << m_deviceId << "task:" << m_taskName;
        setStatus(TaskStatus::FAILED);
        emit taskCompleted(res, QVector<quint16>());
        return QVector<quint16>();
    }
    
    // 检查modbusClient是否是ModbusClient类型
    ModbusClient *client = qobject_cast<ModbusClient*>(modbusClient);
    if (!client) {
        qWarning() << "ModbusClient object is not a ModbusClient instance for task:" << m_taskName;
        setStatus(TaskStatus::FAILED);
        emit taskCompleted(res, QVector<quint16>());
        return QVector<quint16>();
    }
    
    //如果这个任务对应的客户端没连接就返回
    const bool clientConnected = client->isConnected();
    // qDebug() << "[Task][Execute] task=" << m_taskName
    //          << "device=" << m_deviceId
    //          << "isSync=" << m_isSync
    //          << "function=" << static_cast<int>(m_functionCode)
    //          << "startAddress=" << m_startAddress
    //          << "quantity=" << m_quantity
    //          << "client=" << client
    //          << "clientConnected=" << clientConnected
    //          << "taskThread=" << QThread::currentThread()
    //          << "taskObjectThread=" << this->thread();
    if(!clientConnected){
        qWarning() << "ModbusClient object is not connected:" << m_taskName
                   << "device=" << m_deviceId;
        setStatus(TaskStatus::FAILED);
        emit taskCompleted(res, QVector<quint16>());
        return QVector<quint16>();
    }

    // 根据m_isSync属性决定执行模式
    if (m_isSync) {
        // 同步执行模式 - 直接调用同步方法并返回结果
        // qDebug() << "Executing task in sync mode:" << m_taskName;
        
        QVector<quint16> result;
        bool success = false;
        // qDebug() << "[Task][Sync] begin task=" << m_taskName
        //          << "device=" << m_deviceId
        //          << "function=" << static_cast<int>(m_functionCode)
        //          << "writeDataSize=" << m_writeData.size();
        
        // 根据功能码执行同步操作
        switch (m_functionCode) {
            case ModbusFunction::READ_COILS:
                result = client->readCoils(m_deviceId.toInt(), m_startAddress, m_quantity);
                success = !result.isEmpty();
                break;
            case ModbusFunction::READ_DISCRETE_INPUTS:
                result = client->readDiscreteInputs(m_deviceId.toInt(), m_startAddress, m_quantity);
                success = !result.isEmpty();
                break;
            case ModbusFunction::READ_HOLDING_REGISTERS:
                result = client->readHoldingRegisters(m_deviceId.toInt(), m_startAddress, m_quantity);
                success = !result.isEmpty();
                break;
            case ModbusFunction::READ_INPUT_REGISTERS:
                result = client->readInputRegisters(m_deviceId.toInt(), m_startAddress, m_quantity);
                success = !result.isEmpty();
                break;
            case ModbusFunction::WRITE_SINGLE_COIL:
                success = client->writeSingleCoil(m_deviceId.toInt(), m_startAddress, QVector<quint16>(m_quantity, 1));
                break;
            case ModbusFunction::WRITE_SINGLE_REGISTER:
                success = client->writeSingleRegister(m_deviceId.toInt(), m_startAddress, QVector<quint16>(m_quantity, 0));
                break;
            case ModbusFunction::WRITE_MULTIPLE_COILS:
                {
                    QVector<quint16> coilValues = m_writeData.isEmpty() ? QVector<quint16>(m_quantity, 1) : m_writeData;
                    success = client->writeMultipleCoils(m_deviceId.toInt(), m_startAddress, coilValues);
                }
                break;
            case ModbusFunction::WRITE_MULTIPLE_REGISTERS:
                {
                    QVector<quint16> registerValues = m_writeData.isEmpty() ? QVector<quint16>(m_quantity, 0) : m_writeData;
                    success = client->writeMultipleRegisters(m_deviceId.toInt(), m_startAddress, registerValues);
                }
                break;
            default:
                qWarning() << "Unsupported Modbus function code for sync execution:" << static_cast<int>(m_functionCode);
                setStatus(TaskStatus::FAILED);
                emit taskCompleted(res, QVector<quint16>());
                return QVector<quint16>();
        }
        
        // qDebug() << "[Task][Sync] end task=" << m_taskName
        //          << "device=" << m_deviceId
        //          << "success=" << success
        //          << "resultSize=" << result.size();
        if (success) {
            setStatus(TaskStatus::COMPLETED);
            res.isException = false;
            emit taskCompleted(res, result);
            setResult(QVariant::fromValue(result));
            return result;
        } else {
            setStatus(TaskStatus::FAILED);
            emit taskCompleted(res, QVector<quint16>());
            return QVector<quint16>();
        }
    } else {
        // 异步执行模式
        // qDebug() << "Executing task in async mode:" << m_taskName;
        
        // 使用任务ID作为tag的一部分，确保数据隔离
        QString tag = QString("%1:%2:%3:%4:%5").arg(m_deviceId).arg(m_taskName).arg(m_startAddress).arg(m_quantity).arg(m_taskId);
        
        // 连接ModbusClient的信号到Task的槽函数
        // 使用DirectConnection确保信号在正确的线程中处理
        connect(modbusClient, SIGNAL(requestCompleted(QString, ModbusResult)),
                this, SLOT(onModbusResponseReceived(QString, ModbusResult)), Qt::DirectConnection);
        connect(modbusClient, SIGNAL(communicationError(QString)),
                this, SLOT(onModbusErrorOccurred(QString)), Qt::DirectConnection);
        
        bool invokeSuccess = false;
        switch (m_functionCode) {
            case ModbusFunction::READ_COILS:
                client->readCoilsAsync(m_deviceId.toInt(), m_startAddress, m_quantity, tag);
                invokeSuccess = true;
                break;
            case ModbusFunction::READ_DISCRETE_INPUTS:
                client->readDiscreteInputsAsync(m_deviceId.toInt(), m_startAddress, m_quantity, tag);
                invokeSuccess = true;
                break;
            case ModbusFunction::READ_HOLDING_REGISTERS:
                client->readHoldingRegistersAsync(m_deviceId.toInt(), m_startAddress, m_quantity, tag);
                invokeSuccess = true;
                break;
            case ModbusFunction::READ_INPUT_REGISTERS:
                client->readInputRegistersAsync(m_deviceId.toInt(), m_startAddress, m_quantity, tag);
                invokeSuccess = true;
                break;
            case ModbusFunction::WRITE_SINGLE_COIL:
                client->writeSingleCoilAsync(m_deviceId.toInt(), m_startAddress, QVector<quint16>(m_quantity, 1), tag);
                invokeSuccess = true;
                break;
            case ModbusFunction::WRITE_SINGLE_REGISTER:
                client->writeSingleRegisterAsync(m_deviceId.toInt(), m_startAddress, QVector<quint16>(m_quantity, 0), tag);
                invokeSuccess = true;
                break;
            case ModbusFunction::WRITE_MULTIPLE_COILS:
                {
                    QVector<quint16> coilValues = m_writeData.isEmpty() ? QVector<quint16>(m_quantity, 1) : m_writeData;
                    client->writeMultipleCoilsAsync(m_deviceId.toInt(), m_startAddress, coilValues, tag);
                    invokeSuccess = true;
                }
                break;
            case ModbusFunction::WRITE_MULTIPLE_REGISTERS:
                {
                    QVector<quint16> registerValues = m_writeData.isEmpty() ? QVector<quint16>(m_quantity, 0) : m_writeData;
                    client->writeMultipleRegistersAsync(m_deviceId.toInt(), m_startAddress, registerValues, tag);
                    invokeSuccess = true;
                }
                break;
            default:
                qWarning() << "Unsupported Modbus function code:" << static_cast<int>(m_functionCode);
                setStatus(TaskStatus::FAILED);
                emit taskCompleted(res, QVector<quint16>());
                return QVector<quint16>();
        }
        
        if (!invokeSuccess) {
            qWarning() << "Failed to invoke Modbus method for function code:" << static_cast<int>(m_functionCode);
            setStatus(TaskStatus::FAILED);
            emit taskCompleted(res, QVector<quint16>());
            return QVector<quint16>();
        }
        
        qDebug() << "Async task execution started:" << m_taskName << "Function:" << static_cast<int>(m_functionCode);
        return QVector<quint16>(); // 异步模式返回空向量
    }
}

void Task::cancel()
{
    if (m_taskStatus == TaskStatus::RUNNING) {
        qDebug() << "Canceling task:" << m_taskName;
        setStatus(TaskStatus::PENDING);
        // 这里可以添加取消Modbus操作的实现
    }
}

void Task::setStatus(TaskStatus status)
{
    if (m_taskStatus != status) {
        m_taskStatus = status;
        emit statusChanged(status);
    }
}

void Task::setDevice(QObject *device)
{
    qDebug() << "Setting device for task:" << m_taskName << "Device:" << device;
    m_device = device;
}

Task* Task::createFromJson(const QJsonObject &config, QObject *parent)
{
    return new Task(config, parent);
}

int Task::isSync() const
{
    return m_isSync;
}

void Task::setIsSync(int newIsSync)
{
    m_isSync = newIsSync;
}

const QVector<quint16> &Task::writeData() const
{
    return m_writeData;
}

void Task::setWriteData(const QVector<quint16> &newWriteData)
{
    m_writeData = newWriteData;
}

//接收来自modbusclient返回的通信数据
void Task::onModbusResponseReceived(const QString &tag, const ModbusResult &result)
{
    // 验证tag是否包含当前任务的ID，确保数据正确对应
    if (!tag.contains(m_taskId)) {
        qWarning() << "Received response for wrong task! Expected:" << m_taskId << "Got:" << tag;
        return; // 忽略不属于当前任务的数据
    }
    
    qDebug() << "Modbus response received for task:" << m_taskName << "Task ID:" << m_taskId << "value:" << result.values << "Success:" << result.success;
    
    TaskResult res;
    res.deviceId = m_deviceId;
    res.taskName = m_taskName;
    res.taskId = m_taskId;
    res.timestamp = QDateTime::currentDateTime();
    res.isException = !result.success;
    res.remark = m_taskReamark;

    // 处理响应数据
    if (result.success && !result.values.isEmpty()) {
        qDebug() << "Modbus response data for task:" << m_taskName << "Task ID:" << m_taskId << "Values:" << result.values;
        setResult(QVariant::fromValue(result.values));
        setStatus(TaskStatus::COMPLETED);
        
        // 如果是同步任务，结果会通过taskCompleted信号返回给调用方
        
        emit taskCompleted(res, result.values);
    } else {
        qWarning() << "Modbus request failed for task:" << m_taskName << "Task ID:" << m_taskId << "Error:" << result.errorString;
        setStatus(TaskStatus::FAILED);
        res.isException = true;
        
        // 如果是同步任务，失败结果会通过taskCompleted信号返回给调用方
        
        emit taskCompleted(res, QVector<quint16>());
    }
}

void Task::onModbusErrorOccurred(const QString &error)
{
    qWarning() << "Modbus error for task:" << m_taskName << "Error:" << error;
    setStatus(TaskStatus::FAILED);
    TaskResult res;
    res.deviceId = m_deviceId;
    res.taskName = m_taskName;
    res.taskId = m_taskId;
    res.remark = m_taskReamark;
    res.timestamp = QDateTime::currentDateTime();
    res.isException = true;
    
    // 如果是同步任务，失败结果会通过taskCompleted信号返回给调用方
    
    emit taskCompleted(res, QVector<quint16>());
}

void Task::setResult(const QVariant &result)
{
    m_result = result;
    emit resultChanged(result);
}

//字符串转枚举
ModbusFunction Task::parseFunctionCode(const QString &type)
{
    if (type == "READ_COILS") {
        return ModbusFunction::READ_COILS;
    } else if (type == "READ_DISCRETE_INPUTS") {
        return ModbusFunction::READ_DISCRETE_INPUTS;
    } else if (type == "READ_HOLDING_REGISTERS") {
        return ModbusFunction::READ_HOLDING_REGISTERS;
    } else if (type == "READ_INPUT_REGISTERS") {
        return ModbusFunction::READ_INPUT_REGISTERS;
    } else if (type == "WRITE_SINGLE_COIL") {
        return ModbusFunction::WRITE_SINGLE_COIL;
    } else if (type == "WRITE_SINGLE_REGISTER") {
        return ModbusFunction::WRITE_SINGLE_REGISTER;
    } else if (type == "WRITE_MULTIPLE_COILS") {
        return ModbusFunction::WRITE_MULTIPLE_COILS;
    } else if (type == "WRITE_MULTIPLE_REGISTERS") {
        return ModbusFunction::WRITE_MULTIPLE_REGISTERS;
    } else {
        qWarning() << "Unknown function code type:" << type;
        return ModbusFunction::ErrorFunction; // 错误值
    }
}

const QString &Task::taskReamark() const
{
    return m_taskReamark;
}

void Task::setTaskReamark(const QString &newTaskReamark)
{
    m_taskReamark = newTaskReamark;
}

void Task::setDeviceId(const QString &newDeviceId)
{
    m_deviceId = newDeviceId;
}

void Task::onDisconnected()
{
    emit disconnected();
}
