#ifndef REALTIMEPLOT_H
#define REALTIMEPLOT_H

#include <QObject>
#include "oscilloscope.h"
#include "multifunctionplot.h"

class RealTimePlot : public Oscilloscope
{
    Q_OBJECT
public:
    explicit RealTimePlot(QQuickItem *parent = 0);
    ~RealTimePlot();
    void init();
signals:

public slots:

private:
    int begin = 0;
    QTimer *timer = nullptr;
};

#endif // REALTIMEPLOT_H
