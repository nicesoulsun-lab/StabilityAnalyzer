#ifndef MODBUSRTUWIDGET_H
#define MODBUSRTUWIDGET_H

#include <QWidget>
//#include "ModbusRTUClient.h"

namespace Ui {
class ModbusRTUWidget;
}

/**
 * @brief The ModbusRTUWidget class
 * Modbus RTU 通信界面类
 * 暂时没用，这个是我测试使用的，外部如果调用的话直接调用moudbusrtu_client就行
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
//    void onConnectionStateChanged(ModbusRtuClient::ConnectionState state);
//    void onErrorOccurred(ModbusRtuClient::ErrorCode errorCode, const QString &error);
    void onCoilsRead(int slaveAddress, int startAddress, const QVector<bool> &values);
    void onHoldingRegistersRead(int slaveAddress, int startAddress, const QVector<quint16> &values);
    void onWriteCompleted(int slaveAddress, int functionCode, int startAddress, int count);

private:
    Ui::ModbusRTUWidget *ui;
//    ModbusRtuClient *m_modbusClient;
    QThread *m_workerThread;

    void init();
    void updateUI();
    void logMessage(const QString &message);
};

#endif // MODBUSRTUWIDGET_H
