#ifndef CURSORMODEL_H
#define CURSORMODEL_H

#include <QObject>
#include <QPoint>
#include <QJsonArray>
#include <QAbstractListModel>
#include "model/curvelistmodel.h"
#include "model/axismodel.h"

class CursorData : public QObject{
    Q_OBJECT
    Q_PROPERTY(QPointF value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(QString x READ x NOTIFY valueChanged)
    Q_PROPERTY(QString y READ y NOTIFY valueChanged)
    Q_PROPERTY(CurveModel* curve READ curve WRITE setCurve NOTIFY curveChanged)
    Q_PROPERTY(QPointF coord READ coord WRITE setCoord NOTIFY coordChanged)
    Q_PROPERTY(bool checked READ checked WRITE setChecked NOTIFY checkedChanged)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(QPoint pos READ pos WRITE setPos NOTIFY posChanged)
    Q_PROPERTY(bool showLabel READ showLabel WRITE setShowLabel NOTIFY showLabelChanged)
public:
    CursorData(QObject *parent = 0);
    QPointF value() const;
    void setValue(const QPointF &value);

    CurveModel *curve() const;
    void setCurve(CurveModel *curve);

    QPointF coord() const;
    void setCoord(const QPointF &coord);

    bool checked() const;
    void setChecked(bool checked);

    QPoint pos() const;
    void setPos(const QPoint &pos);

    QString label() const;
    void setLabel(QString label);

    bool showLabel() const;
    void setShowLabel(bool showLabel);

    void update(qreal &xVal);
    void update();

    QPointF getPointByX(qreal x, LoopVector<QPointF> *source);

    QPointF getMidPointByX(qreal x,LoopVector<QPointF>* source);

    QString x();
    QString y();
signals:
    void valueChanged();
    void curveChanged();
    void coordChanged();
    void checkedChanged();
    void labelChanged();
    void posChanged();
    void showLabelChanged();
    void removeSignal(CursorData* data);

private:
    CurveModel *m_curve = nullptr;  //所属曲线
    QPointF m_value;      //对应数值
    QPointF m_coord;    //位置
    bool m_checked = false;     //选中
    QString m_label;    //游标文本
    QPoint m_pos = QPoint(0,-25);       //文本的位置
    bool m_showLabel = false;  //显示文本
};

class CursorDataModel:public QAbstractListModel{
    Q_OBJECT
public:
    CursorDataModel(QObject *parent = 0);
    ~CursorDataModel();
    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames()const override;

    void add(CursorData* data);
    void clear();

    void removeByCursor(CursorData* data);

    CursorData* findByCurve(CurveModel *cm);

    void update(qreal &xVal);
    void update();

    Q_INVOKABLE void addLabel(QJsonArray &array);
    /* 取消选中 */
    Q_INVOKABLE void clearChecked();
    /* 全选 */
    Q_INVOKABLE void checkedAll();
    /* 删除选中游标标注 */
    Q_INVOKABLE void removeLabel();
    Q_INVOKABLE void removeAllLabel();

private:
    QVector<CursorData*> modelData;
    QHash<int,QByteArray> m_roles;
};

class CursorModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged)
    Q_PROPERTY(QString xVal READ getXVal  NOTIFY positionChanged)
    Q_PROPERTY(bool init READ init WRITE setInit NOTIFY initChanged)
    Q_PROPERTY(QString posFlag READ posFlag WRITE setPosFlag NOTIFY posFlagChanged)
    Q_PROPERTY(CursorDataModel* cursorDataList READ cursorDataList WRITE setCursorDataList NOTIFY cursorDataListChanged)
public:
    explicit CursorModel(QObject *parent = 0);
    ~CursorModel();

    void getPoints();

    QPointF getPointByX(qreal x,LoopVector<QPointF>* source);

    QPointF getMidPointByX(qreal x,LoopVector<QPointF>* source);

    QPointF getLeft(qreal x,LoopVector<QPointF>* source);
    QPointF getRight(qreal x,LoopVector<QPointF>* source);

    QPointF getNear(qreal pos);

    qreal leftJump();

    qreal rightJump();

    qreal position() const;

    bool visible() const;
    void setVisible(const bool &visible);

    void setModel(CurveListModel *model);

    void update();

    QVariantList pointList() const;
    void setPointList(QVariantList &pointList);

    QVariantList titleList() const;
    void setTitleList(QVariantList &titleList);

    QVariantList colorList() const;
    void setColorList(QVariantList &colorList);

    QVariantList relPosList() const;
    void setRelPosList(QVariantList &relPosList);

    bool init() const;
    void setInit(bool init);

    AxisModel *xAxis() const;
    void setXAxis(AxisModel *xAxis);

    qreal xVal() const;
    void setXVal(const qreal &xVal);
    QString getXVal();
    CursorDataModel *cursorDataList() const;
    void setCursorDataList(CursorDataModel *cursorDataList);

    QString posFlag() const;
    void setPosFlag(QString posFlag);

    Q_INVOKABLE void addLabel(QJsonArray &array);
    /* 取消选中 */
    Q_INVOKABLE void clearChecked();
    /* 全选 */
    Q_INVOKABLE void checkedAll();
    /* 删除选中游标标注 */
    Q_INVOKABLE void removeLabel();

    Q_INVOKABLE void removeAllLabel();
signals:
    void visibleChanged();
    void dataChanged();
    void positionChanged();
    void initChanged();
    void cursorDataListChanged();
    void posFlagChanged();
public slots:

private:
    bool m_init = false;
    bool m_visible = false;    //游标是否显示
    CurveListModel *m_model = nullptr;
    CursorDataModel * m_cursorDataList = nullptr;
    AxisModel *m_xAxis = nullptr;
    qreal m_position;
    bool updating = false;
    qreal m_xVal = 0;
    int xType = 0;
    QString m_posFlag = 0;
};

#endif // CURSORMODEL_H
