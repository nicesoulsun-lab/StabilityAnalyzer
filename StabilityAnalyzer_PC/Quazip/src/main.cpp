#include "QuaZipWidget.h"
#include <QApplication>
#include "JlCompress.h"
#include <QDebug>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QuaZipWidget w;
    w.show();

    //测试
    bool sucess = JlCompress::compressFile("d:/test/test.zip","d:/test/config1.json");
    if(sucess)
        qDebug()<<"压缩文件成功";
    else
        qDebug()<<"压缩文件失败";
    return a.exec();
}
