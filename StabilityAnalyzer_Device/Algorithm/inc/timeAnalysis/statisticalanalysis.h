#ifndef STATISTICALANALYSIS_H
#define STATISTICALANALYSIS_H

#include <QObject>
#include <QVector>
#include "math.h"
#include <QtMath>

class StatisticalAnalysis : public QObject
{
    Q_OBJECT
public:
    explicit StatisticalAnalysis(QObject *parent = 0);
    float Max(float* fData);
    static float Min(float* fData);
    static float Average(float* fData);
    static float RMS(float* fData);
    static float Kurtosis_Factor(float* fData,float RMS);
    static float Crest_Factor(float Peak,float RMS);
signals:

public slots:
};

#endif // STATISTICALANALYSIS_H
