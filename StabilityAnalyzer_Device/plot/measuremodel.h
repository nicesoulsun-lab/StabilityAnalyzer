#ifndef MEASUREMODEL_H
#define MEASUREMODEL_H

#include <QObject>
#include "model/cursormodel.h"
class MeasureModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(CurveLabelModel* point1 READ point1 WRITE setPoint1 NOTIFY point1Changed)
    Q_PROPERTY(CurveLabelModel* point2 READ point2 WRITE setPoint2 NOTIFY point2Changed)
public:
    explicit MeasureModel(QObject *parent = nullptr);
    ~MeasureModel();

    void add(CurveLabelModel*p1,CurveLabelModel*p2);

    CurveLabelModel *point1() const;
    void setPoint1(CurveLabelModel *point1);

    CurveLabelModel *point2() const;
    void setPoint2(CurveLabelModel *point2);

    void update();

signals:
    void point1Changed();
    void point2Changed();
private:
    CurveLabelModel * m_point1 = nullptr;
    CurveLabelModel * m_point2 = nullptr;

};

class MeasurePointListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit MeasurePointListModel(QObject *parent = nullptr);
    // Basic functionality:
   int rowCount(const QModelIndex &parent = QModelIndex()) const override;

   QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

   QHash<int, QByteArray> roleNames()const override;

   void add(CurveLabelModel *model);

   void update();

   void clear();

private:
    QVector<MeasureModel*> modelData;
    bool flag = false;
};

#endif // MEASUREMODEL_H
