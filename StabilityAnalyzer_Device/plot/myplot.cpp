#include "myplot.h"
#include <QDir>
MyPlot::MyPlot(QQuickItem *parent): QQuickPaintedItem(parent)
{
    m_menu = new MenuInfo();

    thread = new QThread();
    plot = new MultifunctionPlot();
    plot->moveToThread(thread);
    thread->start();

    connect(this,&MyPlot::visibleChanged,[=](){
        plot->setVisible(isVisible());
    });
    connect(plot,&MultifunctionPlot::stackVerOffsetChanged,[=](){
        emit stackVerOffsetChanged();
    });
    connect(plot,&MultifunctionPlot::typeChanged,[=](){
        emit typeChanged();
    });


    connect(this,&MyPlot::initSideFreqCursor,plot,&MultifunctionPlot::initSideFreqCursor);
    connect(this,&MyPlot::initMultiFreqCursor,plot,&MultifunctionPlot::initMultiFreqCursor);
    connect(this,&MyPlot::initDoubleCursor,plot,&MultifunctionPlot::initDoubleCursor);
    connect(this,&MyPlot::initSingleCursor,plot,&MultifunctionPlot::initSingleCursor);

    /* 初始化更新画布的计时器 */
    timer = new QTimer();
    timer->setInterval(100);
    connect(timer,&QTimer::timeout,[=](){
        goAhead();
    });

    connect(plot,&MultifunctionPlot::axisRectChanged,[=](){
        emit axisRectChanged();
    });

//    connect(plot,&MultifunctionPlot::plotChanged,this,&MyPlot::flashPlot,Qt::DirectConnection);


    // 以下3行是C++处理鼠标事件的前提，否则所有(C++)鼠标事件直接忽略
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setFlag(ItemAcceptsInputMethod, true);


    /* 播放菜单 */
    MenuInfo *playMenu = new MenuInfo(m_menu);
    playMenu->setName("播放");

    MenuInfo *pauseMenu = new MenuInfo(playMenu);
    pauseMenu->setName("暂停");
    connect(pauseMenu,&MenuInfo::clickSignal,this,[=](){
        setPlayEnable(!playEnable());
        setStep(2);
        pauseMenu->setName(playEnable()?"暂停":"播放");
    });

    MenuInfo *doubleplayMenu = new MenuInfo(playMenu);
    doubleplayMenu->setName("2倍速");
    connect(doubleplayMenu,&MenuInfo::clickSignal,this,[=](){
        setPlayEnable(true);
        setStep(2);
        pauseMenu->setName(playEnable()?"暂停":"播放");
    });

    MenuInfo *fplayMenu = new MenuInfo(playMenu);
    fplayMenu->setName("5倍速");
    connect(fplayMenu,&MenuInfo::clickSignal,this,[=](){
        setPlayEnable(true);
        setStep(5);
        pauseMenu->setName(playEnable()?"暂停":"播放");
    });

    MenuInfo *tplayMenu = new MenuInfo(playMenu);
    tplayMenu->setName("10倍速");
    connect(tplayMenu,&MenuInfo::clickSignal,this,[=](){
        setPlayEnable(true);
        setStep(10);
        pauseMenu->setName(playEnable()?"暂停":"播放");
    });
    /* 复原菜单 */
    MenuInfo *recoverMenu = new MenuInfo(m_menu);
    recoverMenu->setName("复原");
    connect(recoverMenu,&MenuInfo::clickSignal,plot,&MultifunctionPlot::recover);

    /* 标注菜单 */
    MenuInfo *labelMenu = new MenuInfo(m_menu);
    labelMenu->setName("标注");

    MenuInfo *cancalChoose = new MenuInfo(labelMenu);
    cancalChoose->setName("取消选中");
    connect(cancalChoose,&MenuInfo::clickSignal,this,&MyPlot::cancelChooseLabel);

    MenuInfo *delChoose = new MenuInfo(labelMenu);
    delChoose->setName("删除选中");
    connect(delChoose,&MenuInfo::clickSignal,this,&MyPlot::removeChooseLabel);

    MenuInfo *delAll = new MenuInfo(labelMenu);
    delAll->setName("删除全部");
    connect(delAll,&MenuInfo::clickSignal,this,&MyPlot::removeAllLabel);
    m_measurePoints = new MeasurePointListModel(this);
}

