#include "labelmodel.h"

LabelModel::LabelModel(QObject *parent) : QObject(parent)
{

}

QString LabelModel::content() const
{
    return m_content;
}

void LabelModel::setContent(const QString &content)
{
    m_content = content;
    emit contentChanged();
}

QPointF LabelModel::pos() const
{
    return m_pos;
}

void LabelModel::setPos(const QPointF &pos)
{
    m_pos = pos;
}

void LabelModel::setScreenPos(const QPointF &screenPos)
{
    m_screenPos = screenPos;
    emit posChanged();
}

QPointF LabelModel::screenPos() const
{
    return m_screenPos;
}

bool LabelModel::choosed() const
{
    return m_choosed;
}

void LabelModel::setChoosed(bool choosed)
{
    m_choosed = choosed;
    emit choosedChanged();
}
