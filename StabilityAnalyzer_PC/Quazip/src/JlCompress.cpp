/*
Copyright (C) 2010 Roberto Pompermaier
Copyright (C) 2005-2014 Sergey A. Tachenov

This file is part of QuaZip.

QuaZip is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

QuaZip is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with QuaZip.  If not, see <http://www.gnu.org/licenses/>.

See COPYING file for the full LGPL text.

Original ZIP package is copyrighted by Gilles Vollant and contributors,
see quazip/(un)zip.h files for details. Basically it's the zlib license.
*/

#include "JlCompress.h"

/// 复制数据从一个IO设备到另一个IO设备
/**
 * 从输入设备读取数据并写入到输出设备
 * \param inFile 输入文件设备
 * \param outFile 输出文件设备
 * \return 如果复制成功返回true，否则返回false
 */
bool JlCompress::copyData(QIODevice &inFile, QIODevice &outFile)
{
    while (!inFile.atEnd()) {
        char buf[4096];
        qint64 readLen = inFile.read(buf, 4096);
        if (readLen <= 0)
            return false;
        if (outFile.write(buf, readLen) != readLen)
            return false;
    }
    return true;
}

/// 压缩单个文件到已打开的ZIP文件中
/**
 * 将单个文件添加到已打开的ZIP文件中
 * \param zip 已打开的ZIP文件对象指针
 * \param fileName 源文件的完整路径
 * \param fileDest ZIP文件中的目标文件名
 * \return 如果压缩成功返回true，否则返回false
 */
bool JlCompress::compressFile(QuaZip* zip, QString fileName, QString fileDest) {
    // 检查ZIP文件是否已正确打开
    if (!zip) return false;
    if (zip->getMode()!=QuaZip::mdCreate &&
        zip->getMode()!=QuaZip::mdAppend &&
        zip->getMode()!=QuaZip::mdAdd) return false;

    // 打开ZIP文件中的目标文件
    QuaZipFile outFile(zip);
    if(!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(fileDest, fileName))) return false;

    QFileInfo input(fileName);
    if (quazip_is_symlink(input)) {
        // 处理符号链接：符号链接本质上是一个字节数组
        // 在Linux系统中，UTF-8编码是普遍使用的
        QString path = quazip_symlink_target(input);
        QString relativePath = input.dir().relativeFilePath(path);
        outFile.write(QFile::encodeName(relativePath));
    } else {
        // 处理普通文件：打开源文件并复制数据
        QFile inFile;
        inFile.setFileName(fileName);
        if (!inFile.open(QIODevice::ReadOnly))
            return false;
        if (!copyData(inFile, outFile) || outFile.getZipError()!=UNZ_OK)
            return false;
        inFile.close();
    }

    // 关闭文件并检查错误
    outFile.close();
    if (outFile.getZipError()!=UNZ_OK) return false;

    return true;
}

/// 压缩子目录到已打开的ZIP文件中
/**
 * 递归压缩子目录及其内容到已打开的ZIP文件中
 * \param zip 已打开的ZIP文件对象指针
 * \param dir 要压缩的目录路径
 * \param origDir 原始目录路径，用于计算相对路径
 * \param recursive 是否递归压缩子目录
 * \param filters 文件过滤器
 * \return 如果压缩成功返回true，否则返回false
 */
