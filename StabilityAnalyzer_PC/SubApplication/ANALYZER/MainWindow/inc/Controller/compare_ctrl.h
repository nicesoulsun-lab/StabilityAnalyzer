#ifndef COMPARE_CTRL_H
#define COMPARE_CTRL_H

#include <QObject>
#include <QPointer>
#include <QThread>
#include <QVariantList>
#include <QVariantMap>

#include "mainwindow_global.h"

class SqlOrmManager;

class MAINWINDOW_EXPORT compareCtrl : public QObject
{
    Q_OBJECT

public:
    explicit compareCtrl(QObject *parent = nullptr);
    ~compareCtrl() override;

    Q_INVOKABLE int requestInstabilityCompareOverview(const QVariantList &experimentIds);
    Q_INVOKABLE int requestInstabilityCompareCustom(const QVariantList &experimentIds,
                                                    double lowerMm,
                                                    double upperMm);
    Q_INVOKABLE void cancelInstabilityCompareRequest(int requestId = 0);

    Q_INVOKABLE int requestUniformityCompare(const QVariantList &experimentIds);
    Q_INVOKABLE void cancelUniformityCompareRequest(int requestId = 0);

    Q_INVOKABLE int requestLightIntensityAverageCompare(const QVariantList &experimentIds);
    Q_INVOKABLE void cancelLightIntensityAverageCompareRequest(int requestId = 0);

signals:
    void instabilityCompareOverviewRequestStarted(int requestId, QVariantList experimentIds);
    void instabilityCompareOverviewRequestFinished(int requestId, QVariantList experimentIds, QVariantMap payload);
    void instabilityCompareOverviewRequestFailed(int requestId, QVariantList experimentIds, QString message);
    void instabilityCompareOverviewRequestCancelled(int requestId, QVariantList experimentIds, QString reason);

    void instabilityCompareCustomRequestStarted(int requestId,
                                                QVariantList experimentIds,
                                                double lowerMm,
                                                double upperMm);
    void instabilityCompareCustomRequestFinished(int requestId,
                                                 QVariantList experimentIds,
                                                 double lowerMm,
                                                 double upperMm,
                                                 QVariantMap payload);
    void instabilityCompareCustomRequestFailed(int requestId,
                                               QVariantList experimentIds,
                                               double lowerMm,
                                               double upperMm,
                                               QString message);
    void instabilityCompareCustomRequestCancelled(int requestId,
                                                  QVariantList experimentIds,
                                                  double lowerMm,
                                                  double upperMm,
                                                  QString reason);

    void uniformityCompareRequestStarted(int requestId, QVariantList experimentIds);
    void uniformityCompareRequestFinished(int requestId, QVariantList experimentIds, QVariantMap payload);
    void uniformityCompareRequestFailed(int requestId, QVariantList experimentIds, QString message);
    void uniformityCompareRequestCancelled(int requestId, QVariantList experimentIds, QString reason);

    void lightIntensityAverageCompareRequestStarted(int requestId, QVariantList experimentIds);
    void lightIntensityAverageCompareRequestFinished(int requestId, QVariantList experimentIds, QVariantMap payload);
    void lightIntensityAverageCompareRequestFailed(int requestId, QVariantList experimentIds, QString message);
    void lightIntensityAverageCompareRequestCancelled(int requestId, QVariantList experimentIds, QString reason);

private:
    void finishInstabilityOverviewRequest(int requestId,
                                          const QVariantList &experimentIds,
                                          const QVariantMap &payload,
                                          const QString &errorMessage,
                                          bool cancelled);
    void finishInstabilityCustomRequest(int requestId,
                                        const QVariantList &experimentIds,
                                        double lowerMm,
                                        double upperMm,
                                        const QVariantMap &payload,
                                        const QString &errorMessage,
                                        bool cancelled);
    void finishUniformityCompareRequest(int requestId,
                                        const QVariantList &experimentIds,
                                        const QVariantMap &payload,
                                        const QString &errorMessage,
                                        bool cancelled);
    void finishLightIntensityAverageCompareRequest(int requestId,
                                                   const QVariantList &experimentIds,
                                                   const QVariantMap &payload,
                                                   const QString &errorMessage,
                                                   bool cancelled);

private:
    SqlOrmManager *m_dbManager = nullptr;
    QPointer<QThread> m_instabilityOverviewThread;
    QPointer<QThread> m_instabilityCustomThread;
    QPointer<QThread> m_uniformityCompareThread;
    QPointer<QThread> m_lightIntensityAverageCompareThread;
    int m_activeInstabilityOverviewRequestId = 0;
    int m_activeInstabilityCustomRequestId = 0;
    int m_activeUniformityCompareRequestId = 0;
    int m_activeLightIntensityAverageCompareRequestId = 0;
    QVariantList m_activeInstabilityOverviewExperimentIds;
    QVariantList m_activeInstabilityCustomExperimentIds;
    QVariantList m_activeUniformityCompareExperimentIds;
    QVariantList m_activeLightIntensityAverageCompareExperimentIds;
    double m_activeInstabilityCustomLowerMm = 0.0;
    double m_activeInstabilityCustomUpperMm = 0.0;
    int m_requestSerial = 0;
};

#endif // COMPARE_CTRL_H
