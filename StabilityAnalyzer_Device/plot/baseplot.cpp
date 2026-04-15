#include "baseplot.h"
#include "BasePlot.h"

BasePlot::BasePlot(QObject* parent):QObject(parent)
{
}

BasePlot::~BasePlot()
{
    if(timer){
        timer->deleteLater();
    }
    if(m_curveModel){
        m_curveModel->deleteLater();
    }
    if(m_axisModel){
        m_axisModel->deleteLater();
    }
    if(m_grid){
        m_grid->deleteLater();
    }
}

void BasePlot::calculateRange()
{
    QSet<AxisModel *> xSet,ySet;
    if(m_curveModel==nullptr){
        return;
    }
    QPointF xRange,yRange;
    for(int i = 0; i<m_curveModel->rowCount(); i++){
        CurveModel *cm = m_curveModel->getModel(i);
        if(cm->xAxis()==nullptr||cm->yAxis()==nullptr||(!cm->visible()))
            continue;
        cm->getRange(xRange,yRange);
        if(ySet.find(cm->yAxis())!=ySet.end()){
            qreal l = qMin(yRange.x(),cm->yAxis()->lower());
            qreal u = qMax(yRange.y(),cm->yAxis()->upper());
            cm->yAxis()->setMaxRange(QPointF(l,u));
            if(cm->yAxis()->autoRange())
                cm->yAxis()->setRangeNoUpdate(l,u);
        }else{
            cm->yAxis()->setMaxRange(yRange);
            if(cm->yAxis()->autoRange())
                cm->yAxis()->setRangeNoUpdate(yRange.x(),yRange.y());
            ySet.insert(cm->yAxis());
        }
        if(xSet.find(cm->xAxis())!=xSet.end()){
            qreal l = qMin(xRange.x(),cm->xAxis()->lower());
            qreal u = qMax(xRange.y(),cm->xAxis()->upper());
            cm->xAxis()->setMaxRange(QPointF(l,u));
            if(cm->xAxis()->autoRange())
                cm->xAxis()->setRangeNoUpdate(l,u);
        }else{
            cm->xAxis()->setMaxRange(xRange);
            if(cm->xAxis()->autoRange())
                cm->xAxis()->setRangeNoUpdate(xRange.x(),xRange.y());
            xSet.insert(cm->xAxis());
        }
    }

    //    foreach(AxisModel *y,ySet){
    //        qreal dis = (y->upper()-y->lower());
    //        y->setLower(y->lower());
    //        y->setUpper(y->upper()+dis*0.05);
    //        if(y->lower()<0){
    //            y->setLower(y->lower()-dis*0.05);
    //        }
    //    }
}

void BasePlot::updateShow()
{
    if(rangeUpdate)
        calculateRange();
    if(axisUpdate)
        updateAxisCanvas();
    if(curveUpdate)
        updateCurveCanvas();
    if(cursorUpdate)
        updateCursorCanvas();
    rangeUpdate = false;
    axisUpdate = false;
    cursorUpdate = false;
    cursorUpdate = false;
    draw();
}

void BasePlot::setAxisModel(AxisListModel *axisModel)
{
    m_axisModel = axisModel;
    if(m_axisModel==nullptr)
        return;
    connect(m_axisModel,&AxisListModel::styleChanged,this,[=](){
        if(m_visible){
            axisUpdate = true;
            if(timer&&!timer->isActive()){
                timer->start();
            }
        }
    });

    connect(m_axisModel,&AxisListModel::rangeChanged,this,[=](){
        if(m_visible){
            axisUpdate = true;
            curveUpdate = true;
            if(timer&&!timer->isActive()){
                timer->start();
            }
        }
    });
    connect(m_axisModel,&AxisListModel::modelChanged,this,[=](){
        if(m_visible){
            axisUpdate = true;
            if(timer&&!timer->isActive()){
                timer->start();
            }
        }
    });
}

GridModel *BasePlot::grid() const
{
    return m_grid;
}

void BasePlot::setGrid(GridModel *grid)
{
    m_grid = grid;
    if(m_grid==nullptr)
        return;
    connect(m_grid,&GridModel::styleChanged,this,[=](){
        if(m_visible){
            axisUpdate = true;
            if(timer&&!timer->isActive()){
                timer->start();
            }
        }
    });
}

CurveListModel *BasePlot::curveModel() const
{
    return m_curveModel;
}

void BasePlot::setCurveModel(CurveListModel *curveModel)
{
    m_curveModel = curveModel;
    if(m_curveModel==nullptr)
        return;
    connect(m_curveModel,&CurveListModel::sourceChanged,this,[=](){
        rangeUpdate = true;
        if(m_visible){
            axisUpdate = true;
            curveUpdate = true;
            cursorUpdate = true;
            if(timer&&!timer->isActive()){
                timer->start();
            }
        }
    });
    connect(m_curveModel,&CurveListModel::modelChanged,this,[=](){
        rangeUpdate = true;
        if(m_visible){
            axisUpdate = true;
            curveUpdate = true;
            cursorUpdate = true;
            if(timer&&!timer->isActive()){
                timer->start();
            }
        }
    });
    connect(m_curveModel,&CurveListModel::styleChanged,this,[=](){
        if(m_visible){
            curveUpdate = true;
            if(timer&&!timer->isActive()){
                timer->start();
            }
        }
    });
    connect(m_curveModel,&CurveListModel::visibleChanged,this,[=](){
        if(m_visible){
            if(timer&&!timer->isActive()){
                timer->start();
            }
        }
    });
    connect(m_curveModel,&CurveListModel::sortChanged,this,[=](){
        if(m_visible){
            if(timer&&!timer->isActive()){
                timer->start();
            }
        }
    });
}