MyPlot::~MyPlot()
{
    delete plot;
    delete m_menu;
    if(timer!=nullptr){
        if(timer->isActive())
            timer->stop();
        delete timer;
        timer = nullptr;
    }

}
void MyPlot::paint(QPainter *painter)
{
    updating = true;
    if(map.isNull()){
        painter->drawPixmap(QPoint(),map);
    }
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
    updating = false;
}

AxisListModel *MyPlot::axisModel() const
{
    return m_axisModel;
}

void MyPlot::setAxisModel(AxisListModel *axisModel)
{
    m_axisModel = axisModel;
    plot->setAxisModel(m_axisModel);
}

void MyPlot::addXAxis()
{
    if(m_axisModel!=nullptr){
        m_axisModel->addXAxis();
        emit xAxisModelChanged();
    }
}

void MyPlot::addYAxis()
{
    if(m_axisModel!=nullptr){
        m_axisModel->addYAxis();
        emit yAxisModelChanged();
    }
}

bool MyPlot::removeAxis(int index)
{
    if(m_axisModel==nullptr)
        return false;
    AxisModel *model = m_axisModel->getModel(index);
    /* 查询是否存在关联的曲线，若存在，不能删除 */
    if(m_curveModel!=nullptr&&m_curveModel->find(model))
        return false;
    m_axisModel->remove(index);
    emit xAxisModelChanged();
    emit yAxisModelChanged();
    return true;
}

void MyPlot::addLink()
{
    if(m_curveModel==nullptr)
        return;
    m_curveModel->addCurve();
}

void MyPlot::removeLink(int index)
{
    if(m_curveModel==nullptr)
        return;
    m_curveModel->remove(index);
}

void MyPlot::updateCurve()
{
    plot->updateCurveCanvas();
}


void MyPlot::move(QPoint p1, QPoint p2, int mode)
{
    plot->move(p1,p2,mode);
    flashOnMove();
}

void MyPlot::record()
{
    plot->record();
}

QColor MyPlot::popCardColor() const
{
    return m_popCardColor;
}

void MyPlot::setPopCardColor(const QColor &popCardColor)
{
    m_popCardColor = popCardColor;
    emit popCardColorChanged();
}

QVariantList MyPlot::xAxisModel()
{
    QVariantList list;
    if(m_axisModel==nullptr){
        return list;
    }
    for(int i = 0; i< m_axisModel->rowCount(); i++){
        AxisModel *axis = m_axisModel->getModel(i);
        if(axis->type()==AxisModel::AxisType::XAxis1||axis->type()==AxisModel::AxisType::XAxis2){
            axis->setNumber(i);
            list.push_back(QVariant::fromValue<AxisModel *>(axis));
        }
    }
    return list;
}

QVariantList MyPlot::yAxisModel()
{
    QVariantList list;
    if(m_axisModel==nullptr){
        return list;
    }
    for(int i = 0; i< m_axisModel->rowCount(); i++){
        AxisModel *axis = m_axisModel->getModel(i);
        if(axis->type()==AxisModel::AxisType::YAxis1||axis->type()==AxisModel::AxisType::YAxis2){
            axis->setNumber(i);
            list.push_back(QVariant::fromValue<AxisModel *>(axis));
        }
    }
    return list;
}

void MyPlot::setGridX(AxisModel *axis)
{
    if(plot->grid()!=nullptr)
        plot->grid()->setXAxis(axis);
}

void MyPlot::setGridY(AxisModel *axis)
{
    if(plot->grid()!=nullptr)
        plot->grid()->setYAxis(axis);
}

void MyPlot::setGridLine(int index)
{
    if(plot->grid()!=nullptr){
        if(index==0){
            plot->grid()->setHorLineType(GridModel::LineType::Solid);
            plot->grid()->setVerLineType(GridModel::LineType::Solid);
        }else{
            plot->grid()->setHorLineType(GridModel::LineType::Dotted);
            plot->grid()->setVerLineType(GridModel::LineType::Dotted);
        }
    }
}

