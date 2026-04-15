#ifndef SINGLECURSORMODEL_H
#define SINGLECURSORMODEL_H

#include "cursorlistmodel.h"

class SingleCursorModel : public CursorListModel
{
    Q_OBJECT
    Q_PROPERTY(bool checked READ checked WRITE setChecked NOTIFY checkedChanged)
public:
    explicit SingleCursorModel(QObject *parent = 0);

    void initSingleCursor(qreal pos);
    Q_INVOKABLE void move(qreal pos);
    Q_INVOKABLE void leftJump();
    Q_INVOKABLE void rightJump();

    QVariantMap info()override;

    int checked() const;
    void setChecked(int checked);

signals:
    void checkedChanged();
public slots:

private:
    int m_checked = false;
};

class SingleCursorListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    SingleCursorListModel(QObject *parent = 0);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames()const override;
    Q_INVOKABLE void initSingleCursor(qreal pos);
    Q_INVOKABLE void delCursor(int index);

    void setCurveModel(CurveListModel* model);

    void setXAxisModel(AxisModel *model);

    void update();

    Q_INVOKABLE void cancelChoose();
private:
    QVector<SingleCursorModel*> modelData;
    CurveListModel* m_clm = nullptr;
    AxisModel* m_xAxis = nullptr;
};

#endif // SINGLECURSORMODEL_H
