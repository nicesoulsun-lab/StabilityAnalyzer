#ifndef TIMEANALYSIS_H
#define TIMEANALYSIS_H

#include <QObject>
#include "../frequencyAnalysis/filter.h"

class TimeAnalysis : public QObject
{
    Q_OBJECT
public:
    explicit TimeAnalysis(QObject *parent = 0);

    int sampleSize=1024;
    //Filter myFilter;
    void InputSource(QVector<float> &data,bool isFiltered,Filter &f);
    void DealProcess(QVector<float> &data,Filter &fil);
    void OutputSource(QVector<float> &data);

    QList<QPointF> outDatas() const;

signals:

public slots:
private:
    QList<QPointF> m_outDatas;
};

#endif // TIMEANALYSIS_H
