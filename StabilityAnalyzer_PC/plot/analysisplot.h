#ifndef ANALYSISPLOT_H
#define ANALYSISPLOT_H

#include <QObject>
#include <QByteArrayList>
#include <QFile>
#include "oscilloscope.h"

class AnalysisPlot : public Oscilloscope
{
    Q_OBJECT
public:
    explicit AnalysisPlot(QQuickItem *parent = 0);

    /*
     * url:数据源文件的路径
     * paramList：选择的数据索引
     * analysisType：算法类型
    */
    Q_INVOKABLE void init(const QString &url
                          ,QVariantList paramList,int analysisType = 0);

    QList<qreal> readByCSV(const QString &url);
signals:

public slots:

private:
    AxisModel *xAxis;
    AxisModel *yAxis;
};

#endif // ANALYSISPLOT_H
