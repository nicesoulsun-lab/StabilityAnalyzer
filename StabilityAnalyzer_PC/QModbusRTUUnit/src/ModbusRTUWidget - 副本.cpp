#include "ModbusRTUWidget.h"
#include "ui_ModbusRTUWidget.h"
#include <QDebug>
#include <QDateTime>
#include <QSerialPortInfo>
#include "cutelogger/logmanager.h"
ModbusRTUWidget::ModbusRTUWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ModbusRTUWidget)
    , m_modbusClient(new ModbusRTUClient()) //不设置父对象
    , m_workerThread(new QThread(this))     // 线程有父对象
{
    ui->setupUi(this);


    // 初始化界面
    init();

    //线程
    m_modbusClient->moveToThread(m_workerThread);
    //moudbusrtuclient的init方法使用信号的方式进行调用这样可以保证serialport等对象都在子线程里面
    connect(m_workerThread, &QThread::started, m_modbusClient, &ModbusRTUClient::init);
    connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater);

    // 连接信号槽,在对象移动之后进行连接
    connect(m_modbusClient, &ModbusRTUClient::connectionStateChanged,
            this, &ModbusRTUWidget::onConnectionStateChanged);
    connect(m_modbusClient, &ModbusRTUClient::errorOccurred,
            this, &ModbusRTUWidget::onErrorOccurred);
    connect(m_modbusClient, &ModbusRTUClient::coilsRead,
            this, &ModbusRTUWidget::onCoilsRead);
    connect(m_modbusClient, &ModbusRTUClient::holdingRegistersRead,
            this, &ModbusRTUWidget::onHoldingRegistersRead);
    connect(m_modbusClient, &ModbusRTUClient::writeCompleted,
            this, &ModbusRTUWidget::onWriteCompleted);
    m_workerThread->start();

    updateUI();
}

ModbusRTUWidget::~ModbusRTUWidget()
{
    LOG_INFO() << "ModbusRTUWidget析构函数开始执行...";

    // 1. 先断开所有与工作线程相关的信号槽连接
    if (m_modbusClient) {
        // 断开从工作对象到UI的信号槽连接
        disconnect(m_modbusClient, nullptr, this, nullptr);
        // 断开从UI到工作对象的信号槽连接
        disconnect(this, nullptr, m_modbusClient, nullptr);
    }

    // 2. 删除ModbusRTUClient对象（使用deleteLater）
    if (m_modbusClient) {
        LOG_INFO() << "开始删除ModbusRTUClient对象...";

        // 使用deleteLater，让对象在自己的线程中被删除
        m_modbusClient->deleteLater();
        m_modbusClient = nullptr;  // 将指针设为null，防止野指针

        LOG_INFO() << "ModbusRTUClient对象已标记为删除";

        // 处理剩余的事件，确保deleteLater被执行
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    } else {
        LOG_INFO() << "ModbusRTUClient对象为空";
    }

    // 3. 停止工作线程
    if (m_workerThread && m_workerThread->isRunning()) {
        LOG_INFO() << "工作线程正在运行，开始停止线程...";

        // 发送退出命令
        m_workerThread->quit();
        LOG_INFO() << "线程已发送quit信号";

        // 等待线程结束
        if (m_workerThread->wait(2000)) { // 等待2秒
            LOG_INFO() << "线程正常结束";
        } else {
            LOG_WARNING() << "线程超时未结束，强制终止";
            // 如果线程仍未结束，强制终止
            m_workerThread->terminate();
            m_workerThread->wait();
        }

        // 将线程指针设为nullptr，避免后续访问
        m_workerThread = nullptr;
    } else {
        LOG_INFO() << "工作线程未运行或已停止";
    }

    delete ui;
    LOG_INFO() << "ModbusRTUWidget析构函数执行完成";
}

void ModbusRTUWidget::init()
{
    // 填充串口列表
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        ui->cb_port->addItem(port.portName());
    }

    // 设置默认值
    ui->cb_baudrate->addItems({"9600", "19200", "38400", "57600", "115200"});
    ui->cb_baudrate->setCurrentText("9600");

    ui->sb_slaveAddress->setValue(1);
    ui->sb_startAddress->setValue(0);
    ui->sb_count->setValue(10);

    // 设置日志区域
    ui->te_log->setReadOnly(true);
}

void ModbusRTUWidget::on_pb_connect_clicked()
{
    QString portName = ui->cb_port->currentText();

    // 使用信号槽机制跨线程调用connectToDevice
    QMetaObject::invokeMethod(m_modbusClient, "connectToDevice",
                              Qt::QueuedConnection,
                              Q_ARG(QString, portName));

    LOG_INFO()<<"线程ID000............."<<QThread::currentThreadId();
}

void ModbusRTUWidget::on_pb_disconnect_clicked()
{
    // 使用信号槽机制跨线程调用disconnectFromDevice
    QMetaObject::invokeMethod(m_modbusClient, "disconnectFromDevice",
                              Qt::QueuedConnection);
    logMessage("断开连接");
}

void ModbusRTUWidget::on_pb_readCoils_clicked()
{
    int slaveAddress = ui->sb_slaveAddress->value();
    int startAddress = ui->sb_startAddress->value();
    int count = ui->sb_count->value();

    // 使用信号槽机制跨线程调用readCoils
    QMetaObject::invokeMethod(m_modbusClient, "readCoils",
                              Qt::QueuedConnection,
                              Q_ARG(int, slaveAddress),
                              Q_ARG(int, startAddress),
                              Q_ARG(int, count));

    logMessage(QString("发送读线圈请求: 从站%1 地址%2 数量%3")
              .arg(slaveAddress).arg(startAddress).arg(count));
}

