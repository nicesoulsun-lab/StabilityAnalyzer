#include "cursordrawer.h"

CursorDrawer::CursorDrawer(QObject *parent) : QObject(parent)
{

}

//void CursorDrawer::draw(CurveListModel *model, QPointF pos, QRect &rect)
//{
//    if(model==nullptr&&model->rowCount()==0)
//        return;
//    QList<TipsData> tdata;
//    bool flag;
//    for(int i = 0; i<model->rowCount();i++){
//        CurveModel* cm = model->getModel(i);
//        QPointF p = cm->drawCursor(pos,flag);
////        if(cm->xAxis()==nullptr||cm->yAxis()==nullptr||!cm->tipsEnable()||!cm->visible())
////            continue;
////        qreal xUnitLen = rect.width()/(cm->xAxis()->upper()-cm->xAxis()->lower());
////        if(xUnitLen<=0)
////            continue;
////        bool online = false;
//////        QPainter painter(cm->getCursor());

////        QPointF p = getNearPointByX(cm->xAxis()->lower()+pos.x()/xUnitLen,cm,cm->source(),pos,online);
////        if(online){
////            tdata.push_back({cm->lineColor(),p});
////            painter.setPen(cm->lineColor());
////            painter.setBrush(cm->lineColor());
////            QPointF rp = cm->transformCoords(p);
////            painter.drawRoundedRect(rp.x()-2.5,rp.y()-2.5,5,5,5,5);
////        }
//    }
////    if(!tdata.isEmpty()){
////        drawTips(painter,tdata,pos,rect);
////    }
//}

//void CursorDrawer::draw(QPainter &painter, CurveListModel *model, QPointF pos,QRect &rect)
//{
//    if(model==nullptr&&model->rowCount()==0)
//        return;
//    QList<TipsData> tdata;
//    for(int i = 0; i<model->rowCount();i++){
//        CurveModel* cm = model->getModel(i);
//        if(cm->xAxis()==nullptr||cm->yAxis()==nullptr||!cm->tipsEnable()||!cm->visible())
//            continue;
//        qreal xUnitLen = rect.width()/(cm->xAxis()->upper()-cm->xAxis()->lower());
//        if(xUnitLen<=0)
//            continue;
//        bool online = false;
//        QPointF p = getNearPointByX(cm->xAxis()->lower()+pos.x()/xUnitLen,cm,cm->source(),pos,online);
//        if(online){
//            tdata.push_back({cm->lineColor(),p});
//            painter.setPen(cm->lineColor());
//            painter.setBrush(cm->lineColor());
//            QPointF rp = cm->transformCoords(p);
//            painter.drawRoundedRect(rp.x()-2.5,rp.y()-2.5,5,5,5,5);
//        }
//    }
//    if(!tdata.isEmpty()){
//        drawTips(painter,tdata,pos,rect);
//    }
//}

//void CursorDrawer::drawLine(QPainter &painter, CurveListModel *model, QPointF pos, QRect &rect)
//{
//    if(model==nullptr&&model->rowCount()==0)
//        return;
//    QPen p(m_lineColor,1);
//    p.setDashPattern(QVector<qreal>()<<8<<3<<4<<3);
//    painter.setPen(p);
//    painter.drawLine(pos.x()-0.5,0,pos.x()-0.5,rect.height());
//    QList<TipsData>tdata;
//    for(int i = 0; i<model->rowCount();i++){
//        CurveModel* cm = model->getModel(i);
//        if(cm->xAxis()==nullptr||cm->yAxis()==nullptr||!cm->tipsEnable()||!cm->visible())
//            continue;
//        qreal xUnitLen = rect.width()/(cm->xAxis()->upper()-cm->xAxis()->lower());
//        qreal yUnitLen = rect.height()/(cm->yAxis()->upper()-cm->yAxis()->lower());
//        if(xUnitLen<=0)
//            continue;
//        QPointF p = getPointByX(cm->xAxis()->lower()+pos.x()/xUnitLen,cm->source());
//        if(p.isNull())
//            continue;
//        QPointF rp = QPointF(pos.x(),(cm->yAxis()->upper()-p.y())*yUnitLen);
//        tdata.push_back({cm->lineColor(),p});
//        painter.setPen(cm->lineColor());
//        painter.setBrush(cm->lineColor());
//        painter.drawRoundedRect(rp.x()-2.5,rp.y()-2.5,5,5,5,5);
//    }
//    drawTips(painter,tdata,pos,rect);
//}

