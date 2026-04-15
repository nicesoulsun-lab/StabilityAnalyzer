#ifndef REALTIMESTACKPLOT_H
#define REALTIMESTACKPLOT_H

#include <QObject>
#include "oscilloscope.h"
#include "multifunctionstackplot.h"

class RealTimeStackPlot : public Oscilloscope
{
    Q_OBJECT
public:
    explicit RealTimeStackPlot(QQuickItem *parent = nullptr);
    ~RealTimeStackPlot();

public slots:

private:
    int begin = 0;
};

#endif // REALTIMESTACKPLOT_H
