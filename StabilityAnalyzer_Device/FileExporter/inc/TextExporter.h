#ifndef TEXTPORTER_H
#define TEXTPORTER_H

#include "fileexporter_global.h"
#include "IExportInterface.h"
#include <QObject>
#include <QVariant>
#include <QList>
#include <QMap>
#include <QString>
#include <QImage>

class FILEEXPORTER_EXPORT TextExporter : public IExportInterface
{
public:
    TextExporter();
    ~TextExporter() override;
    
    // IExportInterface 接口实现
    bool exportToFile(const QString& filePath, 
                     const QList<QMap<QString, QVariant>>& data,
                     const QMap<QString, QVariant>& options = QMap<QString, QVariant>()) override;

    QStringList getSupportedFormats() override;
    bool isFormatSupported(const QString& format) override;
    QMap<QString, QVariant> getDefaultOptions() override;
    bool validateOptions(const QMap<QString, QVariant>& options) override;

private:
    // 生成CSV文件
    bool generateCsvFile(const QString& filePath, 
                        const QList<QString>& headers,
                        const QList<QList<QVariant>>& data,
                        const QMap<QString, QVariant>& options);
    
    // 生成TXT文件
    bool generateTxtFile(const QString& filePath, 
                        const QList<QString>& headers,
                        const QList<QList<QVariant>>& data,
                        const QMap<QString, QVariant>& options);
    
    // 格式化单元格值
    QString formatCellValue(const QVariant& value, const QString& format);
    
    // 转义CSV特殊字符
    QString escapeCsvValue(const QString& value);
    
    // 转义TXT特殊字符
    QString escapeTxtValue(const QString& value);
    
    // 获取文件格式
    QString getFileFormat(const QString& filePath);
};

#endif // TEXTPORTER_H
