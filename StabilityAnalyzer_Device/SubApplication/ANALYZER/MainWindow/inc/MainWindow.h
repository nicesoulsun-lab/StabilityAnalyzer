#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QVariant>
#include <QList>
#include <QTimer>
#include "mainwindow_global.h"
#include "CurveDataModel.h"
#include "../../TaskScheduler/inc/modbustaskscheduler.h"

/**
 * @brief The MainWindow class
 * @class MainWindow
 *
 * Modbus RTU测试工具的主窗口控制器类
 *
 * 主要功能：
 * - 管理Modbus RTU通信连接
 * - 控制数据采集和曲线显示
 * - 提供QML界面与C++后端的桥梁
 * - 处理截图保存功能
 *
 * 通过Q_PROPERTY暴露属性给QML界面，
 * 并通过Q_INVOKABLE方法提供功能调用接口
 */
class MAINWINDOW_EXPORT MainWindow : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString windowTitle READ windowTitle WRITE setWindowTitle NOTIFY windowTitleChanged)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStatusChanged)
    Q_PROPERTY(QVariantList dataPoints READ dataPoints NOTIFY dataPointsChanged)
    Q_PROPERTY(CurveDataModel* curveModel READ curveModel NOTIFY curveModelChanged)
    Q_PROPERTY(bool schedulerRunning READ schedulerRunning NOTIFY schedulerRunningChanged)

public:
    explicit MainWindow(QObject *parent = nullptr);
    ~MainWindow();

    // QML属性访问器
    QString windowTitle() const;
    void setWindowTitle(const QString &title);
    bool isConnected() const;
    QVariantList dataPoints() const;
    CurveDataModel* curveModel() const;

public slots:

    /**
     * @brief readHoldingRegisters 读取保持寄存器
     * @param address 寄存器地址
     * @param count 寄存器数量
     */
//    void readHoldingRegisters(int address, int count);

    /**
     * @brief writeSingleRegister 写入单个寄存器
     * @param address 寄存器地址
     * @param value 寄存器值
     */
//    void writeSingleRegister(int address, int value);

    /**
     * @brief startDataCollection 开始数据采集
     */
    Q_INVOKABLE void startDataCollection();

    /**
     * @brief stopDataCollection 停止数据采集
     */
    Q_INVOKABLE void stopDataCollection();

    /**
     * @brief saveScreenshot 保存截图
     * @param imageData 图像数据（QVariant格式，包含QImage）
     * @return 保存是否成功
     */
    Q_INVOKABLE bool saveScreenshot(const QVariant &imageData);

    // TaskScheduler相关方法
    Q_INVOKABLE void startScheduler();
    Q_INVOKABLE void stopScheduler();
    Q_INVOKABLE void executeUserTask(const QString &deviceId, const QString &taskName, int isSync = 0, const QVector<quint16> &writeData = QVector<quint16>());

    // TaskScheduler属性访问器
    bool schedulerRunning() const;

signals:
    void windowTitleChanged();
    void connectionStatusChanged();
    void dataPointsChanged();
    void curveModelChanged();
    void dataReceived(const QVariantMap &data);
    void errorOccurred(const QString &errorMessage);
    void connectionEstablished();
    void connectionLost();
    void portNameChanged();
    void schedulerRunningChanged();
    void taskCompleted(const QString &deviceId, const QString &taskName, bool success, const QVariant &result);
    void taskStarted(const QString &deviceId, const QString &taskName);

private slots:
    void onDataReceived();

private:

private:
    QString m_windowTitle;
    bool m_isConnected;

    QVariantList m_dataPoints;
    CurveDataModel *m_curveModel;

    // TaskScheduler相关成员变量
    ModbusTaskScheduler *m_taskScheduler;
    bool m_schedulerRunning;
};
MAINWINDOW_EXPORT MainWindow* mainWindow();

#endif // MAINWINDOW_H
