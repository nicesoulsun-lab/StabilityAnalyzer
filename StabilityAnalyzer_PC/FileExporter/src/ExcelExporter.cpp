#include "ExcelExporter.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
/**
 * 导出文件到excel文件
**/
ExcelExporter::ExcelExporter()
{
}

ExcelExporter::~ExcelExporter()
{
}

//导出文件
bool ExcelExporter::exportToFile(const QString& filePath, 
                                const QList<QMap<QString, QVariant>>& data,
                                const QMap<QString, QVariant>& options)
{
    if (data.isEmpty()) {
        qWarning() << "Empty data for Excel export";
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

//使用模板导出文件
//传递给我的参数，模板文件路径，这个主要是为了区分有多个模板文件的情况
//生成的excel保存到的文件路径，写入单元格的数据
bool ExcelExporter::exportFromTemplate(const QString &templatePath,
                                       const QString &saveAsPath,
                                       QMap<QString,QVariant> &map)
{
    // 复制模板到目标（保持模板只读）
    if (!QFile::copy(templatePath, saveAsPath)) {
        // 已存在则覆盖
        QFile::remove(saveAsPath);
        if (!QFile::copy(templatePath, saveAsPath))
            return false;
    }

    QMap<QString, QString> keyMap;
    // 打开模板文件，获取keymap的时候打开的是模板文件，使用{}作为作用域，离开作用域xlsx销毁了就，因为这个需要切换xlsx文件就使用了{}作用域
    {
        QXlsx::Document xlsx(templatePath);

        //先获取_map_这个sheet页,读取这个sheet页的数据，获取到占位符-位置的map
        xlsx.selectSheet("_map_");
        QXlsx::Worksheet *mapSheet = xlsx.currentWorksheet();
        if (!mapSheet)
            return false;
        auto dim = mapSheet->dimension();
        for (int r = dim.firstRow(); r <= dim.lastRow(); ++r)
        {
            int columnCount = dim.columnCount(); //不遍历列了，默认只有两列，直接获取每一列的数据即可
            //检查下列的大小，如果小于2是不对的，一般是只有2列，分别代表占位符和占位符对应的位置
            if(columnCount<2)
                return false;
            //获取占位符-位置map
            auto *keyCell = mapSheet->cellAt(r, 1); //获取占位符
            auto *valueCell = mapSheet->cellAt(r, 2); //获取占位符在的位置，比如B3这种
            QString key = "";
            QString value = "";
            if (!keyCell ||!valueCell)
                continue;
            key = keyCell->value().toString();
            value = valueCell->value().toString();
            //添加数据到keymap
            if(!key.isEmpty() && !value.isEmpty()){
                keyMap[key] = value;
            }
        }
    }

    // 打开复制的文件副本,打开文件副本的时候一定是在原始文件获取了keymap之后打开
    QXlsx::Document xlsx(saveAsPath);  // 打开副本文件，这个时候关闭了之前的模板文件
    xlsx.deleteSheet("_map_");

    //遍历传的数据map，根据key值去占位符和位置keyMap找到对应的位置，然后把值赋给对应的位置
    QMap<QString, QVariant>::iterator itor;
    for (itor = map.begin(); itor!= map.end(); ++itor)
    {
//        qDebug()<<"读取到map数据："<< itor.key() << "：" <<itor.value();
        QString key = itor.key();
        QString value = itor.value().toString();
        QMap<QString, QString>::iterator it = keyMap.find(key); //根据占位符去找对应的sheet页和单元格位置

        if(it!= keyMap.end()){
            QString position = it.value(); //获取到位置
            //根据位置找到模板副本里面对应的sheet页的excel单元格，有可能有多个sheet页，所以keymap这个位置存储的是以sheet页+单元格位置来存储的
            QStringList list = position.split("!"); //在这个地方使用的！来分割的，因为这个符合我觉着是不太会有的
            if(list.size()<2)
                continue;
            QString destSheet = list.at(0);
            QString destCell = list.at(1);

            //打开目标sheet页，然后给这个sheet页对应的单元格赋值
            xlsx.selectSheet(destSheet);
            QXlsx::Worksheet *destWorkerSheet = xlsx.currentWorksheet();
            //TODO这个地方我感觉是不是需要判断下，就是如果这个写入的单元格已经存在文字的话就拼接显示还是怎么着
            QString text = destWorkerSheet->read(destCell).toString();
            QString writeValue = value;
            if(!text.isEmpty())
                writeValue = text + writeValue; //text + "\n" +writeValue;
            destWorkerSheet->write(destCell ,writeValue); //这个写的形式是使用的字母的形式比如B5，其实就是对应的第五行第二列
        }
    }

    // 最后保存文件，等到所有的操作都执行完之后再保存文件
    return xlsx.save();
}

bool ExcelExporter::exportFile(const QString &templatePath, const QString &saveAsPath,
                               const QString& sheetName, const int& startRow, const QVector<QVector<QVariant> >data)
{
    // 复制模板到目标（保持模板只读）
    if (!QFile::copy(templatePath, saveAsPath)) {
        // 已存在则覆盖
        QFile::remove(saveAsPath);
        if (!QFile::copy(templatePath, saveAsPath))
            return false;
    }

    //打开副本文件
    QXlsx::Document xlsx(saveAsPath);
    xlsx.deleteSheet("_map_");

    //获取要写的sheet页
    xlsx.selectSheet(sheetName);
    QXlsx::Worksheet *destWorkerSheet = xlsx.currentWorksheet();

    for (int i=0; i<data.size();i++ ) {
        QVector<QVariant> szData = data[i];
        for (int j = 0; j < szData.size(); ++j)
            destWorkerSheet->write(startRow+i, j + 1, szData.at(j));
    }

    return xlsx.save();
}

QStringList ExcelExporter::getSupportedFormats()
{
    if (hasAdvancedExcelSupport()) {
        return {"xlsx", "xls", "csv"};
    }
    return {"csv"};
}

bool ExcelExporter::isFormatSupported(const QString& format)
{
    QString lowerFormat = format.toLower();
    if (hasAdvancedExcelSupport()) {
        return lowerFormat == "xlsx" || lowerFormat == "xls" || lowerFormat == "csv";
    }
    return lowerFormat == "csv";
}

QMap<QString, QVariant> ExcelExporter::getDefaultOptions()
{
    QMap<QString, QVariant> options;
    options["encoding"] = "UTF-8";
    options["delimiter"] = ",";
    options["quote"] = "\"";
    options["includeHeader"] = true;
    options["autoSizeColumns"] = true;
    options["dateFormat"] = "yyyy-MM-dd";
    options["timeFormat"] = "hh:mm:ss";
    options["datetimeFormat"] = "yyyy-MM-dd hh:mm:ss";
    
    return options;
}

bool ExcelExporter::validateOptions(const QMap<QString, QVariant>& options)
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
    
    return true;
}

bool ExcelExporter::generateCsvFile(const QString& filePath, 
                                   const QList<QString>& headers,
                                   const QList<QList<QVariant>>& data)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }
    
    QTextStream out(&file);
    out.setCodec("UTF-8");
    
    // 写入表头
    for (int i = 0; i < headers.size(); ++i) {
        out << escapeCsvValue(headers[i]);
        if (i < headers.size() - 1) {
            out << ",";
        }
    }
    out << "\n";
    
    // 写入数据
    for (const auto& row : data) {
        for (int i = 0; i < row.size(); ++i) {
            out << escapeCsvValue(formatCellValue(row[i]));
            if (i < row.size() - 1) {
                out << ",";
            }
        }
        out << "\n";
    }
    
    file.close();
    
    qDebug() << "CSV file exported successfully to:" << filePath;
    return true;
}

