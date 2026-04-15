#ifndef MYPLOT_H
#define MYPLOT_H

#include <QQuickPaintedItem>

#include <QTimer>
#include <QtMath>
#include <QDebug>
#include <QRect>
#include <QJsonDocument>
#include <QQuickItem>
#include "multifunctionplot.h"
#include "model/cursormodel.h"
#include "model/doublecursormodel.h"
#include "model/cursorlistmodel.h"
#include "model/labellistmodel.h"
#include "model/sidecursormodel.h"
#include "model/multicursormodel.h"
#include "model/doublecursormodel.h"
#include "model/singlecursormodel.h"
#include "model/cursorsetmodel.h"
#include "menumodel.h"
#include "plot/measuremodel.h"

//#if _MSC_VER >= 1600
//#pragma execution_character_set("utf-8")
//#endif

class MyPlot : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QVariantList yAxisModel READ yAxisModel NOTIFY yAxisModelChanged)
    Q_PROPERTY(QVariantList xAxisModel READ xAxisModel NOTIFY xAxisModelChanged)
    Q_PROPERTY(QColor popCardColor READ popCardColor WRITE setPopCardColor NOTIFY popCardColorChanged)
    Q_PROPERTY(int speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(bool playEnable READ playEnable WRITE setPlayEnable NOTIFY playEnableChanged)
    Q_PROPERTY(qreal step READ step WRITE setStep NOTIFY stepChanged)
    Q_PROPERTY(QRect axisRect READ axisRect NOTIFY axisRectChanged)
    Q_PROPERTY(qreal barWidth READ barWidth WRITE setBarWidth NOTIFY barWidthChanged)
//    Q_PROPERTY(QVariant singleCursor READ singleCursor NOTIFY singleCursorChanged)
//    Q_PROPERTY(DoubleCursorModel* doubleCursor READ doubleCursor NOTIFY doubleCursorChanged)
    Q_PROPERTY(GridModel* grid READ grid WRITE setGrid NOTIFY gridChanged)
//    Q_PROPERTY(MultiCursorModel* multiFreqCursor READ multiFreqCursor NOTIFY multiFreqCursorChanged)
//    Q_PROPERTY(SideCursorModel* sideFreqCursor READ sideFreqCursor NOTIFY sideFreqCursorChanged)
    Q_PROPERTY(qreal totalLength READ totalLength WRITE setTotalLength NOTIFY totalLengthChanged)
    Q_PROPERTY(qreal progress READ progress WRITE setProgress NOTIFY progressChanged)
    Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor NOTIFY borderColorChanged)
    Q_PROPERTY(int borderWidth READ borderWidth WRITE setBorderWidth NOTIFY borderWidthChanged)
    Q_PROPERTY(int borderType READ borderType WRITE setBorderType NOTIFY borderTypeChanged)
    Q_PROPERTY(int type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(CurveLabelListModel* labelList READ labelList NOTIFY labelListChanged)
    Q_PROPERTY(MultiLabelListModel* multiLabelList READ multiLabelList NOTIFY multiLabelListChanged)
    Q_PROPERTY(qreal stackVerOffset READ stackVerOffset NOTIFY stackVerOffsetChanged)
    Q_PROPERTY(MenuInfo* menu READ menu NOTIFY menuChanged)
    Q_PROPERTY(int measureEnable READ measureEnable WRITE setMeasureEnable NOTIFY measureEnableChanged)
    Q_PROPERTY(SingleCursorListModel* singleList READ singleList NOTIFY singleListChanged)
    Q_PROPERTY(DoubleCursorListModel* doubleList READ doubleList NOTIFY doubleListChanged)
    Q_PROPERTY(SideCursorListModel* sideList READ sideList NOTIFY sideListChanged)
    Q_PROPERTY(MultiCursorListModel* multiList READ multiList NOTIFY multiListChanged)
    Q_PROPERTY(MeasurePointListModel* measurePoints READ measurePoints NOTIFY measurePointsChanged)
public:
    MyPlot(QQuickItem *parent = nullptr);
    ~MyPlot();
    void paint(QPainter *painter) override;

    AxisListModel *axisModel() const;
    void setAxisModel(AxisListModel *axisModel);
//    Q_INVOKABLE void init(const QJsonObject &object);

    /* 添加删除坐标轴 */
    Q_INVOKABLE void addXAxis();
    Q_INVOKABLE void addYAxis();
    Q_INVOKABLE bool removeAxis(int index);

    /* 添加删除通道关联 */
    Q_INVOKABLE void addLink();
    Q_INVOKABLE void removeLink(int index);

    Q_INVOKABLE void updateCurve();

    /* 移动 */
    Q_INVOKABLE void move(QPoint p1,QPoint p2, int mode = 0);
    Q_INVOKABLE void record();      //记录鼠标按下时的位置，用于移动

    /* 鼠标移入时弹出卡片的颜色 */
    QColor popCardColor() const;
    void setPopCardColor(const QColor &popCardColor);

    /* 析构函数执行时，是否释放指针内存 */
    bool clearEnable() const;
    void setClearEnable(bool clearEnable);

    QVariantList xAxisModel();
    QVariantList yAxisModel();

    /* 网格线样式 */
    /* 为网格线绑定坐标轴 */
    Q_INVOKABLE void setGridX(AxisModel *axis);
    Q_INVOKABLE void setGridY(AxisModel *axis);
    /* 网格线线型 */
    Q_INVOKABLE void setGridLine(int index);

    /* 移入提示相关 */
    Q_INVOKABLE void hover(QPoint point);   //移入时更新提示
    Q_INVOKABLE void exit();        //移出时关闭提示

    /* 框选缩放 */
    Q_INVOKABLE void choose(QRect rect,int mode = 0);

    /* 恢复缩放比例 */
    Q_INVOKABLE void recover();

    /* 恢复缩放比例 */
    Q_INVOKABLE void goback();


    /* 判断鼠标位置是否在图表内(显示曲线的部分，不包含坐标轴等空白处) */
    Q_INVOKABLE bool inAxisRect(QPoint point);

    /* 播放&暂停 */
    bool playEnable();
    Q_INVOKABLE void setPlayEnable(bool play);

    /* 设置播放速度 */
    int speed();
    void setSpeed(int msec);

    /* 设置步长 */
    qreal step();
    void setStep(qreal step);

    /* 设置框选矩形 */
    Q_INVOKABLE QRect getRect(const QRect &value);

    QRect axisRect();

    QVariant singleCursor();

    DoubleCursorModel* doubleCursor();

    /* 测距函数 */
    Q_INVOKABLE QVariantList xMeasure(qreal xScale);
    Q_INVOKABLE QVariantList yMeasure(qreal yScale);

    GridModel* grid();

    void setGrid(GridModel*);

    MultiCursorModel *multiFreqCursor() const;

    SideCursorModel *sideFreqCursor() const;

    /* 添加标注 */
//    Q_INVOKABLE void addLabel(QString param1,QString param2);
    Q_INVOKABLE void addLabel(CurveModel* curve,qreal x);
    Q_INVOKABLE void addLabel(CurveModel* curve,qreal x, QString node);
    /* 删除选中标注 */
    Q_INVOKABLE void removeChooseLabel();
    /* 删除全部标注 */
    Q_INVOKABLE void removeAllLabel();
    /* 取消选中 */
    Q_INVOKABLE void removeLabelByIndex(int index);
    /* 取消选中 */
    Q_INVOKABLE void cancelChooseLabel();

    qreal totalLength() const;
    void setTotalLength(qreal totalLength);

    qreal progress() const;
    void setProgress(const qreal &progress);

    /* 设置带宽 */
    Q_INVOKABLE void setBandWidth(qreal minBand, qreal maxBand);

    /* 更新进度条(位置，宽度) */
    void updateProgress();

    /* 进度条拖动 */
    Q_INVOKABLE void dragProgressBar(qreal pos);

    /* 设置窗口宽带 */
    Q_INVOKABLE void setXLength(int len);

    /* 图表曲线显示区边框相关 */
    int borderWidth() const;            //线宽
    void setBorderWidth(int borderWidth);

    int borderType() const;             //线型
    void setBorderType(int borderType);

    QColor borderColor() const;         //颜色
    void setBorderColor(QColor borderColor);

    /* 图表往前移动一格 */
    void goAhead();

    void flashPlot(QPixmap map);

    void flashOnMove();     //移动时更新进度条

//    void updateCursorShow();    //更新游标显示

    int type() const;

    void setType(int type);

    qreal stackVerOffset();

    CurveLabelListModel* labelList();
    MultiLabelListModel* multiLabelList();

    /* 更换视图：0:普通图表、1:堆栈图表、2:瀑布图表 */
    Q_INVOKABLE void transformView(int mode);

    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
//    virtual void hoverEnterEvent(QHoverEvent *event) override;
    virtual void hoverLeaveEvent(QHoverEvent *event) override;
    virtual void hoverMoveEvent(QHoverEvent *event) override;

    /* 初始化频谱图的弹出菜单 */
    Q_INVOKABLE void initFreqMenu();
    /* 初始化时域图的弹出菜单 */
    Q_INVOKABLE void initTimeMenu();

    MenuInfo *menu() const;

    Q_INVOKABLE void recodePoint(QVariantMap var);
    Q_INVOKABLE void recodePoint(CurveModel* curve,QPointF p);

    int measureEnable() const;
    void setMeasureEnable(int measureEnable);

    Q_INVOKABLE QString getDis(qreal p1, qreal p2);

    SingleCursorListModel *singleList() const;

    DoubleCursorListModel *doubleList() const;

    SideCursorListModel *sideList() const;

    MultiCursorListModel *multiList() const;

    MeasurePointListModel *measurePoints() const;

    Q_INVOKABLE void setMargins(qreal left,qreal top,qreal right,qreal bottom);

    qreal barWidth() const;
    void setBarWidth(const qreal &barWidth);

signals:
    void axisModelChanged();
    void xAxisModelChanged();
    void barWidthChanged();
    void yAxisModelChanged();
    void curveModelChanged();
    void popCardColorChanged();
    void isStopChanged();
    void isZoomChanged();
    void speedChanged();
    void stepChanged();
    void axisRectChanged();
    void gridChanged();
    void totalLengthChanged();
    void progressChanged();
    void borderColorChanged();
    void borderWidthChanged();
    void borderTypeChanged();
    void playEnableChanged();
    void typeChanged();
    void popMenu(QPointF pos);
    void labelListChanged();
    void multiLabelListChanged();
    void stackVerOffsetChanged();
    void menuChanged();
    void chooseMenu(QPointF pos,QVariantList list);
    void choosePointError();
    void saveMeasureFinish();
    void measureEnableChanged();
    void singleListChanged();
    void doubleListChanged();
    void sideListChanged();
    void cursorSetModelChanged();
    void multiListChanged();
    void measurePointsChanged();
    void handlerChanged();

    /* 边频游标 */
    void initSideFreqCursor(qreal pos,int sum); //初始化
    /* 倍频游标 */
    void initMultiFreqCursor(int index, qreal pos); //初始化
    /* 双游标 */
    void initDoubleCursor(qreal pos); //初始化
    /* 单游标 */
    void initSingleCursor(qreal pos); //初始化

protected:
    void outputSaveFile();
protected:
    MultifunctionPlot *plot;
    AxisListModel *m_axisModel;
    CurveListModel *m_curveModel;
    QThread *thread = nullptr;

    QJsonObject m_algoJson;
    QTimer *timer;

    QColor m_popCardColor = Qt::white;

    bool updating = false;

    /* 进度条相关 */
    qreal m_totalLength = 0;      //数据范围
    qreal m_progress = 0;
    qreal m_barWidth = 0;

    bool rightPress = false;
    bool leftPress = false;

    bool rm = false;
    QPoint pos;
    QRect chooseRect;
    bool chooseEnable = false;
    int optFlag = 0;//0:回退 1：横向放大 2：纵向放大 3：局部放大

    MenuInfo *m_menu = nullptr;   //这个是右键弹出菜单
    QTime pressTime;

    int m_measureEnable = -1;     //测距标志位：0:取第一个点 1:取第二个点 -1:测距未使能

    MeasurePointListModel * m_measurePoints;
    QPixmap map;
};

#endif // MYPLOT_H
