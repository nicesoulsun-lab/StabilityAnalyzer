#ifndef EXPORTMANAGER_H
#define EXPORTMANAGER_H

#include "fileexporter_global.h"
#include "IExportInterface.h"
#include <QObject>
#include <QMap>
#include <QString>
#include <QList>
#include <QVariant>
#include <QImage>

class FILEEXPORTER_EXPORT ExportManager : public QObject
{
    Q_OBJECT

public:
    static ExportManager& getInstance();
    
    // 注册导出器
    void registerExporter(const QString& format, IExportInterface* exporter);
    
    // 注销导出器
    void unregisterExporter(const QString& format);
    
    // 获取所有支持的格式
    QStringList getSupportedFormats() const;
    
    // 检查是否支持特定格式
    bool isFormatSupported(const QString& format) const;
    
    // 获取特定格式的导出器
    IExportInterface* getExporter(const QString& format) const;
    
    // 通用导出方法
    bool exportToFile(const QString& filePath, 
                     const QList<QMap<QString, QVariant>>& data,
                     const QString& format,
                     const QMap<QString, QVariant>& options = QMap<QString, QVariant>());
    // 获取默认选项
    QMap<QString, QVariant> getDefaultOptions(const QString& format) const;
    
    // 验证选项
    bool validateOptions(const QString& format, const QMap<QString, QVariant>& options) const;

private:
    ExportManager();
    ~ExportManager();
    
    // 禁止拷贝和赋值
    ExportManager(const ExportManager&) = delete;
    ExportManager& operator=(const ExportManager&) = delete;
    
    QMap<QString, IExportInterface*> m_exporters;
};
#define EXPORTMANAGER ExportManager::getInstance()
#endif // EXPORTMANAGER_H