bool JlCompress::compressSubDir(QuaZip* zip, QString dir, QString origDir, bool recursive, QDir::Filters filters) {
    // 检查ZIP文件是否已正确打开
    if (!zip) return false;
    if (zip->getMode()!=QuaZip::mdCreate &&
        zip->getMode()!=QuaZip::mdAppend &&
        zip->getMode()!=QuaZip::mdAdd) return false;

    // Controllo la cartella
    QDir directory(dir);
    if (!directory.exists()) return false;

    QDir origDirectory(origDir);
	if (dir != origDir) {
		QuaZipFile dirZipFile(zip);
		if (!dirZipFile.open(QIODevice::WriteOnly,
            QuaZipNewInfo(origDirectory.relativeFilePath(dir) + QLatin1String("/"), dir), nullptr, 0, 0)) {
				return false;
		}
		dirZipFile.close();
	}


    // Se comprimo anche le sotto cartelle
    if (recursive) {
        // Per ogni sotto cartella
        QFileInfoList files = directory.entryInfoList(QDir::AllDirs|QDir::NoDotAndDotDot|filters);
        for (int index = 0; index < files.size(); ++index ) {
            const QFileInfo & file( files.at( index ) );
            if (!file.isDir()) // needed for Qt < 4.7 because it doesn't understand AllDirs
                continue;
            // Comprimo la sotto cartella
            if(!compressSubDir(zip,file.absoluteFilePath(),origDir,recursive,filters)) return false;
        }
    }

    // Per ogni file nella cartella
    QFileInfoList files = directory.entryInfoList(QDir::Files|filters);
    for (int index = 0; index < files.size(); ++index ) {
        const QFileInfo & file( files.at( index ) );
        // Se non e un file o e il file compresso che sto creando
        if(!file.isFile()||file.absoluteFilePath()==zip->getZipName()) continue;

        // Creo il nome relativo da usare all'interno del file compresso
        QString filename = origDirectory.relativeFilePath(file.absoluteFilePath());

        // Comprimo il file
        if (!compressFile(zip,file.absoluteFilePath(),filename)) return false;
    }

    return true;
}

/// 从已打开的ZIP文件中提取单个文件
/**
 * 从已打开的ZIP文件中提取指定的文件到目标路径
 * \param zip 已打开的ZIP文件对象指针
 * \param fileName ZIP文件中的文件名
 * \param fileDest 目标文件路径
 * \return 如果提取成功返回true，否则返回false
 */
bool JlCompress::extractFile(QuaZip* zip, QString fileName, QString fileDest) {
    // 检查ZIP文件是否已正确打开为解压模式
    if (!zip) return false;
    if (zip->getMode()!=QuaZip::mdUnzip) return false;

    // Apro il file compresso
    if (!fileName.isEmpty())
        zip->setCurrentFile(fileName);
    QuaZipFile inFile(zip);
    if(!inFile.open(QIODevice::ReadOnly) || inFile.getZipError()!=UNZ_OK) return false;

    // Controllo esistenza cartella file risultato
    QDir curDir;
    if (fileDest.endsWith(QLatin1String("/"))) {
        if (!curDir.mkpath(fileDest)) {
            return false;
        }
    } else {
        if (!curDir.mkpath(QFileInfo(fileDest).absolutePath())) {
            return false;
        }
    }

    QuaZipFileInfo64 info;
    if (!zip->getCurrentFileInfo(&info))
        return false;

    QFile::Permissions srcPerm = info.getPermissions();
    if (fileDest.endsWith(QLatin1String("/")) && QFileInfo(fileDest).isDir()) {
        if (srcPerm != 0) {
            QFile(fileDest).setPermissions(srcPerm);
        }
        return true;
    }

    if (info.isSymbolicLink()) {
        QString target = QFile::decodeName(inFile.readAll());
        if (!QFile::link(target, fileDest))
            return false;
        return true;
    }

    // Apro il file risultato
    QFile outFile;
    outFile.setFileName(fileDest);
    if(!outFile.open(QIODevice::WriteOnly)) return false;

    // Copio i dati
    if (!copyData(inFile, outFile) || inFile.getZipError()!=UNZ_OK) {
        outFile.close();
        removeFile(QStringList(fileDest));
        return false;
    }
    outFile.close();

    // Chiudo i file
    inFile.close();
    if (inFile.getZipError()!=UNZ_OK) {
        removeFile(QStringList(fileDest));
        return false;
    }

    if (srcPerm != 0) {
        outFile.setPermissions(srcPerm);
    }
    return true;
}

/// 删除文件列表中的文件
/**
 * 删除指定的文件列表中的所有文件
 * \param listFile 要删除的文件路径列表
 * \return 如果所有文件都成功删除返回true，否则返回false
 */
bool JlCompress::removeFile(QStringList listFile) {
    bool ret = true;
    // 遍历文件列表并删除每个文件
    for (int i=0; i<listFile.count(); i++) {
        ret = ret && QFile::remove(listFile.at(i));
    }
    return ret;
}

/// 压缩单个文件到新的ZIP文件中
/**
 * 创建新的ZIP文件并将单个文件压缩到其中
 * \param fileCompressed 目标ZIP文件路径
 * \param file 要压缩的源文件路径
 * \return 如果压缩成功返回true，否则返回false
 */
