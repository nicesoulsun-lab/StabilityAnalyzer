#ifndef MULTIFUNCTIONSTACKPLOT_H
#define MULTIFUNCTIONSTACKPLOT_H

#include "baseplot.h"

/*多功能图表*/
class MultifunctionStackPlot : public BasePlot
{
    Q_OBJECT
public:
    MultifunctionStackPlot(QObject* parent = nullptr);
    ~MultifunctionStackPlot();

    /* 绘制接口 */
    void draw();

    /* 更新坐标轴画布 */
    void updateAxisCanvas();

    /* 更新曲线画布 */
    void updateCurveCanvas();

    /* 更新游标画布 */
    void updateCursorCanvas();

    void initAxisRect(QPainter *painter);
signals:

private:


};

#endif // MULTIFUNCTIONSTACKPLOT_H
