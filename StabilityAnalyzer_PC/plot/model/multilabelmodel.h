#ifndef MULTILABELMODEL_H
#define MULTILABELMODEL_H

#include <QObject>
#include "curvemodel.h"

class MultiLabel:public QObject
{
    Q_OBJECT
    Q_PROPERTY(QPointF valPoint READ valPoint NOTIFY valPointChanged)
    Q_PROPERTY(QPointF screenPoint READ screenPoint NOTIFY screenPointChanged)
public:
    MultiLabel(CurveModel *parent = nullptr);
    void init(qreal val);
    QPointF screenPoint() const;
    QPointF valPoint() const;
    void update(){emit screenPointChanged();}

signals:
    void valPointChanged();
    void screenPointChanged();

private:
    CurveModel * m_pcurve = nullptr;
    qreal m_val;
    QPointF m_valPoint;
};

class MultiLabelModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(CurveModel* curve READ curve NOTIFY curveChanged)
public:
    explicit MultiLabelModel(CurveModel *parent = nullptr);
    Q_INVOKABLE void add(qreal xVal,int sum,QString name);

    QString name() const;
    void setName(const QString &name);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames()const override;

    void update();

    CurveModel* curve();
signals:
    void nameChanged();
    void curveChanged();
private:
    CurveModel *m_pcurve = nullptr;
    QVector<MultiLabel*> modelData;
    QString m_name;
    QHash<int, QByteArray> m_roles;
};

class MultiLabelListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    MultiLabelListModel(QObject *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames()const override;

    Q_INVOKABLE void addLabel(QString name,qreal val,int sum,CurveModel*curve);
    Q_INVOKABLE void remove(int index);
    void update();
private:
    QVector<MultiLabelModel *> modelData;
    QHash<int, QByteArray> m_roles;
};
#endif // MULTILABELMODEL_H