void CursorDrawer::drawTips(QPainter &painter, QList<TipsData>&tdata, QPointF pos, QRect &rect)
{
    if(tdata.isEmpty())
        return;
    QPixmap pix = QPixmap(rect.width(),rect.height());
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing,true);
    p.setPen(painter.pen());
    p.setFont(QFont("微软雅黑",11));
    QFontMetrics fm = p.fontMetrics();
    int maxWidth = 0;
    int h = fm.height();
    int verOff = 5;
    qreal vc = (h-8)/2;
    foreach(TipsData t, tdata){
        p.setPen(t.color);
        p.setBrush(t.color);
        p.drawRoundedRect(5,verOff+vc,8,8,8,8);
        int w = 20;
        QString x = QString::number(t.point.x());
        QString y = QString::number(t.point.y());
        p.setPen(Qt::black);
        if(t.curve!=nullptr){
            if(t.curve->xAxis()&&t.curve->xAxis()->labelType()==2){
                QDateTime date = QDateTime::fromTime_t(t.point.x());
                x = date.toString("yyyy/MM/dd hh:mm:ss");
            }
        }
        p.drawText(QPointF(w,verOff+h-2.5),"X");
        w+=fm.width("X")+5;
        p.setPen(QColor(75,129,240));
        p.drawText(QPointF(w,verOff+h-2.5),x);
        w+=fm.width(x)+10;
        p.setPen(Qt::black);
        p.drawText(QPointF(w,verOff+h-2.5),"Y");
        w+=fm.width("Y")+5;
        p.setPen(QColor(75,129,240));
        p.drawText(QPointF(w,verOff+h-2.5),y);
        w+=fm.width(y)+5;
        maxWidth = qMax(maxWidth,w);
        verOff+=h+3;
    }
    verOff+=2;
    painter.setPen(m_lineColor);
    painter.setBrush(m_cardColor);
    qreal x=pos.x()+10,y=pos.y()-verOff/2;

    if(pos.x()+maxWidth+10>rect.width()){
        if(pos.x()<maxWidth){
            x = 5;
        }else{
            x = pos.x()-10-maxWidth;
        }
    }
    if(pos.y()<verOff/2)
        y = 0;
    if(pos.y()+verOff/2>rect.height())
        y = rect.height()-verOff;
    painter.drawRoundedRect(x,y,maxWidth,verOff,3,3);
    painter.drawPixmap(QPointF(x,y),pix);
}

//QPointF CursorDrawer::getPointByX(qreal x,QVector<QPointF>* source)
//{
//    int begin = 0;
//    int end = source->size()-1;
//    if(source->isEmpty())
//        return QPointF(0,0);
//    /* 二分法查询 */
//    while (begin<end) {
//        int mid = (end+begin)/2;
//        if(source->at(mid).x()== x){
//            return source->at(mid);
//        }else if(source->at(mid).x()<x){
//            begin = mid+1;
//        }else{
//            end = mid-1;
//        }
//    }

//    QPointF small,big;
//    if(source->at(begin).x()>x){
//        if(begin==0)
//            return QPointF(x,source->at(begin).y());
//        big = source->at(begin);
//        small = source->at(begin-1);
//    }else{
//        if(begin==source->size()-1)
//            return QPointF(x,source->at(begin).y());
//        small = source->at(begin);
//        big = source->at(begin+1);
//    }
//    qreal scale =(x-small.x())
//                  /(big.x()-small.x());

//    qreal y = (big.y()-small.y())*scale+small.y();
//    return QPointF(x,y);
//}

//QPointF CursorDrawer::getNearPointByX(qreal x,CurveModel *model,QVector<QPointF>* source,QPointF pos,bool &onLine)
//{
//    int begin = 0;
//    int end = source->size()-1;
//    if(source->isEmpty())
//        return QPointF(0,0);
//    /* 二分法查询 */
//    while (begin<end) {
//        int mid = (end+begin)/2;
//        if(source->at(mid).x()== x){
//            onLine = true;
//            return source->at(mid);
//        }else if(source->at(mid).x()<x){
//            begin = mid+1;
//        }else{
//            end = mid-1;
//        }
//    }

//    QPointF small,big;
//    if(source->at(begin).x()>x){
//        if(begin==0){
//            onLine = false;
//            return QPointF(x,source->at(begin).y());
//        }
//        big = source->at(begin);
//        small = source->at(begin-1);
//    }else{
//        if(begin==source->size()-1){
//            onLine = false;
//            return QPointF(x,source->at(begin).y());
//        }
//        small = source->at(begin);
//        big = source->at(begin+1);
//    }

//    QPointF p1 = model->transformCoords(small);
//    QPointF p2 = model->transformCoords(big);

//    qreal k = (p2.y()-p1.y())/(p2.x()-p1.x());
//    qreal b = p1.y()-k*p1.x();

//    qreal dis = qAbs(k*pos.x()-pos.y()+b)/qSqrt(k*k+1);
//    onLine = dis<8;

//    if(big.x()-x>x-small.x())
//        return small;
//    else
//        return big;
//}

void CursorDrawer::setCardColor(const QColor &cardColor)
{
    m_cardColor = cardColor;
}

QColor CursorDrawer::cardColor() const
{
    return m_cardColor;
}
