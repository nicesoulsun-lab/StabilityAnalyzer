#ifndef DOUBLECURSORMODEL_H
#define DOUBLECURSORMODEL_H

#include <QObject>
#include <QtMath>
#include "model/cursorlistmodel.h"

class DoubleCursorModel : public CursorListModel
{
    Q_OBJECT    
    Q_PROPERTY(bool locked READ locked WRITE setLocked NOTIFY lockedChanged)
    Q_PROPERTY(qreal dis READ dis WRITE setDis NOTIFY disChanged)
    Q_PROPERTY(bool checked READ checked WRITE setChecked NOTIFY checkedChanged)
public:
    explicit DoubleCursorModel(QObject *parent = 0);

    Q_INVOKABLE void initDoubleCursor(qreal pos); //初始化
    Q_INVOKABLE void doubleCursorMove(int index, qreal pos); //移动
    Q_INVOKABLE void doubleCursorLeftJump(int index);     //左跳
    Q_INVOKABLE void doubleCursorRightJump(int index);    //右跳

    bool locked() const;
    void setLocked(bool locked);

    qreal dis() const;
    void setDis(const qreal &dis);

    QVariantMap info();

    bool checked() const;
    void setChecked(bool checked);

signals:
    void lockedChanged();
    void disChanged();
    void checkedChanged();

private:
    bool m_locked = false;
    qreal m_dis = 1;
    bool m_checked = false;
};


class DoubleCursorListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    DoubleCursorListModel(QObject *parent = 0);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames()const override;
    Q_INVOKABLE void initDoubleCursor(qreal pos);
    Q_INVOKABLE void delCursor(int index);

    void setCurveModel(CurveListModel* model);

    void setXAxisModel(AxisModel *model);

    void update();

    Q_INVOKABLE void cancelChoose();
private:
    QVector<DoubleCursorModel*> modelData;
    CurveListModel* m_clm = nullptr;
    AxisModel* m_xAxis = nullptr;
};
#endif // DOUBLECURSORMODEL_H
