#include "realtimestackplot.h"

RealTimeStackPlot::RealTimeStackPlot(QQuickItem *parent) : Oscilloscope(parent)
{
    initPlotHandler(new MultifunctionStackPlot());
}

RealTimeStackPlot::~RealTimeStackPlot()
{

}
