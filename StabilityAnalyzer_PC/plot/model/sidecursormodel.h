#ifndef SIDECURSORMODEL_H
#define SIDECURSORMODEL_H

#include "cursorlistmodel.h"

class SideCursorModel:public CursorListModel
{
    Q_OBJECT
    Q_PROPERTY(qreal dis READ dis WRITE setDis NOTIFY disChanged)
    Q_PROPERTY(bool checked READ checked WRITE setChecked NOTIFY checkedChanged)
    Q_PROPERTY(qreal bandWidth READ bandWidth NOTIFY disChanged)
public:
    SideCursorModel(QObject *parent = 0);

    Q_INVOKABLE void initSideFreqCursor(qreal pos,int sum);
    Q_INVOKABLE void initSideFreqCursorByFm(qreal fm,int sum);
    Q_INVOKABLE void adjustSideFreqCursor(int index,qreal pos);
    Q_INVOKABLE void moveSideFreqCursor(qreal pos);
    Q_INVOKABLE void sideFreqCursorRightJump();
    Q_INVOKABLE void sideFreqCursorLeftJump();

    qreal dis() const;
    void setDis(const qreal &dis);

    bool checked() const;
    void setChecked(bool checked);

    qreal bandWidth() const;

signals:
    void disChanged();
    void checkedChanged();
private:
    qreal m_dis = 1;
    bool m_checked = false;
};


class SideCursorListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    SideCursorListModel(QObject *parent = 0);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames()const override;
    Q_INVOKABLE void initSideFreqCursor(qreal pos, int sum);
    Q_INVOKABLE void delCursor(int index);

    void setCurveModel(CurveListModel* model);

    void setXAxisModel(AxisModel *model);

    void update();

    Q_INVOKABLE void cancelChoose();
private:
    QVector<SideCursorModel*> modelData;
    CurveListModel* m_clm = nullptr;
    AxisModel* m_xAxis = nullptr;
};

#endif // SIDECURSORMODEL_H