bool JlCompress::compressFile(QString fileCompressed, QString file) {
    // 创建ZIP文件
    QuaZip zip(fileCompressed);
    QDir().mkpath(QFileInfo(fileCompressed).absolutePath());
    if(!zip.open(QuaZip::mdCreate)) {
        QFile::remove(fileCompressed);
        return false;
    }

    // 添加文件到ZIP
    if (!compressFile(&zip,file,QFileInfo(file).fileName())) {
        QFile::remove(fileCompressed);
        return false;
    }

    // 关闭ZIP文件并检查错误
    zip.close();
    if(zip.getZipError()!=0) {
        QFile::remove(fileCompressed);
        return false;
    }

    return true;
}

/// 压缩多个文件到新的ZIP文件中
/**
 * 创建新的ZIP文件并将多个文件压缩到其中
 * \param fileCompressed 目标ZIP文件路径
 * \param files 要压缩的源文件路径列表
 * \return 如果压缩成功返回true，否则返回false
 */
bool JlCompress::compressFiles(QString fileCompressed, QStringList files) {
    // 创建ZIP文件
    QuaZip zip(fileCompressed);
    QDir().mkpath(QFileInfo(fileCompressed).absolutePath());
    if(!zip.open(QuaZip::mdCreate)) {
        QFile::remove(fileCompressed);
        return false;
    }

    // 压缩文件列表中的每个文件
    QFileInfo info;
    for (int index = 0; index < files.size(); ++index ) {
        const QString & file( files.at( index ) );
        info.setFile(file);
        if (!info.exists() || !compressFile(&zip,file,info.fileName())) {
            QFile::remove(fileCompressed);
            return false;
        }
    }

    // 关闭ZIP文件并检查错误
    zip.close();
    if(zip.getZipError()!=0) {
        QFile::remove(fileCompressed);
        return false;
    }

    return true;
}

/// 压缩整个目录到新的ZIP文件中（简化版本）
/**
 * 压缩整个目录到新的ZIP文件中，使用默认过滤器
 * \param fileCompressed 目标ZIP文件路径
 * \param dir 要压缩的目录路径
 * \param recursive 是否递归压缩子目录
 * \return 如果压缩成功返回true，否则返回false
 */
bool JlCompress::compressDir(QString fileCompressed, QString dir, bool recursive) {
    return compressDir(fileCompressed, dir, recursive, QDir::Filters());
}

/// 压缩整个目录到新的ZIP文件中
/**
 * 创建新的ZIP文件并将整个目录压缩到其中
 * \param fileCompressed 目标ZIP文件路径
 * \param dir 要压缩的目录路径
 * \param recursive 是否递归压缩子目录
 * \param filters 文件过滤器
 * \return 如果压缩成功返回true，否则返回false
 */
bool JlCompress::compressDir(QString fileCompressed, QString dir,
                             bool recursive, QDir::Filters filters)
{
    // 创建ZIP文件
    QuaZip zip(fileCompressed);
    QDir().mkpath(QFileInfo(fileCompressed).absolutePath());
    if(!zip.open(QuaZip::mdCreate)) {
        QFile::remove(fileCompressed);
        return false;
    }

    // 添加文件和子目录到ZIP
    if (!compressSubDir(&zip,dir,dir,recursive, filters)) {
        QFile::remove(fileCompressed);
        return false;
    }

    // 关闭ZIP文件并检查错误
    zip.close();
    if(zip.getZipError()!=0) {
        QFile::remove(fileCompressed);
        return false;
    }

    return true;
}

/// 从ZIP文件中提取单个文件
/**
 * 从指定的ZIP文件中提取单个文件到目标路径
 * \param fileCompressed ZIP文件路径
 * \param fileName ZIP文件中的文件名
 * \param fileDest 目标文件路径
 * \return 提取的文件绝对路径，如果失败返回空字符串
 */
QString JlCompress::extractFile(QString fileCompressed, QString fileName, QString fileDest) {
    // 打开ZIP文件
    QuaZip zip(fileCompressed);
    return extractFile(zip, fileName, fileDest);
}

/// 从已打开的ZIP文件中提取单个文件
/**
 * 从已打开的ZIP文件中提取单个文件到目标路径
 * \param zip 已打开的ZIP文件对象
 * \param fileName ZIP文件中的文件名
 * \param fileDest 目标文件路径
 * \return 提取的文件绝对路径，如果失败返回空字符串
 */
