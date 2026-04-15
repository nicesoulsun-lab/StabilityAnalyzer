#include "PdfExporter.h"
#include <QDebug>
#include <QDateTime>
#include "../Quazip/inc/quazip.h"
#include "../Quazip/inc/quazipfile.h"
#include "../Quazip/inc/JlCompress.h"
#include "logmanager.h"

PdfExporter::PdfExporter()
{
}

PdfExporter::~PdfExporter()
{

}

bool PdfExporter::exportToFile(const QString& filePath,
                              const QList<QMap<QString, QVariant>>& data,
                              const QMap<QString, QVariant>& options)
{
    Q_UNUSED(data)
    Q_UNUSED(options)

    qWarning() << "PDF export for generic data not implemented. Use specific export methods.";
    return false;
}

//使用模板导出文件，传递数据然后导出
bool PdfExporter::exportFromTemplate(const QString &templatePath, const QString &saveAsPath,QMap<QString ,QVector<QVector<QString>>>tableMap,
                                     QMap<QString, QVariant> &data, QMap<QString, QString> &images)
{
    int lastIndex = saveAsPath.lastIndexOf("/");
    QString path = saveAsPath.left(lastIndex); //获取模板文件在的文件夹;

    // 复制模板到目标（保持模板只读）
    QString tempFilePath = path + "/temptemplate.docx"; //这个是复制的模板副本
    if (!QFile::copy(templatePath, tempFilePath)) {
        // 已存在则覆盖
        QFile::remove(tempFilePath);
        if (!QFile::copy(templatePath, tempFilePath))
            return false;
    }

    //把模板docx文件压缩为zip文件，然后解压缩，然后找到解压缩之后的文件夹里面的word/document.xml文件
    //然后遍历找这个xml文件里面的占位符然后每个占位符替换为各自的东西，比如文本、表格、图片啥的
    //测试的时候发现不需要先压缩之后再解压缩，直接就解压就行，因为docx本质上就是xml文件和zip文件
    //解压到一个临时目录下，要确保解压的时候和最后保存的文档不在一个路径下，要不然之前的句柄还没放掉又开始新的句柄了
    QString tempDir = path + "/wordZip"; //TODO判断下如果这个文件夹已经存在之前解压的文件就删除
    QDir().mkpath(tempDir);

    //解压文件
    extractFile(tempFilePath, tempDir);
    QString documentPath = tempDir + "/word/document.xml";

    //读取document.xml的文件，然后找占位符，然后替换占位符的数据
    QFile file(documentPath);
    if(!file.exists())
    {
        file.close();
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) return false;
    QByteArray xml = file.readAll();
    file.close();

    //替换文字占位符，对于图片的话也是这样替换，只不过需要找一下对应的xml里面的图片占位符的位置，然后这一段替换
    QString content = QString::fromUtf8(xml);
    for (auto it = data.begin(); it != data.end(); ++it)
        content.replace("{" + it.key() + "}", it.value().toString());

    //以 WriteOnly 重新打开同一文件，把替换后的内容写回去
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    file.write(content.toUtf8());
    file.close(); // 必须关闭，否则后面压缩会失败

    //替换图片
    if(!images.isEmpty()){
        QString mediaPath = tempDir + "/word/media/";
        QDir mediaDir(mediaPath);

        // 确保media目录存在
        if (!mediaDir.exists()) {
            mediaDir.mkpath(".");
        }
        for (auto it = images.begin(); it!=images.end(); ++it) {
            QString placeholder = it.key();       // 图片名称
            QString imagePath = it.value();       // 新图片路径
            // 目标图片路径
            QString destPath = mediaPath + placeholder;

            // 复制图片到media目录
            if (!copyFile(imagePath, destPath)) {
                qWarning() << "图片复制失败:" << imagePath << "->" << destPath;
                continue;
            }
        }
    }

    //替换表格数据，表格数据结构不确定，根据传递的参数赋值，这样的话传递的参数只能是二维数组这种，一维数组里面需要是数组形式的不能是结构体或者类，这样没办法做到通用
    //遍历整个表格数据，根据键值找对应的表格数据然后进行替换
    for (auto &pair : tableMap.toStdMap()) {
        qDebug() << pair.first << ":" << pair.second;
        QString key = pair.first;
        QVector<QVector<QString>>tableData = pair.second;
        bool sucess = replayTableIndex(documentPath,key,tableData);
        if(sucess)
            qDebug()<<"表格数据替换成功：" << key;
    }

    //修改为libreoffice认识的格式
    if (!fixDocxForLibreOffice(tempDir)) {
        qWarning() << "修复 docx 模板标记失败";
        return false;
    }

    // 确保所有必要的文件都存在
    QStringList requiredFiles = {
        "[Content_Types].xml",
        "_rels/.rels",
        "word/document.xml",
        "word/_rels/document.xml.rels",
        "word/theme/theme1.xml",
        "word/settings.xml",
        "word/styles.xml",
        "word/fontTable.xml",
        "docProps/core.xml",
        "docProps/app.xml"
    };

    // 检查必要文件
    bool allFilesExist = true;
    foreach (const QString& file, requiredFiles) {
        QString filePath = tempDir + "/" + file;
        if (!QFile::exists(filePath)) {
            qWarning() << "缺少docx必要文件:" << file;
            allFilesExist = false;
        }
    }

    if (!allFilesExist) {
        qWarning() << "docx文件结构不完整，可能导致LibreOffice无法识别";
        return false;
    }

    // 压缩文件，确保文件顺序和结构正确，这个地方如果解压和压缩用的是一个文件测试发现会出现占用的问题，这个地方用了两个文件
    if (!compressDocxDirectory(tempDir, saveAsPath)) {
        qWarning() << "压缩docx文件失败";
        return false;
    }

    //删除不再用的文件,暂时不删除，测试看下
//    QDir(tempDir).removeRecursively();
    QFile::remove(tempFilePath);

    //导出为pdf文件
    QString pdfPath = path + "/";
    return docxToPdf(saveAsPath, pdfPath);
}

