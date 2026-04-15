#ifndef TASK_H
#define TASK_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QJsonObject>
#include <QVector>
#include <QFuture>
#include <QDateTime>
#include <QSharedPointer>
#include <QMetaType>
#include "taskscheduler_global.h"
#include "../../QModbusRTUUnit/inc/modbus_types.h"
#include "../../QModbusRTUUnit/inc/modbus_client.h"
/**
 * @brief 任务类型枚举
 */
enum class TaskType {
    INIT_TASK,      ///< 初始化任务（启动时自动执行）
    USER_TASK       ///< 用户触发任务
};

/**
 * @brief 任务状态枚举
 */
enum class TaskStatus {
    PENDING,        ///< 等待执行
    RUNNING,        ///< 正在执行
    COMPLETED,        ///< 执行完成
    FAILED          ///< 执行失败
};

/**
 * @brief Modbus功能码枚举
 */
enum class ModbusFunction {
    READ_COILS = 1,             ///< 读线圈状态
    READ_DISCRETE_INPUTS = 2,   ///< 读离散输入
    READ_HOLDING_REGISTERS = 3, ///< 读保持寄存器
    READ_INPUT_REGISTERS = 4,   ///< 读输入寄存器
    WRITE_SINGLE_COIL = 5,      ///< 写单个线圈
    WRITE_SINGLE_REGISTER = 6,  ///< 写单个寄存器
    WRITE_MULTIPLE_COILS = 15,  ///< 写多个线圈
    WRITE_MULTIPLE_REGISTERS = 16, ///< 写多个寄存器
    ErrorFunction ///错误的功能码
};

struct TaskResult //返回的结果结构体，用来在接收数据的地方判断处理是哪个任务的结果
{
    QString deviceId;
    QString taskName;
    QString taskId;           // 唯一任务标识符，用于同步任务匹配
    bool isException=0;
    QDateTime timestamp;      // 时间戳
    QString remark;

    // 生成唯一任务ID
    static QString generateTaskId(const QString &deviceId, const QString &taskName) {
        return QString("%1_%2_%3").arg(deviceId).arg(taskName).arg(QDateTime::currentMSecsSinceEpoch());
    }
    
    bool matches(const QString &targetDeviceId, const QString &targetTaskName) const {
        return deviceId == targetDeviceId && taskName == targetTaskName;
    }
    
    bool matchesId(const QString &targetTaskId) const {
        return taskId == targetTaskId;
    }
};Q_DECLARE_METATYPE(TaskResult)

/**
 * @brief 任务类
 * 
 * 表示一个Modbus通信任务，包含任务的配置信息和执行状态，负责：
 * - 任务属性管理（ID、名称、类型等）
 * - Modbus通信参数配置
 * - 任务执行控制
 * - 状态跟踪和结果处理
 * - 数据转换和缩放
 */
class TASKSCHEDULER_EXPORT Task : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString taskName READ taskName CONSTANT)
    Q_PROPERTY(TaskType taskType READ taskType CONSTANT)
    Q_PROPERTY(TaskStatus status READ status NOTIFY statusChanged)
    Q_PROPERTY(QVariant result READ result NOTIFY resultChanged)

public:
    explicit Task(const QJsonObject &config, QObject *parent = nullptr);
    
    // 属性访问器
    QString taskName() const { return m_taskName; }
    TaskType taskType() const { return m_taskType; }
    TaskStatus status() const { return m_taskStatus; }
    QVariant result() const { return m_result; }
    
    // 任务配置
    QString deviceId() const { return m_deviceId; }
    ModbusFunction functionCode() const { return m_functionCode; }
    quint16 startAddress() const { return m_startAddress; }
    quint16 quantity() const { return m_quantity; }
    int interval() const { return m_interval; }
    
    // 任务标识
    QString taskId() const { return m_taskId; }
    
    // 任务执行控制
    QVector<quint16> execute();
    void cancel();

    // 任务状态设置（用于任务队列管理器）
    void setStatus(TaskStatus status);
    
    // 设置设备实例（用于获取ModbusClient）
    void setDevice(QObject *device);
    
    // 获取设备实例
    QObject *device() const { return m_device; }
    
    // 从JSON配置创建任务
    static Task* createFromJson(const QJsonObject &config, QObject *parent = nullptr);

    int isSync() const;
    void setIsSync(int newIsSync);

    const QVector<quint16> &writeData() const;
    void setWriteData(const QVector<quint16> &newWriteData);

    void setDeviceId(const QString &newDeviceId);

    const QString &taskReamark() const;
    void setTaskReamark(const QString &newTaskReamark);

    public slots:
    //断开连接信号从modbusrtu那边传递过来
    void onDisconnected();

signals:
    void statusChanged(TaskStatus status);
    void resultChanged(const QVariant &result);
//    void taskCompleted(bool success, const QVariant &result);
    void taskCompleted(TaskResult result, const QVector<quint16> &data);
    void disconnected();

private:
//    QVector<quint16> executeSync(QObject* modbusClient); 暂时没用到我写到一个文件里面了
//    void executeAsync(QObject* modbusClient);
    
private slots:
    void onModbusResponseReceived(const QString &tag, const ModbusResult &result);
    void onModbusErrorOccurred(const QString &error);

private:
    void setResult(const QVariant &result);
    
    // 将JSON配置中的type字符串映射到ModbusFunction枚举
    ModbusFunction parseFunctionCode(const QString &type);
    
    QString m_taskName;
    TaskType m_taskType;
    TaskStatus m_taskStatus;
    QVariant m_result;
    
    // 设备实例（用于获取ModbusClient）
    QObject *m_device;
    
    // Modbus任务参数
    QString m_deviceId;
    ModbusFunction m_functionCode; //任务类型
    quint16 m_startAddress; //起始地址
    quint16 m_quantity; //读取数量
    int m_interval = -1;
    int m_isSync=0; //是否同步，这个属性不读取之配置文件，这个是调用者传递过来的
    QVector<quint16>m_writeData; //写入的数据

    // 数据转换配置,这个暂时没用
    QString m_dataType;
    double m_scaleFactor;
    double m_offset;
    
    // 同步任务支持
    QString m_taskId; // 唯一任务标识符
    QString m_taskReamark; //任务备注，标识这个任务的备注
};

#endif // TASK_H
