#include "multifunctionplot.h"
#include <QDebug>

MultifunctionPlot::MultifunctionPlot(QObject* parent):BasePlot(parent)
{
    m_singleCursor = new SingleCursorListModel(this);
    m_doubleCursor = new DoubleCursorListModel(this);
    m_multiFreqCursor = new MultiCursorListModel(this);
    m_sideFreqCursor = new SideCursorListModel(this);
    m_cursorSetModel = new CursorSetModel(this);
}

MultifunctionPlot::~MultifunctionPlot()
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

void MultifunctionPlot::draw()
{
    if(m_width==0||m_height==0){
        return;
    }
    QPixmap map(m_width,m_height);
    map.fill(Qt::transparent);
    if(!map.paintEngine()){
        return;
    }
    QPainter painter(&map);
    if(painter.isActive()){
        painter.setRenderHint(QPainter::HighQualityAntialiasing,true);

        if(m_type==0){
            drawBySimple(&painter);
        }else if(m_type==1){
            drawByStack(&painter);
        }else if(m_type==2){
            drawByWaterFall(&painter);
        }
        painter.drawPixmap(QPointF(),cursorCanvas);
        setCanvas(map);
        emit plotChanged();
    }
}

void MultifunctionPlot::drawBySimple(QPainter *painter)
{
    QPen pen(m_borderColor,m_borderWidth);
    if(m_borderType!=0)
        pen.setStyle(Qt::DashLine);
    painter->setPen(pen);
    painter->drawRect(axisRect);

    painter->drawPixmap(QPoint(),axisCanvas);
    QRect tempRect = QRect(axisRect.x()-10,axisRect.y()-10,axisRect.width()+20,axisRect.height()+20);
    if(m_curveModel!=nullptr){
        for(int i = 0;i<m_curveModel->rowCount();i++){
            if(m_curveModel->getModel(i)->visible()){
                QPixmap pix= m_curveModel->getModel(i)->getCurve();
                painter->drawPixmap(axisRect,pix);
                QPixmap pix2= m_curveModel->getModel(i)->getCursor();
                painter->drawPixmap(tempRect,pix2);
            }
        }
    }

    if(m_curveModel!=nullptr){
        for(int i = 0;i<m_curveModel->rowCount();i++){
            if(m_curveModel->getModel(i)->visible()){
                //                QPixmap pix= m_curveModel->getModel(i)->getCurve();
                //                painter->drawPixmap(axisRect,pix);
                //                QPixmap pix2= m_curveModel->getModel(i)->getCursor();
                //                painter->drawPixmap(tempRect,pix2);
                m_curveModel->getModel(i)->drawAlarmLine(painter,axisRect);
            }
        }
    }
}

void MultifunctionPlot::drawByStack(QPainter *painter)
{
    QPen pen(m_borderColor,m_borderWidth);
    if(m_borderType!=0)
        pen.setStyle(Qt::DashLine);
    painter->setPen(pen);
    painter->drawRect(axisRect);

    painter->drawPixmap(QPoint(),axisCanvas);
    if(m_curveModel!=nullptr){
        QRect tempRect = axisRect;
        QRect tempRect2 = QRect(axisRect.x()-10,axisRect.y()-10,axisRect.width()+20,axisRect.height()+20);
        for(int i = 0;i<m_curveModel->rowCount();i++){
            if(m_curveModel->getModel(i)->visible()){
                QPixmap pix= m_curveModel->getModel(i)->getCurve();
                painter->drawPixmap(tempRect,pix);
                QPixmap pix2= m_curveModel->getModel(i)->getCursor();
                painter->drawPixmap(tempRect2,pix2);
            }
            tempRect.translate(0,-m_verOffset);
            tempRect2.translate(0,-m_verOffset);
        }
    }
}

