#ifndef AXISDRAWER_H
#define AXISDRAWER_H

#include <QObject>
#include <QPainter>
#include "../model/axismodel.h"
#include "../model/gridmodel.h"
#include "./tickers.h"
class AxisModel;
class GridModel;

class AxisDrawer : public QObject
{
    Q_OBJECT
public:
    explicit AxisDrawer(QObject *parent = nullptr);

    qreal draw(QPainter *painter,AxisModel *axis,QRect &rect,qreal offset,GridModel* grid = nullptr);

    qreal draw(QPainter *painter,AxisModel *axis,QRect &rect,QRect &rect2,qreal offset,GridModel* grid = nullptr);

    qreal draw(QPainter *painter,AxisModel *axis,QRect &rect,qreal offset,bool showLabel,GridModel* grid = nullptr);

signals:

private:
    QVector<qreal> m_dash;
};

#endif // AXISDRAWER_H
