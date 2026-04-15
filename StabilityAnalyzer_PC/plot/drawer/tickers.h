#ifndef TICKERS_H
#define TICKERS_H

#include <QObject>
#include "../model/axismodel.h"
#include <QDebug>
#include <QDateTime>

class AxisModel;
class Tickers : public QObject
{
    Q_OBJECT
public:
    explicit Tickers(QObject *parent = nullptr);
    /* 获取有效值,num:输入值， val:有效值， pos:位数 */
    void getEffectiveValue(qreal num, qreal *val, int *pos);
    /* 获取当前位数的数字 */
    int getNumByPos(qreal num,int pos);
    /* 获取步长 */
    qreal getStep(qreal min, qreal max,int sum);
    QList<qreal> getTickNumber(AxisModel *axis,QList<QString>* textList = nullptr);

signals:

};

#endif // TICKERS_H
