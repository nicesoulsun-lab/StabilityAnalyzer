#include "cursorsetmodel.h"

CursorSetModel::CursorSetModel(QObject *parent) : QObject(parent)
{

}

int CursorSetModel::sideSum() const
{
    return m_sideSum;
}

void CursorSetModel::setSideSum(int sideSum)
{
    m_sideSum = sideSum;
    emit sideSumChanged();
}

QColor CursorSetModel::multiCursorColor() const
{
    return m_multiCursorColor;
}

void CursorSetModel::setMultiCursorColor(const QColor &multiCursorColor)
{
    m_multiCursorColor = multiCursorColor;
    emit multiCursorColorChanged();
}

QColor CursorSetModel::sideCursorColor() const
{
    return m_sideCursorColor;
}

void CursorSetModel::setSideCursorColor(const QColor &sideCursorColor)
{
    m_sideCursorColor = sideCursorColor;
    emit sideCursorColorChanged();
}

QColor CursorSetModel::doubleCursorColor() const
{
    return m_doubleCursorColor;
}

void CursorSetModel::setDoubleCursorColor(const QColor &doubleCursorColor)
{
    m_doubleCursorColor = doubleCursorColor;
    emit doubleCursorColorChanged();
}

QColor CursorSetModel::singleCursorColor() const
{
    return m_singleCursorColor;
}

void CursorSetModel::setSingleCursorColor(const QColor &singleCursorColor)
{
    m_singleCursorColor = singleCursorColor;
    emit singleCursorColorChanged();
}

int CursorSetModel::lineType() const
{
    return m_lineType;
}

void CursorSetModel::setLineType(int lineType)
{
    m_lineType = lineType;
    emit lineTypeChanged();
}

int CursorSetModel::multiSum() const
{
    return m_multiSum;
}

void CursorSetModel::setMultiSum(int multiSum)
{
    m_multiSum = multiSum;
    emit multiSumChanged();
}
