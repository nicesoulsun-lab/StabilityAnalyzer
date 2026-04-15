#ifndef STACKPLOTITEM_H
#define STACKPLOTITEM_H

#include <QObject>
#include <QQuickPaintedItem>
#include "multifunctionplot.h"
#include "DataPipeControl.h"

class StackPlotItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QVector<PipeConfigInfo*> source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int pipeSum READ pipeSum NOTIFY pipeSumChanged)
public:
    StackPlotItem();
    ~StackPlotItem();
    void paint(QPainter *painter) override;

    QVector<PipeConfigInfo *> ySource() const;
    void setYSource(const QVector<PipeConfigInfo *> &ySource);


    QVector<PipeConfigInfo *> source() const;
    void setSource(const QVector<PipeConfigInfo *> &source);

    int pipeSum();
signals:
    void sourceChanged();
    void pipeSumChanged();
    void initPlot(const QVector<PipeConfigInfo *> &source);
private:
    int counter = 0;
    MultifunctionPlot *plot;
    QThread thread;
    DataPipeControl *control;
};

#endif // STACKPLOTITEM_H