void MultifunctionPlot::drawByWaterFall(QPainter *painter)
{
    painter->drawPixmap(QPoint(),axisCanvas);
    if(m_curveModel!=nullptr){
        QRect tempRect = axisRect;
        QRect tempRect2 = QRect(axisRect.x()-10,axisRect.y()-10,axisRect.width()+20,axisRect.height()+20);
        int sum = m_curveModel->rowCount();
        qreal a_horOffset = w_horOffset*sum;
        qreal a_verOffset = -w_verOffset*sum;
        tempRect.translate(a_horOffset,a_verOffset);
        tempRect2.translate(a_horOffset,a_verOffset);
        for(int i = 0;i<m_curveModel->rowCount();i++){
            if(m_curveModel->getModel(sum-i-1)->visible()){
                QPixmap pix= m_curveModel->getModel(sum-i-1)->getCurve();
                painter->drawPixmap(tempRect,pix);
                QPixmap pix2= m_curveModel->getModel(sum-i-1)->getCursor();
                painter->drawPixmap(tempRect2,pix2);
            }
            tempRect.translate(-w_horOffset,w_verOffset);
            tempRect2.translate(-w_horOffset,w_verOffset);
        }
    }
}

void MultifunctionPlot::updateAxisCanvas()
{
    if(m_axisModel==nullptr||m_grid==nullptr)
        return;
    if(m_width<=0||m_height<=0)
        return;
    if(m_type==0){
        updateAxisCanvasBySimple();
    }else if(m_type==1){
        updateAxisCanvasByStack();
    }else if(m_type==2){
        updateAxisCanvasByWaterFall();
    }
}

void MultifunctionPlot::updateAxisCanvasBySimple()
{
    QPixmap pix = QPixmap(m_width,m_height); //坐标轴画布
    pix.fill(Qt::transparent);
    if(!pix.paintEngine()){
        return;
    }
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::HighQualityAntialiasing,true);
    if(!painter.isActive())
        return;
    int axisSum = m_axisModel->rowCount();

    /* 开始绘制 */

    initAxisRect(&painter);

    int loffset = 0,roffset = 0,toffset = 0,boffset = 0;
    for(int i = 0;i<axisSum;i++){
        AxisModel* model = m_axisModel->getModel(i);

        switch (model->type()) {
        case AxisModel::AxisType::XAxis1:{
            boffset += axisDrawer.draw(&painter,model,axisRect,boffset,m_grid);
            boffset+=5;
            break;
        }
        case AxisModel::AxisType::XAxis2:{
            toffset += axisDrawer.draw(&painter,model,axisRect,toffset,m_grid);
            toffset+=5;
            break;
        }
        case AxisModel::AxisType::YAxis1:{
            loffset += axisDrawer.draw(&painter,model,axisRect,loffset,m_grid);
            loffset+=5;
            break;
        }
        case AxisModel::AxisType::YAxis2:{
            roffset += axisDrawer.draw(&painter,model,axisRect,roffset,m_grid);
            roffset+=5;
            break;
        }
        }
    }

    axisCanvas = pix;
}

void MultifunctionPlot::updateAxisCanvasByStack()
{
    if(m_curveModel==nullptr||m_height<=0||m_width<=0)
        return;
    QPixmap pix = QPixmap(m_width,m_height); //坐标轴画布
    pix.fill(Qt::transparent);
    if(!pix.paintEngine()){
        return;
    }
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::HighQualityAntialiasing,true);
    if(!painter.isActive())
        return;

    /* 开始绘制 */
    initAxisRect(&painter);
    if(m_curveModel->rowCount()>0){
        QRect tempRect = axisRect;
        for(int i = 0; i<m_curveModel->rowCount(); i++){
            AxisModel *xAxis = m_curveModel->getModel(i)->xAxis();
            AxisModel *yAxis = m_curveModel->getModel(i)->yAxis();
            axisDrawer.draw(&painter,xAxis,tempRect,0,i==0,m_grid);
            axisDrawer.draw(&painter,yAxis,tempRect,0,m_grid);
            painter.setPen(QColor(200,200,200,200));
            painter.drawRect(tempRect);
            tempRect.translate(0,-m_verOffset);
        }
    }else{
        updateAxisCanvasBySimple();
    }

    axisCanvas = pix;
}