void BasePlot::choose(QRect &rect,int model)
{
    if(m_axisModel==nullptr)
        return;
    QRect r;
    QRect r2 = axisRect;

    while (r2.y()>rect.y()){
        r2.translate(0,-m_verOffset);
    }
    r = rect&r2;

    m_axisModel->zoom(r,r2,model);
    axisUpdate = true;
    curveUpdate = true;
    if(timer&&!timer->isActive()){
        timer->start();
    }
}

void BasePlot::goback()
{
    m_axisModel->goback();
    axisUpdate = true;
    curveUpdate = true;
    if(timer&&!timer->isActive()){
        timer->start();
    }
}

void BasePlot::recover()
{
    if(m_axisModel==nullptr)
        return;
    m_axisModel->recover();
    axisUpdate = true;
    curveUpdate = true;
    if(timer&&!timer->isActive()){
        timer->start();
    }
}

void BasePlot::hover(QPoint point)
{
    if(axisRect.contains(point)){
        cursorPos = point;

        cursorUpdate = true;
        if(timer&&!timer->isActive()){
            timer->start();
        }
    }else{
        exit();
    }
}

void BasePlot::exit()
{
    cursorPos = QPointF(-1000,0);
    cursorUpdate = true;
    if(timer&&!timer->isActive()){
        timer->start();
    }
}

void BasePlot::record()
{
    if(m_axisModel==nullptr)
        return;
    m_axisModel->record();
}

void BasePlot::move(QPoint p1, QPoint p2, int mode)
{
    if(m_axisModel==nullptr)
        return;
    QRect r2 = axisRect;
    while (r2.y()>p1.y()){
        r2.translate(0,-m_verOffset);
    }
    if(!r2.contains(p1)){
        return;
    }
    m_axisModel->moveBySacle(p1,p2,axisRect,mode);
    axisUpdate = true;
    curveUpdate = true;
    if(timer&&!timer->isActive()){
        timer->start();
    }
}

qreal BasePlot::step()
{
    return offVal;
}

void BasePlot::setStep(qreal step)
{
    offVal = step;
}

void BasePlot::setBorderColor(const QColor &borderColor)
{
    m_borderColor = borderColor;
}

QColor BasePlot::tipsCardColor()
{
    return cursorDrawer.cardColor();
}

void BasePlot::setTipsCardColor(const QColor &color)
{
    cursorDrawer.setCardColor(color);
    cursorUpdate = true;
}

QRect BasePlot::getAxisRect() const
{
    return axisRect;
}

qreal BasePlot::getTotalLength()
{
    qreal len = 0;
    if(m_curveModel==nullptr||m_curveModel->rowCount()==0){
        return len;
    }
    //    len = m_curveModel->getModel(0)->xRange().y()-m_curveModel->getModel(0)->xRange().x();
    //    for(int i = 1; i<m_curveModel->rowCount(); i++){
    //        len = qMax(len,m_curveModel->getModel(i)->xRange().y()
    //                   -m_curveModel->getModel(i)->xRange().x());
    //    }
    return len;
}

int BasePlot::borderWidth() const
{
    return m_borderWidth;
}

void BasePlot::setBorderWidth(int borderWidth)
{
    m_borderWidth = borderWidth;
}

int BasePlot::borderType() const
{
    return m_borderType;
}

void BasePlot::setBorderType(int borderType)
{
    m_borderType = borderType;
}

QColor BasePlot::borderColor() const
{
    return m_borderColor;
}

bool BasePlot::playEnable() const
{
    return m_playEnable;
}

void BasePlot::setPlayEnable(bool playEnable)
{
    m_playEnable = playEnable;
}

bool BasePlot::goAhead()
{
    if(m_curveModel==nullptr)
        return false;
    QHash<AxisModel*,qreal> axisHash;      //存放x轴指针的集合
    QHash<AxisModel*,qreal>::iterator it;

    if(m_playEnable){
        for(int i = 0; i<m_curveModel->rowCount(); i++){
            CurveModel* model= m_curveModel->getModel(i);
            AxisModel *xAxis = model->xAxis();
            it = axisHash.find(xAxis);
            if(model->sourcePtr()==nullptr||model->sourcePtr()->isEmpty()){
                continue;
            }
            qreal dis = model->sourcePtr()->atLast().x()-model->xAxis()->upper();
            if(dis>0){
                if(it!=axisHash.end()){ //需要移动，计算移动距离
                    qreal dis2 = qMax(dis,it.value());
                    axisHash.insert(xAxis,dis2);
                }else{      //添加
                    axisHash.insert(xAxis,dis);
                }
            }
        }

        foreach(AxisModel* model,axisHash.keys()){

            model->move(axisHash.find(model).value());
        }

        rangeUpdate = true;
        axisUpdate = true;
        curveUpdate = true;
        cursorUpdate = true;
        if(timer&&!timer->isActive()){
            timer->start();
        }

        if(!axisHash.isEmpty()){
            return true;
        }
        return false;
    }else{
        return false;
    }
}

