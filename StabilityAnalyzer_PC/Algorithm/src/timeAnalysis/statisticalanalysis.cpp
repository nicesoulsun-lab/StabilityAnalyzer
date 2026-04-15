#include "inc/timeAnalysis/statisticalanalysis.h"

StatisticalAnalysis::StatisticalAnalysis(QObject *parent) : QObject(parent)
{

}
//最大值计算
float StatisticalAnalysis::Max(float* fData)
{
    float max=fData[0];
    for(int i=1;i<sizeof(fData);i++)
    {
        max=qMax(fData[i],max);
    }
    return max;
}
//最小值计算
float StatisticalAnalysis::Min(float* fData)
{
    float min=fData[0];
    for(int i=1;i<sizeof(fData);i++)
    {
        min=qMin(fData[i],min);
    }
    return min;
}
//平均值计算
float StatisticalAnalysis::Average(float* fData)
{
    float Avrg=0;
    double Sum=0;
    for(int i=0;i<sizeof(fData);i++)
    {
        Sum=Sum+qAbs(fData[i]);
    }
    Avrg=(float)(Sum/(float)sizeof(fData));
    return Avrg;
}
//有效值计算
float StatisticalAnalysis::RMS(float* fData)
{
    float RMS=0;
    double Sum=0;
    for(int i=0;i<sizeof(fData);i++)
    {
        Sum=(float)(Sum+qPow(fData[i],2));
    }
    RMS=(float)qPow((Sum/(float)sizeof(fData)),0.5);
    return RMS;
}
//峭度计算
float StatisticalAnalysis::Kurtosis_Factor(float* fData,float RMS)
{
    float Kurtosis_Factor;
    if(RMS !=0)
    {
        int DataLength =sizeof(fData);
        float* x4 = new float[DataLength];
        float sumx4 = 0;
        for (int i=0;i<DataLength;i++)
        {
            x4[i] = (float)qPow(fData[i],4);
            sumx4 = sumx4 + x4[i];
        }
        float Kurtosis = (float) (sumx4 / (float)sizeof(fData));
        Kurtosis_Factor = (float) (Kurtosis/qPow(RMS,4));
        delete[] x4;
    }else
    {
        Kurtosis_Factor = 0;
    }
    return Kurtosis_Factor;
}
//峰值因子计算
float StatisticalAnalysis::Crest_Factor(float Peak, float RMS)
{
    float C_Factor;
    if(RMS!=0)
    {
        C_Factor = Peak/RMS;
    }else
    {
        C_Factor = 0;
    }
    return C_Factor;
}

