#ifndef STRUCT_H
#define STRUCT_H
#include <QObject>
/**
 * 这个文件里面主要是实现一些通用的结构体，避免头文件之间多层嵌套的问题
**/
struct SerialConfig //串口通信结构体
{
    QString portName;
    int baudRate = 9600;
    int dataBits = 8;
    QString parity = "NoParity";
    int stopBits = 1;
    QString flowControl = "NoFlowControl";
};
#endif // STRUCT_H
