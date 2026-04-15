#ifndef CURVEMODEL_H
#define CURVEMODEL_H

#include <QThread>
#include <QTime>
#include <QSharedPointer>
#include "LoopVector.h"
#include "axismodel.h"
#include <QDebug>
#include <QMutex>
#include <QtMath>
#include <QJsonObject>
#include "LoopVector.h"
#include "labellistmodel.h"

class AxisModel;
class CurveModel;

class CurveLabelModel : public QObject{
    Q_OBJECT
    Q_PROPERTY(QPointF coord READ coord NOTIFY coordChanged)
    Q_PROPERTY(QPointF point READ point WRITE setPoint NOTIFY pointChanged)
    Q_PROPERTY(QString x READ x  NOTIFY positionChanged)
    Q_PROPERTY(QString y READ y NOTIFY positionChanged)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(QColor color READ color NOTIFY colorChanged)
    Q_PROPERTY(int position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(bool checked READ checked WRITE setChecked NOTIFY checkedChanged)
    Q_PROPERTY(CurveModel* curve READ curve NOTIFY curveChanged)
public:
    CurveLabelModel(CurveModel *parent = nullptr);

    QString label() const;
    void setLabel(const QString &label);

    QPointF point();

    QPointF coord();

    void update();

    void setPoint(const QPointF &point);

    int position() const;
    void setPosition(int position);

    QColor color();

    QString x();
    QString y();

    bool checked() const;
    void setChecked(bool checked);

    CurveModel* curve();
signals:
    void pointChanged();
    void labelChanged();
    void coordChanged();
    void colorChanged();
    void positionChanged();
    void checkedChanged();
    void curveChanged();

private:
    CurveModel *m_parent;
    qreal m_realX;
    int m_position = 0; //位置0,1,2,3 象限：1234
    QString m_label;    //标签
    QPointF m_point;
    bool m_checked = false;
};

typedef struct {
    CurveModel * curve;
    CurveLabelModel * label;
}CurveLabel;

class CurveLabelListModel:public QAbstractListModel{
    Q_OBJECT
public:
    CurveLabelListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_INVOKABLE void add(qreal scale,QPoint pos,CurveModel *curve);

    Q_INVOKABLE void add(CurveModel *curve,qreal x);
    Q_INVOKABLE void add(CurveModel *curve,qreal x,QString node);

    void removeByCurve(QObject* obj);

    Q_INVOKABLE void moveToLast(int index);

    /* 删除全部 */
    void removeAll();

    /* 删除选中 */
    void removeChoose();

    /* 根据索引删除 */
    void removeByIndex(int index);

    /* 取消选中 */
    void cancelChoose();

    void update();
    QHash<int, QByteArray> roleNames()const override;
private:
    QVector<CurveLabel> modelData;
};

//class CurveMeasureModel : public QObject{
//    Q_OBJECT
//public:
//    CurveMeasureModel(CurveModel *parent = nullptr);
//private:
//    CurveModel *m_parent;
//    qreal m_realX;
//    int m_position = 0; //位置0,1,2,3 象限：1234
//    QString m_label;    //标签
//    QPointF m_point;
//    bool m_checked = false;
//};

class CurveModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QColor color1 READ color1 WRITE setColor1 NOTIFY color1Changed)
    Q_PROPERTY(QColor color2 READ color2 WRITE setColor2 NOTIFY color2Changed)
    Q_PROPERTY(AxisModel* xAxis READ xAxis WRITE setXAxis NOTIFY xAxisChanged)
    Q_PROPERTY(AxisModel* yAxis READ yAxis WRITE setYAxis NOTIFY yAxisChanged)
    Q_PROPERTY(QColor lineColor READ lineColor WRITE setLineColor NOTIFY lineColorChanged)
    Q_PROPERTY(qreal lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)
    Q_PROPERTY(bool gradientFill READ gradientFill WRITE setGradientFill NOTIFY gradientFillChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(bool tipsEnable READ tipsEnable WRITE setTipsEnable NOTIFY tipsEnableChanged)
    Q_PROPERTY(int lineType READ lineType WRITE setLineType NOTIFY lineTypeChanged)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(int sourceSize READ sourceSize NOTIFY sourceSizeChanged)
    Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)