//docx压缩,遍历所有的文件压缩进去docx，要不然压缩之后的docx文件是损坏的，我在我电脑测试的是wps能打开，libreoffice调用输出pdf的失败，原因应该是因为替换占位符的时候xml结构变了
bool PdfExporter::compressDocxDirectory(const QString &sourceDir, const QString &docxPath)
{
    // 先创建一个临时zip文件
    QString tempZip = docxPath + ".temp.zip";

    // 使用QuaZip手动创建，确保文件顺序和结构正确
    QuaZip zip(tempZip);
    if (!zip.open(QuaZip::mdCreate)) {
        qWarning() << "无法创建zip文件";
        return false;
    }

    QDir source(sourceDir);
    QStringList files = getAllFiles(sourceDir);

    // 检查是否有文件
    if (files.isEmpty()) {
        qWarning() << "源目录为空，无法压缩";
        zip.close();
        QFile::remove(tempZip);
        return false;
    }

    // 按docx标准顺序添加文件，一定要按照顺序添加文件要不然会出现压缩的docx文件格式错误然后输出不了pdf文件
    QStringList orderedFiles = orderDocxFiles(files, sourceDir);

    bool allFilesWritten = true;
    int filesWritten = 0;

    foreach (const QString& relativePath, orderedFiles) {
        QString absolutePath = sourceDir + "/" + relativePath;
        QFileInfo fileInfo(absolutePath);

        if (fileInfo.isDir()) {
            continue;
        }

        QFile inFile(absolutePath);
        if (!inFile.open(QIODevice::ReadOnly)) {
            qWarning() << "无法打开文件:" << absolutePath;
            allFilesWritten = false;
            inFile.close();
            continue;
        }

        QuaZipFile outFile(&zip);
        QuaZipNewInfo newInfo(relativePath, absolutePath);

        if (!outFile.open(QIODevice::WriteOnly, newInfo)) {
            qWarning() << "无法写入zip:" << relativePath;
            inFile.close();
            allFilesWritten = false;
            continue;
        }

        // 复制文件内容
        QByteArray data = inFile.readAll();
        if (outFile.write(data) != data.size()) {
            qWarning() << "写入文件内容失败:" << relativePath;
            allFilesWritten = false;
        } else {
            filesWritten++;
        }

        outFile.close();
        inFile.close();
    }

    zip.close();

    // 检查是否成功写入文件
    if (!allFilesWritten || filesWritten == 0) {
        qWarning() << "压缩失败，成功写入" << filesWritten << "/" << orderedFiles.size() << "个文件";
        QFile::remove(tempZip);
        return false;
    }

    // 重命名为docx
    if (QFile::exists(docxPath)) {
        QFile file(docxPath);
        if(!file.remove())
            qDebug()<<"文件删除失败："<<file.errorString()<<docxPath;
    }

    //把临时的zip重命名为保存的docx
    if (!QFile::rename(tempZip, docxPath)) {
        qWarning() << "重命名临时文件失败";
        QFile::remove(tempZip);
        return false;
    }

    qDebug() << "成功压缩docx文件，包含" << filesWritten << "个文件";
    return true;
}

