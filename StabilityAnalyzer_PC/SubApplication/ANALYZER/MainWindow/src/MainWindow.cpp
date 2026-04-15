#include "MainWindow.h"
#include <QSerialPortInfo>
#include <QDateTime>
#include <QDebug>
#include <QtMath>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QImage>
#include "config_manager.h"
#include "logmanager.h"
#include "task.h"
/**
 * @brief MainWindow构造函数
 * @param parent 父对象指针
 *
 * 初始化Modbus RTU测试工具的主控制器
 * - 设置默认窗口标题、串口参数
 * - 创建Modbus客户端和曲线数据模型
 * - 设置定时器用于数据采集
 * - 连接信号槽用于通信状态监控
 */
MainWindow::MainWindow(QObject *parent) :
    QObject(parent),
    m_windowTitle("Modbus任务调度测试工具"),
    m_isConnected(false),
    m_curveModel(new CurveDataModel(this)),
    m_taskScheduler(new ModbusTaskScheduler(this)),
    m_schedulerRunning(false)
{

    qRegisterMetaType<TaskResult>("TaskResult");
    qRegisterMetaType<QVector<quint16>>("QVector<quint16>");

    // 连接TaskScheduler的信号，用于处理Modbus通信结果
    connect(m_taskScheduler, &ModbusTaskScheduler::taskCompleted, this,
            [this](TaskResult res, QVector<quint16>data) {
        if (!res.isException) { // 处理任务完成后的数据更新
            LOG_INFO()<< __FUNCTION__ << "mainwindow接收到数据...." <<data;
            // 更新连接状态
            bool wasConnected = m_isConnected;
            m_isConnected = true;
            if (wasConnected != m_isConnected) {
                emit connectionStatusChanged();
                emit connectionEstablished();
            }

            // 更新数据点
            if (data.size()>0) {

                //吧qvector<quint16>转换成qvariantlist
                QVariantList list;
                list.reserve(data.size());
                for (quint16 v : data)
                    list << v;          // quint16 可隐式转为 int，进而构造 QVariant

                // 更新曲线数据模型
                m_curveModel->updateFromModbusData(list);

                // 更新数据点列表，这个地方重复了和updateFromModbusData
//                for (int i = 0; i < list.size(); ++i) {
//                    QVariantMap dataPoint;
//                    dataPoint["timestamp"] = QDateTime::currentMSecsSinceEpoch();
//                    dataPoint["value"] = list[i];
//                    dataPoint["address"] = i;

//                    m_dataPoints.append(dataPoint);

//                    // 保持最多1000个数据点
//                    if (m_dataPoints.size() > 1000) {
//                        m_dataPoints.removeFirst();
//                    }
//                }

//                emit dataPointsChanged();
            }
        } else {
            // 任务失败时更新连接状态
            bool wasConnected = m_isConnected;
            m_isConnected = false;
            if (wasConnected != m_isConnected) {
                emit connectionStatusChanged();
                emit connectionLost();
            }
        }
    });

    // 连接TaskScheduler的其他信号
    connect(m_taskScheduler, &ModbusTaskScheduler::runningStatusChanged, this, [this](bool running) {
        bool wasRunning = m_schedulerRunning;
        m_schedulerRunning = running;

        if (wasRunning != m_schedulerRunning) {
            emit schedulerRunningChanged();
        }
    });

    connect(m_taskScheduler, &ModbusTaskScheduler::taskStarted, this, &MainWindow::taskStarted);
    connect(m_taskScheduler, &ModbusTaskScheduler::errorOccurred, this, [this](const QString &error) {
        emit errorOccurred("任务调度器错误: " + error);
    });
}

MainWindow::~MainWindow()
{
}

QString MainWindow::windowTitle() const
{
    return m_windowTitle;
}

void MainWindow::setWindowTitle(const QString &title)
{
    if (m_windowTitle != title) {
        m_windowTitle = title;
        emit windowTitleChanged();
    }
}

bool MainWindow::isConnected() const
{
    return m_isConnected;
}

