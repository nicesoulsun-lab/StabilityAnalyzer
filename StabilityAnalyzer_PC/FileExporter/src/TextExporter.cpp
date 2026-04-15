#include "TextExporter.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QFileInfo>

TextExporter::TextExporter()
{
}

TextExporter::~TextExporter()
{
}

bool TextExporter::exportToFile(const QString& filePath, 
                               const QList<QMap<QString, QVariant>>& data,
                               const QMap<QString, QVariant>& options)
{
    if (data.isEmpty()) {
        qWarning() << "Empty data for text export";
        return false;
    }
    
    // 提取表头
    QList<QString> headers;
    if (!data.isEmpty()) {
        headers = data.first().keys();
    }
    
    // 转换数据格式
    QList<QList<QVariant>> tableData;
    for (const auto& row : data) {
        QList<QVariant> rowData;
        for (const auto& header : headers) {
            rowData.append(row.value(header));
        }
        tableData.append(rowData);
    }
    
    return true;
}

QStringList TextExporter::getSupportedFormats()
{
    return {"csv", "txt"};
}

bool TextExporter::isFormatSupported(const QString& format)
{
    QString lowerFormat = format.toLower();
    return lowerFormat == "csv" || lowerFormat == "txt";
}

QMap<QString, QVariant> TextExporter::getDefaultOptions()
{
    QMap<QString, QVariant> options;
    options["encoding"] = "UTF-8";
    options["delimiter"] = ",";
    options["quote"] = "\"";
    options["includeHeader"] = true;
    options["dateFormat"] = "yyyy-MM-dd";
    options["timeFormat"] = "hh:mm:ss";
    options["datetimeFormat"] = "yyyy-MM-dd hh:mm:ss";
    options["txtDelimiter"] = "\t";
    options["txtAlignment"] = "left";
    
    return options;
}

bool TextExporter::validateOptions(const QMap<QString, QVariant>& options)
{
    // 验证编码
    if (options.contains("encoding")) {
        QString encoding = options["encoding"].toString();
        QStringList validEncodings = {"UTF-8", "GBK", "GB2312", "ISO-8859-1"};
        if (!validEncodings.contains(encoding, Qt::CaseInsensitive)) {
            qWarning() << "Invalid encoding:" << encoding;
            return false;
        }
    }
    
    // 验证分隔符
    if (options.contains("delimiter")) {
        QString delimiter = options["delimiter"].toString();
        if (delimiter.length() != 1) {
            qWarning() << "Invalid delimiter length:" << delimiter;
            return false;
        }
    }
    
    // 验证对齐方式
    if (options.contains("txtAlignment")) {
        QString alignment = options["txtAlignment"].toString();
        QStringList validAlignments = {"left", "right", "center"};
        if (!validAlignments.contains(alignment, Qt::CaseInsensitive)) {
            qWarning() << "Invalid alignment:" << alignment;
            return false;
        }
    }
    
    return true;
}

bool TextExporter::generateCsvFile(const QString& filePath, 
                                  const QList<QString>& headers,
                                  const QList<QList<QVariant>>& data,
                                  const QMap<QString, QVariant>& options)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }
    
    QTextStream out(&file);
    QString encoding = options.value("encoding", "UTF-8").toString();
    out.setCodec(encoding.toUtf8());
    
    QString delimiter = options.value("delimiter", ",").toString();
    bool includeHeader = options.value("includeHeader", true).toBool();
    
    // 写入表头
    if (includeHeader) {
        for (int i = 0; i < headers.size(); ++i) {
            out << escapeCsvValue(headers[i]);
            if (i < headers.size() - 1) {
                out << delimiter;
            }
        }
        out << "\n";
    }
    
    // 写入数据
    for (const auto& row : data) {
        for (int i = 0; i < row.size(); ++i) {
            out << escapeCsvValue(formatCellValue(row[i], "csv"));
            if (i < row.size() - 1) {
                out << delimiter;
            }
        }
        out << "\n";
    }
    
    file.close();
    
    qDebug() << "CSV file exported successfully to:" << filePath;
    return true;
}

bool TextExporter::generateTxtFile(const QString& filePath, 
                                  const QList<QString>& headers,
                                  const QList<QList<QVariant>>& data,
                                  const QMap<QString, QVariant>& options)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }
    
    QTextStream out(&file);
    QString encoding = options.value("encoding", "UTF-8").toString();
    out.setCodec(encoding.toUtf8());
    
    QString delimiter = options.value("txtDelimiter", "\t").toString();
    bool includeHeader = options.value("includeHeader", true).toBool();
    
    // 写入表头
    if (includeHeader) {
        for (int i = 0; i < headers.size(); ++i) {
            out << escapeTxtValue(headers[i]);
            if (i < headers.size() - 1) {
                out << delimiter;
            }
        }
        out << "\n";
    }
    
    // 写入数据
    for (const auto& row : data) {
        for (int i = 0; i < row.size(); ++i) {
            out << escapeTxtValue(formatCellValue(row[i], "txt"));
            if (i < row.size() - 1) {
                out << delimiter;
            }
        }
        out << "\n";
    }
    
    file.close();
    
    qDebug() << "TXT file exported successfully to:" << filePath;
    return true;
}

QString TextExporter::formatCellValue(const QVariant& value, const QString& format)
{
    if (value.canConvert<QDateTime>()) {
        QString formatStr = format == "csv" ? "yyyy-MM-dd hh:mm:ss" : "yyyy-MM-dd hh:mm:ss";
        return value.toDateTime().toString(formatStr);
    } else if (value.canConvert<QDate>()) {
        QString formatStr = format == "csv" ? "yyyy-MM-dd" : "yyyy-MM-dd";
        return value.toDate().toString(formatStr);
    } else if (value.canConvert<QTime>()) {
        QString formatStr = format == "csv" ? "hh:mm:ss" : "hh:mm:ss";
        return value.toTime().toString(formatStr);
    } else if (value.type() == QVariant::Double) {
        return QString::number(value.toDouble(), 'f', 6);
    } else if (value.type() == QVariant::Int || value.type() == QVariant::LongLong) {
        return QString::number(value.toLongLong());
    } else if (value.type() == QVariant::Bool) {
        return value.toBool() ? "TRUE" : "FALSE";
    }
    
    return value.toString();
}

QString TextExporter::escapeCsvValue(const QString& value)
{
    if (value.contains(',') || value.contains('\n') || value.contains('\r') || value.contains('"')) {
        QString escapedValue = value;
        escapedValue.replace('"', "\"\"");
        return '"' + escapedValue + '"';
    }
    return value;
}

QString TextExporter::escapeTxtValue(const QString& value)
{
    // 对于TXT文件，只需要处理换行符
    QString escapedValue = value;
    escapedValue.replace('\n', ' ');
    escapedValue.replace('\r', ' ');
    return escapedValue;
}

QString TextExporter::getFileFormat(const QString& filePath)
{
    QString extension = QFileInfo(filePath).suffix().toLower();
    if (extension == "csv" || extension == "txt") {
        return extension;
    }
    
    // 默认返回txt格式
    return "txt";
}
