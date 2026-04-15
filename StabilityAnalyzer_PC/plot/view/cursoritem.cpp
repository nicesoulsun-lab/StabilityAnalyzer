#include "cursoritem.h"

CursorItem::CursorItem(QQuickItem *parent):QQuickPaintedItem(parent)
{

}

void CursorItem::paint(QPainter *painter)
{
    painter->setRenderHint(QPainter::HighQualityAntialiasing);
    QPen pen;
    pen.setWidthF(1);
    pen.setColor(m_color);
    painter->setPen(pen);
    painter->setBrush(m_color);
    painter->translate(0.5,0.5);
    if(m_isMainCursor){
        painter->drawRect(0,0,width(),4);
        painter->drawRect(0,height()-4,width(),4);
    }else{
        painter->drawLine(width()/2.0-4,0,width()/2.0+4,0);
        painter->drawLine(width()/2.0-4,height()-1,width()/2.0+4,height()-1);
        painter->drawLine(width()/2.0-4,0,width()/2.0-4,3);
        painter->drawLine(width()/2.0+4,0,width()/2.0+4,3);
        painter->drawLine(width()/2.0-4,height()-1,width()/2.0-4,height()-4);
        painter->drawLine(width()/2.0+4,height()-1,width()/2.0+4,height()-4);
    }
    if(m_lineType!=0){
        QPen pen;
        pen.setWidthF(1);
        pen.setColor(m_color);
        pen.setDashPattern(QVector<qreal>()<<8<<3<<4<<3);
        painter->setPen(pen);
    }
    painter->drawLine(width()/2.0,0,width()/2.0,height());
}

QColor CursorItem::color() const
{
    return m_color;
}

void CursorItem::setColor(const QColor &color)
{
    m_color = color;
    emit colorChanged();
    update();
}

bool CursorItem::isMainCursor() const
{
    return m_isMainCursor;
}

void CursorItem::setIsMainCursor(bool isMainCursor)
{
    m_isMainCursor = isMainCursor;
    update();
    emit isMainCursorChanged();
}

int CursorItem::lineType() const
{
    return m_lineType;
}

void CursorItem::setLineType(int lineType)
{
    m_lineType = lineType;
    update();
    emit lineTypeChanged();
}
