#ifndef EXCELEXPORTER_H
#define EXCELEXPORTER_H

#include "fileexporter_global.h"
#include "IExportInterface.h"
#include <QObject>
#include <QVariant>
#include <QList>
#include <QMap>
#include <QString>
#include <QImage>-
#include "xlsxdocument.h"
#include "xlsxchartsheet.h"
#include "xlsxcellrange.h"
#include "xlsxchart.h"
/**
 * excel文件导出
**/
using namespace QXlsx;

class FILEEXPORTER_EXPORT ExcelExporter : public IExportInterface
{
public:
    ExcelExporter();
    ~ExcelExporter() override;
    
    // IExportInterface 接口实现
    bool exportToFile(const QString& filePath,
                      const QList<QMap<QString, QVariant>>& data,
                      const QMap<QString, QVariant>& options = QMap<QString, QVariant>()) override;

    //使用模板导出文件，模板的样式是确定的，每个项目如果导出的excel不一样就传递不同的数据，然后插入到表格中，模板资源暂时放在文件运行目录下 "xlsx/template.xlsx"
    bool exportFromTemplate(const QString &templatePath,
                            const QString &saveAsPath, QMap<QString,QVariant> &data);

    //这个是针对模板然后是有多行相似数据的方法
    bool exportFile(const QString &templatePath,const QString &saveAsPath,
                    const QString& sheetName,const int& startRow, const QVector<QVector<QVariant>>);

    QStringList getSupportedFormats() override;
    bool isFormatSupported(const QString& format) override;
    QMap<QString, QVariant> getDefaultOptions() override;
    bool validateOptions(const QMap<QString, QVariant>& options) override;

private:
    // 生成Excel文件内容（使用CSV格式作为基础实现）
    bool generateCsvFile(const QString& filePath, 
                        const QList<QString>& headers,
                        const QList<QList<QVariant>>& data);
    
    // 格式化单元格值
    QString formatCellValue(const QVariant& value);
    
    // 转义CSV特殊字符
    QString escapeCsvValue(const QString& value);
    
    // 检查是否支持高级Excel功能
    bool hasAdvancedExcelSupport();
};

#endif // EXCELEXPORTER_H
