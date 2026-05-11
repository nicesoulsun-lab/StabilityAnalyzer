#ifndef REPORT_CTRL_H
#define REPORT_CTRL_H

#include <QObject>
#include <QPointer>
#include <QThread>
#include <QVariantMap>

#include "mainwindow_global.h"

class SqlOrmManager;

class MAINWINDOW_EXPORT reportCtrl : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool exportRunning READ exportRunning NOTIFY exportRunningChanged)

public:
    explicit reportCtrl(QObject *parent = nullptr);
    ~reportCtrl() override;

    bool exportRunning() const;

    Q_INVOKABLE QString prepareReportTempDirectory(int experimentId) const;
    Q_INVOKABLE QString defaultReportOutputDirectory() const;
    Q_INVOKABLE QString normalizeDirectoryInput(const QString &pathOrUrl) const;
    Q_INVOKABLE QString toFileUrl(const QString &path) const;
    Q_INVOKABLE bool startExportExperimentReport(int experimentId,
                                                 const QString &rawCurveImagePath,
                                                 const QString &referenceCurveImagePath,
                                                 const QString &instabilityImagePath,
                                                 const QString &outputDirectory = QString());

signals:
    void exportRunningChanged();
    void reportExportFinished(QVariantMap result);

private:
    void finishExport(const QVariantMap &result);
    QVariantMap buildFailureResult(int experimentId, const QString &message) const;

    SqlOrmManager *m_dbManager = nullptr;
    QPointer<QThread> m_exportThread;
    bool m_exportRunning = false;
};

#endif // REPORT_CTRL_H
