#ifndef MULTIFUNCTIONPLOT_H
#define MULTIFUNCTIONPLOT_H

#include "baseplot.h"
#include "model/cursormodel.h"
#include "model/cursorlistmodel.h"
#include "model/sidecursormodel.h"
#include "model/multicursormodel.h"
#include "model/doublecursormodel.h"
#include "model/singlecursormodel.h"
#include "model/multilabelmodel.h"
#include "model/cursorsetmodel.h"

/*多功能图表*/
class MultifunctionPlot : public BasePlot
{
    Q_OBJECT
public:
    MultifunctionPlot(QObject* parent = nullptr);
    ~MultifunctionPlot();

    /* 绘制接口 */
    void draw();
    void drawBySimple(QPainter *painter);
    void drawByStack(QPainter *painter);
    void drawByWaterFall(QPainter *painter);


    /* 更新坐标轴画布 */
    void updateAxisCanvas();
    void updateAxisCanvasBySimple();
    void updateAxisCanvasByStack();
    void updateAxisCanvasByWaterFall();

    /* 更新曲线画布 */
    void updateCurveCanvas();

    /* 更新游标画布 */
    void updateCursorCanvas();

    /* 初始化轴矩形 */
    void initAxisRect(QPainter *painter);
    void initAxisRectBySimple(QPainter *painter);       //普通
    void initAxisRectByStack(QPainter *painter);        //堆栈
    void initAxisRectByWaterFall(QPainter *painter);    //瀑布

    virtual void setAxisModel(AxisListModel *axisModel);

    void setGrid(GridModel *grid);

    void setCurveModel(CurveListModel *curveModel);

    /* 视图切换 */
    void toSimple();
    void toStack();
    void toWaterFall();

    void choose(QRect &rect,int model);
    void move(QPoint p1, QPoint p2, int mode);

    qreal stackVerOffset() const;

    void setMargins(qreal left,qreal top,qreal right,qreal bottom);


    MultiLabelListModel *multiLabelList() const;

    SingleCursorListModel *singleCursor() const;

    DoubleCursorListModel *doubleCursor() const;

    MultiCursorListModel *multiFreqCursor() const;

    SideCursorListModel *sideFreqCursor() const;

    /* 边频游标 */
    Q_INVOKABLE void initSideFreqCursor(qreal pos,int sum); //初始化

    /* 倍频游标 */
    Q_INVOKABLE void initMultiFreqCursor(int index, qreal pos); //初始化

    /* 双游标 */
    Q_INVOKABLE void initDoubleCursor(qreal pos); //初始化

    /* 单游标 */
    Q_INVOKABLE void initSingleCursor(qreal pos); //初始化

    void addLabel(QPoint pos);
    QVariantList getPointsInfo(QPoint pos);

    CurveLabelListModel * labelList() const;

    bool inAxisRect(QPoint pos);

    QRect getChooseArea(QPoint pos1, QPoint pos2);
signals:
    void stackVerOffsetChanged();
    void typeChanged();

private:
    int m_type = 0;//显示类型：0:普通，1:堆栈，2:瀑布
    SingleCursorListModel* m_singleCursor = nullptr;   //单游标
    DoubleCursorListModel* m_doubleCursor = nullptr;   //双游标
    MultiCursorListModel* m_multiFreqCursor = nullptr;   //倍频
    SideCursorListModel* m_sideFreqCursor = nullptr;   //边频
    CurveLabelListModel *m_labelList = nullptr;   //标注列表
    CursorSetModel *m_cursorSetModel = nullptr;

    MultiLabelListModel *m_multiLabelList  = nullptr;   //多倍频标注列表


};

#endif // MULTIFUNCTIONPLOT_H