void ModbusRTUWidget::on_pb_readRegisters_clicked()
{
    int slaveAddress = ui->sb_slaveAddress->value();
    int startAddress = ui->sb_startAddress->value();
    int count = ui->sb_count->value();

    // 使用信号槽机制跨线程调用readHoldingRegisters
    QMetaObject::invokeMethod(m_modbusClient, "readHoldingRegisters",
                              Qt::QueuedConnection,
                              Q_ARG(int, slaveAddress),
                              Q_ARG(int, startAddress),
                              Q_ARG(int, count));

    logMessage(QString("发送读寄存器请求: 从站%1 地址%2 数量%3")
              .arg(slaveAddress).arg(startAddress).arg(count));
}

void ModbusRTUWidget::on_pb_writeCoil_clicked()
{
    int slaveAddress = ui->sb_slaveAddress->value();
    int address = ui->sb_startAddress->value();
    bool value = ui->cb_coilValue->currentText() == "ON";

    // 使用信号槽机制跨线程调用writeSingleCoil
    QMetaObject::invokeMethod(m_modbusClient, "writeSingleCoil",
                              Qt::QueuedConnection,
                              Q_ARG(int, slaveAddress),
                              Q_ARG(int, address),
                              Q_ARG(bool, value));

    logMessage(QString("发送写线圈请求: 从站%1 地址%2 值%3")
              .arg(slaveAddress).arg(address).arg(value ? "ON" : "OFF"));
}

void ModbusRTUWidget::on_pb_writeRegister_clicked()
{
    int slaveAddress = ui->sb_slaveAddress->value();
    int address = ui->sb_startAddress->value();
    quint16 value = static_cast<quint16>(ui->sb_registerValue->value());

    // 使用信号槽机制跨线程调用writeSingleRegister
    QMetaObject::invokeMethod(m_modbusClient, "writeSingleRegister",
                              Qt::QueuedConnection,
                              Q_ARG(int, slaveAddress),
                              Q_ARG(int, address),
                              Q_ARG(quint16, value));

    logMessage(QString("发送写寄存器请求: 从站%1 地址%2 值%3")
              .arg(slaveAddress).arg(address).arg(value));
}

void ModbusRTUWidget::onConnectionStateChanged(ModbusRTUClient::ConnectionState state)
{
    QString stateText;
    switch (state) {
        case ModbusRTUClient::Disconnected: stateText = "未连接"; break;
        case ModbusRTUClient::Connecting: stateText = "连接中"; break;
        case ModbusRTUClient::Connected: stateText = "已连接"; break;
        case ModbusRTUClient::Error: stateText = "错误"; break;
    }

    ui->lb_status->setText("状态: " + stateText);
    updateUI();
}

void ModbusRTUWidget::onErrorOccurred(ModbusRTUClient::ErrorCode errorCode, const QString &error)
{
    logMessage("串口通信错误: " + error);
    updateUI();
}

void ModbusRTUWidget::onCoilsRead(int slaveAddress, int startAddress, const QVector<bool> &values)
{
    QString result = QString("读线圈响应: 从站%1 地址%2 值[").arg(slaveAddress).arg(startAddress);
    for (int i = 0; i < values.size(); i++) {
        result += values[i] ? "1" : "0";
        if (i < values.size() - 1) result += ", ";
    }
    result += "]";

    logMessage(result);
}

void ModbusRTUWidget::onHoldingRegistersRead(int slaveAddress, int startAddress, const QVector<quint16> &values)
{
    QString result = QString("读寄存器响应: 从站%1 地址%2 值[").arg(slaveAddress).arg(startAddress);
    for (int i = 0; i < values.size(); i++) {
        result += QString::number(values[i]);
        if (i < values.size() - 1) result += ", ";
    }
    result += "]";

    logMessage(result);
}

void ModbusRTUWidget::onWriteCompleted(int slaveAddress, int functionCode, int startAddress, int count)
{
    QString operation;
    switch (functionCode) {
        case 0x05: operation = "写线圈"; break;
        case 0x06: operation = "写寄存器"; break;
        case 0x0F: operation = "写多个线圈"; break;
        case 0x10: operation = "写多个寄存器"; break;
        default: operation = "写操作"; break;
    }

    logMessage(QString("%1完成: 从站%2 地址%3 数量%4")
              .arg(operation).arg(slaveAddress).arg(startAddress).arg(count));
}

void ModbusRTUWidget::updateUI()
{
    bool connected = (m_modbusClient->connectionState() == ModbusRTUClient::Connected);

    ui->pb_connect->setEnabled(!connected);
    ui->pb_disconnect->setEnabled(connected);
    ui->pb_readCoils->setEnabled(connected);
    ui->pb_readRegisters->setEnabled(connected);
    ui->pb_writeCoil->setEnabled(connected);
    ui->pb_writeRegister->setEnabled(connected);

    ui->cb_port->setEnabled(!connected);
    ui->cb_baudrate->setEnabled(!connected);
}

void ModbusRTUWidget::logMessage(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    ui->te_log->append("[" + timestamp + "] " + message);
}
