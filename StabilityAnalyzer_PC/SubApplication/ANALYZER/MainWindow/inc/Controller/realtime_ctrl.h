#ifndef REALTIME_CTRL_H
#define REALTIME_CTRL_H

#include <QObject>
#include <QHash>
#include <QVector>
#include <QVariantList>
#include <QVariantMap>

#include "mainwindow_global.h"

class DataTransmitController;

class MAINWINDOW_EXPORT realtimeCtrl : public QObject
{
    Q_OBJECT

public:
    explicit realtimeCtrl(QObject *parent = nullptr);

    void setDataTransmitController(DataTransmitController *controller);

    Q_INVOKABLE void setActiveSession(int channel, int experimentId, int tabIndex, bool active);
    Q_INVOKABLE QVariantList getLightCurves(int experimentId) const;
    Q_INVOKABLE QVariantList getProcessedLightIntensityCurves(int experimentId,
                                                              int referenceScanId,
                                                              double lowerMm,
                                                              double upperMm,
                                                              bool useReference) const;
    Q_INVOKABLE QVariantMap getUniformityChartData(int experimentId) const;
    Q_INVOKABLE QVariantMap getInstabilitySeriesChartData(int experimentId,
                                                          double lowerMm,
                                                          double upperMm,
                                                          const QString &segmentKey,
                                                          const QString &title) const;
    Q_INVOKABLE QVariantMap getInstabilityRadarChartData(int experimentId,
                                                         double minHeightMm,
                                                         double maxHeightMm) const;
    Q_INVOKABLE void clearExperimentSession(int experimentId);

public slots:
    void handleExperimentStopped(int channel, int experimentId);

signals:
    void lightCurvesChanged(int channel, int experimentId, int curveCount);
    void experimentSessionCleared(int channel, int experimentId, QString reason);

private slots:
    void handleStreamMessage(const QVariantMap &message);

private:
    struct ActiveSession {
        bool active = false;
        int channel = -1;
        int experimentId = 0;
        int tabIndex = -1;
        int subscriptionId = 0;
    };

    bool shouldTrackMessage(int channel, int experimentId) const;
    bool shouldNotifyActiveSession(int channel, int experimentId) const;
    QVector<QVariantMap> sortedCurves(int experimentId) const;
    void rememberChannelExperiment(int channel, int experimentId);
    void trimExperimentCache(int experimentId);
    void trimGlobalCache();
    void emitLightCurvesChanged(int channel, int experimentId);

    DataTransmitController *m_dataTransmitCtrl = nullptr;
    QHash<int, QHash<int, QVariantMap> > m_curveCacheByExperiment;
    QHash<int, int> m_channelByExperiment;
    QHash<int, int> m_experimentByChannel;
    ActiveSession m_activeSession;
};

#endif // REALTIME_CTRL_H