QVariantList MainWindow::dataPoints() const
{
    return m_dataPoints;
}

CurveDataModel* MainWindow::curveModel() const
{
    return m_curveModel;
}

// 连接状态通过TaskScheduler的任务执行结果自动更新

void MainWindow::startDataCollection()
{
    if (!m_schedulerRunning) {
        emit errorOccurred("请先启动任务调度器");
        return;
    }

    m_dataPoints.clear();
    emit dataPointsChanged();

    qDebug() << "开始数据采集";
}

void MainWindow::stopDataCollection()
{
    // 清空数据点
    m_dataPoints.clear();
    emit dataPointsChanged();

    qDebug() << "数据采集已完全停止";
}



void MainWindow::onDataReceived()
{
    // 数据接收处理函数 - 由MOC系统调用
    // 实际的数据处理已通过信号连接完成，这里不需要额外操作
}

bool MainWindow::saveScreenshot(const QVariant &imageData)
{
    if (imageData.isNull()) {
        qWarning() << "截图保存失败: 图像数据为空";
        return false;
    }

    // 尝试从QVariant中提取QImage
    QImage image;

    // 方法1：直接转换为QImage
    if (imageData.canConvert<QImage>()) {
        image = imageData.value<QImage>();
    }

    // 方法2：如果无法直接转换，尝试其他方式
    if (image.isNull()) {
        qWarning() << "无法从QVariant中提取QImage，尝试其他方法";
        return false;
    }

    if (image.isNull()) {
        qWarning() << "截图保存失败: 图像数据为空";
        return false;
    }

    // 生成文件名
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss-zzz");
    QString filename = QString("曲线截图_%1.png").arg(timestamp);

    // 优先保存到用户文档目录
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString screenshotsDir = documentsPath + "/DetectionSystem_Screenshots/";
    QString savePath = screenshotsDir + filename;

    qDebug() << "尝试保存截图到:" << savePath;

    // 确保目录存在
    QDir dir(screenshotsDir);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "创建目录失败:" << screenshotsDir;

            // 尝试备用路径：应用程序数据目录
            QString appPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            QString appScreenshotsDir = appPath + "/screenshots/";
            savePath = appScreenshotsDir + filename;

            qDebug() << "尝试备用路径:" << savePath;

            QDir appDir(appScreenshotsDir);
            if (!appDir.exists() && !appDir.mkpath(".")) {
                qWarning() << "创建备用目录失败:" << appScreenshotsDir;
                return false;
            }
        }
    }

    // 保存图像
    if (image.save(savePath, "PNG")) {
        qDebug() << "截图保存成功:" << savePath;
        return true;
    } else {
        qWarning() << "截图保存失败:" << savePath;
        return false;
    }
}

// TaskScheduler相关方法实现
void MainWindow::startScheduler()
{
//    if (m_configFile.isEmpty()) {
//        emit errorOccurred("请先设置配置文件路径");
//        return;
//    }

    if (!m_taskScheduler->startScheduler()) {
        LOG_INFO()<<"启动任务调度器失败";
        emit errorOccurred("启动任务调度器失败");
        return;
    }
    
    m_schedulerRunning = true;
    emit schedulerRunningChanged();
}

void MainWindow::stopScheduler()
{
    m_taskScheduler->stopScheduler();
    
    m_schedulerRunning = false;
    emit schedulerRunningChanged();
}



void MainWindow::executeUserTask(const QString &deviceId, const QString &taskName, int isSync, const QVector<quint16> &writeData)
{
    if (!m_schedulerRunning) {
        emit errorOccurred("任务调度器未运行");
        return;
    }

    QVector<quint16>result;
    //m_taskScheduler->executeUserTask(deviceId, taskName, isSync, result, const_cast<QVector<quint16>&>(writeData));
}

bool MainWindow::schedulerRunning() const
{
    return m_schedulerRunning;
}

MAINWINDOW_EXPORT MainWindow* mainWindow()
{
    static MainWindow instance;
    return &instance;
}
