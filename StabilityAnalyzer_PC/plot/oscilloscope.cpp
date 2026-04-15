#include "oscilloscope.h"

Oscilloscope::Oscilloscope(QQuickItem *parent):QQuickPaintedItem(parent)
{
    // 以下3行是C++处理鼠标事件的前提，否则所有(C++)鼠标事件直接忽略
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemAcceptsInputMethod, true);
}

Oscilloscope::~Oscilloscope()
{
    if(thread&&thread->isRunning())
    {
        thread->quit();
        thread->wait();
        delete thread;
        thread = nullptr;
    }

    if(plot)
        delete plot;
    if(cModel){
        delete cModel;
    }
    if(aModel){
        delete aModel;
    }
    if(gModel){
        delete gModel;
    }
}

void Oscilloscope::paint(QPainter *painter)
{
    if(!painter->isActive()){
        return;
    }
    painter->drawPixmap(QPoint(),plot->canvas());
    if(chooseEnable){
        //0:回退 灰色 1：横向放大 红 2：纵向放大 绿 3：局部放大 蓝
        QString text;
        if(optFlag==0){
            painter->setPen(QColor(150,150,150,190));
            painter->setBrush(QColor(150,150,150,50));
            text = "回退";
        }else if(optFlag==1){
            painter->setPen(QColor(200,150,150,190));
            painter->setBrush(QColor(200,150,150,80));
            text = "横向";
        }else if(optFlag==2){
            painter->setPen(QColor(150,200,150,190));
            painter->setBrush(QColor(150,200,150,80));
            text = "纵向";
        }else if(optFlag==3){
            painter->setPen(QColor(150,150,200,190));
            painter->setBrush(QColor(75,129,240,40));
            text="对角";
        }
        if(!chooseRect.isEmpty()){
            painter->drawRect(chooseRect);
        }
        painter->setFont(QFont("微软雅黑",10));
        painter->setPen(Qt::white);
        painter->drawText(chooseRect,Qt::AlignCenter,text);
    }
}

void Oscilloscope::flashPlot()
{
    update();
}

void Oscilloscope::initPlotHandler(BasePlot *plot)
{
    this->plot = plot;
    qRegisterMetaType<QList<qreal>>("QList<qreal>&");
    qRegisterMetaType<QPoint>("QPoint");
    qRegisterMetaType<QRect>("QRect&");
    qRegisterMetaType<QMap<int,qreal>>("QMap<int,qreal>&");
    qRegisterMetaType<QList<QList<qreal>>>("QList<QList<qreal>>&");
    thread = new QThread();
    plot->moveToThread(thread);

    connect(this,&Oscilloscope::readyInit,plot,&BasePlot::init);
    connect(this,QOverload<QMap<int,qreal> &,qreal>::of(&Oscilloscope::readyPush)
            ,plot,QOverload<QMap<int,qreal> &,qreal>::of(&BasePlot::pushData));
    connect(this,QOverload<QList<qreal> &,qreal>::of(&Oscilloscope::readyPush)
            ,plot,QOverload<QList<qreal> &,qreal>::of(&BasePlot::pushData));

    connect(this,&Oscilloscope::readyPushs,plot,&BasePlot::pushDatas);
    connect(plot,&BasePlot::plotChanged,this,&Oscilloscope::flashPlot);
    connect(this,&Oscilloscope::readyResize,plot,&BasePlot::reSize);
    connect(this,&Oscilloscope::readyHover,plot,&BasePlot::hover);
    connect(this,&Oscilloscope::readyReocde,plot,&BasePlot::record);
    connect(this,&Oscilloscope::readyExit,plot,&BasePlot::exit);
    connect(this,&Oscilloscope::readyGoBack,plot,&BasePlot::goback);
    connect(this,&Oscilloscope::readyChoose,plot,&BasePlot::choose);
    connect(this,&Oscilloscope::readyMove,plot,&BasePlot::move);
    connect(this,&Oscilloscope::readyGoAhead,plot,&BasePlot::goAhead);
    connect(this,&Oscilloscope::readySetXRange,plot,&BasePlot::setXRange);
    connect(this,&Oscilloscope::widthChanged,this,[=](){
        if(thread->isRunning())
            emit readyResize(width(),height());
    });
    connect(this,&Oscilloscope::heightChanged,this,[=](){
        if(thread->isRunning())
            emit readyResize(width(),height());
    });




    cModel = new CurveListModel();
    aModel = new AxisListModel();
    gModel = new GridModel();
    xAxis = aModel->addXAxis();
    yAxis = aModel->addYAxis();
    cModel->moveToThread(thread);
    aModel->moveToThread(thread);
    gModel->moveToThread(thread);
    emit axisModelChanged();
    emit curveModelChanged();
}

void Oscilloscope::pushData(QMap<int,qreal> &list, qreal begin)
{
    emit readyPush(list, begin);
}

//void Oscilloscope::init()
//{
//    AxisModel *xAxis = aModel->addXAxis();
//    AxisModel *yAxis = aModel->addYAxis();
//    xAxis->setLower(0);
//    xAxis->setUpper(1000);
//    xAxis->setAutoRange(false);
//    gModel->setXAxis(xAxis);
//    gModel->setYAxis(yAxis);

