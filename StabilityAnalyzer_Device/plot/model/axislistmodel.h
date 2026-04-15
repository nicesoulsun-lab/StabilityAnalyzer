#ifndef AXISLISTMODEL_H
#define AXISLISTMODEL_H

#include <QAbstractListModel>
#include "axismodel.h"
#include <QVariant>
#include <QDebug>

class AxisListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit AxisListModel(QObject *parent = nullptr);
    ~AxisListModel();
    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_INVOKABLE AxisModel* addXAxis();
    Q_INVOKABLE AxisModel* addYAxis();
//    void addAxis(AxisModel *axis);

//    void removeNoDelete(AxisModel *axis);

    void remove(int index);
    void removeAll();

    AxisModel* getModel(int index);

//    void clearNoDelete();

    void zoom(QRect& cRect,QRect& aRect, int mode = 0);

    void save();
    void recover();
    void goback();

    void record();
    void moveBySacle(QPointF p1,QPointF p2,QRect &rect, int mode = 0);

    void setItemColor(QColor color);

    QHash<int,QByteArray> roleNames()const override;

    AxisModel* getFirstX();

    AxisModel* getFirstY();

    QPointF getLableRange(QPainter *painter);

signals:
    void modelChanged();
    void rangeChanged();
    void styleChanged();

private:
    QVector<AxisModel*> modelList;
    QHash<int,QByteArray> m_roles;
};

#endif // AXISLISTMODEL_H