public:
    explicit CurveModel(QObject *parent = nullptr);
    ~CurveModel();

    AxisModel *xAxis() const;
    void setXAxis(AxisModel *xAxis);

    AxisModel *yAxis() const;
    void setYAxis(AxisModel *yAxis);

    QColor color1() const;
    void setColor1(const QColor &color1);

    QColor color2() const;
    void setColor2(const QColor &color2);

    QColor lineColor() const;
    void setLineColor(const QColor &lineColor);

    qreal lineWidth() const;
    void setLineWidth(const qreal &lineWidth);

    bool gradientFill() const;
    void setGradientFill(bool gradientFill);

    void draw();
    QPointF drawCursor(QPointF pos,bool &online);

    void drawAlarmLine(QPainter *painter,QRect &rect);
    void drawLine(QPainter *painter,QRect &rect,qreal val,QString text);

    void clearCursor();

    /* 坐标转换 */
    QPointF transformCoords(QPointF point);
    void setWidth(const qreal &width);

    void setHeight(const qreal &height);

    QPixmap getCurve();

    QPixmap getCursor();

    QString title() const;
    void setTitle(const QString &title);

    void stop();

    bool tipsEnable() const;
    void setTipsEnable(bool tipsEnable);

    int lineType() const;
    void setLineType(int lineType);

    int sourceSize();

    bool visible() const;
    void setVisible(bool visible);

    void initSource(int cacheSum);
    void initSource(QSharedPointer<LoopVector<QPointF> > s);

    QPointF getLabel(qreal x);

    int getPointByX(qreal x);

    /* 这个算的是交点*/
    QPointF getIntersectionByX(qreal x);
    /* 这个算的是离得最近的点*/
    QPointF getNearPointByX(qreal x);
    QPointF getNearPointByX(qreal x,QPointF pos,bool &onLine);

    CurveLabelListModel *curveLabelList() const;
    void setCurveLabelList(CurveLabelListModel *curveLabelList);

    QPointF addLabel(qreal scale,QPoint pos,bool& flag);
    QPointF addLabel(qreal x);      //查找最近的点位置插入

    /* 求两点间的距离 */
    qreal getDis(QPointF p1,QPointF p2);
    int index() const;
    void setIndex(int index);

    int dataType() const;
    void setDataType(int dataType);

    LoopVector<QPointF>* sourcePtr();

    void getRange(QPointF &xRange, QPointF &yRange);

signals:
    void styleChanged();
    void color1Changed();
    void color2Changed();
    void xAxisChanged();
    void yAxisChanged();
    void lineColorChanged();
    void lineWidthChanged();
    void gradientFillChanged();
    void sourceChanged();
    void titleChanged();
    void tipsEnableChanged();
    void lineTypeChanged();
    void visibleChanged();
    void sourceSizeChanged();
    void indexChanged();

private:
    AxisModel * m_xAxis = nullptr;
    AxisModel * m_yAxis = nullptr;
    QColor m_color1;                //渐变色1
    QColor m_color2;                //渐变色2
    QColor m_lineColor = QColor(1,1,1,255); //线颜色

    bool m_warning = false;

    int m_lineType = 0;         // 线型 0：实线 1：虚线
    qreal m_lineWidth = 1;          //线宽
    bool m_gradientFill = false;    //渐变填充

    QPixmap cursorCanvas;         //游标画布
    QPixmap canvas;         //画布
    qreal m_width=0;        //画布大小
    qreal m_height=0;       //画布大小
    qreal xUnitLen = 1;         //单位长度
    qreal yUnitLen = 1;         //单位长度

    QString m_title = "";     //标题

    int  m_dataType = 0;
    bool m_tipsEnable = true;     //移入提示

    bool runEnable = true;      //线程开启标志位

    bool m_visible = true;

    bool m_cutEnable = false;

    QMutex mutex;
    bool outSource = false;
    //LoopVector<QPointF>* m_source = nullptr;
    QSharedPointer<LoopVector<QPointF>> m_source;
    QJsonObject m_spectrumValue;
    qreal m_sample = 25600;

    int m_index;
};

#endif // CURVEMODEL_H