// 获取目录下所有文件
QStringList PdfExporter::getAllFiles(const QString &dirPath)
{
    QStringList files;
    QDir dir(dirPath);

    QDirIterator it(dirPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        it.next();
        QString relativePath = dir.relativeFilePath(it.filePath());
        files.append(relativePath);
    }

    return files;
}

// 按docx标准顺序排序文件
QStringList PdfExporter::orderDocxFiles(const QStringList &files, const QString &baseDir)
{
    QStringList ordered;

    // 按照docx规范，[Content_Types].xml应该是第一个文件
    QStringList priorityFiles = {
        "[Content_Types].xml",
        "_rels/.rels",
        "docProps/core.xml",
        "docProps/app.xml",
        "word/document.xml",
        "word/_rels/document.xml.rels",
        "word/theme/theme1.xml",
        "word/settings.xml",
        "word/styles.xml",
        "word/fontTable.xml",
        "word/webSettings.xml"
    };

    // 添加优先级文件
    foreach (const QString& priorityFile, priorityFiles) {
        if (files.contains(priorityFile)) {
            ordered.append(priorityFile);
        }
    }

    // 添加其他文件
    foreach (const QString& file, files) {
        if (!ordered.contains(file)) {
            ordered.append(file);
        }
    }

    return ordered;
}

//替换表格,传递二维数组形式的数据不管这个表格是固定的还是不固定的都可以进行替换
//二维数组遍历，一维数组直接按照顺序赋值
bool PdfExporter::replayTableIndex(const QString &templatePath, QString tableKey, QVector<QVector<QString>> TableData)
{
    QString docXmlPath = templatePath;
    QFile file(docXmlPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法打开XML文件:" << docXmlPath;
        return false;
    }

    QDomDocument doc;
    QString errorMsg;
    int errorLine, errorColumn;
    if (!doc.setContent(&file, false, &errorMsg, &errorLine, &errorColumn)) {
        qWarning() << "XML解析错误:" << errorMsg
                   << "行:" << errorLine << "列:" << errorColumn;
        file.close();
        return false;
    }
    file.close();

    // 查找所有表格
    QDomNodeList tables = doc.elementsByTagName("w:tbl");
    if (tables.isEmpty()) {
        qWarning() << "未找到表格";
        return false;
    }

    QDomElement targetTable;
    for (int i = 0; i < tables.count(); i++) {
        QDomElement table = tables.at(i).toElement();
        QString tableText = table.text();
        if (tableText.contains(tableKey)) {
            targetTable = table;
            break;
        }
    }

    if (targetTable.isNull()) {
        qWarning() << "未找到包含占位符" << tableKey << "的表格";
        return false;
    }

    // 查找模板行（包含占位符的行）
    QDomNodeList rows = targetTable.elementsByTagName("w:tr");
    QDomElement templateRow;

    for (int i = 0; i < rows.count(); i++) {
        QDomElement row = rows.at(i).toElement();
        if (row.text().contains(tableKey)) {
            templateRow = row;
            break;
        }
    }

    if (templateRow.isNull()) {
        qWarning() << "未找到模板行";
        return false;
    }

    // 处理每一行数据
    for (int i = 0; i < TableData.size(); i++) {
        QVector<QString> rowData = TableData.at(i);
        if (rowData.isEmpty()) continue;

        // 克隆模板行
        QDomElement newRow = templateRow.cloneNode(true).toElement();

        // 查找新行中的所有文本节点
        QDomNodeList textNodes = newRow.elementsByTagName("w:t");

        // 替换文本
        for (int j = 0; j < textNodes.count() && j < rowData.size(); j++) {
            QDomElement textElem = textNodes.at(j).toElement();
            if (!textElem.isNull()) {
                QDomNode textNode = textElem.firstChild();
                if (!textNode.isNull() && textNode.isText()) {
                    // 转义XML特殊字符
                    QString escapedText = escapeXml(rowData.at(j));
                    textNode.setNodeValue(escapedText);
                }
            }
        }

        // 在模板行后插入新行
        targetTable.insertAfter(newRow, templateRow);
    }

    // 删除模板行
    targetTable.removeChild(templateRow);

    // 保存XML，确保格式正确
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << "无法写入XML文件";
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    doc.save(out, 2, QDomNode::EncodingFromTextStream);
    file.close();

    return true;
}

