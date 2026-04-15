#ifndef DATAPROCESSOR_H
#define DATAPROCESSOR_H

#include <QPoint>
#include <QVector>
#include <QUuid>
#include <QThread>
#include <iostream>
#include <fstream>
#include <QDebug>
#include <QFile>
#include <QTimer>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QJsonObject>
#include <math.h>
#include <QJsonArray>
#include <QDateTime>
#include "LoopVector.h"

class DataProcessor : public QObject
{
    Q_OBJECT
   
public:
    explicit DataProcessor(QObject *parent = 0);
    ~DataProcessor();

    QVector<QPointF> readPoint(qreal lower, qreal upper);

    int getPointByX(qreal x);

    QVector<QPointF> read(qreal lower, qreal upper);

    void beginInitSource(LoopVector<QPointF> *yArray);

    void flash();

    int dataSum();

    void getRange(QPointF &xRange,QPointF &yRange);

signals:
    void dataChanged(QPointF xRange, QPointF yRange);

private:
    LoopVector<QPointF> *m_data = nullptr;
    QVector<QPointF> m_source;
};

#endif // DATAPROCESSOR_H
