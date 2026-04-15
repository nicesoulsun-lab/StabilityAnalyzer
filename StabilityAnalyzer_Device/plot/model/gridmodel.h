#ifndef GRIDMODEL_H
#define GRIDMODEL_H

#include <QObject>
#include "axismodel.h"

class AxisModel;
class GridModel : public QObject
{
    Q_OBJECT
    Q_ENUMS(LineType)
    Q_PROPERTY(AxisModel* xAxis READ xAxis WRITE setXAxis NOTIFY xAxisChanged)
    Q_PROPERTY(AxisModel* yAxis READ yAxis WRITE setYAxis NOTIFY yAxisChanged)
    Q_PROPERTY(bool xGridVisible READ xGridVisible WRITE setXGridVisible)
    Q_PROPERTY(bool yGridVisible READ yGridVisible WRITE setYGridVisible)
    Q_PROPERTY(QColor lineColor READ lineColor WRITE setLineColor NOTIFY lineColorChanged)
    Q_PROPERTY(LineType horLineType READ horLineType WRITE setHorLineType)
    Q_PROPERTY(LineType verLineType READ verLineType WRITE setVerLineType)
    Q_PROPERTY(qreal lineWidth READ lineWidth WRITE setLineWidth)
public:
    explicit GridModel(QObject *parent = nullptr);
    ~GridModel();

    enum class LineType{    //网格线类型
        None,
        Solid,              //实线
        Dotted,             //虚线
    };

    bool xGridVisible() const;
    void setXGridVisible(bool xGridVisible);

    bool yGridVisible() const;
    void setYGridVisible(bool yGridVisible);

    LineType horLineType() const;
    void setHorLineType(const LineType &horLineType);

    LineType verLineType() const;
    void setVerLineType(const LineType &verLineType);

    qreal lineWidth() const;
    void setLineWidth(const qreal &lineWidth);

    QColor lineColor() const;
    void setLineColor(const QColor &lineColor);

    AxisModel *xAxis() const;
    void setXAxis(AxisModel *xAxis);

    AxisModel *yAxis() const;
    void setYAxis(AxisModel *yAxis);

signals:
    void styleChanged();
    void xAxisChanged();
    void yAxisChanged();
    void lineColorChanged();

private:
    AxisModel *m_xAxis = nullptr;
    AxisModel *m_yAxis = nullptr;
    bool m_xGridVisible = true;
    bool m_yGridVisible = true;

    LineType m_horLineType = LineType::Dotted;
    LineType m_verLineType = LineType::Dotted;
    qreal m_lineWidth = 1;
    QColor m_lineColor = QColor(120,139,206);
};

#endif // GRIDMODEL_H