void MyPlot::hover(QPoint point)
{
    plot->hover(point);
    plot->updateCursorCanvas();
    update();
}

void MyPlot::exit()
{
    plot->exit();
    plot->updateCursorCanvas();
    update();
}

void MyPlot::choose(QRect rect,int mode)
{
    plot->choose(rect,mode);
    //    if(m_curveModel!=nullptr){
    //        updateCursorShow();
    //    }
}

void MyPlot::recover()
{
    plot->recover();
    //    updateCursorShow();
}

void MyPlot::goback()
{
    plot->goback();
    // updateCursorShow();
}


bool MyPlot::inAxisRect(QPoint point)
{
    return plot->inAxisRect(point);
}

bool MyPlot::playEnable()
{
    if(timer!=nullptr){
        return timer->isActive();
    }else{
        return false;
    }
}

void MyPlot::setPlayEnable(bool play)
{
    if(timer!=nullptr){
        if(play){
            timer->start();
        }else{
            timer->stop();
        }
        emit playEnableChanged();
    }
}

int MyPlot::speed()
{
    if(timer!=nullptr){
        return timer->interval();
    }
    return false;
}

void MyPlot::setSpeed(int msec)
{
    if(timer!=nullptr){
        timer->setInterval(msec);
    }
}


qreal MyPlot::step()
{
    return plot->step();
}

void MyPlot::setStep(qreal step)
{
    plot->setStep(step);
    emit stepChanged();
}

QRect MyPlot::getRect(const QRect &value)
{
    return value&(plot->getAxisRect());
}

QRect MyPlot::axisRect()
{
    return plot->getAxisRect();
}

QVariantList MyPlot::xMeasure(qreal xScale)
{
    if(m_axisModel==nullptr)
        return QVariantList();
    QVariantList xList;

    for(int i = 0; i<m_axisModel->rowCount(); i++){
        AxisModel *am = m_axisModel->getModel(i);
        if(am->type()==AxisModel::AxisType::XAxis1||am->type()==AxisModel::AxisType::XAxis2){
            QList<QString> list;
            if(am->title()!=""){
                list.push_back(am->title());
            }else{
                list.push_back("未命名轴"+QString::number(am->number()));
            }
            list.push_back(QString::number(xScale*(am->upper()-am->lower()),'g',2));
            xList.push_back(QVariant::fromValue<QList<QString>>(list));
        }
    }
    return xList;
}

QVariantList MyPlot::yMeasure(qreal yScale)
{
    if(m_axisModel==nullptr)
        return QVariantList();

    QVariantList yList;

    for(int i = 0; i<m_axisModel->rowCount(); i++){
        AxisModel *am = m_axisModel->getModel(i);
        if(am->type()==AxisModel::AxisType::YAxis1||am->type()==AxisModel::AxisType::YAxis2){
            QList<QString> list;
            if(am->title()!=""){
                list.push_back(am->title());
            }else{
                list.push_back("未命名轴"+QString::number(am->number()));
            }
            list.push_back(QString::number(yScale*(am->upper()-am->lower()),'g',2));
            yList.push_back(QVariant::fromValue<QList<QString>>(list));
        }
    }
    return yList;
}

GridModel *MyPlot::grid()
{
    return plot->grid();
}

void MyPlot::setGrid(GridModel *grid)
{
    plot->setGrid(grid);
}


void MyPlot::removeChooseLabel()
{
    plot->labelList()->removeChoose();
    //    m_singleCursor->removeLabel();
    //    m_doubleCursor->removeLabel();
    //    m_sideFreqCursor->removeLabel();
    //    m_multiFreqCursor->removeLabel();
}

void MyPlot::removeAllLabel()
{
    plot->labelList()->removeAll();
    //    m_singleCursor->removeAllLabel();
    //    m_doubleCursor->removeAllLabel();
    //    m_sideFreqCursor->removeAllLabel();
    //    m_multiFreqCursor->removeAllLabel();
}

void MyPlot::removeLabelByIndex(int index)
{
    plot->labelList()->removeByIndex(index);
}

void MyPlot::cancelChooseLabel()
{
    plot->labelList()->cancelChoose();
    //    m_singleCursor->clearChecked();
    //    m_doubleCursor->clearChecked();
    //    m_sideFreqCursor->clearChecked();
    //    m_multiFreqCursor->clearChecked();
}

