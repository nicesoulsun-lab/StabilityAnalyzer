#ifndef HILBERTTRANSFORM_H
#define HILBERTTRANSFORM_H

#include <QObject>
#include <QDebug>
#include <QPointF>
#include "../frequencyAnalysis/fouriertransform.h"
#define pi  3.1415926

class HilbertTransform : public QObject
{
    Q_OBJECT
public:
    explicit HilbertTransform(QObject *parent = nullptr);
    virtual ~HilbertTransform();
    double  blackman(long n,long i);
    bool Transform(float *pX,float *pY,long size);
    long Create(long length=127,long size=1024);
    void Convolution(float *datain,int dataLen);//卷积，原始信号与1/（PI*t）卷积，进行Hilbert变换
    void DataEnvelopmentAnalysis (float *datain,int dataLen);
    float *m_pFilterBuf;
    long m_FilterBufSize;//2的N次方
    long m_FilterLength;//建议单数.
    double *OutHTData;//Hilbert变换后的数据，后续用于生成解析信号，供包络解析使用
    double *OutEnvelopmentData;//包络数据

    int m_type;//窗口型号，0无,1矩形窗,2图基窗,3三角窗,4汉宁窗,5海明窗,6布拉克曼窗,7凯泽窗.

    float *ldata;
    int hilbert(float *data , float *filterdata, int dn);
    int firwin_e(int n,int band,int fl,int fh,int fs,int wn,int *data,int *result,int dn);
    int conv(int *h,int *data,int *result,int hn,int dn);


    int fft(float *data,complex <double> *a,int L);
    int ifft(complex <double> *a,float *data,int L);
    double window(int type,int n,int i,double beta);
    double kaiser(int i,int n,double beta);
    double bessel0(double x);
    int conv_f(double *h,int *data,int *result,int hn,int dn);
    int fft_f(double *data,complex <double> *a,int L);

    void hilbtf(double* x,int n);
    void fht(double* x,int n);
    void fht1(double *Data,int n);
    double *outdata;


    QVector<QPointF> hilbtf(QVector<QPointF> &source);

signals:

};

#endif // HILBERTTRANSFORM_H
