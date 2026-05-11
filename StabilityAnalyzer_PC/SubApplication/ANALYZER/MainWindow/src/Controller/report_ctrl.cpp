#include "Controller/report_ctrl.h"

#include "PdfExporter.h"
#include "SqlOrmManager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QStandardPaths>
#include <QUrl>

namespace {

QString toLocalPath(const QString &pathOrUrl)
{
    const QString trimmed = pathOrUrl.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }

    const QUrl directUrl(trimmed);
    if (directUrl.isLocalFile()) {
        return directUrl.toLocalFile();
    }

    const QUrl userInputUrl = QUrl::fromUserInput(trimmed);
    if (userInputUrl.isLocalFile()) {
        return userInputUrl.toLocalFile();
    }

    return trimmed;
}

QString normalizedPath(const QString &path)
{
    const QString localPath = toLocalPath(path);
    if (localPath.isEmpty()) {
        return QString();
    }

    return QDir::fromNativeSeparators(QDir::cleanPath(QFileInfo(localPath).absoluteFilePath()));
}

QString safeFilePart(const QString &value)
{
    QString result = value.trimmed();
    if (result.isEmpty()) {
        return QStringLiteral("report");
    }

    static const QString invalidChars = QStringLiteral("\\/:*?\"<>|");
    for (const QChar ch : invalidChars) {
        result.replace(ch, QChar('_'));
    }
    result.replace(' ', '_');
    return result;
}

QString uniqueFilePath(const QString &directoryPath, const QString &baseName, const QString &suffix)
{
    QDir dir(directoryPath);
    QString candidate = dir.filePath(baseName + QLatin1Char('.') + suffix);
    if (!QFileInfo::exists(candidate)) {
        return normalizedPath(candidate);
    }

    for (int index = 1; index < 1000; ++index) {
        candidate = dir.filePath(QStringLiteral("%1_%2.%3").arg(baseName).arg(index).arg(suffix));
        if (!QFileInfo::exists(candidate)) {
            return normalizedPath(candidate);
        }
    }

    return normalizedPath(dir.filePath(baseName
                                       + QStringLiteral("_")
                                       + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmsszzz"))
                                       + QLatin1Char('.')
                                       + suffix));
}

QString boolToChinese(bool value)
{
    return value ? QStringLiteral("\u662f") : QStringLiteral("\u5426");
}

QVariantMap resolveProjectData(SqlOrmManager *dbManager, int projectId)
{
    QVariantMap projectData;
    if (!dbManager || projectId <= 0) {
        return projectData;
    }

    const QVector<QVariantMap> projects = dbManager->getAllProjects();
    for (const QVariantMap &row : projects) {
        if (row.value(QStringLiteral("id")).toInt() == projectId) {
            projectData = row;
            break;
        }
    }
    return projectData;
}

bool moveFileToTarget(const QString &sourcePath, const QString &targetPath)
{
    if (sourcePath == targetPath) {
        return QFileInfo::exists(targetPath);
    }

    QFile::remove(targetPath);
    if (QFile::rename(sourcePath, targetPath)) {
        return true;
    }

    if (!QFile::copy(sourcePath, targetPath)) {
        return false;
    }

    QFile::remove(sourcePath);
    return true;
}

} // namespace

reportCtrl::reportCtrl(QObject *parent)
    : QObject(parent)
    , m_dbManager(SqlOrmManager::instance())
{
}

reportCtrl::~reportCtrl()
{
    if (m_exportThread) {
        m_exportThread->requestInterruption();
        m_exportThread->wait(3000);
    }
}

bool reportCtrl::exportRunning() const
{
    return m_exportRunning;
}

QString reportCtrl::prepareReportTempDirectory(int experimentId) const
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir::tempPath();
    }

    const QString rootPath = QDir(baseDir).filePath(QStringLiteral("StabilityAnalyzer/report_export"));
    QDir().mkpath(rootPath);

    const QString tempPath = QDir(rootPath).filePath(
        QStringLiteral("experiment_%1_%2")
            .arg(qMax(experimentId, 0))
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmsszzz"))));
    QDir().mkpath(tempPath);
    return normalizedPath(tempPath);
}

QString reportCtrl::defaultReportOutputDirectory() const
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (baseDir.isEmpty()) {
        baseDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }

    const QString outputDir = QDir(baseDir).filePath(QStringLiteral("StabilityAnalyzerReports"));
    QDir().mkpath(outputDir);
    return normalizedPath(outputDir);
}

QString reportCtrl::normalizeDirectoryInput(const QString &pathOrUrl) const
{
    return normalizedPath(pathOrUrl);
}

QString reportCtrl::toFileUrl(const QString &path) const
{
    const QString localPath = normalizedPath(path);
    if (localPath.isEmpty()) {
        return QString();
    }

    return QUrl::fromLocalFile(localPath).toString();
}