qreal MyPlot::totalLength() const
{
    return m_totalLength;
}

void MyPlot::setTotalLength(qreal totalLength)
{
    m_totalLength = totalLength;
    emit totalLengthChanged();
}

qreal MyPlot::progress() const
{
    return m_progress;
}

void MyPlot::setProgress(const qreal &progress)
{
    m_progress = progress;
    emit progressChanged();
}

qreal MyPlot::barWidth() const
{
    return m_barWidth;
}

void MyPlot::setBarWidth(const qreal &barWidth)
{
    m_barWidth = barWidth;
    emit barWidthChanged();
}

MultiCursorListModel *MyPlot::multiList() const
{
    return plot->multiFreqCursor();
}

SideCursorListModel *MyPlot::sideList() const
{
    return plot->sideFreqCursor();
}

DoubleCursorListModel *MyPlot::doubleList() const
{
    return plot->doubleCursor();
}

SingleCursorListModel *MyPlot::singleList() const
{
    return plot->singleCursor();
}

void MyPlot::updateProgress()
{
    setTotalLength(plot->getTotalLength());
    if(m_axisModel==nullptr||m_axisModel->rowCount()==0||m_totalLength==0){
        setBarWidth(0);
        setProgress(0);
        return;
    }

    AxisModel *am = nullptr;
    for(int i = 0; i<m_axisModel->rowCount(); i++){
        AxisModel *a = m_axisModel->getModel(i);
        if(a->type()==AxisModel::AxisType::XAxis1||a->type()==AxisModel::AxisType::XAxis2){
            am = a;
            break;
        }
    }
    qreal w = 0, p = 0;

    if(am!=nullptr){
        w = (am->upper()-am->lower())/m_totalLength;
        p = (am->lower()-am->maxRange().x())/m_totalLength;
    }
    setBarWidth(w);
    setProgress(p);
}

void MyPlot::dragProgressBar(qreal pos)
{
    if(m_axisModel==nullptr)
        return;
    AxisModel *am = nullptr;
    for(int i = 0; i<m_axisModel->rowCount(); i++){
        AxisModel *a = m_axisModel->getModel(i);
        if(a->type()==AxisModel::AxisType::XAxis1||a->type()==AxisModel::AxisType::XAxis2){
            am = a;
            break;
        }
    }

    if(am!=nullptr){
        if(pos<0)
            pos = 0;
        m_progress = pos;
        qreal lower,upper;
        upper = pos*m_totalLength+m_totalLength*m_barWidth;
        lower = pos*m_totalLength;
        if(upper>m_totalLength){
            upper=m_totalLength;
            lower = upper-m_totalLength*m_barWidth;
        }
        am->setRange(lower+am->maxRange().x(),upper+am->maxRange().x());

        plot->updateAxisCanvas();
        plot->updateCurveCanvas();
        plot->updateCursorCanvas();
        update();
    }
}

void MyPlot::setXLength(int len)
{
    if(m_axisModel==nullptr)
        return;
    AxisModel *am = nullptr;
    for(int i = 0; i<m_axisModel->rowCount(); i++){
        AxisModel *a = m_axisModel->getModel(i);
        if(a->type()==AxisModel::AxisType::XAxis1||a->type()==AxisModel::AxisType::XAxis2){
            am = a;
            break;
        }
    }
    if(am!=nullptr){
        qreal upper = am->lower()+len;
        qreal lower = am->lower();
        if(upper>m_totalLength){
            upper = m_totalLength;
            lower = upper-len;
            lower = lower<0?0:lower;
        }
        am->setAutoRange(false);
        am->setRange(lower,upper);
        plot->updateCurveCanvas();
        plot->updateAxisCanvas();
        plot->updateCursorCanvas();
        updateProgress();
    }

    plot->record();
}

int MyPlot::borderWidth() const
{
    return plot->borderWidth();
}

void MyPlot::setBorderWidth(int borderWidth)
{
    plot->setBorderWidth(borderWidth);
    emit borderWidthChanged();
}

int MyPlot::borderType() const
{
    return plot->borderType();
}

