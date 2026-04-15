#include "axisdrawer.h"
#include <QDebug>
AxisDrawer::AxisDrawer(QObject *parent) : QObject(parent)
{
    m_dash<<3<<4.5;
}

qreal AxisDrawer::draw(QPainter *painter,AxisModel *axis, QRect &rect, qreal offset, GridModel *grid)
{
    if(!painter->isActive()||axis==nullptr)
        return 0;
    QList<QString> textList;
    Tickers ticker;
    QList<qreal> list = ticker.getTickNumber(axis,&textList);//获取刻度文本数组
    QFontMetrics fm = painter->fontMetrics();    //QFontMetrics 计算文本宽高的类

    painter->setPen(QPen(axis->lineColor(),axis->lineWidth()));
    switch (axis->type()) {
    case AxisModel::AxisType::XAxis1:{
        qreal begin = rect.x();
        qreal end = rect.x()+rect.width();
        qreal unitLen = (end-begin)/(axis->upper()-axis->lower());
        qreal subUnitLen = axis->step()/axis->subTickSum()*unitLen;    //小刻度的实际值

        for(int i = 0;i<list.size();i++){
            QString str = textList[i];
            qreal num = list[i];
            qreal xTemp = begin+(num-axis->lower())*unitLen;
            if(axis->subTickVisible()){
                for(int i = 0; i<axis->subTickSum(); i++){
                    qreal pos = xTemp+subUnitLen*i;
                    if(pos>=begin&&pos<=end)
                        painter->drawLine(pos,rect.y()+rect.height()+offset
                                          ,pos,rect.y()+rect.height()+offset-axis->subTickLen());
                }
            }
            if(xTemp>=begin&&xTemp<=end){
                if(axis->labelVisible()){
                    painter->setPen(axis->fontColor());
                    painter->drawText(xTemp-fm.width(str)/2,rect.y()+rect.height()+offset+axis->getSpacing(),fm.width(str)
                                 ,fm.height(),Qt::AlignBottom,str);
                }
            }
            if(xTemp>begin&&xTemp<end){
                if(grid!=nullptr&&grid->xAxis()!=nullptr&&grid->xAxis()==axis){
                    if(grid->verLineType()!=GridModel::LineType::None){
                        QPen pen(grid->lineColor(),grid->lineWidth());
                        if(grid->verLineType()==GridModel::LineType::Dotted)
                            pen.setDashPattern(m_dash);
                        painter->setPen(pen);
                        painter->drawLine(xTemp,rect.y()
                                          ,xTemp,rect.y()+rect.height());
                    }
                }
                painter->setPen(QPen(axis->lineColor(),axis->lineWidth()));
                if(axis->mainTickVisible()){
                    painter->drawLine(xTemp,rect.y()+rect.height()+offset
                                      ,xTemp,rect.y()+rect.height()+offset-axis->mainTickLen());
                }
            }
            if(xTemp>=begin&&xTemp<=end){
                painter->setPen(QPen(axis->lineColor(),axis->lineWidth()));
                if(axis->mainTickVisible()){
                    painter->drawLine(xTemp,rect.y()+rect.height()+offset
                                      ,xTemp,rect.y()+rect.height()+offset-axis->mainTickLen());
                }
            }
        }
        if(axis->axisLineVisible())
            painter->drawLine(begin,rect.y()+rect.height()+offset,end,rect.y()+rect.height()+offset);
        if(axis->title().isNull()||axis->title()==""||!axis->titleVisible())
            return fm.height()+axis->mainTickLen()+axis->getSpacing();
        else{
            painter->drawText(QPointF(begin+(rect.width()-fm.width(axis->title()))/2
                                      ,rect.y()+rect.height()+fm.height()*2+offset+axis->getSpacing()+2),axis->title());
        }

        return fm.height()*2+axis->mainTickLen()+axis->getSpacing()*2;
    }
    case AxisModel::AxisType::XAxis2:{
        qreal begin = rect.x();
        qreal end = rect.x()+rect.width();
        qreal unitLen = (end-begin)/(axis->upper()-axis->lower());
        qreal subUnitLen = axis->step()/axis->subTickSum()*unitLen;    //小刻度的实际值
        for(int i = 0;i<list.size();i++){
            QString str = textList[i];
            qreal num = list[i];
            qreal xTemp = begin+(num-axis->lower())*unitLen;
            if(axis->subTickVisible()){
                for(int i = 0; i<axis->subTickSum(); i++){
                    qreal pos = xTemp+subUnitLen*i;
                    if(pos>=begin&&pos<=end)
                        painter->drawLine(pos,rect.y()-offset
                                          ,pos,rect.y()-offset+axis->subTickLen());
                }
            }
            if(xTemp>=begin&&xTemp<=end){
                if(axis->labelVisible()){
                    painter->setPen(axis->fontColor());
                    painter->drawText(xTemp-fm.width(str)/2,rect.y()-offset-fm.height()-axis->getSpacing(),fm.width(str)
                                 ,fm.height(),Qt::AlignBottom,str);
                }
            }
            if(xTemp>begin&&xTemp<end){
                if(grid!=nullptr&&grid->xAxis()!=nullptr&&grid->xAxis()==axis){
                    if(grid->verLineType()!=GridModel::LineType::None){
                        QPen pen(grid->lineColor(),grid->lineWidth());
                        if(grid->verLineType()==GridModel::LineType::Dotted)
                            pen.setDashPattern(m_dash);
                        painter->setPen(pen);
                        painter->drawLine(xTemp,rect.y()
                                          ,xTemp,rect.y()+rect.height());
                    }
                }
                painter->setPen(QPen(axis->lineColor(),axis->lineWidth()));
                if(axis->mainTickVisible()){
                    painter->drawLine(xTemp,rect.y()-offset
                                      ,xTemp,rect.y()-offset+axis->mainTickLen());
                }
            }
            if(xTemp>=begin&&xTemp<=end){
                painter->setPen(QPen(axis->lineColor(),axis->lineWidth()));
                if(axis->mainTickVisible()){
                    painter->drawLine(xTemp,rect.y()-offset
                                      ,xTemp,rect.y()-offset+axis->mainTickLen());
                }
            }
        }
        if(axis->axisLineVisible())
            painter->drawLine(begin,rect.y()-offset,end,rect.y()-offset);
        if(axis->title().isNull()||axis->title()==""||!axis->titleVisible())
            return fm.height()+axis->mainTickLen()+axis->getSpacing();
        else{
            painter->drawText(QPointF(begin+(rect.width()-fm.width(axis->title()))/2
                                      ,rect.y()-offset-axis->getSpacing()-fm.height()-2),axis->title());
        }
        return fm.height()*2+axis->mainTickLen()+axis->getSpacing()*2;
    }
    case AxisModel::AxisType::YAxis1:{
        qreal begin = rect.y()+rect.height();
        qreal end = rect.y();
        qreal unitLen = (begin-end)/(axis->upper()-axis->lower());
        qreal subUnitLen = axis->step()/axis->subTickSum()*unitLen;    //小刻度的实际值
        qreal tw = 0;
        for(int i = 0;i<list.size();i++){
            QString str = textList[i];
            qreal num = list[i];
            qreal yTemp = begin-(num-axis->lower())*unitLen;
            if(axis->subTickVisible()){
                for(int i = 0; i<axis->subTickSum(); i++){
                    qreal pos = yTemp-subUnitLen*i;
                    if(pos>=end&&pos<=begin)
                        painter->drawLine(rect.x()-offset,pos
                                          ,rect.x()-offset+axis->subTickLen(),pos);
                }
            }
            if(yTemp>=end&&yTemp<=begin){
                if(axis->labelVisible()){
                    painter->setPen(axis->fontColor());
                    qreal w = fm.width(str);
                    tw = qMax(w,tw);
                    painter->drawText(rect.x()-w-axis->getSpacing()-offset,yTemp-fm.height()/2,w
                                 ,fm.height(),Qt::AlignLeft,str);
                }
            }
            if(yTemp>end&&yTemp<begin){
                if(grid!=nullptr&&grid->yAxis()!=nullptr&&grid->yAxis()==axis){
                    if(grid->horLineType()!=GridModel::LineType::None){
                        QPen pen(grid->lineColor(),grid->lineWidth());
                        if(grid->horLineType()==GridModel::LineType::Dotted)
                            pen.setDashPattern(m_dash);
                        painter->setPen(pen);
                        painter->drawLine(rect.x(),yTemp
                                          ,rect.x()+rect.width(),yTemp);
                    }
                }
                painter->setPen(QPen(axis->lineColor(),axis->lineWidth()));
                if(axis->mainTickVisible()){
                    painter->drawLine(rect.x()-offset,yTemp
                                      ,rect.x()-offset+axis->mainTickLen(),yTemp);
                }
            }
            if(yTemp>=end&&yTemp<=begin){
                painter->setPen(QPen(axis->lineColor(),axis->lineWidth()));
                if(axis->mainTickVisible()){
                    painter->drawLine(rect.x()-offset,yTemp
                                      ,rect.x()-offset+axis->mainTickLen(),yTemp);
                }
            }
        }
        if(axis->axisLineVisible())
            painter->drawLine(rect.x()-offset,rect.y(),rect.x()-offset,rect.y()+rect.height());
        if(axis->title().isNull()||axis->title()==""||!axis->titleVisible()){
            return tw+axis->getSpacing()+axis->mainTickLen();
        }else{
            qreal tw2 = fm.width(axis->title());
            painter->translate(rect.x()-offset-2-axis->getSpacing()-tw,rect.y()+rect.height()/2);
            painter->rotate(-90);
            painter->drawText(QPointF(-tw2/2,-fm.height()/2),axis->title());
            painter->resetTransform();
        }
        return  tw+axis->getSpacing()*2+axis->mainTickLen()+fm.height()+4;
    }
    case AxisModel::AxisType::YAxis2:{
        qreal begin = rect.y()+rect.height();
        qreal end = rect.y();
        qreal unitLen = (begin-end)/(axis->upper()-axis->lower());
        qreal subUnitLen = axis->step()/axis->subTickSum()*unitLen;    //小刻度的实际值
        qreal tw = 0;
        for(int i = 0;i<list.size();i++){
            QString str = textList[i];
            qreal num = list[i];
            qreal yTemp = begin-(num-axis->lower())*unitLen;
            if(axis->subTickVisible()){
                for(int i = 0; i<axis->subTickSum(); i++){
                    qreal pos = yTemp-subUnitLen*i;
                    if(pos>=end&&pos<=begin)
                        painter->drawLine(rect.x()+rect.width()+offset,pos
                                          ,rect.x()+rect.width()+offset-axis->subTickLen(),pos);
                }
            }
            if(yTemp>=end&&yTemp<=begin){
                if(axis->labelVisible()){
                    painter->setPen(axis->fontColor());
                    qreal w = fm.width(str);
                    tw = qMax(w,tw);
                    painter->drawText(rect.x()+rect.width()+axis->getSpacing()+offset
                                      ,yTemp-fm.height()/2,w
                                 ,fm.height(),Qt::AlignBottom,str);
                }
            }
            if(yTemp>end&&yTemp<begin){
                if(grid!=nullptr&&grid->yAxis()!=nullptr&&grid->yAxis()==axis){
                    if(grid->horLineType()!=GridModel::LineType::None){
                        QPen pen(grid->lineColor(),grid->lineWidth());
                        if(grid->horLineType()==GridModel::LineType::Dotted)
                            pen.setDashPattern(m_dash);
                        painter->setPen(pen);
                        painter->drawLine(rect.x(),yTemp
                                          ,rect.x()+rect.width(),yTemp);
                    }
                }
                painter->setPen(QPen(axis->lineColor(),axis->lineWidth()));
                if(axis->mainTickVisible()){
                    painter->drawLine(rect.x()+rect.width()+offset,yTemp
                                      ,rect.x()+rect.width()+offset-axis->mainTickLen(),yTemp);
                }
            }
            if(yTemp>=end&&yTemp<=begin){
                painter->setPen(QPen(axis->lineColor(),axis->lineWidth()));
                if(axis->mainTickVisible()){
                    painter->drawLine(rect.x()+rect.width()+offset,yTemp
                                      ,rect.x()+rect.width()+offset-axis->mainTickLen(),yTemp);
                }
            }
        }
        if(axis->axisLineVisible())
            painter->drawLine(rect.x()+rect.width()+offset,rect.y()
                              ,rect.x()+rect.width()+offset,rect.y()+rect.height());
        if(axis->title().isNull()||axis->title()==""||!axis->titleVisible()){
            return tw+axis->getSpacing()+axis->mainTickLen();
        }else{
            qreal tw2 = fm.width(axis->title());
            painter->translate(rect.x()+rect.width()+offset+2+tw+axis->getSpacing(),rect.y()+(rect.height()-tw2)/2);
            painter->rotate(-90);
            painter->drawText(QPointF(-tw2,fm.height()),axis->title());
            painter->resetTransform();
        }
        return tw+axis->getSpacing()+axis->mainTickLen()+fm.height()+4;
    }
    }
    return 0;
}