QString JlCompress::extractFile(QuaZip &zip, QString fileName, QString fileDest)
{
    if(!zip.open(QuaZip::mdUnzip)) {
        return QString();
    }

    // 提取文件
    if (fileDest.isEmpty())
        fileDest = fileName;
    if (!extractFile(&zip,fileName,fileDest)) {
        return QString();
    }

    // 关闭ZIP文件并检查错误
    zip.close();
    if(zip.getZipError()!=0) {
        removeFile(QStringList(fileDest));
        return QString();
    }
    return QFileInfo(fileDest).absoluteFilePath();
}

/// 从ZIP文件中提取多个文件
/**
 * 从指定的ZIP文件中提取多个文件到目标目录
 * \param fileCompressed ZIP文件路径
 * \param files 要提取的文件名列表
 * \param dir 目标目录路径
 * \return 提取的文件绝对路径列表，如果失败返回空列表
 */
QStringList JlCompress::extractFiles(QString fileCompressed, QStringList files, QString dir) {
    // 创建ZIP文件对象
    QuaZip zip(fileCompressed);
    return extractFiles(zip, files, dir);
}

/// 从已打开的ZIP文件中提取多个文件
/**
 * 从已打开的ZIP文件中提取多个文件到目标目录
 * \param zip 已打开的ZIP文件对象
 * \param files 要提取的文件名列表
 * \param dir 目标目录路径
 * \return 提取的文件绝对路径列表，如果失败返回空列表
 */
QStringList JlCompress::extractFiles(QuaZip &zip, const QStringList &files, const QString &dir)
{
    if(!zip.open(QuaZip::mdUnzip)) {
        return QStringList();
    }

    // 提取文件列表中的每个文件
    QStringList extracted;
    for (int i=0; i<files.count(); i++) {
        QString absPath = QDir(dir).absoluteFilePath(files.at(i));
        if (!extractFile(&zip, files.at(i), absPath)) {
            removeFile(extracted);
            return QStringList();
        }
        extracted.append(absPath);
    }

    // 关闭ZIP文件并检查错误
    zip.close();
    if(zip.getZipError()!=0) {
        removeFile(extracted);
        return QStringList();
    }

    return extracted;
}

/// 从ZIP文件中提取整个目录（支持文件名编码）
/**
 * 从指定的ZIP文件中提取整个目录到目标路径，支持文件名编码
 * \param fileCompressed ZIP文件路径
 * \param fileNameCodec 文件名编码器
 * \param dir 目标目录路径
 * \return 提取的文件绝对路径列表，如果失败返回空列表
 */
QStringList JlCompress::extractDir(QString fileCompressed, QTextCodec* fileNameCodec, QString dir) {
    // 打开ZIP文件并设置文件名编码
    QuaZip zip(fileCompressed);
    if (fileNameCodec)
        zip.setFileNameCodec(fileNameCodec);
    return extractDir(zip, dir);
}

/// 从ZIP文件中提取整个目录
/**
 * 从指定的ZIP文件中提取整个目录到目标路径
 * \param fileCompressed ZIP文件路径
 * \param dir 目标目录路径
 * \return 提取的文件绝对路径列表，如果失败返回空列表
 */
QStringList JlCompress::extractDir(QString fileCompressed, QString dir) {
    return extractDir(fileCompressed, nullptr, dir);
}

/// 从已打开的ZIP文件中提取整个目录
/**
 * 从已打开的ZIP文件中提取整个目录到目标路径
 * \param zip 已打开的ZIP文件对象
 * \param dir 目标目录路径
 * \return 提取的文件绝对路径列表，如果失败返回空列表
 */
QStringList JlCompress::extractDir(QuaZip &zip, const QString &dir)
{
    if(!zip.open(QuaZip::mdUnzip)) {
        return QStringList();
    }
    QString cleanDir = QDir::cleanPath(dir);
    QDir directory(cleanDir);
    QString absCleanDir = directory.absolutePath();
    if (!absCleanDir.endsWith('/')) // 只有在文件系统根目录时才以/结尾
        absCleanDir += '/';
    QStringList extracted;
    if (!zip.goToFirstFile()) {
        return QStringList();
    }
    do {
        QString name = zip.getCurrentFileName();
        QString absFilePath = directory.absoluteFilePath(name);
        QString absCleanPath = QDir::cleanPath(absFilePath);
        if (!absCleanPath.startsWith(absCleanDir))
            continue;
        if (!extractFile(&zip, QLatin1String(""), absFilePath)) {
            removeFile(extracted);
            return QStringList();
        }
        extracted.append(absFilePath);
    } while (zip.goToNextFile());

    // 关闭ZIP文件并检查错误
    zip.close();
    if(zip.getZipError()!=0) {
        removeFile(extracted);
        return QStringList();
    }

    return extracted;
}

