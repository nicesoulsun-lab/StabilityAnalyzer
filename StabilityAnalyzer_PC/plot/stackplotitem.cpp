#include "stackplotitem.h"

StackPlotItem::StackPlotItem()
{
    plot = new MultifunctionPlot();
    plot->moveToThread(&thread);
    thread.start();

    qRegisterMetaType<QVector<PipeConfigInfo *>>("QVector<PipeConfigInfo *>&");
//    m_curveModel = new CurveListModel();
//    m_axisModel = new AxisListModel();
//    plot->setAxisModel(m_axisModel);
//    plot->setCurveModel(m_curveModel);
    plot->setMargins(2,5,5,10);

    connect(this,&QQuickItem::widthChanged,this,[=](){
        plot->resize(width(),height());
    });
    connect(this,&QQuickItem::heightChanged,this,[=](){
        plot->resize(width(),height());
    });

    connect(this,&QQuickItem::visibleChanged,this,[=](){
        plot->setVisible(isVisible());
    });
    connect(plot,&MultifunctionPlot::plotChanged,this,[=](){
        update();
    });
   // connect(this,&StackPlotItem::initPlot,plot,&MultifunctionPlot::init);
    connect(this, SIGNAL(initPlot(const QVector<PipeConfigInfo *> &source)), this, SLOT(MultifunctionPlot::init(const QVector<PipeConfigInfo *>& source)));

    connect(plot,&MultifunctionPlot::initFinish,this,[=](){
        emit pipeSumChanged();
        plot->flash();
    });

    control = DataPipeControl::instance();
    connect(control,&DataPipeControl::updatePlot,plot,&MultifunctionPlot::flash);
}

StackPlotItem::~StackPlotItem()
{
    thread.quit();
    thread.wait();
    delete plot;
}

void StackPlotItem::paint(QPainter *painter)
{
    plot->draw(painter);
}

QVector<PipeConfigInfo *> StackPlotItem::source() const
{
    return QVector<PipeConfigInfo *>();
}

void StackPlotItem::setSource(const QVector<PipeConfigInfo *> &source)
{
    emit initPlot(source);
    emit sourceChanged();
}

int StackPlotItem::pipeSum()
{
    return plot->curveModel()->rowCount();
}

