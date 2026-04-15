#include "inc/frequencyAnalysis/filter.h"
#include <QDebug>

Filter::Filter() :
    ImpactData(nullptr),
    OutData(nullptr),
    FilterType(1),
    WinType(0),
    Scala(64),
    Freq(25600),
    lFreq(100),
    hFreq(300),
    RawData(nullptr)
{
    //init();
}

Filter::Filter(int fType, int wType, int scala, double freq, double lfreq, double hfreq) :
    ImpactData(nullptr),
    OutData(nullptr),
    FilterType(fType),
    WinType(wType),
    Scala(scala),
    Freq(25600),
    lFreq(lfreq),
    hFreq(hfreq),
    RawData(nullptr)
{
    //init();
}

Filter::Filter(const Filter &f)
{
    FilterType = f.FilterType;
    WinType = f.WinType;
    Scala = f.Scala;
    Freq = f.Freq;
    lFreq = f.lFreq;
    hFreq = f.hFreq;
    if(ImpactData != nullptr){
        delete [] ImpactData;
        ImpactData = nullptr;
    }
    if(OutData != nullptr){
        delete [] OutData;
        OutData = nullptr;
    }
    if(RawData != nullptr){
        delete [] RawData;
        RawData = nullptr;
    }
}

Filter &Filter::operator=(const Filter &f)
{
    if(this != &f){
        FilterType = f.FilterType;
        WinType = f.WinType;
        Scala = f.Scala;
        Freq = f.Freq;
        lFreq = f.lFreq;
        hFreq = f.hFreq;
        if(ImpactData != nullptr){
            delete [] ImpactData;
            ImpactData = nullptr;
        }
        if(OutData != nullptr){
            delete [] OutData;
            OutData = nullptr;
        }
        if(RawData != nullptr){
            delete [] RawData;
            RawData = nullptr;
        }
    }
    return *this;
}

Filter::~Filter()
{
    if(ImpactData != nullptr) {
        delete [] ImpactData;
        ImpactData = nullptr;
    }
    if(OutData != nullptr) {
        delete [] OutData;
        OutData = nullptr;
    }
    if(RawData != nullptr) {
        delete [] RawData;
        RawData = nullptr;
    }
}

void Filter::init()
{
    double Beta = 0.0;
    if(KAISER == WinType)
        Beta = 7.865;
    int Mid = 0, Len = 0;
    if(Scala % 2 == 0) {
        Len = Scala / 2 - 1;
        Mid = 1;
    }
    else {
        Len = Scala / 2;
        Mid = 0;
    }
    ImpactData = new double[static_cast<uint>(Scala + 1)];
    double Delay = Scala / 2.0;
    double AngFreq = 2.0 * PI * (lFreq / Freq);
    double hAngFreq = 0;
    if(BANDPASS == FilterType || BANDSTOP == FilterType)
        hAngFreq = 2.0 * PI * (hFreq / Freq);
    switch (FilterType) {
    case LOWPASS:
    {
        for (int i = 0; i <= Len; i++) {
            double s = i - Delay;
            double wVal = calcByWindowType(WinType, Scala + 1, i, Beta);
            double val = (sin(AngFreq * s) / (PI * s)) * wVal;
            ImpactData[i] = val;
            ImpactData[Scala - i] = ImpactData[i];
        }
        if(1 == Mid) {
            ImpactData[Scala / 2] = AngFreq / PI;
        }
        break;
    }
    case HIGHPASS:
    {
        for (int i = 0; i <= Len; i++) {
            double s = i - Delay;
            double wVal = calcByWindowType(WinType, Scala + 1, i, Beta);
            ImpactData[i] = (sin(PI * s) - sin(AngFreq * s)) / (PI * s) * wVal;
            ImpactData[Scala - i] = ImpactData[i];
        }
        if(1 == Mid)
            ImpactData[Scala / 2] = 1.0 - AngFreq / PI;
        break;
    }
    case BANDPASS:
    {
        for (int i = 0; i <= Len; i++) {
            double s = i - Delay;
            double wVal = calcByWindowType(WinType, Scala + 1, i, Beta);
            ImpactData[i] = (sin(hAngFreq * s) - sin(AngFreq * s)) / (PI * s) * wVal;
            ImpactData[Scala - i] = ImpactData[i];
        }
        if(1 == Mid)
            ImpactData[Scala / 2] = (hAngFreq - AngFreq) / PI;
        break;
    }
    case BANDSTOP:
    {
        for (int i = 0; i <= Len; i++) {
            double s = i - Delay;
            double wVal = calcByWindowType(WinType, Scala + 1, i, Beta);
            ImpactData[i] = (sin(AngFreq * s) + sin(PI * s) - sin(hAngFreq * s)) / (PI * s) * wVal;
            ImpactData[Scala - i] = ImpactData[i];
        }
        if(1 == Mid)
            ImpactData[Scala / 2] = (AngFreq + PI - hAngFreq) / PI;
        break;
    }
    default:
        break;
    }
}