bool reportCtrl::startExportExperimentReport(int experimentId,
                                             const QString &rawCurveImagePath,
                                             const QString &referenceCurveImagePath,
                                             const QString &instabilityImagePath,
                                             const QString &outputDirectory)
{
    if (m_exportRunning) {
        emit reportExportFinished(buildFailureResult(
            experimentId,
            tr("\u5df2\u6709\u62a5\u544a\u6b63\u5728\u5bfc\u51fa\uff0c\u8bf7\u7a0d\u540e\u518d\u8bd5")));
        return false;
    }

    const QString rawImage = normalizedPath(rawCurveImagePath);
    const QString referenceImage = normalizedPath(referenceCurveImagePath);
    const QString instabilityImage = normalizedPath(instabilityImagePath);
    const QString finalOutputDir = outputDirectory.trimmed().isEmpty()
        ? defaultReportOutputDirectory()
        : normalizedPath(outputDirectory);

    if (experimentId <= 0) {
        emit reportExportFinished(buildFailureResult(
            experimentId,
            tr("\u5bfc\u51fa\u5931\u8d25\uff1a\u5b9e\u9a8c\u8bb0\u5f55\u65e0\u6548")));
        return false;
    }
    if (!QFileInfo::exists(rawImage) || !QFileInfo(rawImage).isFile()) {
        emit reportExportFinished(buildFailureResult(
            experimentId,
            tr("\u5bfc\u51fa\u5931\u8d25\uff1a\u539f\u59cb\u5149\u5f3a\u56fe\u4e0d\u5b58\u5728")));
        return false;
    }
    if (!QFileInfo::exists(referenceImage) || !QFileInfo(referenceImage).isFile()) {
        emit reportExportFinished(buildFailureResult(
            experimentId,
            tr("\u5bfc\u51fa\u5931\u8d25\uff1a\u53c2\u6bd4\u5149\u5f3a\u56fe\u4e0d\u5b58\u5728")));
        return false;
    }
    if (!QFileInfo::exists(instabilityImage) || !QFileInfo(instabilityImage).isFile()) {
        emit reportExportFinished(buildFailureResult(
            experimentId,
            tr("\u5bfc\u51fa\u5931\u8d25\uff1a\u4e0d\u7a33\u5b9a\u6027\u56fe\u4e0d\u5b58\u5728")));
        return false;
    }
    if (!QDir().mkpath(finalOutputDir)) {
        emit reportExportFinished(buildFailureResult(
            experimentId,
            tr("\u5bfc\u51fa\u5931\u8d25\uff1a\u65e0\u6cd5\u521b\u5efa\u62a5\u544a\u76ee\u5f55")));
        return false;
    }

    const QString tempWorkspace = normalizedPath(QFileInfo(rawImage).absolutePath());
    QString templatePath = normalizedPath(
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("template_cn/template_cn.docx")));
    if (!QFileInfo::exists(templatePath)) {
        const QString fallbackTemplatePath = normalizedPath(
            QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../docs/template_cn.docx")));
        if (QFileInfo::exists(fallbackTemplatePath)) {
            templatePath = fallbackTemplatePath;
        }
    }
    if (!QFileInfo::exists(templatePath)) {
        emit reportExportFinished(buildFailureResult(
            experimentId,
            tr("\u5bfc\u51fa\u5931\u8d25\uff1a\u672a\u627e\u5230\u62a5\u544a\u6a21\u677f")));
        return false;
    }

    m_exportRunning = true;
    emit exportRunningChanged();

    QPointer<reportCtrl> self(this);
    QThread *workerThread = QThread::create([self,
                                            experimentId,
                                            rawImage,
                                            referenceImage,
                                            instabilityImage,
                                            finalOutputDir,
                                            tempWorkspace,
                                            templatePath]() {
        QVariantMap result;
        result.insert(QStringLiteral("success"), false);
        result.insert(QStringLiteral("experimentId"), experimentId);

        auto finalize = [self, &result]() {
            if (!self) {
                return;
            }

            const QVariantMap copiedResult = result;
            QMetaObject::invokeMethod(self, [self, copiedResult]() {
                if (self) {
                    self->finishExport(copiedResult);
                }
            }, Qt::QueuedConnection);
        };

        SqlOrmManager *dbManager = SqlOrmManager::instance();
        if (!dbManager) {
            result.insert(QStringLiteral("message"),
                          QObject::tr("\u5bfc\u51fa\u5931\u8d25\uff1a\u6570\u636e\u5e93\u7ba1\u7406\u5668\u4e0d\u53ef\u7528"));
            finalize();
            return;
        }

        const QVariantMap experimentData = dbManager->getExperimentById(experimentId);
        if (experimentData.isEmpty()) {
            result.insert(QStringLiteral("message"),
                          QObject::tr("\u5bfc\u51fa\u5931\u8d25\uff1a\u672a\u627e\u5230\u5bf9\u5e94\u5b9e\u9a8c\u8bb0\u5f55"));
            finalize();
            return;
        }

        const QVariantMap projectData = resolveProjectData(dbManager, experimentData.value(QStringLiteral("project_id")).toInt());
        const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmss"));
        const QString baseName = safeFilePart(QStringLiteral("experiment_report_%1_%2")
                                              .arg(experimentId)
                                              .arg(timestamp));
        const QString tempDocxPath = normalizedPath(QDir(tempWorkspace).filePath(baseName + QStringLiteral(".docx")));
        const QString tempPdfPath = normalizedPath(QDir(tempWorkspace).filePath(baseName + QStringLiteral(".pdf")));
        const QString finalDocxPath = uniqueFilePath(finalOutputDir, baseName, QStringLiteral("docx"));
        const QString finalPdfPath = uniqueFilePath(finalOutputDir, baseName, QStringLiteral("pdf"));

        QMap<QString, QVariant> textData;
        textData.insert(QStringLiteral("project_name"), projectData.value(QStringLiteral("project_name")).toString());
        textData.insert(QStringLiteral("id"), experimentData.value(QStringLiteral("id")).toString());
        textData.insert(QStringLiteral("sample_name"), experimentData.value(QStringLiteral("sample_name")).toString());
        textData.insert(QStringLiteral("description"), experimentData.value(QStringLiteral("description")).toString());
        textData.insert(QStringLiteral("created_at"), experimentData.value(QStringLiteral("created_at")).toString());
        textData.insert(QStringLiteral("operator_name"), experimentData.value(QStringLiteral("operator_name")).toString());
        textData.insert(QStringLiteral("duration"), experimentData.value(QStringLiteral("duration")).toString());
        textData.insert(QStringLiteral("interval"), experimentData.value(QStringLiteral("interval")).toString());
        textData.insert(QStringLiteral("count"), experimentData.value(QStringLiteral("count")).toString());
        textData.insert(QStringLiteral("temperature_control"),
                        boolToChinese(experimentData.value(QStringLiteral("temperature_control")).toBool()));
        textData.insert(QStringLiteral("target_temp"), experimentData.value(QStringLiteral("target_temp")).toString());
        textData.insert(QStringLiteral("scan_range_start"), experimentData.value(QStringLiteral("scan_range_start")).toString());
        textData.insert(QStringLiteral("scan_range_end"), experimentData.value(QStringLiteral("scan_range_end")).toString());
        textData.insert(QStringLiteral("scan_step"), experimentData.value(QStringLiteral("scan_step")).toString());

        QMap<QString, QString> imageData;
        imageData.insert(QStringLiteral("image1.png"), rawImage);
        imageData.insert(QStringLiteral("image2.png"), referenceImage);
        imageData.insert(QStringLiteral("image3.png"), instabilityImage);

        QMap<QString, QVector<QVector<QString>>> tableMap;
        PdfExporter exporter;
        const bool exportOk = exporter.exportFromTemplate(templatePath, tempDocxPath, tableMap, textData, imageData);
        if (!exportOk || !QFileInfo::exists(tempPdfPath)) {
            QString failureMessage = QObject::tr("\u5bfc\u51fa\u5931\u8d25\uff1a\u62a5\u544a\u751f\u6210\u6216 PDF \u8f6c\u6362\u672a\u5b8c\u6210");
            if (QFileInfo::exists(tempDocxPath) && moveFileToTarget(tempDocxPath, finalDocxPath)) {
                result.insert(QStringLiteral("docxPath"), finalDocxPath);
                failureMessage = QObject::tr("\u672a\u80fd\u751f\u6210 PDF\uff0c\u5df2\u5bfc\u51fa Word \u62a5\u544a\u5230: %1")
                                     .arg(QDir::toNativeSeparators(finalDocxPath));
            }

            result.insert(QStringLiteral("message"), failureMessage);
            QDir(tempWorkspace).removeRecursively();
            finalize();
            return;
        }

        if (!moveFileToTarget(tempPdfPath, finalPdfPath)) {
            result.insert(QStringLiteral("message"),
                          QObject::tr("\u5bfc\u51fa\u5931\u8d25\uff1a\u65e0\u6cd5\u4fdd\u5b58 PDF \u5230\u76ee\u6807\u76ee\u5f55"));
            QDir(tempWorkspace).removeRecursively();
            finalize();
            return;
        }

        QFile::remove(tempDocxPath);
        QDir(tempWorkspace).removeRecursively();

        result.insert(QStringLiteral("success"), true);
        result.insert(QStringLiteral("pdfPath"), finalPdfPath);
        result.insert(QStringLiteral("message"),
                      QObject::tr("\u62a5\u544a\u5df2\u5bfc\u51fa\u5230: %1").arg(QDir::toNativeSeparators(finalPdfPath)));
        finalize();
    });

    m_exportThread = workerThread;
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);
    workerThread->start();
    return true;
}

void reportCtrl::finishExport(const QVariantMap &result)
{
    m_exportThread = nullptr;
    m_exportRunning = false;
    emit exportRunningChanged();
    emit reportExportFinished(result);
}

QVariantMap reportCtrl::buildFailureResult(int experimentId, const QString &message) const
{
    QVariantMap result;
    result.insert(QStringLiteral("success"), false);
    result.insert(QStringLiteral("experimentId"), experimentId);
    result.insert(QStringLiteral("message"), message);
    return result;
}
