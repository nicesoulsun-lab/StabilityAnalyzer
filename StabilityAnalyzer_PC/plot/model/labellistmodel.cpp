#include "labellistmodel.h"

LabelListModel::LabelListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_roles.insert(0,"label");
}

LabelListModel::~LabelListModel()
{
    qDeleteAll(modelData);
}

int LabelListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return modelData.size();
}

QVariant LabelListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    int rowIndex = index.row();
    return QVariant::fromValue<LabelModel*>(modelData.at(rowIndex));
}

void LabelListModel::add(QPointF point)
{
    beginInsertRows(QModelIndex(),modelData.size(),modelData.size());
    LabelModel *lm = new LabelModel();
    lm->setPos(point);
    modelData.push_back(lm);
    endInsertRows();
}

void LabelListModel::add(LabelModel *label)
{
    beginInsertRows(QModelIndex(),modelData.size(),modelData.size());
    modelData.push_back(label);
    endInsertRows();
}

void LabelListModel::remove(int i)
{
    beginRemoveRows(QModelIndex(),i,i);
    delete modelData.at(i);
    modelData.remove(i);
    endRemoveRows();
}

void LabelListModel::remove(LabelModel* label)
{
    for(int i = 0; i<modelData.size(); i++){
        if(label==modelData.at(i)){
            beginRemoveRows(QModelIndex(),i,i);
            delete modelData.at(i);
            modelData.remove(i);
            endRemoveRows();
            break;
        }
    }
}

QHash<int, QByteArray> LabelListModel::roleNames() const
{
    return m_roles;
}

LabelModel *LabelListModel::getModel(int index)
{
    if(index<0&&index>modelData.size()-1){
        return nullptr;
    }
    return modelData.at(index);
}

void LabelListModel::removeChoose()
{
    QList<int> list;
    for(int i = 0; i<modelData.size(); i++) {
        LabelModel* lm = modelData.at(i);
        if(lm->choosed())
            list<<i;
    }
    while (!list.isEmpty()) {
        int index = list.at(list.size()-1);
        list.pop_back();
        remove(index);
    }
}

void LabelListModel::removeAll()
{
    beginResetModel();
    qDeleteAll(modelData);
    modelData.clear();
    endResetModel();
}

void LabelListModel::cancelChoose()
{
    foreach (LabelModel *lm, modelData) {
        if(lm->choosed())
            lm->setChoosed(false);
    }
}
