#ifndef FREQUENCYANALYSIS_H
#define FREQUENCYANALYSIS_H

#include <QObject>
#include <QDebug>
#include "filter.h"

class FrequencyAnalysis : public QObject
{
    Q_OBJECT
public:
    explicit FrequencyAnalysis(QObject *parent = 0);

    void InputSource(QVector<float> &data,Filter &f);
    void DealProcess(QVector<float> &data,Filter &fil);
    void OutputSource(QVector<float> &datax,QVector<float> &datay);

    QVector<QPointF> outDatas() const;
signals:

public slots:

private:
    QVector<QPointF> m_outDatas;
};

#endif // FREQUENCYANALYSIS_H
