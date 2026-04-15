#ifndef FILTER_H
#define FILTER_H
#include <math.h>
#include <qvector.h>
#include "inc/common/common.h"
#include "src/common/transform.cpp"
#include <QPointF>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QVector>

class Filter
{
public:
    Filter();
    /* fType:过滤器类型
     * wType:窗口类型
     * scala:阶数
     * freq:频率
     * lfreq:下限频率
     * hfreq:上限频率
     */
    Filter(int fType, int wType, int scala, double freq, double lfreq, double hfreq);
    Filter(const Filter &f);
    Filter& operator=(const Filter &f);
    ~Filter();

    void init();

    // 获得滤波器系数后,对输入信号和系数做卷积积分,得到滤波器输出,d:原始数据，dataLen:数据长度
    void Convolution(const float *d, int dataLen);
    // 滤波器类型
    enum FILTERTYPE{
        LOWPASS,        // 低通
        HIGHPASS,       // 高通
        BANDPASS,       // 带通
        BANDSTOP        // 带阻
    };
    // 窗口类型
    enum WINDOWTYPE {
        RECTANGLE,      // 矩形
        TUKEY,          // 图基
        TRIANGLE,       // 三角
        HANN,           // 汉宁
        HANNING,        // 汉明
        BRACKMAN,       // 布拉克曼
        KAISER          // 凯塞
    };
protected:
    double calcByWindowType(int WinType, int Scala, int Index, double Beta);
    double calcKaisar(int Scala, int Index, double Beta);
    double bessel(double Beta);

public:
    double *ImpactData;     // 冲击响应数据
    double *OutData;

    QVector<QPointF> test(int type, QVector<QPointF> &source);//测试频域分析

    /* 时域转频域 */
    QVector<QPointF> fft(QVector<QPointF> &source);

    /* 滤波 */
    QVector<QPointF> fliter(QVector<QPointF> &source);

    int FilterType;
    int WinType;
    int Scala;
    double Freq;
    double lFreq;
    double hFreq;
    float *RawData;
};

#endif // FILTER_H