void MyPlot::setBorderType(int borderType)
{
    plot->setBorderType(borderType);
    emit borderTypeChanged();
}

QColor MyPlot::borderColor() const
{
    return plot->borderColor();
}

void MyPlot::setBorderColor(QColor borderColor)
{
    plot->setBorderColor(borderColor);
    emit borderColorChanged();
}


void MyPlot::goAhead()
{
    if(plot->goAhead()){
        flashOnMove();
    }
}

void MyPlot::flashPlot(QPixmap map)
{
    updateProgress();
    this->map = map;
    if(!updating){
        update();
    }
}

void MyPlot::flashOnMove()
{
    if(m_curveModel==nullptr){
        return;
    }
    updateProgress();
    if(!updating){
        update();
    }
}

//void MyPlot::updateCursorShow()
//{
//    m_singleList->update();
//    m_doubleList->update();
//    m_sideList->update();
//    m_multiList->update();
//    m_measurePoints->update();
//}

int MyPlot::type() const
{
    return plot->type();
}

void MyPlot::setType(int type)
{
    plot->setType(type);
}

qreal MyPlot::stackVerOffset()
{
    return plot->stackVerOffset();
}

CurveLabelListModel *MyPlot::labelList()
{
    return plot->labelList();
}

MultiLabelListModel *MyPlot::multiLabelList()
{
    return plot->multiLabelList();
}

void MyPlot::addLabel(CurveModel *curve, qreal x)
{
    plot->labelList()->add(curve,x);
}

void MyPlot::addLabel(CurveModel *curve, qreal x, QString node)
{
    qDebug()<<x<<node;
    plot->labelList()->add(curve,x,node);
}

void MyPlot::transformView(int mode)
{
    if(mode==0){
        plot->toSimple();
    }else if(mode==1){
        plot->toStack();
    }else{
        plot->toWaterFall();
    }
}

void MyPlot::mouseDoubleClickEvent(QMouseEvent *event)
{
    QPoint p = event->pos();
    if(m_measureEnable==-1){
        plot->addLabel(p);
    }else{
        QVariantList list = plot->getPointsInfo(p);
        if(list.size()==1){
            recodePoint(list[0].toMap());
        }else if (list.size()>1) {
            emit chooseMenu(p,list);
        }
    }
}

void MyPlot::mouseMoveEvent(QMouseEvent *event)
{
    QPoint p = event->pos();
    if(leftPress){
        chooseEnable = true;
        chooseRect = plot->getChooseArea(pos,p);
        if(chooseRect.width()==0||chooseRect.height()==0){
            return;
        }
        //0:回退 1：横向放大 2：纵向放大 3：局部放大
        if(p.x()<pos.x()&&p.y()<pos.y()){
            if(chooseRect.width()>80&&chooseRect.height()<80){
                optFlag = 1;
            }else if(chooseRect.width()<80&&chooseRect.height()>80){
                optFlag = 2;
            }else{
                optFlag = 0;
            }
        }else if(chooseRect.width()>100&&chooseRect.height()<100){
            optFlag = 1;
        }else if(chooseRect.width()<100&&chooseRect.height()>100){
            optFlag = 2;
        }else{
            if(chooseRect.width()>80&&chooseRect.height()>80&&chooseRect.width()<100&&chooseRect.height()<100){
                if(chooseRect.width()>=chooseRect.height()){
                    optFlag = 1;
                }else{
                    optFlag = 2;
                }
            }else{
                optFlag = 3;
            }
        }
        if(chooseRect.width()<40&&chooseRect.height()<40){
            chooseEnable = false;
        }
        update();
    }else if(rightPress){
        rm = true;
        /* 参数3：移动模式 0：横向、1：纵向、3：双向 */
        move(pos,p,3);
    }else{
        if(plot->getAxisRect().contains(p)){
            /* 鼠标悬停 */
            hover(p);
        }else{
            exit();
        }
    }
}

void MyPlot::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton){
        leftPress = true;
        pos = event->pos();
    }else if(event->button() == Qt::RightButton){
        // setCursor(Qt::ClosedHandCursor);
        rightPress = true;
        pos = event->pos();
        record();
    }
    exit();
    pressTime.start();
}