// XML转义函数
QString PdfExporter::escapeXml(const QString &text)
{
    QString escaped = text;
    escaped.replace("&", "&amp;");
    escaped.replace("<", "&lt;");
    escaped.replace(">", "&gt;");
    escaped.replace("\"", "&quot;");
    escaped.replace("'", "&apos;");
    return escaped;
}

// zipPath 参数传.docx文档路径	targetDir 参数传解压路径
bool PdfExporter::extractFile(const QString &zipPath, const QString &targetDir)
{
    QuaZip* zip = new QuaZip(zipPath);

    if (!zip->open(QuaZip::mdUnzip)) {
        qWarning() << "Failed to open ZIP file:" << zipPath
                   << "Error:" << zip->getZipError();
        return false;
    }

    QDir targetDirObj(targetDir);
    if (!targetDirObj.exists() && !targetDirObj.mkpath(".")) {
        qWarning() << "Failed to create target directory:" << targetDir;
        return false;
    }

    const int blockSize = 65536;  // 64KB 块大小
    QuaZipFileInfo fileInfo;
    QuaZipFile file(zip);

    for (bool more = zip->goToFirstFile(); more; more = zip->goToNextFile()) {
        if (!zip->getCurrentFileInfo(&fileInfo)) {
            qWarning() << "Failed to get file info, skipping. Error:" << zip->getZipError();
            continue;
        }

        QString absPath = targetDirObj.absoluteFilePath(fileInfo.name);
        QFileInfo fi(absPath);

        // 创建目录结构
        if (!QDir().mkpath(fi.path())) {
            qWarning() << "Failed to create path:" << fi.path();
            continue;
        }

        // 处理目录
        if (fileInfo.name.endsWith('/')) {
            QDir dir(absPath);
            if (!dir.exists() && !dir.mkpath(".")) {
                qWarning() << "Failed to create directory:" << absPath;
            }
            continue;
        }

        // 打开文件
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Failed to open ZIP entry:" << fileInfo.name
                       << "Error:" << file.getZipError();
            file.close();
            continue;
        }

        // 创建目标文件
        QFile outFile(absPath);
        if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning() << "Failed to open output file:" << absPath;
            file.close();
            continue;
        }

        // 分块读写
        char buffer[blockSize];
        qint64 totalBytes = 0;
        while (!file.atEnd()) {
            qint64 bytesRead = file.read(buffer, blockSize);
            if (bytesRead <= 0) break;

            qint64 bytesWritten = outFile.write(buffer, bytesRead);
            if (bytesWritten != bytesRead) {
                qWarning() << "Write error for:" << absPath
                           << "Expected:" << bytesRead << "Actual:" << bytesWritten;
                break;
            }
            totalBytes += bytesWritten;
        }

        outFile.close();
        file.close();

        // 设置文件权限
//        if (fileInfo.externalAttr != 0) {
//            QFile::setPermissions(absPath,
//                static_cast<QFile::Permissions>(fileInfo.externalAttr >> 16));
//        }

        // 设置文件时间戳
        QDateTime dt;
        dt.setDate(QDate(fileInfo.dateTime.date().year() + 1900,
                         fileInfo.dateTime.date().month() + 1,
                         fileInfo.dateTime.date().day()));
        dt.setTime(QTime(fileInfo.dateTime.time().hour(),
                         fileInfo.dateTime.time().minute(),
                         fileInfo.dateTime.time().second()));
//        QFile(absPath).setFileTime(dt, QFileDevice::FileModificationTime);

        if (totalBytes != fileInfo.uncompressedSize) {
            qWarning() << "Size mismatch for:" << absPath
                       << "Expected:" << fileInfo.uncompressedSize
                       << "Actual:" << totalBytes;
        }
    }

    return zip->getZipError() == UNZ_OK;
}