QString ExcelExporter::formatCellValue(const QVariant& value)
{
    if (value.canConvert<QDateTime>()) {
        return value.toDateTime().toString("yyyy-MM-dd hh:mm:ss");
    } else if (value.canConvert<QDate>()) {
        return value.toDate().toString("yyyy-MM-dd");
    } else if (value.canConvert<QTime>()) {
        return value.toTime().toString("hh:mm:ss");
    } else if (value.type() == QVariant::Double) {
        return QString::number(value.toDouble(), 'f', 6);
    } else if (value.type() == QVariant::Int || value.type() == QVariant::LongLong) {
        return QString::number(value.toLongLong());
    } else if (value.type() == QVariant::Bool) {
        return value.toBool() ? "TRUE" : "FALSE";
    }
    
    return value.toString();
}

QString ExcelExporter::escapeCsvValue(const QString& value)
{
    if (value.contains(',') || value.contains('\n') || value.contains('\r') || value.contains('"')) {
        QString escapedValue = value;
        escapedValue.replace('"', "\"\"");
        return '"' + escapedValue + '"';
    }
    return value;
}

bool ExcelExporter::hasAdvancedExcelSupport()
{
    // 检查是否安装了支持高级Excel功能的库
    // 目前返回false，只支持CSV格式
    return false;
}
