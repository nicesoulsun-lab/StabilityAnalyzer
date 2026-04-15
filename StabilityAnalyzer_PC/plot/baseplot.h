#ifndef BASEPLOT_H
#define BASEPLOT_H

#include <QPainter>
#include <QTimer>
#include <QThread>
#include <QtMath>
#include <QMutex>
#include <QMutexLocker>
#include "drawer/axisdrawer.h"
#include "drawer/curvedrawer.h"
#include "drawer/cursordrawer.h"
#include "model/axislistmodel.h"
#include "model/curvelistmodel.h"
#include "model/cursormodel.h"

#ifndef PLOTMARGINS
#define PLOTMARGINS
typedef struct {
    qreal top = 0;
    qreal bottom = 0;
    qreal left = 0;
    qreal right = 0;
} PlotMargins;
#endif

/*多功能图表*/
class BasePlot : public QObject
{
    Q_OBJECT
public:
    BasePlot(QObject* parent = nullptr);
    ~BasePlot();

    /* 绘制接口 */
    virtual void draw() = 0;

    /* 计算自动量程 */
    void calculateRange();

    /* 更新坐标轴画布 */
    void updateShow();

    virtual void updateAxisCanvas(){}

    /* 更新曲线画布 */
    virtual void updateCurveCanvas(){}

    /* 更新游标画布 */
    virtual void updateCursorCanvas(){}

    /* 初始化轴矩形 */
    virtual void initAxisRect(QPainter *painter){}

    AxisListModel *axisModel() const;
    virtual void setAxisModel(AxisListModel *axisModel);

    GridModel *grid() const;
    virtual void setGrid(GridModel *grid);

    CurveListModel *curveModel() const;
    virtual void setCurveModel(CurveListModel *curveModel);

    /* 框选 */
    virtual void choose(QRect &rect,int mode = 0);
    /* 回到上一次缩放 */
    void goback();
    /* 框选恢复 */
    void recover();

    /* 鼠标移入提示 */
    void hover(QPoint point);
    void exit();

    /* 记录 */
    Q_INVOKABLE void record();

    /* 移动 */
    virtual void move(QPoint p1,QPoint p2, int mode = 0);

    /* 设置步长，坐标轴一次移动的长度 */
    qreal step();
    void setStep(qreal step);

    /*样式*/
    void setBorderColor(const QColor &borderColor);
    QColor tipsCardColor();
    void setTipsCardColor(const QColor &color);

    QRect getAxisRect() const;

    qreal getTotalLength();

    int borderWidth() const;
    void setBorderWidth(int borderWidth);

    int borderType() const;
    void setBorderType(int borderType);

    QColor borderColor() const;

    bool playEnable() const;
    void setPlayEnable(bool playEnable);

    /* 图表向前读取数据 */
    bool goAhead();

    bool visible() const;
    void setVisible(bool visible);

    /* 框选：计算鼠标的范围 */
    virtual QRect getChooseArea(QPoint pos1,QPoint pos2);
    /* 判断鼠标点是否在绘图区内 */
    bool inAxisRect(QPoint pos);

    void setMargins(qreal left,qreal top,qreal right,qreal bottom);

    /* 初始化表 */
    void init(AxisListModel *alist, CurveListModel*clist,GridModel *gmodel);
    /* 填入数据 */
    void pushData(QList<qreal> &list ,qreal begin = 0);
    /* 填入数据 */
    void pushData(QMap<int, qreal> &list ,qreal begin = 0);
    /* 填入数据 */
    void pushDatas(QList<QList<qreal>> &list,qreal begin,int step);

    QPixmap canvas() ;
    void setCanvas(const QPixmap &canvas);

    void reSize(int width,int height);

    void setXRange(qreal begin,qreal end);
signals:
    void plotUpdate();
    void axisRectChanged();
    void plotChanged();

protected:
    QPixmap axisCanvas; //坐标轴画布
    QPixmap cursorCanvas; //曲线画布
    QPixmap m_canvas;
    AxisDrawer axisDrawer;  //坐标轴绘制器
    CurveDrawer curveDrawer;  //曲线绘制器
    CursorDrawer cursorDrawer;  //曲线绘制器

    AxisListModel* m_axisModel = nullptr;
    CurveListModel* m_curveModel = nullptr;

    qreal m_width = 0;
    qreal m_height = 0;

    QRect axisRect; //轴矩形，曲线绘制的区域
    GridModel* m_grid = nullptr; //网格线

    qreal offVal = 1;

    bool m_playEnable = true;

    QPointF cursorPos;

    QColor m_borderColor = QColor(120,139,206);
    int m_borderWidth = 1;
    int m_borderType = 0;   //0:实线 1:虚线

    qreal m_verOffset = 0;

    qreal w_verOffset = 15;
    qreal w_horOffset = 30;

    bool m_visible = true;

    PlotMargins m_margins;

    QMutex mutex;
    QTimer *timer = nullptr;   //延时刷新界面的定时器
    bool rangeUpdate = true;
    bool axisUpdate = true;
    bool curveUpdate = true;
    bool cursorUpdate = true;

};

#endif // BASEPLOT_H
