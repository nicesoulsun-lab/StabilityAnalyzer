#ifndef LABELLISTMODEL_H
#define LABELLISTMODEL_H

#include <QAbstractListModel>
#include <QPointF>
#include "labelmodel.h"

class LabelListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit LabelListModel(QObject *parent = 0);
    ~LabelListModel();

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_INVOKABLE void add(QPointF point);
    Q_INVOKABLE void add(LabelModel *label);

    void remove(int i);
    void remove(LabelModel* label);

    QHash<int, QByteArray> roleNames()const override;

    LabelModel* getModel(int index);

    void removeChoose();
    void removeAll();
    void cancelChoose();

private:
    QVector<LabelModel*> modelData;
    QHash<int, QByteArray> m_roles;
};

#endif // LABELLISTMODEL_H
