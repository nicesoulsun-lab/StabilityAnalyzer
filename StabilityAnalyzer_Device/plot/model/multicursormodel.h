#ifndef MULTICURSORMODEL_H
#define MULTICURSORMODEL_H

#include "cursorlistmodel.h"

class MultiCursorModel:public CursorListModel
{
    Q_OBJECT
    Q_PROPERTY(qreal dis READ dis WRITE setDis NOTIFY disChanged)
    Q_PROPERTY(bool checked READ checked WRITE setChecked NOTIFY checkedChanged)
public:
    MultiCursorModel(QObject *parent = nullptr);

    Q_INVOKABLE void initMultiFreqCursor(int index, qreal pos,int maxsum);
    Q_INVOKABLE void initMultiFreqCursor(int index, qreal pos);
    Q_INVOKABLE void initMultiFreqCursorByFm(qreal fm);
    Q_INVOKABLE void multiFreqCursorRightJump(int index);
    Q_INVOKABLE void multiFreqCursorLeftJump(int index);

    qreal dis() const;
    void setDis(const qreal &dis);

    bool checked() const;
    void setChecked(bool checked);

signals:
    void disChanged();
    void checkedChanged();
private:
    qreal m_dis = 1;
    bool m_checked = false;
    int m_maxsum = 10;
};

class MultiCursorListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    MultiCursorListModel(QObject *parent = 0);

    // Basic functionality:
   int rowCount(const QModelIndex &parent = QModelIndex()) const override;

   QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

   QHash<int, QByteArray> roleNames()const override;
   Q_INVOKABLE void initMultiCursor(int index,qreal pos,int sum);
   Q_INVOKABLE void delCursor(int index);

   void setCurveModel(CurveListModel* model);

   void setXAxisModel(AxisModel *model);

   void update();

   Q_INVOKABLE void cancelChoose();
private:
   QVector<MultiCursorModel*> modelData;
   CurveListModel* m_clm = nullptr;
   AxisModel* m_xAxis = nullptr;
};

#endif // MULTICURSORMODEL_H