qreal AxisDrawer::draw(QPainter *painter,AxisModel *axis, QRect &rect, QRect &rect2, qreal offset, GridModel *grid)
{
    Tickers ticker;
    QList<QString> textList;
    QList<qreal> list = ticker.getTickNumber(axis,&textList);//获取刻度文本数组
    QFontMetrics fm = painter->fontMetrics();    //QFontMetrics 计算文本宽高的类
    painter->setPen(QPen(axis->lineColor(),axis->lineWidth()));
    qreal horOffset = rect2.x()-rect.x();
    qreal verOffset = rect.y()-rect2.y();
    switch (axis->type()) {
    case AxisModel::AxisType::XAxis1:{
        qreal begin = rect.x();
        qreal end = rect.x()+rect.width();
        qreal unitLen = (end-begin)/(axis->upper()-axis->lower());
        qreal subUnitLen = axis->step()/axis->subTickSum()*unitLen;    //小刻度的实际值
        for(int i = 0; i<list.size(); i++){
            qreal num = list[i];
            QString str = textList[i];
            qreal xTemp = begin+(num-axis->lower())*unitLen;
            if(axis->subTickVisible()){
                for(int i = 0; i<axis->subTickSum(); i++){
                    qreal pos = xTemp+subUnitLen*i;
                    if(pos>=begin&&pos<=end)
                        painter->drawLine(pos,rect.y()+rect.height()+offset
                                          ,pos,rect.y()+rect.height()+offset-axis->subTickLen());
                }
            }
            if(xTemp>=begin&&xTemp<=end){
                if(axis->labelVisible()){
                    painter->setPen(axis->fontColor());
                    painter->drawText(xTemp-fm.width(str)/2,rect.y()+rect.height()+offset+axis->getSpacing(),fm.width(str)
                                 ,fm.height(),Qt::AlignBottom,str);
                }
            }
            if(xTemp>=begin&&xTemp<=end){
                if(grid->xAxis()!=nullptr&&grid->xAxis()==axis){
                    if(grid->verLineType()!=GridModel::LineType::None){
                        QPen pen(grid->lineColor(),grid->lineWidth());
                        if(grid->verLineType()==GridModel::LineType::Dotted)
                            pen.setDashPattern(m_dash);
                        painter->setPen(pen);
                        painter->drawLine(xTemp+horOffset,rect2.y()
                                          ,xTemp+horOffset,rect2.y()+rect2.height());
                    }
                }
                painter->setPen(QPen(grid->lineColor(),axis->lineWidth()));
                if(axis->mainTickVisible()){
                    painter->drawLine(xTemp,rect.y()+rect.height()+offset
                                      ,xTemp,rect.y()+rect.height()+offset-axis->mainTickLen());
                }
            }
        }
        if(axis->axisLineVisible())
            painter->drawLine(begin,rect.y()+rect.height()+offset,end,rect.y()+rect.height()+offset);
        if(axis->title().isNull()||axis->title()==""||!axis->titleVisible())
            return fm.height()+axis->mainTickLen()+axis->getSpacing();
        else{
            painter->drawText(QPointF(begin+(rect.width()-fm.width(axis->title()))/2
                                      ,rect.y()+rect.height()+fm.height()*2+offset+axis->getSpacing()+2),axis->title());
        }

        return fm.height()*2+axis->mainTickLen()+axis->getSpacing()*2;
    }
    case AxisModel::AxisType::YAxis1:{
        qreal begin = rect.y()+rect.height();
        qreal end = rect.y();
        qreal unitLen = (begin-end)/(axis->upper()-axis->lower());
        qreal subUnitLen = axis->step()/axis->subTickSum()*unitLen;    //小刻度的实际值
        qreal tw = 0;
        for(int i = 0; i<list.size(); i++){
            qreal num = list[i];
            QString str = textList[i];
            qreal yTemp = begin-(num-axis->lower())*unitLen;
            if(axis->subTickVisible()){
                for(int i = 0; i<axis->subTickSum(); i++){
                    qreal pos = yTemp-subUnitLen*i;
                    if(pos>=end&&pos<=begin)
                        painter->drawLine(rect.x()-offset,pos
                                          ,rect.x()-offset+axis->subTickLen(),pos);
                }
            }
            if(yTemp>=end&&yTemp<=begin){
                if(axis->labelVisible()){
                    painter->setPen(axis->fontColor());
                    qreal w = fm.width(str);
                    tw = qMax(w,tw);
                    painter->drawText(rect.x()-w-axis->getSpacing()-offset,yTemp-fm.height()/2,w
                                 ,fm.height(),Qt::AlignLeft,str);
                }
            }
            if(yTemp>=end&&yTemp<=begin){
                if(grid->yAxis()!=nullptr&&grid->yAxis()==axis){
                    if(grid->horLineType()!=GridModel::LineType::None){
                        QPen pen(grid->lineColor(),grid->lineWidth());
                        if(grid->horLineType()==GridModel::LineType::Dotted)
                            pen.setDashPattern(m_dash);
                        painter->setPen(pen);
                        painter->drawLine(rect2.x(),yTemp-verOffset
                                          ,rect2.x()+rect2.width(),yTemp-verOffset);
                        painter->drawLine(rect.x()-offset,yTemp
                                          ,rect.x()-offset+horOffset,yTemp-verOffset);
                    }
                }
                painter->setPen(QPen(grid->lineColor(),axis->lineWidth()));
                if(axis->mainTickVisible()){
                    painter->drawLine(rect.x()-offset,yTemp
                                      ,rect.x()-offset+axis->mainTickLen(),yTemp);
                }
            }
        }
        if(axis->axisLineVisible()){
            painter->drawLine(rect.x()-offset,rect.y(),rect.x()-offset,rect.y()+rect.height());
            painter->drawRect(rect2);
            painter->drawLine(rect.x(),rect.y(),rect2.x(),rect2.y());
            painter->drawLine(rect.x()+rect.width(),rect.y()+rect.height()
                              ,rect2.x()+rect2.width(),rect2.y()+rect2.height());
            painter->drawLine(rect.x(),rect.y()+rect.height()
                              ,rect2.x(),rect2.y()+rect2.height());
        }
        if(axis->title().isNull()||axis->title()==""||!axis->titleVisible()){
            return tw+axis->getSpacing()+axis->mainTickLen();
        }else{
            qreal tw2 = fm.width(axis->title());
            painter->translate(rect.x()-offset-2-axis->getSpacing()-tw,rect.y()+rect.height()/2);
            painter->rotate(-90);
            painter->drawText(QPointF(-tw2/2,-fm.height()/2),axis->title());
            painter->resetTransform();
        }
        return  tw+axis->getSpacing()*2+axis->mainTickLen()+fm.height()+4;
    }
    default:
        break;
    }
    return 0;
}