bool BasePlot::visible() const
{
    return m_visible;
}

void BasePlot::setVisible(bool visible)
{
    m_visible = visible;
    if(m_visible){
        axisUpdate = true;
        curveUpdate = true;
        cursorUpdate = true;
        if(timer&&!timer->isActive()){
            timer->start();
        }
    }
}


QRect BasePlot::getChooseArea(QPoint pos1, QPoint pos2)
{
    QRect r1 = QRect(qMin(pos1.x(),pos2.x())
                     ,qMin(pos1.y(),pos2.y())
                     ,qAbs(pos1.x()-pos2.x())
                     ,qAbs(pos1.y()-pos2.y()));
    return r1&axisRect;
}

bool BasePlot::inAxisRect(QPoint pos)
{
    return axisRect.contains(pos);
}

void BasePlot::setMargins(qreal left, qreal top, qreal right, qreal bottom)
{
    m_margins.left = left;
    m_margins.top = top;
    m_margins.right = right;
    m_margins.bottom = bottom;
    axisUpdate = true;
    curveUpdate = true;
    cursorUpdate = true;
    if(timer&&!timer->isActive()){
        timer->start();
    }
}


void BasePlot::init(AxisListModel *alist, CurveListModel *clist,GridModel *gmodel)
{
    setAxisModel(alist);
    setCurveModel(clist);
    setGrid(gmodel);

    timer = new QTimer();
    timer->setInterval(50);
    timer->setSingleShot(true);
    connect(timer,&QTimer::timeout,this,&BasePlot::updateShow);
    if(timer&&!timer->isActive()){
        timer->start();
    }
}

void BasePlot::pushDatas(QList<QList<qreal>> &list, qreal begin, int step)
{
    if(m_curveModel==nullptr||list.size()<curveModel()->rowCount()){
        qDebug()<<"curve sum not match";
        return;
    }
    for(int i = 0; i<m_curveModel->rowCount(); i++){
        int b = begin;
        for(int j = 0; j<list.at(i).size();j++){
            m_curveModel->getModel(i)->sourcePtr()->push_back(QPointF(b,list.at(i).at(j)));
            b+=step;
        }
    }
    goAhead();
}

void BasePlot::pushData(QList<qreal> &list, qreal begin)
{
    if(m_curveModel==nullptr||list.size()<curveModel()->rowCount()){
        qDebug()<<"curve sum not match";
        return;
    }
    for(int i = 0; i<m_curveModel->rowCount(); i++){
        m_curveModel->getModel(i)->sourcePtr()->push_back(QPointF(begin,list.at(i)));
    }
    rangeUpdate = true;
    axisUpdate = true;
    curveUpdate = true;
    cursorUpdate = true;
    if(timer&&!timer->isActive()){
        timer->start();
    }
    goAhead();
}

void BasePlot::pushData(QMap<int, qreal> &list, qreal begin)
{
    //qDebug()<<"--------------------------------";
    if(m_curveModel==nullptr){
        return;
    }
    QMap<int, qreal>::iterator it = list.begin();
    for(;it!=list.end(); it++){
        if(it.key()>m_curveModel->rowCount()){
            qDebug()<<__FUNCTION__<<"index out of:"<<it.key();
            continue;
        }
        m_curveModel->getModel(it.key())->sourcePtr()->push_back(QPointF(begin,it.value()));
    }

    rangeUpdate = true;
    axisUpdate = true;
    curveUpdate = true;
    cursorUpdate = true;
    if(timer&&!timer->isActive()){
        timer->start();
    }
    goAhead();
}

QPixmap BasePlot::canvas()
{
    QMutexLocker locker(&mutex);
    return m_canvas;
}

void BasePlot::setCanvas(const QPixmap &canvas)
{
    QMutexLocker locker(&mutex);
    m_canvas = canvas;
}

void BasePlot::reSize(int width, int height)
{
    m_width = width;
    m_height = height;
    axisUpdate = true;
    curveUpdate = true;
    cursorUpdate = true;
    if(timer&&!timer->isActive()){
        timer->start();
    }
}

void BasePlot::setXRange(qreal begin, qreal end)
{
    for(int i = 0; i<m_axisModel->rowCount(); i++){
        if(m_axisModel->getModel(i)->isX())
            m_axisModel->getModel(i)->setRangeNoUpdate(begin,end);
    }
    axisUpdate = true;
    curveUpdate = true;
    cursorUpdate = true;
    rangeUpdate = true;
    if(timer&&!timer->isActive()){
        timer->start();
    }
}
