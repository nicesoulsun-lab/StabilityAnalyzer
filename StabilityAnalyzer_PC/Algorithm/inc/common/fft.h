#ifndef FFT_H
#define FFT_H

#include <QObject>
#include <QtMath>
#include "common.h"

class FFT : public QObject
{
    Q_OBJECT
public:
    explicit FFT(QObject *parent = 0);

signals:

public slots:
};

#endif // FFT_H
