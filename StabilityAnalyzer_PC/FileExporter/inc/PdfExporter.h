#ifndef PDFEXPORTER_H
#define PDFEXPORTER_H

#include "IExportInterface.h"
#include <QPrinter>
#include <QPainter>
#include <QTextDocument>
#include <QDomDocument>
#include <QPdfWriter>

class FILEEXPORTER_EXPORT PdfExporter : public IExportInterface
{
public:
    PdfExporter();
    ~PdfExporter() override;
    
    // IExportInterface 接口实现
    bool exportToFile(const QString& filePath, 
                     const QList<QMap<QString, QVariant>>& data,
                     const QMap<QString, QVariant>& options = QMap<QString, QVariant>()) override;

    //使用模板导出文件，模板的样式是确定的，每个项目如果导出的word不一样就传递不同的数据，然后插入到表格中，模板资源暂时放在文件运行目录下 "word/template.docx"
//    bool exportFromTemplate(const QString &templatePath,const QString &saveAsPath,QString tableKey, QVector<QVector<QString>>tableDta,
//                             QMap<QString,QVariant> &data, QMap<QString, QString> &images);
    bool exportFromTemplate(const QString &templatePath,const QString &saveAsPath, QMap<QString ,QVector<QVector<QString>>>tableMap,
                             QMap<QString,QVariant> &data, QMap<QString, QString> &images);


    QStringList getSupportedFormats() override;
    bool isFormatSupported(const QString& format) override;
    QMap<QString, QVariant> getDefaultOptions() override;
    bool validateOptions(const QMap<QString, QVariant>& options) override;

private:
    //     bool replayTableIndex(const QString &templatePath, QString tableKey, QList<TableValue> TableData);
    //替换docx文档中的表格，这个地方我使用的是二维数组，这样的话不管是固定还是不固定的表格都可以赋值
    bool replayTableIndex(const QString &templatePath, QString tableKey, QVector<QVector<QString>> TableData);
    //解压文件
    bool extractFile(const QString &zipPath, const QString &targetDir);
    //压缩文件
    bool compressFile(const QString &zipPath, const QString &sourceDir);
    //修正压缩的docx文件为libreoffice可以识别的，否则的话输出不了pdf的
    bool fixDocxForLibreOffice(const QString &tempDir);
    //复制文件到指定的文件夹
    bool copyFile(const QString &sourcePath, const QString &destinationPath);
    //导出word到pdf文件，这个是调用外部程序输出pdf文件
    bool docxToPdf(const QString &docxPath, const QString &pdfPath);
    //压缩docx文件
    bool compressDocxDirectory(const QString &sourceDir, const QString &docxPath);
    //获取所有文件
    QStringList getAllFiles(const QString &dirPath);
    //按照顺序添加docx文件
    QStringList orderDocxFiles(const QStringList &files, const QString &baseDir);
    // XML转义函数
    QString escapeXml(const QString &text);

};

#endif // PDFEXPORTER_H
