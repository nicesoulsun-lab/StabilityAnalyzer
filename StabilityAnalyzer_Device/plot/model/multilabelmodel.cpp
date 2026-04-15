#include "multilabelmodel.h"

MultiLabelModel::MultiLabelModel(CurveModel *parent) : QAbstractListModel(parent)
{
    m_pcurve = parent;
    m_roles.insert(0,"lable");
}

void MultiLabelModel::add(qreal xVal, int sum, QString name)
{
    beginResetModel();
    for(int i = 1; i<=sum; i++){
        MultiLabel *label = new MultiLabel(m_pcurve);
        label->init(xVal*i);
        modelData.push_back(label);
    }
    setName(name);
    endResetModel();
}

QString MultiLabelModel::name() const
{
    return m_name;
}

void MultiLabelModel::setName(const QString &name)
{
    m_name = name;
    emit nameChanged();
}

int MultiLabelModel::rowCount(const QModelIndex &parent) const
{
    return modelData.size();
}

QVariant MultiLabelModel::data(const QModelIndex &index, int role) const
{
    return QVariant::fromValue<MultiLabel*>(modelData.at(index.row()));
}

QHash<int, QByteArray> MultiLabelModel::roleNames() const
{
    return m_roles;
}

void MultiLabelModel::update()
{
    foreach(MultiLabel *label,modelData){
        label->update();
    }
}

CurveModel *MultiLabelModel::curve()
{
    return m_pcurve;
}

MultiLabel::MultiLabel(CurveModel *parent) : QObject(parent)
{
    m_pcurve = parent;
}

void MultiLabel::init(qreal val)
{
    if(m_pcurve!=nullptr){
        m_valPoint = m_pcurve->getIntersectionByX(val);
    }
}

QPointF MultiLabel::screenPoint() const
{
    if(m_pcurve==nullptr)
        return QPointF(-1,-1);
    return m_pcurve->transformCoords(m_valPoint);
}


QPointF MultiLabel::valPoint() const
{
    return m_valPoint;
}

MultiLabelListModel::MultiLabelListModel(QObject *parent):QAbstractListModel(parent)
{
    m_roles.insert(0,"multiLable");
}

int MultiLabelListModel::rowCount(const QModelIndex &parent) const
{
    return modelData.size();
}

QVariant MultiLabelListModel::data(const QModelIndex &index, int role) const
{

    return QVariant::fromValue<MultiLabelModel*>(modelData.at(index.row()));
}

QHash<int, QByteArray> MultiLabelListModel::roleNames() const
{
    return m_roles;
}

void MultiLabelListModel::addLabel(QString name, qreal val,int sum, CurveModel *curve)
{
    beginResetModel();
    MultiLabelModel *model = new MultiLabelModel(curve);
    model->add(val,sum,name);

    connect(model,&MultiLabelModel::destroyed,this,[=](QObject *obj){
        beginResetModel();
        for(int i = 0; i<modelData.size(); i++){
            if(obj == modelData.at(i)){
                modelData.remove(i);
                break;
            }
        }
        endResetModel();
    });
    modelData.push_back(model);
    endResetModel();
}

void MultiLabelListModel::remove(int i)
{
    MultiLabelModel *model = modelData.at(i);
    beginRemoveRows(QModelIndex(),i,i);
    modelData.remove(i);
    endRemoveRows();
    delete model;
}

void MultiLabelListModel::update()
{
    foreach(MultiLabelModel *model,modelData){
        model->update();
    }
}