qreal AxisDrawer::draw(QPainter *painter, AxisModel *axis, QRect &rect, qreal offset, bool showLabel, GridModel *grid)
{
    if(!painter->isActive()||axis==nullptr)
        return 0;
    QList<QString> textList;
    Tickers ticker;
    QList<qreal> list = ticker.getTickNumber(axis,&textList);//获取刻度文本数组
    QFontMetrics fm = painter->fontMetrics();    //QFontMetrics 计算文本宽高的类

    painter->setPen(QPen(axis->lineColor(),axis->lineWidth()));
    switch (axis->type()) {
    case AxisModel::AxisType::XAxis1:{
        qreal begin = rect.x();
        qreal end = rect.x()+rect.width();
        qreal unitLen = (end-begin)/(axis->upper()-axis->lower());
        qreal subUnitLen = axis->step()/axis->subTickSum()*unitLen;    //小刻度的实际值

        for(int i = 0;i<list.size();i++){
            QString str = textList[i];
            qreal num = list[i];
            qreal xTemp = begin+(num-axis->lower())*unitLen;
            if(axis->subTickVisible()){
                for(int i = 0; i<axis->subTickSum(); i++){
                    qreal pos = xTemp+subUnitLen*i;
                    if(pos>=begin&&pos<=end)
                        painter->drawLine(pos,rect.y()+rect.height()+offset
                                          ,pos,rect.y()+rect.height()+offset-axis->subTickLen());
                }
            }
            if(xTemp>=begin&&xTemp<=end){
                if(showLabel){
                    painter->setPen(axis->fontColor());
                    painter->drawText(xTemp-fm.width(str)/2,rect.y()+rect.height()+offset+axis->getSpacing(),fm.width(str)
                                 ,fm.height(),Qt::AlignBottom,str);
                }
            }
            if(xTemp>begin&&xTemp<end){
                if(grid!=nullptr&&grid->xAxis()!=nullptr&&grid->xAxis()==axis){
                    if(grid->verLineType()!=GridModel::LineType::None){
                        QPen pen(grid->lineColor(),grid->lineWidth());
                        if(grid->verLineType()==GridModel::LineType::Dotted)
                            pen.setDashPattern(m_dash);
                        painter->setPen(pen);
                        painter->drawLine(xTemp,rect.y()
                                          ,xTemp,rect.y()+rect.height());
                    }
                }
                painter->setPen(QPen(axis->lineColor(),axis->lineWidth()));
                if(axis->mainTickVisible()){
                    painter->drawLine(xTemp,rect.y()+rect.height()+offset
                                      ,xTemp,rect.y()+rect.height()+offset-axis->mainTickLen());
                }
            }
            if(xTemp>=begin&&xTemp<=end){
                painter->setPen(QPen(axis->lineColor(),axis->lineWidth()));
                if(axis->mainTickVisible()){
                    painter->drawLine(xTemp,rect.y()+rect.height()+offset
                                      ,xTemp,rect.y()+rect.height()+offset-axis->mainTickLen());
                }
            }
        }
        if(axis->axisLineVisible())
            painter->drawLine(begin,rect.y()+rect.height()+offset,end,rect.y()+rect.height()+offset);
        if(axis->title().isNull()||axis->title()==""||!axis->titleVisible()||!showLabel)
            return fm.height()+axis->mainTickLen()+axis->getSpacing();
        else{
            painter->drawText(QPointF(begin+(rect.width()-fm.width(axis->title()))/2
                                      ,rect.y()+rect.height()+fm.height()*2+offset+axis->getSpacing()+2),axis->title());
        }

        return fm.height()*2+axis->mainTickLen()+axis->getSpacing()*2;
    }
    case AxisModel::AxisType::XAxis2:{
        qreal begin = rect.x();
        qreal end = rect.x()+rect.width();
        qreal unitLen = (end-begin)/(axis->upper()-axis->lower());
        qreal subUnitLen = axis->step()/axis->subTickSum()*unitLen;    //小刻度的实际值
        for(int i = 0;i<list.size();i++){
            QString str = textList[i];
            qreal num = list[i];
            qreal xTemp = begin+(num-axis->lower())*unitLen;
            if(axis->subTickVisible()){
                for(int i = 0; i<axis->subTickSum(); i++){
                    qreal pos = xTemp+subUnitLen*i;
                    if(pos>=begin&&pos<=end)
                        painter->drawLine(pos,rect.y()-offset
                                          ,pos,rect.y()-offset+axis->subTickLen());
                }
            }
            if(xTemp>=begin&&xTemp<=end){
                if(showLabel){
                    painter->setPen(axis->fontColor());
                    painter->drawText(xTemp-fm.width(str)/2,rect.y()-offset-fm.height()-axis->getSpacing(),fm.width(str)
                                 ,fm.height(),Qt::AlignBottom,str);
                }
            }
            if(xTemp>begin&&xTemp<end){
                if(grid!=nullptr&&grid->xAxis()!=nullptr&&grid->xAxis()==axis){
                    if(grid->verLineType()!=GridModel::LineType::None){
                        QPen pen(grid->lineColor(),grid->lineWidth());
                        if(grid->verLineType()==GridModel::LineType::Dotted)
                            pen.setDashPattern(m_dash);
                        painter->setPen(pen);
                        painter->drawLine(xTemp,rect.y()
                                          ,xTemp,rect.y()+rect.height());
                    }
                }
                painter->setPen(QPen(axis->lineColor(),axis->lineWidth()));
                if(axis->mainTickVisible()){
                    painter->drawLine(xTemp,rect.y()-offset
                                      ,xTemp,rect.y()-offset+axis->mainTickLen());
                }
            }
            if(xTemp>=begin&&xTemp<=end){
                painter->setPen(QPen(axis->lineColor(),axis->lineWidth()));
                if(axis->mainTickVisible()){
                    painter->drawLine(xTemp,rect.y()-offset
                                      ,xTemp,rect.y()-offset+axis->mainTickLen());
                }
            }
        }
        if(axis->axisLineVisible())
            painter->drawLine(begin,rect.y()-offset,end,rect.y()-offset);
        if(axis->title().isNull()||axis->title()==""||!axis->titleVisible()||!showLabel)
            return fm.height()+axis->mainTickLen()+axis->getSpacing();
        else{
            painter->drawText(QPointF(begin+(rect.width()-fm.width(axis->title()))/2
                                      ,rect.y()-offset-axis->getSpacing()-fm.height()-2),axis->title());
        }
        return fm.height()*2+axis->mainTickLen()+axis->getSpacing()*2;
    }
    case AxisModel::AxisType::YAxis1:{
        qreal begin = rect.y()+rect.height();
        qreal end = rect.y();
        qreal unitLen = (begin-end)/(axis->upper()-axis->lower());
        qreal subUnitLen = axis->step()/axis->subTickSum()*unitLen;    //小刻度的实际值
        qreal tw = 0;
        for(int i = 0;i<list.size();i++){
            QString str = textList[i];
            qreal num = list[i];
            qreal yTemp = begin-(num-axis->lower())*unitLen;
            if(axis->subTickVisible()){
                for(int i = 0; i<axis->subTickSum(); i++){
                    qreal pos = yTemp-subUnitLen*i;
                    if(pos>=end&&pos<=begin)
                        painter->drawLine(rect.x()-offset,pos
                                          ,rect.x()-offset+axis->subTickLen(),pos);
                }
            }
            if(yTemp>=end&&yTemp<=begin){
                if(showLabel){
                    painter->setPen(axis->fontColor());
                    qreal w = fm.width(str);
                    tw = qMax(w,tw);
                    painter->drawText(rect.x()-w-axis->getSpacing()-offset,yTemp-fm.height()/2,w
                                 ,fm.height(),Qt::AlignLeft,str);
                }
            }
            if(yTemp>end&&yTemp<begin){
                if(grid!=nullptr&&grid->yAxis()!=nullptr&&grid->yAxis()==axis){
                    if(grid->horLineType()!=GridModel::LineType::None){
                        QPen pen(grid->lineColor(),grid->lineWidth());
                        if(grid->horLineType()==GridModel::LineType::Dotted)
                            pen.setDashPattern(m_dash);
                        painter->setPen(pen);
                        painter->drawLine(rect.x(),yTemp
                                          ,rect.x()+rect.width(),yTemp);
                    }
                }
                painter->setPen(QPen(axis->lineColor(),axis->lineWidth()));
                if(axis->mainTickVisible()){
                    painter->drawLine(rect.x()-offset,yTemp
                                      ,rect.x()-offset+axis->mainTickLen(),yTemp);
                }
            }
            if(yTemp>=end&&yTemp<=begin){
                painter->setPen(QPen(axis->lineColor(),axis->lineWidth()));
                if(axis->mainTickVisible()){
                    painter->drawLine(rect.x()-offset,yTemp
                                      ,rect.x()-offset+axis->mainTickLen(),yTemp);
                }
            }
        }
        if(axis->axisLineVisible())
            painter->drawLine(rect.x()-offset,rect.y(),rect.x()-offset,rect.y()+rect.height());
        if(axis->title().isNull()||axis->title()==""||!axis->titleVisible()||!showLabel){
            return tw+axis->getSpacing()+axis->mainTickLen();
        }else{
            qreal tw2 = fm.width(axis->title());
            painter->translate(rect.x()-offset-2-axis->getSpacing()-tw,rect.y()+rect.height()/2);
            painter->rotate(-90);
            painter->drawText(QPointF(-tw2/2,-fm.height()/2),axis->title());
            painter->resetTransform();
        }
        return  tw+axis->getSpacing()*2+axis->mainTickLen()+fm.height()+4;
    }
    case AxisModel::AxisType::YAxis2:{
        qreal begin = rect.y()+rect.height();
        qreal end = rect.y();
        qreal unitLen = (begin-end)/(axis->upper()-axis->lower());
        qreal subUnitLen = axis->step()/axis->subTickSum()*unitLen;    //小刻度的实际值
        qreal tw = 0;
        for(int i = 0;i<list.size();i++){
            QString str = textList[i];
            qreal num = list[i];
            qreal yTemp = begin-(num-axis->lower())*unitLen;
            if(axis->subTickVisible()){
                for(int i = 0; i<axis->subTickSum(); i++){
                    qreal pos = yTemp-subUnitLen*i;
                    if(pos>=end&&pos<=begin)
                        painter->drawLine(rect.x()+rect.width()+offset,pos
                                          ,rect.x()+rect.width()+offset-axis->subTickLen(),pos);
                }
            }
            if(yTemp>=end&&yTemp<=begin){
                if(showLabel){
                    painter->setPen(axis->fontColor());
                    qreal w = fm.width(str);
                    tw = qMax(w,tw);
                    painter->drawText(rect.x()+rect.width()+axis->getSpacing()+offset
                                      ,yTemp-fm.height()/2,w
                                 ,fm.height(),Qt::AlignBottom,str);
                }
            }
            if(yTemp>end&&yTemp<begin){
                if(grid!=nullptr&&grid->yAxis()!=nullptr&&grid->yAxis()==axis){
                    if(grid->horLineType()!=GridModel::LineType::None){
                        QPen pen(grid->lineColor(),grid->lineWidth());
                        if(grid->horLineType()==GridModel::LineType::Dotted)
                            pen.setDashPattern(m_dash);
                        painter->setPen(pen);
                        painter->drawLine(rect.x(),yTemp
                                          ,rect.x()+rect.width(),yTemp);
                    }
                }
                painter->setPen(QPen(axis->lineColor(),axis->lineWidth()));
                if(axis->mainTickVisible()){
                    painter->drawLine(rect.x()+rect.width()+offset,yTemp
                                      ,rect.x()+rect.width()+offset-axis->mainTickLen(),yTemp);
                }
            }
            if(yTemp>=end&&yTemp<=begin){
                painter->setPen(QPen(axis->lineColor(),axis->lineWidth()));
                if(axis->mainTickVisible()){
                    painter->drawLine(rect.x()+rect.width()+offset,yTemp
                                      ,rect.x()+rect.width()+offset-axis->mainTickLen(),yTemp);
                }
            }
        }
        if(axis->axisLineVisible())
            painter->drawLine(rect.x()+rect.width()+offset,rect.y()
                              ,rect.x()+rect.width()+offset,rect.y()+rect.height());
        if(axis->title().isNull()||axis->title()==""||!axis->titleVisible()||!showLabel){
            return tw+axis->getSpacing()+axis->mainTickLen();
        }else{
            qreal tw2 = fm.width(axis->title());
            painter->translate(rect.x()+rect.width()+offset+2+tw+axis->getSpacing(),rect.y()+(rect.height()-tw2)/2);
            painter->rotate(-90);
            painter->drawText(QPointF(-tw2,fm.height()),axis->title());
            painter->resetTransform();
        }
        return tw+axis->getSpacing()+axis->mainTickLen()+fm.height()+4;
    }
    }
    return 0;
}
