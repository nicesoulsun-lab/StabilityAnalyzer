#ifndef PLOTDATAHANDLER_H
#define PLOTDATAHANDLER_H

#include <QObject>
#include <QJsonObject>
#include "LoopVector.h"
#include <QVector>
#include <QPointF>
#include <QAbstractListModel>

#include <QTimer>

typedef struct{
    bool enable = false;    //显示该数据
    QString name;           //变量名
    int xType = 0;          //x坐标单位 0:普通 1:角度
    QString uint;           //单位
}CurveInfo;

class PlotDataHandler : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit PlotDataHandler(QObject *parent = 0);

    /* 初始化plot */
    void initPlot(/*param*/);

    /* 修改数据的显示隐藏 */
    void modifyCurve(int index, bool flag);

    /* 添加数据 */
    void push(QList<qreal> &source,int begin = -1,int step = 1);

    /* 添加数据 */
    void push(QList<QList<qreal>> &source,int begin = -1,int step = 1);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;


    QHash<int, QByteArray> roleNames() const override;

signals:
    /* 初始化plot信号 */
    void readyInit(QList<CurveInfo> &clist);

    /* 数据更新时发送该信号 */
    void readyUpdate();

    /* 更改显示的通道 */
    void readyModify(int index, bool flag);

public slots:

private:
    QList<CurveInfo> clist;
    int dataSum = 1000;
    QHash<int,QByteArray> m_roles;
};

#endif // PLOTDATAHANDLER_H
