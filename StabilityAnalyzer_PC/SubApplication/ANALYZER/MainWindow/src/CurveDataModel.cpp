#include "CurveDataModel.h"
#include <QTimer>
#include <QDateTime>
#include <QDebug>
#include <QtMath>
#include "logmanager.h"

/**
 * @brief CurveDataModel构造函数
 * @param parent 父对象指针
 *
 * 初始化曲线数据模型：
 * - 设置最大数据点数为1000
 * - 默认数据采集状态为停止
 * - 设置采样率为100ms
 * - 启用测试数据模式（用于演示）
 * - 初始化采样定时器
 *
 * 数据管理策略：
 * - 使用QQueue进行高效的数据缓冲
 * - 自动限制数据点数量防止内存溢出
 * - 支持实时数据流和批量更新
 */
CurveDataModel::CurveDataModel(QObject *parent)
    : QObject(parent)
    , m_maxDataPoints(1000)
    , m_isCollecting(false)
    , m_currentValue(0.0)
    , m_samplingRate(1000)
    , m_samplingTimer(nullptr)
    , m_testValue(0.0)
    , m_useTestData(false) // 默认使用真实Modbus数据
{
    initializeTimer();
}

CurveDataModel::~CurveDataModel()
{
    cleanupTimer();
}

QVariantList CurveDataModel::dataPoints() const
{
    return m_dataPoints;
}

int CurveDataModel::maxDataPoints() const
{
    return m_maxDataPoints;
}

void CurveDataModel::setMaxDataPoints(int max)
{
    if (m_maxDataPoints != max && max > 0) {
        m_maxDataPoints = max;

        if (m_dataPoints.size() > m_maxDataPoints) {
            m_dataPoints = m_dataPoints.mid(m_dataPoints.size() - m_maxDataPoints);
            emit dataPointsChanged();
        }

        emit maxDataPointsChanged();
    }
}

bool CurveDataModel::isCollecting() const
{
    return m_isCollecting;
}

qreal CurveDataModel::currentValue() const
{
    return m_currentValue;
}

QString CurveDataModel::currentTime() const
{
    return m_currentTime;
}

int CurveDataModel::samplingRate() const
{
    return m_samplingRate;
}

void CurveDataModel::setSamplingRate(int rate)
{
    if (m_samplingRate != rate && rate > 0) {
        m_samplingRate = rate;

        if (m_samplingTimer) {
            m_samplingTimer->setInterval(m_samplingRate);
        }

        emit samplingRateChanged();
    }
}

void CurveDataModel::startCollection()
{
    if (m_isCollecting) {
        return;
    }

    m_isCollecting = true;
    m_startTime = QDateTime::currentDateTime();

    if (m_samplingTimer) {
        m_samplingTimer->start();
    }

    emit isCollectingChanged();
    qDebug() << "曲线数据采集已启动";
}

void CurveDataModel::stopCollection()
{
    if (!m_isCollecting) {
        return;
    }

    m_isCollecting = false;

    if (m_samplingTimer) {
        m_samplingTimer->stop();
    }

    emit isCollectingChanged();
    qDebug() << "曲线数据采集已停止";
}

void CurveDataModel::addDataPoint(qreal value)
{
    if (!m_isCollecting) {
        return;
    }

    // 使用索引作为X坐标，实现滚动显示效果
    qreal index = m_dataPoints.size();

    QVariantMap dataPoint;
    dataPoint["timestamp"] = index; // 使用索引而不是时间戳
    dataPoint["value"] = value;
    dataPoint["address"] = 0; // 测试数据使用地址0

    // 直接添加到数据点列表
    m_dataPoints.append(dataPoint);

    // 如果数据点超过最大长度，移除最早的数据点
    if (m_dataPoints.size() > m_maxDataPoints) {
        m_dataPoints.removeFirst();
    }

    // 更新当前值
    m_currentValue = value;
    m_currentTime = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

}

void CurveDataModel::clearData()
{
    m_dataPoints.clear();
    m_currentValue = 0.0;
    m_currentTime = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

    emit dataPointsChanged();
    emit currentValueChanged();
    emit currentTimeChanged();
    emit dataUpdated();
}

