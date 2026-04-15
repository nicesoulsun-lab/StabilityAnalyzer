#ifndef FOURIERTRANSFORM_H
#define FOURIERTRANSFORM_H

#include <QObject>
#include "../common/common.h"

class FourierTransform : public QObject
{
    Q_OBJECT
public:
    explicit FourierTransform(QObject *parent = nullptr);
    static void rfft(float *x,long n);
    static bool ShortTimeDFT(float *pS,float *pD,long size,double freq);
    static bool MyDFT(float *x,float n,double freq,double &Re,double &Im);
    static bool MyFFT(float *x, float *y, float *a, float *b, int n, bool sign);
    static bool MyDFT(float *x, float *y, float *a, float *b, int n, bool sign,double bf,double ef);
    static bool FFT(float *x, float *y, int n, bool sign);
    static bool DFT(float *x,float *y,float *a,float *b,int n,bool sign);
    int Power2(int n);
    void Inverse(float *a[2],int L );
signals:

};

#endif // FOURIERTRANSFORM_H
