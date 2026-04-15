#ifndef CURVELISTMODEL_H
#define CURVELISTMODEL_H

#include <QAbstractListModel>
#include <QJsonDocument>
#include "curvemodel.h"

class CurveListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(CurveModel* firstCurve READ firstCurve NOTIFY firstCurveChanged)
    Q_PROPERTY(CurveModel* selectCurve READ selectCurve WRITE setSelectCurve NOTIFY selectCurveChanged)
    Q_PROPERTY(int curveSum READ curveSum NOTIFY modelChanged)
    Q_PROPERTY(bool isSelectAll READ isSelectAll NOTIFY isSelectAllChanged)
public:
    explicit CurveListModel(QObject *parent = nullptr);
    ~CurveListModel();

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    CurveModel * addCurve();
//    void addCurve(CurveModel*curve);

    Q_INVOKABLE void remove(int index);
    void removeAll();

    CurveModel* getModel(int index);

    /* 查询是否存在该坐标轴的曲线关联 */
    bool find(AxisModel *model);

    //void move();

    QHash<int, QByteArray> roleNames()const override;

//    /* 获取坐标轴范围 */
//    void getRange(QPointF& range, AxisModel *axis);

    Q_INVOKABLE void calculate(QString jsonStr);

    CurveModel* firstCurve();

    Q_INVOKABLE void move(int from, int to);

    int curveSum();

    Q_INVOKABLE void select(int index);

    CurveModel *selectCurve() const;
    void setSelectCurve(CurveModel *selectCurve);

    void updateCurveIndex();

    bool isSelectAll();

    Q_INVOKABLE void checkedAll(bool flag);

signals:
    void modelChanged();
    void styleChanged();
    void visibleChanged();
    void sourceChanged();
    void zoomChanged();
    void firstCurveChanged();
    void sortChanged();
    void addSignal(CurveModel *curve);
    void selectCurveChanged();
    void spectrumPlotChanged();
    void isSelectAllChanged();
private:
    QVector<CurveModel*> modelList;
    QHash<int,QByteArray> m_roles;
    CurveModel* m_selectCurve;
    int colorChooseIndex = 0;
    QVector<QColor> colorBuffer;
};

#endif // CURVELISTMODEL_H
