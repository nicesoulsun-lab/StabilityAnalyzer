#ifndef MODBUSRTUWIDGET_H
#define MODBUSRTUWIDGET_H

#include <QWidget>
#include "ModbusRTUClient.h"

namespace Ui {
class ModbusRTUWidget;
}

/**
 * @brief The ModbusRTUWidget class
 * Modbus RTU 通信界面类
 * 提供配置界面和操作界面，方便用户使用
 */
class ModbusRTUWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ModbusRTUWidget(QWidget *parent = nullptr);
    ~ModbusRTUWidget();

private slots:
    //按钮点击事件
    void on_pb_connect_clicked();
    void on_pb_disconnect_clicked();
    void on_pb_readCoils_clicked();
    void on_pb_readRegisters_clicked();
    void on_pb_writeCoil_clicked();
    void on_pb_writeRegister_clicked();
    
    //槽函数
    void onConnectionStateChanged(ModbusRTUClient::ConnectionState state);
    void onErrorOccurred(ModbusRTUClient::ErrorCode errorCode, const QString &error);
    void onCoilsRead(int slaveAddress, int startAddress, const QVector<bool> &values);
    void onHoldingRegistersRead(int slaveAddress, int startAddress, const QVector<quint16> &values);
    void onWriteCompleted(int slaveAddress, int functionCode, int startAddress, int count);

private:
    Ui::ModbusRTUWidget *ui;
    ModbusRTUClient *m_modbusClient;
    QThread *m_workerThread;

    void init();
    void updateUI();
    void logMessage(const QString &message);
};

#endif // MODBUSRTUWIDGET_H
