#include "gridmodel.h"

GridModel::GridModel(QObject *parent) : QObject(parent)
{

}

GridModel::~GridModel()
{

}

bool GridModel::xGridVisible() const
{
    return m_xGridVisible;
}

void GridModel::setXGridVisible(bool xGridVisible)
{
    m_xGridVisible = xGridVisible;
    emit styleChanged();
}

bool GridModel::yGridVisible() const
{
    return m_yGridVisible;
}

void GridModel::setYGridVisible(bool yGridVisible)
{
    m_yGridVisible = yGridVisible;
    emit styleChanged();
}

GridModel::LineType GridModel::horLineType() const
{
    return m_horLineType;
}

void GridModel::setHorLineType(const LineType &horLineType)
{
    m_horLineType = horLineType;
    emit styleChanged();
}

GridModel::LineType GridModel::verLineType() const
{
    return m_verLineType;
}

void GridModel::setVerLineType(const LineType &verLineType)
{
    m_verLineType = verLineType;
    emit styleChanged();
}

qreal GridModel::lineWidth() const
{
    return m_lineWidth;
}

void GridModel::setLineWidth(const qreal &lineWidth)
{
    m_lineWidth = lineWidth;
    emit styleChanged();
}

QColor GridModel::lineColor() const
{
    return m_lineColor;
}

void GridModel::setLineColor(const QColor &lineColor)
{
    m_lineColor = lineColor;
    emit styleChanged();
}

AxisModel *GridModel::xAxis() const
{
    return m_xAxis;
}

void GridModel::setXAxis(AxisModel *xAxis)
{
    m_xAxis = xAxis;
    emit xAxisChanged();
    emit styleChanged();
}

AxisModel *GridModel::yAxis() const
{
    return m_yAxis;
}

void GridModel::setYAxis(AxisModel *yAxis)
{
    m_yAxis = yAxis;
    emit yAxisChanged();
    emit styleChanged();
}