void MultifunctionPlot::updateAxisCanvasByWaterFall()
{
    if(m_curveModel==nullptr||m_height<=0||m_width<=0)
        return;
    QPixmap pix = QPixmap(m_width,m_height); //坐标轴画布
    if(!pix.paintEngine()){
        return;
    }
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::HighQualityAntialiasing,true);
    if(!painter.isActive())
        return;

    /* 开始绘制 */
    initAxisRect(&painter);
    QRect tempRect = axisRect;
    if(m_curveModel->rowCount()>0){
        int sum = m_curveModel->rowCount()+1;
        tempRect.translate(w_horOffset*sum,-w_verOffset*sum);
    }
    AxisModel *xAxis = m_axisModel->getFirstX();
    AxisModel *yAxis = m_axisModel->getFirstY();
    axisDrawer.draw(&painter,xAxis,axisRect,tempRect,0,m_grid);
    axisDrawer.draw(&painter,yAxis,axisRect,tempRect,0,m_grid);
    axisCanvas = pix;
}

void MultifunctionPlot::updateCurveCanvas()
{
    if(m_curveModel==nullptr)
        return;
    if(m_width<=0||m_height<=0)
        return;
    curveDrawer.draw(m_curveModel,axisRect);
    /* 暂时注掉，后续需要再添加，未new */
    //    m_labelList->update();
    //    m_multiLabelList->update();
}

void MultifunctionPlot::updateCursorCanvas()
{
    QPixmap pix(m_width,m_height);
    pix.fill(Qt::transparent);
    if(!pix.paintEngine()){
        return;
    }

    QPainter painter(&pix);
    if(!painter.isActive()){
        return;
    }
    if(m_width<=0||m_height<=0)
        return;
    if(m_curveModel==nullptr||m_curveModel->rowCount()==0){
        return;
    }
    QPointF pos;
    bool online;
    QList<TipsData> tdata;
    QRect r = axisRect;
    if(m_type==0||m_type==2){
        pos.setX(cursorPos.x()-axisRect.x());
        pos.setY(cursorPos.y()-axisRect.y());
        for(int i = 0; i<m_curveModel->rowCount(); i++){
            CurveModel* cm = m_curveModel->getModel(i);
            if(!cm->visible()){
                continue;
            }
            QPointF p = cm->drawCursor(pos,online);
            if(online){
                tdata.push_back({cm->lineColor(),p,cm});
            }
        }
    }else{
        int counter = 0;
        while (r.y()>cursorPos.y()){
            r.translate(0,-m_verOffset);
            counter++;
        }
        pos.setX(cursorPos.x()-r.x());
        pos.setY(cursorPos.y()-r.y());
        for(int i = 0; i<m_curveModel->rowCount(); i++){
            CurveModel* cm = m_curveModel->getModel(i);
            if(!cm->visible()){
                continue;
            }
            if(i==counter){
                QPointF p = cm->drawCursor(pos,online);
                if(online){
                    tdata.push_back({cm->lineColor(),p,cm});
                }
            }else{
                cm->clearCursor();
            }
        }
    }

    painter.translate(r.x(),r.y());
    cursorDrawer.drawTips(painter,tdata,pos,r);
    cursorCanvas = pix;
}

void MultifunctionPlot::initAxisRect(QPainter *painter)
{
    if(m_width==0||m_height==0||m_axisModel==nullptr)
        return;
    if(m_type==0){
        initAxisRectBySimple(painter);
    }else if(m_type==1){
        initAxisRectByStack(painter);
    }else if(m_type==2){
        initAxisRectByWaterFall(painter);
    }
    axisRect.setX(axisRect.x()+m_margins.left);
    axisRect.setY(axisRect.y()+m_margins.top);
    axisRect.setWidth(axisRect.width()-m_margins.left-m_margins.right);
    axisRect.setHeight(axisRect.height()-m_margins.top-m_margins.bottom);
}

