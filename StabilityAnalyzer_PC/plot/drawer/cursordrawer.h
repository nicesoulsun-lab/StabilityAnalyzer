#ifndef CURSORDRAWER_H
#define CURSORDRAWER_H

#include <QObject>
#include <QPainter>
#include <QtMath>
#include <QMutex>
#include <QMutexLocker>
#include "../model/curvelistmodel.h"

typedef struct{
    QColor color;
    QPointF point;
    CurveModel* curve;
} TipsData;

typedef struct{
    QColor color;
    QPointF point;
    QPointF coord;
} PointData;

Q_DECLARE_METATYPE(TipsData)

class CursorDrawer : public QObject
{
    Q_OBJECT
public:
    explicit CursorDrawer(QObject *parent = nullptr);

//    void draw(CurveListModel *model, QPointF pos ,QRect &rect);

//    void draw(QPainter &painter,CurveListModel *model, QPointF pos ,QRect &rect);

//    /* 之前版本的移入提示，这个是一条竖线，显示与线相交的点 */
//    void drawLine(QPainter &painter,CurveListModel *model, QPointF pos ,QRect &rect);

    void drawTips(QPainter &painter,QList<TipsData>&tdata,QPointF pos,QRect &rect);

//    /* 这个是算的交点 */
//    QPointF getPointByX(qreal x,QVector<QPointF>* source);

//    /* 这个算的是离得最近的点和交点 */
//    QPointF getNearPointByX(qreal x,CurveModel *model,QVector<QPointF>* source,QPointF pos,bool &onLine);

    void setCardColor(const QColor &cardColor);

    QColor cardColor() const;

    /* 小数保留两位有效数字 */

signals:

private:
    QColor m_lineColor=QColor(100,100,100,150);
    QColor m_cardColor=QColor(255,255,255,255);
    QMutex mutex;
};

#endif // CURSORDRAWER_H
