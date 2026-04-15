#ifndef CURVEDATAMODEL_H
#define CURVEDATAMODEL_H

#include <QObject>
#include <QVariantList>
#include <QTimer>
#include <QDateTime>
#include <QVector>
#include <QQueue>
#include "mainwindow_global.h"

/**
 * @brief The CurveDataModel class
 * @class CurveDataModel
 * 
 * 实时曲线数据模型，负责管理曲线显示的数据源
 * 
 * 核心功能：
 * - 实时数据采集和缓存管理
 * - 数据点数量限制和内存优化
 * - 采样率控制和定时器管理
 * - Modbus数据适配和转换
 * - 测试数据生成和模拟
 * 
 * 数据流架构：
 * - 外部数据源（Modbus/传感器）→ 数据队列 → 数据点列表 → QML界面
 * - 使用队列机制实现高效的数据缓冲
 * - 支持实时数据流和批量数据处理
 * 
 * 优点：
 * - 数据点数量限制防止内存溢出，目前设置的固定1000个点，这个可以考虑重写一个loopvector
 * - 定时器驱动的采样控制
 * - 队列机制避免数据丢失
 * - 数据更新和信号发射
 */
class MAINWINDOW_EXPORT CurveDataModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList dataPoints READ dataPoints NOTIFY dataPointsChanged)
    Q_PROPERTY(int maxDataPoints READ maxDataPoints WRITE setMaxDataPoints NOTIFY maxDataPointsChanged)
    Q_PROPERTY(bool isCollecting READ isCollecting NOTIFY isCollectingChanged)
    Q_PROPERTY(qreal currentValue READ currentValue NOTIFY currentValueChanged)
    Q_PROPERTY(QString currentTime READ currentTime NOTIFY currentTimeChanged)
    Q_PROPERTY(int samplingRate READ samplingRate WRITE setSamplingRate NOTIFY samplingRateChanged)

public:
    explicit CurveDataModel(QObject *parent = nullptr);
    ~CurveDataModel();

    // QML属性访问器
    QVariantList dataPoints() const;
    int maxDataPoints() const;
    void setMaxDataPoints(int max);
    bool isCollecting() const;
    qreal currentValue() const;
    QString currentTime() const;
    int samplingRate() const;
    void setSamplingRate(int rate);

public slots:
    /**
     * @brief startCollection 开始数据采集
     */
    Q_INVOKABLE void startCollection();
    
    /**
     * @brief stopCollection 停止数据采集
     */
    Q_INVOKABLE void stopCollection();
    
    /**
     * @brief addDataPoint 添加数据点
     * @param value 数据值
     */
    Q_INVOKABLE void addDataPoint(qreal value);
    
    /**
     * @brief clearData 清除所有数据
     */
    Q_INVOKABLE void clearData();
    
    /**
     * @brief updateFromModbusData 从Modbus数据更新
     * @param data 从Modbus读取的数据
     */
    Q_INVOKABLE void updateFromModbusData(const QVariantList &data);

signals:
    void dataPointsChanged();
    void maxDataPointsChanged();
    void isCollectingChanged();
    void currentValueChanged();
    void currentTimeChanged();
    void samplingRateChanged();
    void dataUpdated();

private slots:
    void onSamplingTimer();

private:
    void initializeTimer();
    void cleanupTimer();
    void addPointWithTime(qreal value);
    void updateDataPointsFromQueue();

private:
    QVariantList m_dataPoints;
    QQueue<QVariantMap> m_dataQueue;  // 单队列数据存储
    int m_maxDataPoints;
    bool m_isCollecting;
    qreal m_currentValue;
    QString m_currentTime;
    int m_samplingRate;
    QTimer *m_samplingTimer;
    QDateTime m_startTime;
    qreal m_testValue;
    bool m_useTestData;
    int m_channelCount;
    QVariantList m_channelNames;
};

#endif // CURVEDATAMODEL_H