void MultifunctionPlot::initAxisRectBySimple(QPainter *painter)
{
    QRect tempRect(5,13,m_width-10,m_height-18);
    int axisSum = m_axisModel->rowCount();
    for(int i = 0;i<axisSum;i++){
        AxisModel *model = m_axisModel->getModel(i);
        switch (model->type()) {
        case AxisModel::AxisType::XAxis1:{
            qreal textHeight = model->labelHeight(painter);
            if(model->title()!=""&&!model->title().isEmpty()){
                textHeight*=2;
            }
            textHeight += 5;
            tempRect.setHeight(tempRect.height()-textHeight-model->mainTickLen()-5);
            break;
        }
        case AxisModel::AxisType::XAxis2:{
            qreal textHeight = model->labelHeight(painter);
            if(model->title()!=""&&!model->title().isEmpty()){
                textHeight*=2;
            }
            textHeight += 5;
            tempRect.setY(tempRect.y()+textHeight+model->mainTickLen()+5);
            break;
        }
        case AxisModel::AxisType::YAxis1:{
            qreal textWidth = model->labelWidth(painter);
            if(model->title()!=""&&!model->title().isEmpty()){
                textWidth+=model->labelHeight(painter);
            }
            textWidth += 5;
            tempRect.setX(tempRect.x()+textWidth+model->mainTickLen()+5);
            break;
        }
        case AxisModel::AxisType::YAxis2:{
            qreal textWidth = model->labelWidth(painter);
            if(model->title()!=""&&!model->title().isEmpty()){
                textWidth+=model->labelHeight(painter);
            }
            textWidth += 5;
            tempRect.setWidth(tempRect.width()-textWidth-model->mainTickLen()-5);
            break;
        }
        }
    }
    int v_margin = 10;
    int h_margin = 20;
    tempRect.setX(qMax(h_margin,tempRect.x()));
    tempRect.setY(qMax(v_margin,tempRect.y()));
    if(tempRect.x()+tempRect.width()>m_width-h_margin)
        tempRect.setWidth(m_width-h_margin-tempRect.x());
    if(tempRect.y()+tempRect.height()>m_height-v_margin)
        tempRect.setHeight(m_height-v_margin-tempRect.y());

    if(axisRect!=tempRect){
        axisRect = tempRect;
        emit axisRectChanged();
    }
}

void MultifunctionPlot::initAxisRectByStack(QPainter *painter)
{
    int sum = 1;
    if(m_curveModel!=nullptr)
        sum = m_curveModel->rowCount();
    if(sum<=0)
        sum = 1;
    qreal leftSpacing = 20;
    qreal bottomSpacing = 20;
    QPointF labelRange = m_axisModel->getLableRange(painter);
    leftSpacing = labelRange.x()+15;
    bottomSpacing = labelRange.y()+15;
    qreal m_horOffset = 0;
    qreal space = 20;
    qreal h = (m_height-bottomSpacing-13+space-sum*space)/sum;
    m_verOffset = h+space;
    qreal w =(m_width-leftSpacing-20)-m_horOffset*(sum-1);
    QRect tempRect = QRect(leftSpacing,m_height-h-bottomSpacing,w,h);
    if(axisRect!=tempRect){
        axisRect=tempRect;
        emit axisRectChanged();
    }

    emit stackVerOffsetChanged();
}

void MultifunctionPlot::initAxisRectByWaterFall(QPainter *painter)
{

    if(m_width<=0||m_height<=0)
        return;
    int sum = 1;
    if(m_curveModel!=nullptr)
        sum = m_curveModel->rowCount();
    if(sum<=0)
        sum = 1;

    qreal spacing = 10;
    qreal leftSpacing = 20;
    qreal bottomSpacing = 20;

    QPointF labelRange = m_axisModel->getLableRange(painter);
    leftSpacing = labelRange.x()+15;
    bottomSpacing = labelRange.y()+15;

    qreal h = (m_height-bottomSpacing-spacing)-w_verOffset*(sum+1);
    qreal w =(m_width-leftSpacing-spacing)-w_horOffset*(sum+1);
    QRect tempRect = QRect(leftSpacing,m_height-h-bottomSpacing,w,h);
    if(axisRect!=tempRect){
        axisRect=tempRect;
        emit axisRectChanged();
    }
}

void MultifunctionPlot::setAxisModel(AxisListModel *axisModel)
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


void MultifunctionPlot::setGrid(GridModel *grid)
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