void MyPlot::mouseReleaseEvent(QMouseEvent *event)
{
    rightPress = false;
    leftPress = false;
    if(m_measureEnable!=-1){
        // setCursor(QCursor(QPixmap(":/img/measure.png")));
    }else{
        // setCursor(Qt::ArrowCursor);
    }
    int interval = pressTime.elapsed();
    if(chooseEnable){
        /*左上：回退到上一次缩放*/
        if(optFlag==0){
            goback();
        }else if (optFlag==1) {
            /* 参数2：放大模式 0：横向、1：纵向、3：对角 */
            choose(chooseRect);
        }else if (optFlag==2) {
            choose(chooseRect,1);
        }else if (optFlag==3) {
            choose(chooseRect,2);
        }
    }
    if(interval<300){
        QPoint p = event->pos();
        if(event->button()==Qt::RightButton&&plot->inAxisRect(p)){
            //弹出菜单栏
            m_menu->setPos(p);
            qDebug() <<__FUNCTION__<< "====================";
            emit popMenu(p);
        }
    }else {
        rm = false;
    }
    chooseEnable = false;
}

void MyPlot::hoverLeaveEvent(QHoverEvent *event)
{
    exit();
}

void MyPlot::hoverMoveEvent(QHoverEvent *event)
{
    QPoint p = event->pos();
    if(plot->inAxisRect(p)){
        /* 鼠标悬停 */
        hover(p);
    }else{
        exit();
    }
}

void MyPlot::initFreqMenu()
{
    /********************* 添加游标 *****************************/
    MenuInfo *cursorMenu = new MenuInfo(m_menu);
    cursorMenu->setName("添加游标");
    MenuInfo *sCursorMenu = new MenuInfo(cursorMenu);
    sCursorMenu->setName("单游标");
    connect(sCursorMenu,&MenuInfo::clickSignal,this,[=](){
        qreal scale = (m_menu->pos().x()-plot->getAxisRect().x())/(double)(plot->getAxisRect().width());
        initSingleCursor(scale);
    });

    MenuInfo *dCursorMenu = new MenuInfo(cursorMenu);
    dCursorMenu->setName("双游标");
    connect(dCursorMenu,&MenuInfo::clickSignal,this,[=](){
        qreal scale = (m_menu->pos().x()-plot->getAxisRect().x())/(double)(plot->getAxisRect().width());
        initDoubleCursor(scale);
    });

    MenuInfo *mCursorMenu = new MenuInfo(cursorMenu);
    mCursorMenu->setName("倍频游标");
    connect(mCursorMenu,&MenuInfo::clickSignal,this,[=](){
        qreal scale = (m_menu->pos().x()-plot->getAxisRect().x())/(double)(plot->getAxisRect().width());
        initMultiFreqCursor(0,scale);
    });
    MenuInfo *sdCursorMenu = new MenuInfo(cursorMenu);
    sdCursorMenu->setName("边频游标");
    connect(sdCursorMenu,&MenuInfo::clickSignal,this,[=](){
        qreal scale = (m_menu->pos().x()-plot->getAxisRect().x())/(double)(plot->getAxisRect().width());
        initSideFreqCursor(scale,4);
    });

    /********************* 删除游标 *****************************/
    MenuInfo *delCursorMenu = new MenuInfo(m_menu);
    delCursorMenu->setName("删除游标");
    MenuInfo *dsCursorMenu = new MenuInfo(delCursorMenu);
    dsCursorMenu->setName("单游标");

    MenuInfo *ddCursorMenu = new MenuInfo(delCursorMenu);
    ddCursorMenu->setName("双游标");

    MenuInfo *dmCursorMenu = new MenuInfo(delCursorMenu);
    dmCursorMenu->setName("倍频游标");

    MenuInfo *dsdCursorMenu = new MenuInfo(delCursorMenu);
    dsdCursorMenu->setName("边频游标");

    /********************* 测距 *****************************/
    MenuInfo *measureMenu = new MenuInfo(m_menu);
    measureMenu->setName("测距");
    connect(measureMenu,&MenuInfo::clickSignal,this,[=](){
        m_measureEnable = m_measureEnable==-1?0:-1;
        measureMenu->setName(m_measureEnable==-1?"测距":"取消测距");
        ////TODO_liubo         setCursor(m_measureEnable==-1?Qt::ArrowCursor
        //                                  :QCursor(QPixmap(":/img/measure.png")));
        if(m_measureEnable==-1){
            m_measurePoints->clear();
        }
    });
}

