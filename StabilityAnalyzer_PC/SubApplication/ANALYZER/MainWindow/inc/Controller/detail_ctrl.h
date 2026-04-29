#ifndef DETAIL_CTRL_H
#define DETAIL_CTRL_H

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QThread>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include "mainwindow_global.h"

class SqlOrmManager;

class MAINWINDOW_EXPORT detailCtrl : public QObject
{
    Q_OBJECT

public:
    explicit detailCtrl(QObject *parent = nullptr);
    ~detailCtrl();

    Q_INVOKABLE int requestLightCurves(int experimentId, int pointsPerCurve);
    Q_INVOKABLE void cancelLightCurveRequest(int requestId = 0);
    Q_INVOKABLE int requestInstabilityOverview(int experimentId, double minHeightMm, double maxHeightMm);
    Q_INVOKABLE int requestInstabilityCustomSeries(int experimentId, double lowerMm, double upperMm);
    Q_INVOKABLE void cancelInstabilityRequest(int requestId = 0);
    Q_INVOKABLE void clearExperimentCache(int experimentId = 0);
    Q_INVOKABLE QVariantList getProcessedLightIntensityCurves(int experimentId,
                                                              int referenceScanId,
                                                              double lowerMm,
                                                              double upperMm,
                                                              bool useReference) const;

signals:
    void lightCurveRequestStarted(int requestId, int experimentId);
    void lightCurveRequestFinished(int requestId, int experimentId, QVariantMap payload);
    void lightCurveRequestFailed(int requestId, int experimentId, QString message);
    void lightCurveRequestCancelled(int requestId, int experimentId, QString reason);
    void instabilityOverviewRequestStarted(int requestId, int experimentId);
    void instabilityOverviewRequestFinished(int requestId, int experimentId, QVariantMap payload);
    void instabilityOverviewRequestFailed(int requestId, int experimentId, QString message);
    void instabilityOverviewRequestCancelled(int requestId, int experimentId, QString reason);
    void instabilityCustomSeriesRequestStarted(int requestId, int experimentId, double lowerMm, double upperMm);
    void instabilityCustomSeriesRequestFinished(int requestId, int experimentId, QVariantMap payload);
    void instabilityCustomSeriesRequestFailed(int requestId, int experimentId, QString message);
    void instabilityCustomSeriesRequestCancelled(int requestId, int experimentId, QString reason);

private:
    void finishLightCurveRequest(int requestId,
                                 int experimentId,
                                 const QVector<QVariantMap> &curves,
                                 const QVariantMap &payload,
                                 const QString &errorMessage,
                                 bool cancelled);
    void finishInstabilityOverviewRequest(int requestId,
                                          int experimentId,
                                          const QVariantMap &payload,
                                          const QString &errorMessage,
                                          bool cancelled);
    void finishInstabilityCustomRequest(int requestId,
                                        int experimentId,
                                        double lowerMm,
                                        double upperMm,
                                        const QVariantMap &payload,
                                        const QString &errorMessage,
                                        bool cancelled);

    SqlOrmManager *m_dbManager = nullptr;
    QPointer<QThread> m_lightCurveRequestThread;
    QPointer<QThread> m_instabilityOverviewRequestThread;
    QPointer<QThread> m_instabilityCustomRequestThread;
    int m_activeLightCurveRequestId = 0;
    int m_activeLightCurveExperimentId = 0;
    int m_activeInstabilityOverviewRequestId = 0;
    int m_activeInstabilityOverviewExperimentId = 0;
    int m_activeInstabilityCustomRequestId = 0;
    int m_activeInstabilityCustomExperimentId = 0;
    double m_activeInstabilityCustomLowerMm = 0.0;
    double m_activeInstabilityCustomUpperMm = 0.0;
    int m_requestSerial = 0;
    QHash<int, QVector<QVariantMap> > m_lightCurveCacheByExperiment;
};

#endif // DETAIL_CTRL_H
