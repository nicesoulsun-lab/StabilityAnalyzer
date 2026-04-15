#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QDateTime>
#include "inc/ExportManager.h"
#include "inc/ExcelExporter.h"
#include "inc/PdfExporter.h"
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qDebug() << "Starting file export module tests...";

    //测试excel文件导出
    ExcelExporter exporter;
    QMap<QString, QVariant>map1;
    map1.insert("%school%","曲阜师范大学");
    map1.insert("%zsnum%","666");
    map1.insert("%jknum%","8");

    QMap<QString, QVariant>map2;
    map2.insert("%name%","张三三");
    map2.insert("%mz%","汉族");
    map2.insert("%xl%","本科");
    map2.insert("%xl%","本科");
    map2.insert("%note%","表现666，可以录取");

    exporter.exportFromTemplate("D:/test/exceltemplate/template1.xlsx","D:/test/testexcel1.xlsx",map1);
    exporter.exportFromTemplate("D:/test/exceltemplate/template2.xlsx","D:/test/testexcel2.xlsx",map2);

    QVector<QVector<QVariant>>list;
    QVector<QVariant> line{"西北大学",7,0,553,554,545,540,530,546};
    QVector<QVariant> line2{"西安交通大学",7,0,553,554,545,540,530,546};
    QVector<QVariant> line3{"曲阜师范大学",7,0,553,554,545,540,530,546};
    list.append(line);
    list.append(line2);
    list.append(line3);

//    exporter.exportFile("D:/test/exceltemplate/template1.xlsx","D:/test/testexcel1.xlsx","sheet1",11,list);
    qDebug() << "All tests completed!";

    //-------------测试下pdf文件
    QMap<QString, QVariant>map3;
    map3.insert("{REPORT_TITLE}", "测试");
    map3.insert("{REPORT_DATE}", "2026-01-16");
    map3.insert("{AUTHOR}", "哆啦A梦");

    QMap<QString,QString>imageMap;
    imageMap.insert("image1.png","D:/test/pic2.png");
    imageMap.insert("image2.png","D:/test/pic.png");

    //表格数据，以字典的形式传递，因为一个word里面可能会有多个表格，键是每个表格的键，值是每个表格的数据，使用的二维数组，因为表格可能不固定，字段也可能不固定
    QMap<QString,QVector<QVector<QString>>>tableMap;
    QVector<QVector<QString>>tableData1;
    QVector<QString> btData1; //标题行
    btData1.append("姓名");
    btData1.append("性别");
    btData1.append("学校");
    btData1.append("专业");
    QVector<QString>data1;
    data1.append("火影忍者");
    data1.append("男");
    data1.append("学校1");
    data1.append("软件工程");
    tableData1.append(data1);
    QVector<QString>data11;
    data11.append("小新新");
    data11.append("男");
    data11.append("学校222");
    data11.append("软件工程");
    tableData1.append(data11);
    tableData1.append(btData1);
    tableMap.insert("${value1}",tableData1);

    QVector<QVector<QString>>tableData2;
    QVector<QString> btData2; //标题行
    btData2.append("姓名");
    btData2.append("性别");
    btData2.append("学校");

    QVector<QString>data2;
    data2.append("小花啦啦啦啦啦啦啦啦啦啦啦啦啦啊啦啦啦");
    data2.append("男");
    data2.append("学校111111");
    tableData2.append(data2);
    tableData2.append(btData2);
    tableMap.insert("${name}",tableData2);

    PdfExporter pdfExporter;
//    bool sucess = pdfExporter.exportFromTemplate("D:/test/wordtemplate/template1.docx","D:/test/testWord.docx","${value1}",tableData,map3,imageMap);

    bool sucess = pdfExporter.exportFromTemplate("D:/test/wordtemplate/template1.docx","D:/test/testWord.docx",tableMap,map3,imageMap);
    qDebug()<<"根据模板导出pdf文件：" << sucess;
    return 0;
}