void MyPlot::initTimeMenu()
{
    /********************* 添加游标 *****************************/
    MenuInfo *cursorMenu = new MenuInfo(m_menu);
    cursorMenu->setName("添加游标");
    MenuInfo *sCursorMenu = new MenuInfo(cursorMenu);
    sCursorMenu->setName("单游标");
    connect(sCursorMenu,&MenuInfo::clickSignal,this,[=](){
        qreal scale = (m_menu->pos().x()-plot->getAxisRect().x())/(double)(plot->getAxisRect().width());
        initSingleCursor(scale);
    });

    MenuInfo *dCursorMenu = new MenuInfo(cursorMenu);
    dCursorMenu->setName("双游标");
    connect(dCursorMenu,&MenuInfo::clickSignal,this,[=](){
        qreal scale = (m_menu->pos().x()-plot->getAxisRect().x())/(double)(plot->getAxisRect().width());
        initDoubleCursor(scale);
    });

    MenuInfo *mCursorMenu = new MenuInfo(cursorMenu);
    mCursorMenu->setName("倍频游标");
    connect(mCursorMenu,&MenuInfo::clickSignal,this,[=](){
        qreal scale = (m_menu->pos().x()-plot->getAxisRect().x())/(double)(plot->getAxisRect().width());
        initMultiFreqCursor(0,scale);
    });

    MenuInfo *sdCursorMenu = new MenuInfo(cursorMenu);
    sdCursorMenu->setName("边频游标");
    connect(sdCursorMenu,&MenuInfo::clickSignal,this,[=](){
        qreal scale = (m_menu->pos().x()-plot->getAxisRect().x())/(double)(plot->getAxisRect().width());
        initSideFreqCursor(scale,4);
    });


    /********************* 测距 *****************************/
    MenuInfo *measureMenu = new MenuInfo(m_menu);
    measureMenu->setName("测距");
    connect(measureMenu,&MenuInfo::clickSignal,this,[=](){
        m_measureEnable = m_measureEnable==-1?0:-1;
        measureMenu->setName(m_measureEnable==-1?"测距":"取消测距");
        ////TODO_liubo         setCursor(m_measureEnable==-1?Qt::ArrowCursor
        //                                  :QCursor(QPixmap(":/img/measure.png")));
        if(m_measureEnable==-1){
            m_measurePoints->clear();
        }
    });
}

MenuInfo *MyPlot::menu() const
{
    return m_menu;
}

void MyPlot::recodePoint(QVariantMap var)
{
    CurveModel * c = var.find("curve").value().value<CurveModel *>();
    CurveLabelModel* measurePoint = new CurveLabelModel(c);
    measurePoint->setPoint(var.find("point").value().value<QPointF>());
    m_measurePoints->add(measurePoint);
}

void MyPlot::recodePoint(CurveModel *curve, QPointF p)
{
    CurveLabelModel* measurePoint = new CurveLabelModel(curve);
    measurePoint->setPoint(p);
    m_measurePoints->add(measurePoint);
}

int MyPlot::measureEnable() const
{
    return m_measureEnable;
}

void MyPlot::setMeasureEnable(int measureEnable)
{
    m_measureEnable = measureEnable;
}

QString MyPlot::getDis(qreal p1, qreal p2)
{
    return QString::number(qAbs(p1-p2));
}

MeasurePointListModel *MyPlot::measurePoints() const
{
    return m_measurePoints;
}

void MyPlot::setMargins(qreal left, qreal top, qreal right, qreal bottom)
{
    plot->setMargins(left,top,right,bottom);
}


void MyPlot::setBandWidth(qreal min, qreal max)
{
    if(m_axisModel!=nullptr){
        AxisModel *axis = m_axisModel->getFirstX();
        if(axis!=nullptr){
            axis->setBandWidth(min,max);
            qreal w = (axis->upper()-axis->lower())/m_totalLength;
            setBarWidth(w);
        }

    }
}










