#ifndef REALTIMECURVECTRL_H
#define REALTIMECURVECTRL_H

#include <QObject>
#include <QVariantList>
#include <QVector>
#include <QPointF>
#include "mainwindow_global.h"

class ExperimentCtrl;
class dataCtrl;

class MAINWINDOW_EXPORT RealtimeCurveCtrl : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int channel READ channel WRITE setChannel NOTIFY channelChanged)
    Q_PROPERTY(bool hasData READ hasData NOTIFY dataUpdated)
    Q_PROPERTY(qreal minHeight READ minHeight NOTIFY dataUpdated)
    Q_PROPERTY(qreal maxHeight READ maxHeight NOTIFY dataUpdated)
    Q_PROPERTY(qreal minTransmission READ minTransmission NOTIFY dataUpdated)
    Q_PROPERTY(qreal maxTransmission READ maxTransmission NOTIFY dataUpdated)
    Q_PROPERTY(qreal minBackscatter READ minBackscatter NOTIFY dataUpdated)
    Q_PROPERTY(qreal maxBackscatter READ maxBackscatter NOTIFY dataUpdated)
    Q_PROPERTY(QVariantList transmissionPoints READ transmissionPoints NOTIFY dataUpdated)
    Q_PROPERTY(QVariantList backscatterPoints READ backscatterPoints NOTIFY dataUpdated)

public:
    explicit RealtimeCurveCtrl(ExperimentCtrl *experimentCtrl, dataCtrl *dataCtrl, QObject *parent = nullptr);
    ~RealtimeCurveCtrl();

    int channel() const;
    void setChannel(int ch);
    bool hasData() const;
    qreal minHeight() const;
    qreal maxHeight() const;
    qreal minTransmission() const;
    qreal maxTransmission() const;
    qreal minBackscatter() const;
    qreal maxBackscatter() const;
    QVariantList transmissionPoints() const;
    QVariantList backscatterPoints() const;

    Q_INVOKABLE void clearData();

signals:
    void channelChanged();
    void dataUpdated();

private slots:
    void onScanDataChunkReady(int channel, int experimentId, int scanId,
                              bool scanCompleted, const QVariantList &rows);
    void onExperimentStarted(int channel, int experimentId);

private:
    void rebuildCurve(const QVariantList &rows);
    void loadLatestScanData();
    void appendIncrementalData(const QVariantList &rows);
    void flushIncrementalCurve();
    QVariantList toVariantList(const QVector<QPointF> &points) const;

    ExperimentCtrl *m_experimentCtrl;
    dataCtrl *m_dataCtrl;
    int m_channel;                 // 当前监听的通道号
    int m_currentExperimentId;

    QVariantList m_transmissionPoints;
    QVariantList m_backscatterPoints;
    qreal m_minHeight;
    qreal m_maxHeight;
    qreal m_minTransmission;
    qreal m_maxTransmission;
    qreal m_minBackscatter;
    qreal m_maxBackscatter;
    bool m_hasData;

    int m_lastScanId;                            // 上一次的扫描ID，用于检测新扫描开始
    QVector<QPointF> m_accumulatedTransPoints;    // 累积的透射点（增量绘制用）
    QVector<QPointF> m_accumulatedBackPoints;     // 累积的背向散射点（增量绘制用）
};

#endif