void MultifunctionPlot::setCurveModel(CurveListModel *curveModel)
{
    m_curveModel = curveModel;
    if(m_curveModel==nullptr)
        return;
    m_singleCursor->setCurveModel(curveModel);
    m_doubleCursor->setCurveModel(curveModel);
    m_sideFreqCursor->setCurveModel(curveModel);
    m_singleCursor->setCurveModel(curveModel);
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

void MultifunctionPlot::choose(QRect &rect,int model)
{
    if(m_axisModel==nullptr)
        return;
    QRect r;
    QRect r2 = axisRect;
    if(m_type==0||m_type==2){
        r = rect&axisRect;
    }else{
        while (r2.y()>rect.y()){
            r2.translate(0,-m_verOffset);
        }
        r = rect&r2;
    }
    m_axisModel->zoom(r,r2,model);
    axisUpdate = true;
    curveUpdate = true;
    if(timer&&!timer->isActive()){
        timer->start();
    }
}

void MultifunctionPlot::move(QPoint p1, QPoint p2, int mode)
{
    if(m_axisModel==nullptr)
        return;
    if(m_type==0||m_type==2){
        if(!axisRect.contains(p1)){
            return;
        }
    }else{
        QRect r2 = axisRect;
        while (r2.y()>p1.y()){
            r2.translate(0,-m_verOffset);
        }
        if(!r2.contains(p1)){
            return;
        }
    }
    m_axisModel->moveBySacle(p1,p2,axisRect,mode);
    axisUpdate = true;
    curveUpdate = true;
    if(timer&&!timer->isActive()){
        timer->start();
    }
}


void MultifunctionPlot::addLabel(QPoint pos)
{
    if(m_curveModel==nullptr){
        return;
    }
    if(m_type==0){
        qreal scale = (qreal)(pos.x()-axisRect.x())/axisRect.width();
        QPoint p = QPoint(pos.x()-axisRect.x(),pos.y()-axisRect.y());
        for(int i = 0; i<m_curveModel->rowCount(); i++){
            CurveModel *cm = m_curveModel->getModel(i);
            m_labelList->add(scale,p,cm);
        }
    }else if(m_type==1){
        QRect r2 = axisRect;
        int counter = 0;
        while (r2.y()>pos.y()){
            r2.translate(0,-m_verOffset);
            counter++;
        }
        qreal scale = (qreal)(pos.x()-r2.x())/r2.width();
        QPoint p = QPoint(pos.x()-r2.x(),pos.y()-r2.y());
        if(counter<m_curveModel->rowCount()){
            CurveModel *cm = m_curveModel->getModel(counter);
            m_labelList->add(scale,p,cm);
        }
    }
}

QVariantList MultifunctionPlot::getPointsInfo(QPoint pos)
{
    QVariantList list;
    if(m_curveModel==nullptr){
        return list;
    }
    if(m_type==0){
        qreal scale = (qreal)(pos.x()-axisRect.x())/axisRect.width();
        QPoint p = QPoint(pos.x()-axisRect.x(),pos.y()-axisRect.y());
        for(int i = 0; i<m_curveModel->rowCount(); i++){
            CurveModel *cm = m_curveModel->getModel(i);
            bool flag;
            qreal x = cm->xAxis()->lower()+scale*(cm->xAxis()->upper()-cm->xAxis()->lower());
            QPointF p2 = cm->getNearPointByX(x,p,flag);

            if(flag){
                QVariantMap map;
                map.insert("curve",QVariant::fromValue<CurveModel *>(cm));
                map.insert("point",p2);
                list.push_back(map);
            }
        }
    }else if(m_type==1){
        QRect r2 = axisRect;
        int counter = 0;
        while (r2.y()>pos.y()){
            r2.translate(0,-m_verOffset);
            counter++;
        }
        qreal scale = (qreal)(pos.x()-r2.x())/r2.width();
        QPoint p = QPoint(pos.x()-r2.x(),pos.y()-r2.y());
        if(counter<m_curveModel->rowCount()){
            CurveModel *cm = m_curveModel->getModel(counter);

            bool flag;
            qreal x = cm->xAxis()->lower()+scale*(cm->xAxis()->upper()-cm->xAxis()->lower());
            QPointF p2 = cm->getNearPointByX(x,p,flag);
            if(flag){
                QVariantMap map;
                map.insert("curve",QVariant::fromValue<CurveModel *>(cm));
                map.insert("point",p2);
                list.push_back(map);
            }
        }
    }
    return list;
}

CurveLabelListModel *MultifunctionPlot::labelList() const
{
    return m_labelList;
}

QRect MultifunctionPlot::getChooseArea(QPoint pos1, QPoint pos2)
{
    QRect r1 = QRect(qMin(pos1.x(),pos2.x())
                     ,qMin(pos1.y(),pos2.y())
                     ,qAbs(pos1.x()-pos2.x())
                     ,qAbs(pos1.y()-pos2.y()));

    /*    if(m_type==0||m_type==2){
        return r1&axisRect;
    }else*/{
        QRect r2 = axisRect;

        while (r2.y()>pos1.y()&&m_verOffset>0){
            r2.translate(0,-m_verOffset);
        }
        return r1&r2;
    }

}

bool MultifunctionPlot::inAxisRect(QPoint pos)
{
    if(m_type==0||m_type==2){
        return axisRect.contains(pos);
    }else{
        QRect r2 = axisRect;
        while (r2.y()>pos.y()){
            r2.translate(0,-m_verOffset);
        }
        return r2.contains(pos);
    }
}

void MultifunctionPlot::toSimple()
{
    if(m_type==0)
        return;
    m_type=0;
    axisUpdate = true;
    curveUpdate = true;
    cursorUpdate = true;
    if(timer&&!timer->isActive()){
        timer->start();
    }
    emit typeChanged();
}

void MultifunctionPlot::toStack()
{
    if(m_type==1)
        return;
    m_type=1;
    axisUpdate = true;
    curveUpdate = true;
    cursorUpdate = true;
    if(timer&&!timer->isActive()){
        timer->start();
    }
    emit typeChanged();
    if(timer&&!timer->isActive()){
        timer->start();
    }
}

void MultifunctionPlot::toWaterFall()
{
    if(m_type==2)
        return;
    m_type=2;
    axisUpdate = true;
    curveUpdate = true;
    cursorUpdate = true;
    if(timer&&!timer->isActive()){
        timer->start();
    }
    emit typeChanged();
}

qreal MultifunctionPlot::stackVerOffset() const
{
    return m_verOffset;
}

void MultifunctionPlot::setMargins(qreal left, qreal top, qreal right, qreal bottom)
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

MultiLabelListModel *MultifunctionPlot::multiLabelList() const
{
    return m_multiLabelList;
}

SingleCursorListModel *MultifunctionPlot::singleCursor() const
{
    return m_singleCursor;
}

DoubleCursorListModel *MultifunctionPlot::doubleCursor() const
{
    return m_doubleCursor;
}

MultiCursorListModel *MultifunctionPlot::multiFreqCursor() const
{
    return m_multiFreqCursor;
}

SideCursorListModel *MultifunctionPlot::sideFreqCursor() const
{
    return m_sideFreqCursor;
}

void MultifunctionPlot::initSideFreqCursor(qreal pos, int sum)
{
    m_sideFreqCursor->setXAxisModel( m_axisModel->getFirstX());
    m_sideFreqCursor->initSideFreqCursor(pos,sum);
}

void MultifunctionPlot::initMultiFreqCursor(int index, qreal pos)
{
    m_multiFreqCursor->setXAxisModel(m_axisModel->getFirstX());
    if(m_cursorSetModel==nullptr){
        m_multiFreqCursor->initMultiCursor(index, pos,10);
    }else{
        m_multiFreqCursor->initMultiCursor(index, pos,m_cursorSetModel->multiSum());
    }
}

void MultifunctionPlot::initDoubleCursor(qreal pos)
{
    m_doubleCursor->setXAxisModel(m_axisModel->getFirstX());
    m_doubleCursor->initDoubleCursor(pos);
}


void MultifunctionPlot::initSingleCursor(qreal pos)
{
    m_singleCursor->setXAxisModel(m_axisModel->getFirstX());
    m_singleCursor->initSingleCursor(pos);
}

