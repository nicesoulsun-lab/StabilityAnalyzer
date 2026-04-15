#include "curvedrawer.h"
#include <QtMath>

CurveDrawer::CurveDrawer(QObject *parent) : QObject(parent)
{
}

void CurveDrawer::draw(CurveListModel *curveList,QRect& rect)
{
    for(int i = 0; i<curveList->rowCount(); i++){
        CurveModel* model= curveList->getModel(i);
        if(model->xAxis()==nullptr||model->yAxis()==nullptr||(!model->visible()))
            continue;

        model->setWidth(rect.width());
        model->setHeight(rect.height());
        model->draw();
    }
}