/// 获取ZIP文件中的文件列表
/**
 * 获取指定ZIP文件中的所有文件和目录列表
 * \param fileCompressed ZIP文件路径
 * \return ZIP文件中的文件列表，如果失败返回空列表
 */
QStringList JlCompress::getFileList(QString fileCompressed) {
    // 打开ZIP文件
    QuaZip* zip = new QuaZip(QFileInfo(fileCompressed).absoluteFilePath());
    return getFileList(zip);
}

/// 获取已打开的ZIP文件中的文件列表
/**
 * 获取已打开的ZIP文件中的所有文件和目录列表
 * \param zip 已打开的ZIP文件对象指针
 * \return ZIP文件中的文件列表，如果失败返回空列表
 */
QStringList JlCompress::getFileList(QuaZip *zip)
{
    if(!zip->open(QuaZip::mdUnzip)) {
        delete zip;
        return QStringList();
    }

    // 提取文件名
    QStringList lst;
    QuaZipFileInfo64 info;
    for(bool more=zip->goToFirstFile(); more; more=zip->goToNextFile()) {
      if(!zip->getCurrentFileInfo(&info)) {
          delete zip;
          return QStringList();
      }
      lst << info.name;
      //info.name.toLocal8Bit().constData()
    }

    // 关闭ZIP文件并检查错误
    zip->close();
    if(zip->getZipError()!=0) {
        delete zip;
        return QStringList();
    }
    delete zip;
    return lst;
}

/// 从IO设备中提取整个目录（支持文件名编码）
/**
 * 从指定的IO设备中提取整个目录到目标路径，支持文件名编码
 * \param ioDevice IO设备指针
 * \param fileNameCodec 文件名编码器
 * \param dir 目标目录路径
 * \return 提取的文件绝对路径列表，如果失败返回空列表
 */
QStringList JlCompress::extractDir(QIODevice* ioDevice, QTextCodec* fileNameCodec, QString dir)
{
    QuaZip zip(ioDevice);
    if (fileNameCodec)
        zip.setFileNameCodec(fileNameCodec);
    return extractDir(zip, dir);
}

/// 从IO设备中提取整个目录
/**
 * 从指定的IO设备中提取整个目录到目标路径
 * \param ioDevice IO设备指针
 * \param dir 目标目录路径
 * \return 提取的文件绝对路径列表，如果失败返回空列表
 */
QStringList JlCompress::extractDir(QIODevice *ioDevice, QString dir)
{
    return extractDir(ioDevice, nullptr, dir);
}

/// 获取IO设备中的文件列表
/**
 * 获取指定IO设备中的ZIP文件列表
 * \param ioDevice IO设备指针
 * \return ZIP文件中的文件列表，如果失败返回空列表
 */
QStringList JlCompress::getFileList(QIODevice *ioDevice)
{
    QuaZip *zip = new QuaZip(ioDevice);
    return getFileList(zip);
}

/// 从IO设备中提取单个文件
/**
 * 从指定的IO设备中提取单个文件到目标路径
 * \param ioDevice IO设备指针
 * \param fileName ZIP文件中的文件名
 * \param fileDest 目标文件路径
 * \return 提取的文件绝对路径，如果失败返回空字符串
 */
QString JlCompress::extractFile(QIODevice *ioDevice, QString fileName, QString fileDest)
{
    QuaZip zip(ioDevice);
    return extractFile(zip, fileName, fileDest);
}

/// 从IO设备中提取多个文件
/**
 * 从指定的IO设备中提取多个文件到目标目录
 * \param ioDevice IO设备指针
 * \param files 要提取的文件名列表
 * \param dir 目标目录路径
 * \return 提取的文件绝对路径列表，如果失败返回空列表
 */
QStringList JlCompress::extractFiles(QIODevice *ioDevice, QStringList files, QString dir)
{
    QuaZip zip(ioDevice);
    return extractFiles(zip, files, dir);
} 
