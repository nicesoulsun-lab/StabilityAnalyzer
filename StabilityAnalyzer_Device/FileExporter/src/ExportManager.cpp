#include "ExportManager.h"
#include "PdfExporter.h"
#include "ExcelExporter.h"
#include "TextExporter.h"
#include <QDebug>
#include <QFileInfo>

ExportManager& ExportManager::getInstance()
{
    static ExportManager instance;
    return instance;
}

ExportManager::ExportManager()
{
    // 注册默认导出器
    registerExporter("pdf", new PdfExporter());
    registerExporter("xlsx", new ExcelExporter());
    registerExporter("xls", new ExcelExporter());
    registerExporter("csv", new TextExporter());
    registerExporter("txt", new TextExporter());
}

ExportManager::~ExportManager()
{
    // 清理所有导出器
    for (auto exporter : m_exporters) {
        delete exporter;
    }
    m_exporters.clear();
}

void ExportManager::registerExporter(const QString& format, IExportInterface* exporter)
{
    if (!exporter) {
        qWarning() << "Cannot register null exporter for format:" << format;
        return;
    }
    
    QString normalizedFormat = format.toLower();
    if (m_exporters.contains(normalizedFormat)) {
        qWarning() << "Exporter for format" << normalizedFormat << "already exists. Replacing.";
        delete m_exporters[normalizedFormat];
    }
    
    m_exporters[normalizedFormat] = exporter;
    qDebug() << "Registered exporter for format:" << normalizedFormat;
}

void ExportManager::unregisterExporter(const QString& format)
{
    QString normalizedFormat = format.toLower();
    if (m_exporters.contains(normalizedFormat)) {
        delete m_exporters[normalizedFormat];
        m_exporters.remove(normalizedFormat);
        qDebug() << "Unregistered exporter for format:" << normalizedFormat;
    } else {
        qWarning() << "No exporter found for format:" << normalizedFormat;
    }
}

QStringList ExportManager::getSupportedFormats() const
{
    return m_exporters.keys();
}

bool ExportManager::isFormatSupported(const QString& format) const
{
    QString normalizedFormat = format.toLower();
    
    // 首先检查文件扩展名
    if (m_exporters.contains(normalizedFormat)) {
        return true;
    }
    
    // 检查每个导出器是否支持该格式
    for (auto exporter : m_exporters) {
        if (exporter->isFormatSupported(normalizedFormat)) {
            return true;
        }
    }
    
    return false;
}

IExportInterface* ExportManager::getExporter(const QString& format) const
{
    QString normalizedFormat = format.toLower();
    
    // 首先尝试直接匹配
    if (m_exporters.contains(normalizedFormat)) {
        return m_exporters[normalizedFormat];
    }
    
    // 查找支持该格式的导出器
    for (auto exporter : m_exporters) {
        if (exporter->isFormatSupported(normalizedFormat)) {
            return exporter;
        }
    }
    
    return nullptr;
}

bool ExportManager::exportToFile(const QString& filePath, 
                                const QList<QMap<QString, QVariant>>& data,
                                const QString& format,
                                const QMap<QString, QVariant>& options)
{
    QString fileExtension = QFileInfo(filePath).suffix().toLower();
    QString targetFormat = format.isEmpty() ? fileExtension : format.toLower();
    
    IExportInterface* exporter = getExporter(targetFormat);
    if (!exporter) {
        qWarning() << "No exporter found for format:" << targetFormat;
        return false;
    }
    
    return exporter->exportToFile(filePath, data, options);
}

QMap<QString, QVariant> ExportManager::getDefaultOptions(const QString& format) const
{
    IExportInterface* exporter = getExporter(format);
    if (exporter) {
        return exporter->getDefaultOptions();
    }
    
    qWarning() << "No exporter found for format:" << format;
    return QMap<QString, QVariant>();
}

bool ExportManager::validateOptions(const QString& format, const QMap<QString, QVariant>& options) const
{
    IExportInterface* exporter = getExporter(format);
    if (exporter) {
        return exporter->validateOptions(options);
    }
    
    qWarning() << "No exporter found for format:" << format;
    return false;
}
