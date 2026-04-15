#include "realtimeplot.h"

RealTimePlot::RealTimePlot(QQuickItem *parent) : Oscilloscope(parent)
{
    initPlotHandler(new MultifunctionPlot());
    /* test */
    timer = new QTimer();
    connect(timer,&QTimer::timeout,this,[=](){
        QList<qreal> list;
        for(int i = 0; i<100; i++){
            list.push_back(qrand()%1000/10.0);
        }
        pushData(list,begin);
        begin++;
    });
    connect(timer,&QTimer::timeout,plot,&BasePlot::goAhead);

//    /* test2 */
//    init();
//    QList<QList<qreal>> list;
//    for(int i = 0; i<100; i++){
//        QList<qreal> l;
//        for(int j = 0; j<1000; j++){
//            l.push_back(qrand()%1000/10.0);
//        }
//        list.push_back(l);
//    }
//    pushDatas(list,0,1);

    init();
}

RealTimePlot::~RealTimePlot()
{
    if(timer)
    {
        timer->stop();
        delete timer;
    }
}

void RealTimePlot::init()
{

    xAxis->setLower(0);
    xAxis->setUpper(1000);
    xAxis->setAutoRange(false);
    gModel->setXAxis(xAxis);
    gModel->setYAxis(yAxis);

    for(int i = 0; i<100; i++){
        CurveModel *curve = cModel->addCurve();
        curve->setXAxis(xAxis);
        curve->setYAxis(yAxis);
        curve->sourcePtr()->reserve(1030);
        if(i<20){
            curve->setVisible(true);
        }else{
            curve->setVisible(false);
        }
    }
    start();
    timer->start(20);
}
