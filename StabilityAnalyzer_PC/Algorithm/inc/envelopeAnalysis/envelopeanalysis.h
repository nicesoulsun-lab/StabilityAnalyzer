#ifndef ENVELOPEANALYSIS_H
#define ENVELOPEANALYSIS_H

#include <QObject>
#include "hilberttransform.h"
#include "../frequencyAnalysis/filter.h"

class EnvelopeAnalysis : public QObject
{
    Q_OBJECT
public:
    explicit EnvelopeAnalysis(QObject *parent = 0);
    void InputSource(QVector<float> &data,Filter &f);
    void DealProcess(QVector<float> &data,Filter &fil);
    void OutputSource(double *,int len);

    bool AddStandardWave(float *pBuffer_in, int bufferSize,double frequency,double angle,double volume);
    QList<QPointF> outDatas() const;
signals:

public slots:

private:
    QList<QPointF> m_outDatas;
    HilbertTransform HTProcess;
};

#endif // ENVELOPEANALYSIS_H