// zipPath 参数传目标 .docx全路径		sourceDir 参数传需要压缩的文件夹路径
bool PdfExporter::compressFile(const QString &zipPath, const QString &sourceDir)
{
    QuaZip newZip(zipPath);

    if (!newZip.open(QuaZip::mdCreate)) {
        qWarning() << "Failed to create ZIP file:" << zipPath
                   << "Error:" << newZip.getZipError();
        return false;
    }

    QDir sourceDirObj(sourceDir);

    QSet<QString> addedDirs;
    const int blockSize = 65536;  // 64KB 块大小

    // 递归处理目录
    QDirIterator it(sourceDir, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString filePath = it.next();
        QFileInfo fi(filePath);
        QString relativePath = sourceDirObj.relativeFilePath(filePath);

        // 处理目录
        if (fi.isDir()) {
            QString zipDirPath = relativePath + '/';
            if (!addedDirs.contains(zipDirPath)) {
                QuaZipFile zipFile(&newZip);
                QuaZipNewInfo newInfo(zipDirPath);
//                newInfo.externalAttr = (fi.permissions() | 0x10) << 16; // 保留权限+目录标志

                if (!zipFile.open(QIODevice::WriteOnly, newInfo)) {
                    qWarning() << "Failed to create directory entry:" << zipDirPath
                               << "Error:" << zipFile.getZipError();
                    continue;
                }
                zipFile.close();
                addedDirs.insert(zipDirPath);
            }
            continue;
        }

        // 处理文件
        QFile sourceFile(filePath);
        if (!sourceFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Failed to open source file:" << filePath;
            continue;
        }

        QuaZipFile zipFile(&newZip);
        QuaZipNewInfo newInfo(relativePath, filePath);
//        newInfo.externalAttr = fi.permissions() << 16; // 保留文件权限

        if (!zipFile.open(QIODevice::WriteOnly, newInfo)) {
            qWarning() << "Failed to open ZIP entry:" << relativePath
                       << "Error:" << zipFile.getZipError();
            sourceFile.close();
            continue;
        }

        // 分块读写提高大文件处理效率
        qint64 totalBytes = 0;
        char buffer[blockSize];
        while (!sourceFile.atEnd()) {
            qint64 bytesRead = sourceFile.read(buffer, blockSize);
            if (bytesRead <= 0) break;

            qint64 bytesWritten = zipFile.write(buffer, bytesRead);
            if (bytesWritten != bytesRead) {
                qWarning() << "Write error for:" << relativePath
                           << "Expected:" << bytesRead << "Actual:" << bytesWritten;
                break;
            }
            totalBytes += bytesWritten;
        }

        zipFile.close();
        sourceFile.close();

        if (totalBytes != fi.size()) {
            qWarning() << "File size mismatch:" << relativePath
                       << "Original:" << fi.size() << "Compressed:" << totalBytes;
        }
    }

    return newZip.getZipError() == UNZ_OK;
}

//把 docx 内部两个 XML 的“模板”标记去掉，让 LibreOffice 识别为普通文档
bool PdfExporter::fixDocxForLibreOffice(const QString &tempDir)
{
    // [Content_Types].xml - 必须处理
    QString ctFile = tempDir + "/[Content_Types].xml";
    QFile f1(ctFile);
    if (!f1.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开 [Content_Types].xml 文件";
        return false;
    }

    QByteArray ctData = f1.readAll();
    f1.close();

    QString ct = QString::fromUtf8(ctData);
    bool ctModified = false;

    // 检查并替换模板相关的Content-Type
    if (ct.contains("template", Qt::CaseInsensitive)) {
        // 把 *.template.* 改成 *.document.*
        if (ct.contains("application/vnd.ms-word.template.macroEnabledTemplate.main+xml")) {
            ct.replace("application/vnd.ms-word.template.macroEnabledTemplate.main+xml",
                       "application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml");
            ctModified = true;
        }
        if (ct.contains("application/vnd.openxmlformats-officedocument.wordprocessingml.template.main+xml")) {
            ct.replace("application/vnd.openxmlformats-officedocument.wordprocessingml.template.main+xml",
                       "application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml");
            ctModified = true;
        }

        if (ctModified) {
            if (!f1.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                qWarning() << "无法写入 [Content_Types].xml 文件";
                return false;
            }
            f1.write(ct.toUtf8());
            f1.close();
        }
    }

    // docProps/app.xml - 可选处理
    QString appFile = tempDir + "/docProps/app.xml";
    QFile f2(appFile);
    if (!f2.open(QIODevice::ReadOnly)) {
        // 这个文件可选，不存在也没关系
        return true;
    }

    QByteArray appData = f2.readAll();
    f2.close();

    QString app = QString::fromUtf8(appData);
    bool appModified = false;

    // 删掉 <Template>xxx</Template> 整行
    QRegularExpression re("<Template[^>]*>.*</Template>");
    if (app.contains(re)) {
        app.remove(re);
        appModified = true;
    }

    // 把 Application 改成普通 Word
    if (app.contains("Microsoft Macintosh Word")) {
        app.replace("Application>Microsoft Macintosh Word</Application>",
                    "Application>Word</Application>");
        appModified = true;
    }

    if (appModified) {
        if (!f2.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            qWarning() << "无法写入 docProps/app.xml 文件";
            return false;
        }
        f2.write(app.toUtf8());
        f2.close();
    }

    return true;
}