void Filter::Convolution(const float *d, int dataLen)
{
    RawData = new float[static_cast<uint>(dataLen)];
    OutData = new double[static_cast<uint>(dataLen)];
    memcpy(RawData, d, static_cast<uint>(dataLen) * sizeof (float));
    if(dataLen > Scala) {
        for (int i = 0; i < dataLen; i++) {
            OutData[i] = 0;
            double sum = 0;
            if(i < Scala) {
                for (int j = 0; j <= i; j++) {
                    sum = sum + static_cast<double>(RawData[j]) * ImpactData[i - j];
                }
            }
            if(i >= Scala && i < dataLen) {
                int temp = Scala - 1;
                for (int j = i - Scala; j < i; j++) {
                    sum = sum + static_cast<double>(RawData[j]) * ImpactData[temp];
                    --temp;
                }
            }
            if(i > dataLen) {
                int temp = dataLen - 1;
                for (int j = 0; j < Scala; j++) {
                    sum = sum + static_cast<double>(RawData[temp]) * ImpactData[j];
                    --temp;
                }
            }
            OutData[i] = sum;
        }
    }
    else {
    }
}

double Filter::calcByWindowType(int WinType, int Scala, int Index, double Beta)
{
    double Value = 1.0;
    switch (WinType) {
    case RECTANGLE:
        Value = 1.0;
        break;
    case TUKEY:
    {
        int k = (Scala - 2) / 10;
        if(Index <= k)
            Value = 0.5 * (1.0 - cos(Index * PI / (k + 1)));
        if(Index > (Scala - k - 2))
            Value = 0.5 * (1.0 - cos((Scala - Index - 1) * PI / (k + 1)));
        break;
    }
    case TRIANGLE:
        Value = 1.0 - fabs(1.0 - 2 * Index / (Scala * 1.0));
        break;
    case HANN:
        Value = 0.5 * (1.0 - cos(2 * Index * PI / (Scala - 1)));
        break;
    case HANNING:
        Value = 0.54 - 0.36 * cos(2 * Index * PI / (Scala - 1));
        break;
    case BRACKMAN:
        Value = 0.42 - 0.5 * cos(2 * Index * PI / (Scala - 1)) + 0.08 * cos(4 * Index * PI / (Scala - 1));
        break;
    case KAISER:
        Value = calcKaisar(Scala, Index, Beta);
        break;
    default:
        break;
    }
    return Value;
}

double Filter::calcKaisar(int Scala, int Index, double Beta)
{
    double a = 2.0 * Index / (Scala - 1) - 1.0;
    return  bessel(Beta * sqrt(1.0 - a * a)) / bessel(Beta);
}

double Filter::bessel(double Beta)
{
    double d = 1.0, d2 = 0, sum = 1.0;
    for (int i = 1; i <= 25; i++) {
        d = d * Beta / 2.0 / i;
        d2 = d * d;
        sum += d2;
        if(d2 < sum * (1.0e-8))
            break;
    }
    return sum;
}

