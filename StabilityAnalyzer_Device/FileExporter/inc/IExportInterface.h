#ifndef IEXPORTINTERFACE_H
#define IEXPORTINTERFACE_H

#include "fileexporter_global.h"
#include <QString>
#include <QVariant>
#include <QList>
#include <QMap>

class FILEEXPORTER_EXPORT IExportInterface
{
public:
    virtual ~IExportInterface() = default;
    
    // 导出数据到文件
    virtual bool exportToFile(const QString& filePath, 
                             const QList<QMap<QString, QVariant>>& data,
                             const QMap<QString, QVariant>& options = QMap<QString, QVariant>()) = 0;
    // 获取支持的格式列表
    virtual QStringList getSupportedFormats() = 0;
    
    // 检查是否支持特定格式
    virtual bool isFormatSupported(const QString& format) = 0;
    
    // 获取默认选项
    virtual QMap<QString, QVariant> getDefaultOptions() = 0;
    
    // 验证选项
    virtual bool validateOptions(const QMap<QString, QVariant>& options) = 0;
};

#endif // IEXPORTINTERFACE_H