//    for(int i = 0; i<100; i++){
//        CurveModel *curve = cModel->addCurve();
//        curve->setXAxis(xAxis);
//        curve->setYAxis(yAxis);
//        curve->sourcePtr()->reserve(1030);
//        if(i<20){
//            curve->setVisible(true);
//        }else{
//            curve->setVisible(false);
//        }
//    }
//}

void Oscilloscope::pushData(QList<qreal> &list,qreal begin)
{
    emit readyPush(list, begin);
}

void Oscilloscope::pushDatas(QList<QList<qreal>> &list, qreal begin, int step)
{
    emit readyPushs(list, begin,step);
}

void Oscilloscope::mouseDoubleClickEvent(QMouseEvent *event)
{
//    QPoint p = event->pos();
//    if(m_measureEnable==-1){
//        plot->addLabel(p);
//    }else{
//        QVariantList list = plot->getPointsInfo(p);
//        if(list.size()==1){
//            recodePoint(list[0].toMap());
//        }else if (list.size()>1) {
//            emit chooseMenu(p,list);
//        }
//    }
}

//void Oscilloscope::mouseMoveEvent(QMouseEvent *event)
//{
//    QPoint p = event->pos();
//    if(leftPress){
//        chooseEnable = true;
//        chooseRect = plot->getChooseArea(pos,p);
//        if(chooseRect.width()==0||chooseRect.height()==0){
//            return;
//        }
//        //0:回退 1：横向放大 2：纵向放大 3：局部放大
//        if(p.x()<pos.x()&&p.y()<pos.y()){
//            if(chooseRect.width()>80&&chooseRect.height()<80){
//                optFlag = 1;
//            }else if(chooseRect.width()<80&&chooseRect.height()>80){
//                optFlag = 2;
//            }else{
//                optFlag = 0;
//            }
//        }else if(chooseRect.width()>100&&chooseRect.height()<100){
//            optFlag = 1;
//        }else if(chooseRect.width()<100&&chooseRect.height()>100){
//            optFlag = 2;
//        }else{
//            if(chooseRect.width()>80&&chooseRect.height()>80&&chooseRect.width()<100&&chooseRect.height()<100){
//                if(chooseRect.width()>=chooseRect.height()){
//                    optFlag = 1;
//                }else{
//                    optFlag = 2;
//                }
//            }else{
//                optFlag = 3;
//            }
//        }
//        if(chooseRect.width()<40&&chooseRect.height()<40){
//            chooseEnable = false;
//        }
//        update();
//    }else if(rightPress){
//        rm = true;
//        /* 参数3：移动模式 0：横向、1：纵向、3：双向 */
//        emit readyMove(pos,p,3);
//    }else{
//        if(plot->getAxisRect().contains(p)){
//            /* 鼠标悬停 */
//            emit readyHover(p);
//        }else{
//            emit readyExit();
//        }
//    }
//}

//void Oscilloscope::mousePressEvent(QMouseEvent *event)
//{
//    if(event->button() == Qt::LeftButton){
//        leftPress = true;
//        pos = event->pos();
//    }else if(event->button() == Qt::RightButton){
//        setCursor(Qt::PointingHandCursor);
//        rightPress = true;
//        pos = event->pos();
//        emit readyReocde();
//    }
//    emit readyExit();
//    pressTime.start();
//}

//void Oscilloscope::mouseReleaseEvent(QMouseEvent *event)
//{
//    rightPress = false;
//    leftPress = false;
////    if(m_measureEnable!=-1){
////        // setCursor(QCursor(QPixmap(":/img/measure.png")));
////    }else{
////        // setCursor(Qt::ArrowCursor);
////    }
//    setCursor(Qt::ArrowCursor);
//    int interval = pressTime.elapsed();
//    if(chooseEnable){
//        /*左上：回退到上一次缩放*/
//        if(optFlag==0){
//            emit readyGoBack();
//        }else if (optFlag==1) {
//            /* 参数2：放大模式 0：横向、1：纵向、3：对角 */
//            emit readyChoose(chooseRect);
//        }else if (optFlag==2) {
//            emit readyChoose(chooseRect,1);
//        }else if (optFlag==3) {
//            emit readyChoose(chooseRect,2);
//        }
//    }
//    if(interval<300){
////        QPoint p = event->pos();
////        if(event->button()==Qt::RightButton&&plot->inAxisRect(p)){
////            //弹出菜单栏
////            m_menu->setPos(p);
////            qDebug() <<__FUNCTION__<< "====================";
////            emit popMenu(p);
////        }
//    }else {
//        rm = false;
//    }
//    chooseEnable = false;
//}

void Oscilloscope::hoverLeaveEvent(QHoverEvent *event)
{
    emit readyExit();
}

void Oscilloscope::hoverMoveEvent(QHoverEvent *event)
{
    QPoint p = event->pos();
    if(plot->inAxisRect(p)){
        /* 鼠标悬停 */
        emit readyHover(p);
    }else{
        emit readyExit();
    }
}

void Oscilloscope::start()
{
    thread->start();
    emit readyInit(aModel,cModel,gModel);
    emit readyResize(width(),height());
}

bool Oscilloscope::quit()
{
    if(thread->isRunning()){
        thread->quit();
        bool flag = thread->wait();
        return flag;
    }
    return true;
}

CurveListModel *Oscilloscope::curveModel()
{
    return cModel;
}

AxisListModel *Oscilloscope::axisModel()
{
    return aModel;
}