//频域分析，需要数据长度和采集频率
QVector<QPointF> Filter::test(int type, QVector<QPointF> &source)
{
    //读取数据源
//    QString path = "C:/Users/QHY/Desktop/37#43#46#机组振动导出CSV数据/时域波形数据/华能铁岭大兴风电场（北车）_37_齿轮箱_低速轴径向_25600Hz_加速度_20170506060745_1646.38RPM.csv";

//    QFileInfo fileInfo(path);
//    if(!fileInfo.exists()){ //不存在先从服务器下载
//    }
//    QFile file(path);
//    file.open(QIODevice::ReadOnly);
//    QTextStream * out = new QTextStream(&file);//文本流
//    QStringList temp = out->readAll().split(",");//一行中的单元格以，区分


    int sampleSize = source.size();//采样数据点数

    float *datain = new float[static_cast<uint>(sampleSize)];
    for(int i = 0; i< sampleSize; i++){
        datain[i] = source.at(i).y();
    }

    QVector<QPointF> points;
    int Len = 1;
    while(Len < sampleSize)
        Len *= 2;
    Len /= 2;
    valarray<complex<long double> > pp(static_cast<size_t>(Len));
    valarray<complex<long double> > f(static_cast<size_t>(Len));
    QVector<double> x(sampleSize), y(sampleSize);
    double MaxValue = 0, MinValue = 0;
    for (int i = 0; i < sampleSize; i++)
    {
        x[i] = i;
        y[i] = static_cast<double>(datain[i]);
        if(i < Len)
            pp[static_cast<size_t>(i)] = complex<long double>(static_cast<long double>(datain[i]), static_cast<long double>(0.0));

        if(y[i] > MaxValue)
            MaxValue = y[i];
        if(y[i] < MinValue)
            MinValue = y[i];
    }

    /*
    WaveIn->SetxAxisRange(0, sampleSize);
    WaveIn->SetyAxisRange(-MaxValue, MaxValue);
    WaveIn->SetData(x, y);
    */
    // 输入信号频谱
    FourierTransform(pp, f, 0, 1);
    double MaxFreqS = 0, MinFreqS = 0;
    int fftSize=Len / 2 + 1 ;
    QVector<double> xFreq(fftSize), yFreqS(fftSize);

    for (int i = 0; i < fftSize; i++)
    {
        xFreq[i] = i / (fftSize / (Freq / 2));
        //xFreq[i] = i;
        yFreqS[i] = static_cast<double>(pp[static_cast<size_t>(i)].real() / fftSize * 2);
        if(yFreqS[i] > MaxFreqS)
            MaxFreqS = yFreqS[i];
        if(yFreqS[i] < MinFreqS)
            MinFreqS = yFreqS[i];
    }

    /*
    WaveInSignal->SetxAxisRange(0, Freq / 2);
    WaveInSignal->SetyAxisRange(MinFreqS, MaxFreqS);
    WaveInSignal->SetData(xFreq, yFreqS);
    */

    //滤波器时域波形
    QVector<double> xImpuls(Scala), yImpuls(Scala);
    double MaxImpuls = 0, MinImpuls = 0;
    for (int i = 0; i < Scala; i++) {
        xImpuls[i] = i;
        yImpuls[i] = ImpactData[i];
        if(yImpuls[i] > MaxImpuls)
            MaxImpuls = yImpuls[i];
        if(yImpuls[i] < MinImpuls)
            MinImpuls = yImpuls[i];
    }

    /*
    WaveImpuls->SetxAxisRange(0, Scala);
    WaveImpuls->SetyAxisRange(MinImpuls, MaxImpuls);
    WaveImpuls->SetData(xImpuls, yImpuls);
    */
    // 输出信号时域波形
    Convolution(datain, sampleSize);
    if(OutData == nullptr)
        return points;

    valarray<complex<long double> > ppOut(static_cast<size_t>(Len));
    valarray<complex<long double> > fOut(static_cast<size_t>(Len));
    QVector<double> xOut(sampleSize), yOut(sampleSize);
    double MaxOut = 0, MinOut = 0;
    for (int i = 0; i < sampleSize; i++) {
        xOut[i] = i;
        yOut[i] = this->OutData[i];
        if(i < Len)
            ppOut[static_cast<size_t>(i)] = complex<long double>(static_cast<long double>(yOut[i]), static_cast<long double>(0.0));
        if(yOut[i] > MaxOut)
            MaxOut = yOut[i];
        if(yOut[i] < MinOut)
            MinOut = yOut[i];
    }
    /*
    WaveOut->SetxAxisRange(0, sampleSize);
    WaveOut->SetyAxisRange(MinOut, MaxOut);
    WaveOut->SetData(xOut, yOut);
    */
    //滤波器的频谱
    FourierTransform(ppOut, fOut, 0, 1);
    double MaxFreqSOut = 0, MinFreqSOut = 0;
    QVector<double> xFreqOut(fftSize), yFreqSOut(fftSize);
    for (int i = 0; i < fftSize; i++) {
        xFreqOut[i] = (Freq / 2) * i / fftSize ;
        yFreqSOut[i] = static_cast<double>(ppOut[static_cast<size_t>(i)].real() / fftSize * 2);
        if(yFreqSOut[i] > MaxFreqSOut)
            MaxFreqSOut = yFreqS[i];
        if(yFreqSOut[i] < MinFreqSOut)
            MinFreqSOut = yFreqSOut[i];
    }
    /*
    WaveSignalOut->SetxAxisRange(0, Freq / 2);
    WaveSignalOut->SetyAxisRange(MinFreqS, MaxFreqS);
    WaveSignalOut->SetData(xFreq, yFreqS);
    */
    //输出信号的频谱
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
        xF[i] = i / ((sampleSize / 2 + 1) / (Freq / 2));
        yF[i] = 20.0 * log(fabs(static_cast<double>(Impulspp[static_cast<size_t>(i)].real())));
        if(yF[i] > MaxF)
            MaxF = yF[i];
        if(yF[i] < MinF)
            MinF = yF[i];
    }
    /*
    WaveFreq->SetxAxisRange(0, Freq / 2);
    WaveFreq->SetyAxisRange(MinF, MaxF);
    WaveFreq->SetData(xF, yF);
    */
    //传递数据
    if(type==1){//原始数据时域波形，菜单栏时域分析数据接口
        for (int i = 0; i < sampleSize; i++) {
            QPointF p(x[i],y[i]);
            points.push_back(p);
        }
    }else if(type==2){//原始数据频谱（未滤波）,菜单栏的频谱分析数据接口
        for (int i = 0; i <  xFreq.size(); i++) {
            QPointF p(xFreq[i],yFreqS[i]);
            points.push_back(p);
        }

    }else if(type==3){//滤波器时域波形
        for (int i = 0; i < Scala; i++) {
            QPointF p(xImpuls[i],yImpuls[i]);
            points.push_back(p);
        }
    }else if(type==4){//滤波器频谱
        for (int i = 0; i < sampleSize / 2 + 1; i++) {
            QPointF p(xF[i],yF[i]);
            points.push_back(p);
        }
    }else if(type==5){//经过滤波器滤波后的原始数据时域波形
        for (int i = 0; i < sampleSize; i++) {
            QPointF p(xOut[i],yOut[i]);
            points.push_back(p);
        }
    }else if(type==6){//经过滤波器滤波后的原始数据频谱
        for (int i = 0; i < fftSize; i++) {
            QPointF p(xFreqOut[i],yFreqSOut[i]);
            points.push_back(p);
        }
    }
    delete[] datain;
    return points;
}


