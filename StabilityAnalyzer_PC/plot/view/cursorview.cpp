#include "cursorview.h"

CursorView::CursorView(QObject *parent) : QObject(parent)
{

}

void CursorView::init()
{
    m_cursor1 = QPixmap(m_width+2,m_height);
    m_cursor2 = QPixmap(m_width+2,m_height);
    m_cursor3 = QPixmap(m_width+2,m_height);
    m_cursor4 = QPixmap(m_width+2,m_height);
    m_chooseRect = QPixmap(m_width+2,m_height);
    paintCursor(m_cursor1, 1);
    paintCursor(m_cursor2, 2);
    paintCursor(m_cursor3, 3);
    paintCursor(m_cursor4, 4);
    paintCursor(m_chooseRect, 5);
}

qreal CursorView::height() const
{
    return m_height;
}

void CursorView::setHeight(const qreal &height)
{
    m_height = height;
}

QColor CursorView::color() const
{
    return m_color;
}

void CursorView::setColor(const QColor &color)
{
    m_color = color;
}

void CursorView::paintCursor(QPixmap &pix, int type)
{
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::HighQualityAntialiasing);
    painter.translate(1,0);
    QPen pen;
    pen.setWidthF(0.8);
    pen.setColor(m_color);
    painter.setPen(pen);
    painter.setBrush(m_color);
    if(type==5){
        painter.setBrush(QColor(m_color.red(),m_color.green(),m_color.blue(),100));
    }
    if(type==3||type==4){
        painter.drawRect(0,0,m_width,m_height);
        return;
    }else{
        painter.drawLine(m_width/2.0-5,0,m_width/2.0+5,0);
        painter.drawLine(m_width/2.0-5,m_height-1,m_width/2.0+5,m_height-1);
        painter.drawLine(m_width/2.0-5,0,m_width/2.0-5,3);
        painter.drawLine(m_width/2.0+5,0,m_width/2.0+5,3);
        painter.drawLine(m_width/2.0-5,m_height-1,m_width/2.0-5,m_height-4);
        painter.drawLine(m_width/2.0+5,m_height-1,m_width/2.0+5,m_height-4);
    }
    if(type==2||type==4){
        QPen pen;
        pen.setWidthF(0.8);
        pen.setColor(m_color);
        pen.setDashPattern(QVector<qreal>()<<8<<3<<4<<3);
        painter.setPen(pen);
    }
    painter.drawLine(m_width/2.0,0,m_width/2.0,m_height);
}
