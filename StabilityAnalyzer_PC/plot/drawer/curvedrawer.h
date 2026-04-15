#ifndef CURVEDRAWER_H
#define CURVEDRAWER_H

#include <QObject>
#include "model/curvelistmodel.h"

class CurveDrawer : public QObject
{
    Q_OBJECT
public:
    explicit CurveDrawer(QObject *parent = nullptr);
    void draw(CurveListModel* curveList,QRect& rect);

signals:

private:
};

#endif // CURVEDRAWER_H
