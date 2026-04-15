#include "inc/timeAnalysis/timeanalysis.h"

TimeAnalysis::TimeAnalysis(QObject *parent) : QObject(parent)
{
}

void TimeAnalysis::InputSource(QVector<float> &data,bool isFiltered,Filter &f)
{
    //选择进行滤波
    int Freq;
    Freq=f.Freq;//采样频率
    int Scala;
    Scala=f.Scala;
    if(isFiltered){
        //float *data = new float[static_cast<uint>(1024)];
        DealProcess(data,f);
    }else{
        OutputSource(data);
    }
}

void TimeAnalysis::DealProcess(QVector<float> &datain,Filter &fil)
{
//    float*datain;
//    for(int i=0;i<data.size();i++){
//        datain[i]=data[i];
//    }
    int Freq;
    Freq=fil.Freq;//采样频率
    int Scala;
    Scala=fil.Scala;
    int Len = 1;
    sampleSize = datain.size();
    while(Len < sampleSize)
        Len *= 2;//进行FFT的点数
    Len /= 2;//频谱只需要正向部分
    valarray<complex<long double> > pp(static_cast<size_t>(Len));//输入信号，复数
    valarray<complex<long double> > f(static_cast<size_t>(Len));//输入信号频谱，复数
    QVector<double> x(sampleSize), y(sampleSize);
    double MaxValue = 0;
    //输入信号
    for (int i = 0; i < sampleSize; i++) {
        x[i] = i;
        y[i] = static_cast<double>(datain[i]);
        if(i < Len)
            pp[static_cast<size_t>(i)] = complex<long double>(static_cast<long double>(datain[i]), static_cast<long double>(0.0));
        if(fabs(y[i]) > MaxValue)
            MaxValue = fabs(y[i]);
    }

    /*************************输入信号频谱*************************/
    FourierTransform(pp, f, 0, 1);
    /************************************************************/

    double MaxFreqS = 0, MinFreqS = 0;

    QVector<double> xFreq(Len / 2 + 1), yFreqS(Len / 2 + 1);
    for (int i = 0; i < Len / 2 + 1; i++) {
        xFreq[i] = i / ((Len / 2.0 + 1) / (Freq / 2.0));
        yFreqS[i] = static_cast<double>(pp[static_cast<size_t>(i)].real() / (Len / 2 + 1) * 2);
        if(yFreqS[i] > MaxFreqS)
            MaxFreqS = yFreqS[i];
        if(yFreqS[i] < MinFreqS)
            MinFreqS = yFreqS[i];
    }
    QVector<double> xImpuls(Scala), yImpuls(Scala);
    double MaxImpuls = 0, MinImpuls = 0;
    for (int i = 0; i < Scala; i++) {
        xImpuls[i] = i;
        yImpuls[i] =fil.ImpactData[i];
        if(yImpuls[i] > MaxImpuls)
            MaxImpuls = yImpuls[i];
        if(yImpuls[i] < MinImpuls)
            MinImpuls = yImpuls[i];
    }
    //滤波器的频率响应
    valarray<complex<long double> > Impulspp(static_cast<size_t>(sampleSize));
    valarray<complex<long double> > Impulsf(static_cast<size_t>(sampleSize));
    for (int i = 0; i < sampleSize; i++) {
        if(i < Scala)
            Impulspp[static_cast<size_t>(i)] = complex<long double>(static_cast<long double>(yImpuls[i]), static_cast<long double>(0.0));
        else
            Impulspp[static_cast<size_t>(i)] = complex<long double>(static_cast<long double>(0.0), static_cast<long double>(0.0));
    }
    FourierTransform(Impulspp, Impulsf, 0, 1);
    double MaxF = 0, MinF = 0;
    QVector<double> xF(sampleSize / 2 + 1), yF(sampleSize / 2 + 1);

    for (int i = 0; i < sampleSize / 2 + 1; i++) {
        xF[i] = i / ((sampleSize / 2.0 + 1) / (Freq / 2.0));
        yF[i] = 20.0 * log(fabs(static_cast<double>(Impulspp[static_cast<size_t>(i)].real())));
        if(yF[i] > MaxF)
            MaxF = yF[i];
        if(yF[i] < MinF)
            MinF = yF[i];
    }
    // 输出波形
    float* dataPoint;
    if(!datain.isEmpty())
        dataPoint = &datain[0];
    fil.Convolution(dataPoint, sampleSize);

    int LenOut = 1;
    while(LenOut < sampleSize)
        LenOut *= 2;
    LenOut /= 2;//频谱图只画正向频率部分即可
    valarray<complex<long double> > ppOut(static_cast<size_t>(LenOut));
    valarray<complex<long double> > fOut(static_cast<size_t>(LenOut));
    QVector<float> xOut(sampleSize), yOut(sampleSize);
    double MaxOut = 0, MinOut = 0;
    for (int i = 0; i < sampleSize; i++) {
        xOut[i] = i;
        yOut[i] = fil.OutData[i];
        if(i < Len)
            ppOut[static_cast<size_t>(i)] = complex<long double>(static_cast<long double>(yOut[i]), static_cast<long double>(0.0));
        if(yOut[i] > MaxOut)
            MaxOut = yOut[i];
        if(yOut[i] < MinOut)
            MinOut = yOut[i];
    }
    //输出波形的频率响应
    FourierTransform(ppOut, fOut, 0, 1);
    double MaxFreqSOut = 0, MinFreqSOut = 0;
    QVector<double> xFreqOut(Len / 2 + 1), yFreqSOut(Len / 2 + 1);
    for (int i = 0; i < Len / 2 + 1; i++) {
        xFreqOut[i] = i / ((Len / 2 + 1) / (double)(Freq / 2));
        yFreqSOut[i] = static_cast<double>(ppOut[static_cast<size_t>(i)].real() / (Len / 2 + 1) * 2);
        if(yFreqSOut[i] > MaxFreqSOut)
            MaxFreqSOut = yFreqS[i];
        if(yFreqSOut[i] < MinFreqSOut)
            MinFreqSOut = yFreqSOut[i];
    }

    OutputSource(yOut);
}

void TimeAnalysis::OutputSource(QVector<float> &data)
{
    //往绘图区传递数据
    for (int i = 0; i < data.size(); i++) {
        QPointF p(i,data[i]);
        m_outDatas.push_back(p);
    }
}

QList<QPointF> TimeAnalysis::outDatas() const
{
    return m_outDatas;
}