//fft
QVector<QPointF> Filter::fft(QVector<QPointF> &source)
{
    int sampleSize = source.size();//采样数据点数

    QVector<QPointF> points;
    int Len = 1;
    while(Len < sampleSize)
        Len *= 2;
    Len /= 2;
    valarray<complex<long double> > pp(static_cast<size_t>(Len));
    valarray<complex<long double> > f(static_cast<size_t>(Len));
    QVector<double> x(sampleSize), y(sampleSize);
    for (int i = 0; i < sampleSize; i++){
        x[i] = i;
        y[i] = static_cast<double>(source.at(i).y());
        if(i < Len)
            pp[static_cast<size_t>(i)] = complex<long double>(static_cast<long double>(source.at(i).y()), static_cast<long double>(0.0));
    }

    // 输入信号频谱
    FourierTransform(pp, f, 0, 1);
    int fftSize=Len / 2 + 1 ;

    for (int i = 0; i < fftSize; i++){
        double xTemp  = i / (fftSize / (Freq / 2));
        double yTemp = static_cast<double>(pp[static_cast<size_t>(i)].real() / fftSize * 2);
        points.push_back(QPointF(xTemp,yTemp));
    }

    return points;
}

QVector<QPointF> Filter::fliter(QVector<QPointF> &source)
{
    int sampleSize = source.size();//采样数据点数

    QVector<QPointF> points;

    if(sampleSize > Scala) {
        for (int i = 0; i < sampleSize; i++) {
            double sum = 0;
            if(i < Scala) {
                for (int j = 0; j <= i; j++) {
                    sum = sum + source.at(j).y() * ImpactData[i - j];
                }
            }
            if(i >= Scala) {
                int temp = Scala - 1;
                for (int j = i - Scala; j < i; j++) {
                    sum = sum + source.at(j).y() * ImpactData[temp];
                    --temp;
                }
            }
            points.push_back(QPointF(i,sum));
        }
    }

    return points;
}
