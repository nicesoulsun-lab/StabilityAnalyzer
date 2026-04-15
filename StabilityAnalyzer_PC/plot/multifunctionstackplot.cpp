#include "multifunctionstackplot.h"

MultifunctionStackPlot::MultifunctionStackPlot(QObject* parent):BasePlot(parent)
{
    setMargins(5,5,5,5);
}

MultifunctionStackPlot::~MultifunctionStackPlot()
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

void MultifunctionStackPlot::draw()
{
    if(m_width==0||m_height==0){
        return;
    }
    if(!m_visible){
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
        painter.drawPixmap(QPointF(),axisCanvas);
        painter.drawPixmap(QPointF(),cursorCanvas);
        if(m_curveModel){
            for(int i = 0; i<m_curveModel->rowCount(); i++){
                if(m_curveModel->getModel(i)->visible()){
                    painter.drawPixmap(axisRect,m_curveModel->getModel(i)->getCurve());
                    painter.translate(0,axisRect.height());
                }
            }
        }

        setCanvas(map);
        emit plotChanged();
    }
}

void MultifunctionStackPlot::updateAxisCanvas()
{
    if(m_curveModel==nullptr||m_width<=0||m_height<=0)
        return;

    QPixmap pix = QPixmap(m_width,m_height); //坐标轴画布
    pix.fill(Qt::transparent);
    if(!pix.paintEngine()){
        return;
    }
    QPainter painter(&pix);
    if(!painter.isActive())
        return;
    QFont f;
    f.setPixelSize(12);
    f.setFamily("微软雅黑");
    painter.setFont(f);
    painter.setRenderHint(QPainter::HighQualityAntialiasing,true);

    /* 开始绘制 */
    initAxisRect(&painter);
    QPen p(Qt::red);
    p.setWidthF(0.3);
    p.setDashPattern(QVector<qreal>()<<6<<4.5);
    painter.setPen(p);
    for(int i = 0; i<m_curveModel->rowCount(); i++){
        if(m_curveModel->getModel(i)->visible()){
            painter.setPen(p);
            painter.drawLine(axisRect.x(),axisRect.y()+axisRect.height()/2
                             ,axisRect.x()+axisRect.width(),axisRect.y()+axisRect.height()/2);
            painter.setPen(QColor(45,45,45));
            painter.drawText(axisRect.x()-5-painter.fontMetrics().width(m_curveModel->getModel(i)->title())
                            ,axisRect.height()/2+axisRect.y()+painter.fontMetrics().height()/4
                            ,m_curveModel->getModel(i)->title());
            painter.translate(0,axisRect.height());
        }
    }
    axisCanvas = pix;
}

void MultifunctionStackPlot::updateCurveCanvas()
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

void MultifunctionStackPlot::updateCursorCanvas()
{
//    if(m_width<=0||m_height<=0)
//        return;
//    if(m_curveModel==nullptr||m_curveModel->rowCount()==0){
//        return;
//    }
//    QPointF pos;
//    bool online;
//    QList<TipsData> tdata;
//    QRect r = axisRect;
//    pos.setX(cursorPos.x()-axisRect.x());
//    pos.setY(cursorPos.y()-axisRect.y());
//    for(int i = 0; i<m_curveModel->rowCount(); i++){
//        CurveModel* cm = m_curveModel->getModel(i);
//        if(!cm->visible()){
//            continue;
//        }
//        QPointF p = cm->drawCursor(pos,online);
//        if(online){
//            tdata.push_back({cm->lineColor(),p,cm});
//        }
//    }
//    QPixmap pix(m_width,m_height);
//    pix.fill(Qt::transparent);
//    QPainter painter(&pix);
//    painter.translate(r.x(),r.y());
//    cursorDrawer.drawTips(painter,tdata,pos,r);
//    cursorCanvas = pix;
}

void MultifunctionStackPlot::initAxisRect(QPainter *painter)
{
    if(m_axisModel==nullptr||m_grid==nullptr)
        return;
    if(m_curveModel==nullptr||m_width<=0||m_height<=0)
        return;
//    QPixmap pix = QPixmap(m_width,m_height); //坐标轴画布
//    pix.fill(Qt::transparent);
    if(!painter->isActive())
        return;
    painter->setRenderHint(QPainter::HighQualityAntialiasing,true);

    int sum = 0;
    int textWidth = 0;
    QFontMetrics fm = painter->fontMetrics();
    for(int i = 0; i<m_curveModel->rowCount(); i++){
        if(curveModel()->getModel(i)->visible()){
            textWidth = qMax(fm.width(curveModel()->getModel(i)->title()),textWidth);
            sum++;
        }
    }
    if(sum>0){
        int w = m_width-m_margins.left-m_margins.right-textWidth-5;
        int h = m_height-m_margins.top-m_margins.bottom-2;
        h/=sum;
        axisRect = QRect(m_margins.left+textWidth+5,m_margins.top,w,h);
    }else{
        axisRect = QRect();
    }
}
