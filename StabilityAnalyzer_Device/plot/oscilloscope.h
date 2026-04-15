#ifndef OSCILLOSCOPE_H
#define OSCILLOSCOPE_H

#include <QQuickPaintedItem>
#include <QThread>
#include <QCursor>
#include "baseplot.h"
#include <QQuickItem>

class Oscilloscope : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(CurveListModel* curveModel READ curveModel NOTIFY curveModelChanged)
    Q_PROPERTY(AxisListModel* axisModel READ axisModel NOTIFY axisModelChanged)
public:
    Oscilloscope(QQuickItem *parent = nullptr);
    virtual ~Oscilloscope();

    void paint(QPainter* painter) override;

    void flashPlot();

    void initPlotHandler(BasePlot *plot);
//    void init();

    void pushData(QMap<int,qreal> &list,qreal begin);
    void pushData(QList<qreal> &list,qreal begin);
    void pushDatas(QList<QList<qreal>> &list,qreal begin = 0,int step = 1);

    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
//    virtual void mouseMoveEvent(QMouseEvent *event) override;
//    virtual void mousePressEvent(QMouseEvent *event) override;
//    virtual void mouseReleaseEvent(QMouseEvent *event) override;
//    virtual void hoverEnterEvent(QHoverEvent *event) override;
    virtual void hoverLeaveEvent(QHoverEvent *event) override;
    virtual void hoverMoveEvent(QHoverEvent *event) override;

    /* 开启绘制线程 */
    void start();
    /* 关闭绘制线程 */
    bool quit();

    CurveListModel* curveModel();
    AxisListModel* axisModel();
signals:
    void readyInit(AxisListModel *axisModel,CurveListModel *curveModel,GridModel *gridModel);
    void readyGoAhead();
    void readyPush(QList<qreal> &list,qreal begin);
    void readyPushs(QList<QList<qreal>> &list,qreal begin,int step);

    void readyPush(QMap<int,qreal> &list,qreal begin);

    void readyResize(int width,int height);
    void readyHover(QPoint p);
    void readyMove(QPoint p1,QPoint p2,int flag);
    void readyExit();
    void readyReocde();
    void readyGoBack();
    void readyChoose(QRect &r,int flag = 0);
    void curveModelChanged();
    void axisModelChanged();
    void readySetXRange(qreal begin,qreal end);

public slots:

private:
    QThread *thread;
    QList<QColor> curvecolor = {
        QColor(255,69,0)
        ,QColor(69,137,148)
        ,QColor(255,150,128)
        ,QColor(0,90,171)
        ,QColor(222,125,44)
        ,QColor(174,221,129)
        ,QColor(137,157,192)
        ,QColor(250,227,113)
        ,QColor(87,96,105)
        ,QColor(96,143,159)
    };
    QPixmap pix;


    bool rightPress = false;
    bool leftPress = false;

    bool rm = false;
    QPoint pos;
    QRect chooseRect;
    bool chooseEnable = false;
    int optFlag = 0;//0:回退 1：横向放大 2：纵向放大 3：局部放大

//    MenuInfo *m_menu = nullptr;   //这个是右键弹出菜单
    QTime pressTime;

protected:
    BasePlot *plot = nullptr;
    CurveListModel *cModel = nullptr;
    AxisListModel *aModel = nullptr;
    GridModel *gModel = nullptr;
    AxisModel *xAxis = nullptr;
    AxisModel *yAxis = nullptr;
};

#endif // OSCILLOSCOPE_H