QStringList PdfExporter::getSupportedFormats()
{
    return {"pdf"};
}

bool PdfExporter::isFormatSupported(const QString& format)
{
    return format.toLower() == "pdf";
}

QMap<QString, QVariant> PdfExporter::getDefaultOptions()
{
    QMap<QString, QVariant> options;
    options["title"] = "Exported Document";
    options["author"] = "Detection System";
    options["subject"] = "Exported Data";
    options["pageSize"] = "A4";
    options["orientation"] = "Portrait";
    options["fontFamily"] = "Arial";
    options["fontSize"] = 10;
    options["showHeader"] = true;
    options["showFooter"] = true;
    options["headerText"] = "Document Header";
    options["footerText"] = "Page %page of %total";
    options["tableHeaderColor"] = QColor(240, 240, 240);
    options["tableGridColor"] = QColor(200, 200, 200);

    return options;
}

bool PdfExporter::validateOptions(const QMap<QString, QVariant>& options)
{
    // 验证页面尺寸
    if (options.contains("pageSize")) {
        QString pageSize = options["pageSize"].toString();
        QStringList validSizes = {"A0", "A1", "A2", "A3", "A4", "A5", "A6", "Letter", "Legal"};
        if (!validSizes.contains(pageSize, Qt::CaseInsensitive)) {
            qWarning() << "Invalid page size:" << pageSize;
            return false;
        }
    }

    // 验证字体大小
    if (options.contains("fontSize")) {
        bool ok;
        int fontSize = options["fontSize"].toInt(&ok);
        if (!ok || fontSize < 6 || fontSize > 72) {
            qWarning() << "Invalid font size:" << fontSize;
            return false;
        }
    }

    return true;
}

//覆盖文件
bool PdfExporter::copyFile(const QString &sourcePath, const QString &destinationPath) {
    //检查目标文件是否存在
    if (QFile::exists(destinationPath)) {
        qDebug() << "目标文件已存在，尝试删除...";
        //尝试删除目标文件
        if (!QFile::remove(destinationPath)) {
            qWarning() << "无法删除旧文件:" << destinationPath;
            return false;
        }
        qDebug() << "旧文件删除成功。";
    }

    //执行复制操作
    bool ok = QFile::copy(sourcePath, destinationPath);
    if (ok) {
        qDebug() << "文件复制成功！从" << sourcePath << "到" << destinationPath;
    } else {
        //如果复制失败，打印错误信息
        qWarning() << "文件复制失败!";
    }
    return ok;
}

//生成pdf文件
bool PdfExporter::docxToPdf(const QString &docxPath, const QString &pdfPath)
{
    QProcess process;
    QString libreOfficeCmd = "";

    // 根据自己的安装路径设置，一定要绝对路径，相对路径不行，会找不到soffice
#ifdef Q_OS_UNIX
    libreOfficeCmd = "/opt/libreoffice25.8/program/soffice"; // Linux路径
#else
//    libreOfficeCmd = "D:/soft/libreoffice/program/soffice.exe"; // Windows路径 C:\Program Files\LibreOffice
    libreOfficeCmd = "C:/Program Files/LibreOffice/program/soffice.exe"; //window路径，打包libreoffice进exe里面，libreoffice安装路径默认为 C:\Program Files\LibreOffice
        #endif

            // 测试命令：
            // /opt/libreoffice25.8/program/soffice  --headless  -convert-to  pdf  --outdir  pdf文件输出路径   xxx.docx输入路径
    QStringList args = {
        "--headless",
        "--norestore",
        "--nologo",
        "--nodefault",
        "--nofirststartwizard",
        "--convert-to", "pdf",
        "--outdir", pdfPath,
        docxPath
};

    process.start(libreOfficeCmd, args);
    if (!process.waitForFinished(15000)) {
        qWarning() << "PDF转换超时:" << process.errorString();
        return false;
    }
    qDebug()<<"llllllll"<<pdfPath<<docxPath;
    qDebug() << "退出码:" << process.exitCode();
    qDebug() << "标准输出:" << process.readAllStandardOutput();
    qDebug() << "标准错误:" << process.readAllStandardError(); // 这里最关键！
    return process.exitCode() == 0;

}