//绘制曲线，接收mdbus通信数据更新
void CurveDataModel::updateFromModbusData(const QVariantList &data)
{
    if (data.isEmpty()) {
        return;
    }
    qDebug()<<__FUNCTION__<<"更新界面曲线，接收数据数量："<<data.size();

    // 使用索引作为X坐标，实现滚动显示效果
    qreal baseIndex = m_dataPoints.size();

    // 添加新数据点：每个数据点有独立的X坐标（索引）
    for (int i = 0; i < data.size(); ++i) {
        bool ok = false;
        qreal value = data[i].toReal(&ok);
        if (ok) {
            QVariantMap dataPoint;
            // X轴：递增的索引，实现滚动显示
            dataPoint["timestamp"] = baseIndex + i;
            dataPoint["value"] = value;
            dataPoint["address"] = i; // 寄存器地址

            m_dataPoints.append(dataPoint);
        }
    }

    // 精确的数据移除策略：移除的点数等于新添加的点数
    int newPointsCount = data.size();
    if (m_dataPoints.size() > m_maxDataPoints) {
        // 计算需要移除的点数
        int pointsToRemove = qMin(m_dataPoints.size() - m_maxDataPoints + newPointsCount, m_dataPoints.size());

        // 移除最早的点
        for (int i = 0; i < pointsToRemove; ++i) {
            m_dataPoints.removeFirst();
        }

        qDebug()<<"移除"<<pointsToRemove<<"个最早的数据点";
    }

    // 更新当前值（取最后一个寄存器值）
    if (data.size() >= 1) {
        bool ok = false;
        qreal lastValue = data.last().toReal(&ok);
        if (ok) {
            m_currentValue = lastValue;
            m_currentTime = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
            emit currentValueChanged();
            emit currentTimeChanged();
        }
    }

    // 通知QML界面更新
    emit dataPointsChanged();
    emit dataUpdated();

    qDebug()<<"更新曲线完毕，当前数据点数量："<<m_dataPoints.size()<<"，新添加点数："<<data.size();
}

//这个是测试的，到了定时器时间生成测试数据绘制曲线
void CurveDataModel::onSamplingTimer()
{
    if (!m_isCollecting) {
        return;
    }

    if (m_useTestData) {
        // 生成测试数据：正弦波 + 随机噪声
        qint64 elapsed = m_startTime.msecsTo(QDateTime::currentDateTime());
        qreal time = elapsed / 1000.0; // 转换为秒

        // 正弦波：频率0.5Hz，幅度50，偏移50
        qreal sineValue = 50.0 * qSin(2 * M_PI * 0.5 * time) + 50.0;

        // 添加随机噪声：±5
        qreal noise = (qrand() % 100 - 50) / 10.0;

        qreal testValue = sineValue + noise;

        addDataPoint(testValue);
    } else {
        // 可以添加一些状态指示，比如显示"等待Modbus数据..."
        if (m_dataPoints.isEmpty()) {
            qDebug() << "等待Modbus实时数据...";
        }
    }
}

void CurveDataModel::initializeTimer()
{
    if (!m_samplingTimer) {
        m_samplingTimer = new QTimer(this);
        m_samplingTimer->setInterval(m_samplingRate);
        connect(m_samplingTimer, &QTimer::timeout, this, &CurveDataModel::onSamplingTimer);
    }
}

void CurveDataModel::cleanupTimer()
{
    if (m_samplingTimer) {
        if (m_samplingTimer->isActive()) {
            m_samplingTimer->stop();
        }
        m_samplingTimer->deleteLater();
        m_samplingTimer = nullptr;
    }
}

void CurveDataModel::addPointWithTime(qreal value)
{
    qint64 elapsed = m_startTime.msecsTo(QDateTime::currentDateTime());
    qreal time = elapsed / 1000.0; // 转换为秒

    QVariantMap point;
    point["x"] = time;
    point["y"] = value;

    // 使用队列机制：先进先出(FIFO)
    m_dataQueue.enqueue(point);

    // 如果队列超过最大长度，移除最早的数据点
    if (m_dataQueue.size() > m_maxDataPoints) {
        m_dataQueue.dequeue();
    }

    // 将队列转换为列表供QML使用
//    updateDataPointsFromQueue();

    emit dataPointsChanged();
}

