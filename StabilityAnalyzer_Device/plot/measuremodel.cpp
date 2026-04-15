#include "measuremodel.h"

MeasureModel::MeasureModel(QObject *parent) : QObject(parent)
{

}

MeasureModel::~MeasureModel()
{
    delete m_point1;
    delete m_point2;
}

void MeasureModel::add(CurveLabelModel *p1, CurveLabelModel *p2)
{
    m_point1 = p1;
    m_point2 = p2;

    emit point1Changed();
    emit point2Changed();
}

CurveLabelModel *MeasureModel::point1() const
{
    return m_point1;
}

void MeasureModel::setPoint1(CurveLabelModel *point1)
{
    m_point1 = point1;
    emit point1Changed();
}

CurveLabelModel *MeasureModel::point2() const
{
    return m_point2;
}

void MeasureModel::setPoint2(CurveLabelModel *point2)
{
    m_point2 = point2;
    emit point2Changed();
}

void MeasureModel::update()
{
    if(m_point1!=nullptr){
        m_point1->update();
    }

    if(m_point2!=nullptr){
        m_point2->update();
    }
}

MeasurePointListModel::MeasurePointListModel(QObject *parent):QAbstractListModel(parent)
{

}


int MeasurePointListModel::rowCount(const QModelIndex &parent) const
{
    return  modelData.size();
}

QVariant MeasurePointListModel::data(const QModelIndex &index, int role) const
{
    return QVariant::fromValue<MeasureModel*>(modelData.at(index.row()));
}

QHash<int, QByteArray> MeasurePointListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(0,"pModel");
    return roles;
}

void MeasurePointListModel::add(CurveLabelModel *model)
{
    beginInsertRows(QModelIndex(),modelData.size(),modelData.size());
    if(!flag){
        MeasureModel *mm = new MeasureModel(this);
        mm->setPoint1(model);
        modelData.push_back(mm);
    }else{
        MeasureModel *mm = modelData.at(modelData.size()-1);
        mm->setPoint2(model);
    }
    flag = !flag;
    endInsertRows();
}

void MeasurePointListModel::update()
{
    foreach(MeasureModel *mm,modelData){
        mm->update();
    }
}

void MeasurePointListModel::clear()
{
    beginResetModel();
    qDeleteAll(modelData);
    modelData.clear();
    endResetModel();
}
