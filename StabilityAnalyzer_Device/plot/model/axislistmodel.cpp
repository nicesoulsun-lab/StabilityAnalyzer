#include "axislistmodel.h"

AxisListModel::AxisListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_roles.insert(0,"axis");
}

AxisListModel::~AxisListModel()
{
    qDeleteAll(modelList);
}

int AxisListModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    return modelList.size();
}

QVariant AxisListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if(role == 0)
        return QVariant::fromValue<AxisModel*>(modelList.at(index.row()));
    return QVariant();
}

AxisModel* AxisListModel::addXAxis()
{
    beginInsertRows(QModelIndex(),modelList.size(),modelList.size());
    AxisModel* axis = new AxisModel();
    axis->setType(AxisModel::AxisType::XAxis1);
    modelList.push_back(axis);
    connect(axis,&AxisModel::styleChanged,[=](){
        emit styleChanged();
    });
    connect(axis,&AxisModel::rangeChanged,[=](){
        emit rangeChanged();
    });
    emit modelChanged();
    endInsertRows();
    return axis;
}

AxisModel* AxisListModel::addYAxis()
{
    beginInsertRows(QModelIndex(),modelList.size(),modelList.size());
    AxisModel* axis = new AxisModel();
    axis->setType(AxisModel::AxisType::YAxis1);
    modelList.push_back(axis);
    connect(axis,&AxisModel::styleChanged,[=](){
        emit styleChanged();
    });
    connect(axis,&AxisModel::rangeChanged,[=](){
        emit rangeChanged();
    });
    emit modelChanged();
    endInsertRows();
    return axis;
}

//void AxisListModel::addAxis(AxisModel *axis)
//{
//    beginInsertRows(QModelIndex(),modelList.size(),modelList.size());
//    modelList.push_back(axis);
//    connect(axis,&AxisModel::styleChanged,[=](){
//        emit styleChanged();
//    });
//    connect(axis,&AxisModel::rangeChanged,[=](){
//        emit rangeChanged();
//    });
//    emit modelChanged();
//    endInsertRows();
//}

//void AxisListModel::removeNoDelete(AxisModel *axis)
//{
//    if(axis==nullptr)
//        return;
//    for(int i = 0; i<modelList.size();i++){
//        if(modelList[i] == axis){
//            beginRemoveRows(QModelIndex(),i,i);
//            disconnect(axis);
//            modelList.remove(i);
//            emit modelChanged();
//            endRemoveRows();
//        }
//    }
//}

void AxisListModel::remove(int i)
{
    beginRemoveRows(QModelIndex(),i,i);
    AxisModel *axis = modelList[i];
    if(axis==nullptr)
        return;
    disconnect(axis);
    delete axis;
    modelList.remove(i);
    emit modelChanged();
    endRemoveRows();
}

void AxisListModel::removeAll()
{
    beginResetModel();
    qDeleteAll(modelList);
    modelList.clear();
    endResetModel();
}

AxisModel* AxisListModel::getModel(int index)
{
    if(index>modelList.size()-1)
        return nullptr;
    return modelList[index];
}

//void AxisListModel::clearNoDelete()
//{
//    modelList.clear();
//}

void AxisListModel::zoom(QRect& cRect,QRect& aRect, int mode)
{
    if(mode == 0){      //横向缩放
        qreal scale1 = (cRect.x()-aRect.x())/(double)aRect.width();
        qreal scale2 = (cRect.x()+cRect.width()-aRect.x())/(double)aRect.width();
        foreach(AxisModel* am,modelList){
            if(am->type()==AxisModel::AxisType::XAxis1||am->type()==AxisModel::AxisType::XAxis2){
                qreal l = am->lower();
                qreal u = am->upper();
                qreal l2 = l+(u-l)*scale1;
                qreal u2 = l+(u-l)*scale2;
                if(u2-l2<0){
                    continue;
                }
                am->setRangeNoUpdateAndSave(l2,u2);
            }
        }
    }else if(mode == 1){    //纵向缩放
        qreal scale1 = (aRect.height()-cRect.height()-cRect.y()+aRect.y())/(double)aRect.height();
        qreal scale2 = (aRect.height()-cRect.y()+aRect.y())/(double)aRect.height();
        foreach(AxisModel* am,modelList){
            if(am->type()==AxisModel::AxisType::YAxis1||am->type()==AxisModel::AxisType::YAxis2){
                qreal l = am->lower();
                qreal u = am->upper();
                qreal l2 = l+(u-l)*scale1;
                qreal u2 = l+(u-l)*scale2;
                if(u2-l2<0){
                    continue;
                }
                am->setRangeNoUpdateAndSave(l2,u2);
            }
        }
    }else{  //对角缩放
        qreal scale1 = (cRect.x()-aRect.x())/(double)aRect.width();
        qreal scale2 = (cRect.x()+cRect.width()-aRect.x())/(double)aRect.width();
        qreal scale3 = (aRect.height()-cRect.height()-cRect.y()+aRect.y())/(double)aRect.height();
        qreal scale4 = (aRect.height()-cRect.y()+aRect.y())/(double)aRect.height();
        foreach(AxisModel* am,modelList){
            if(am->type()==AxisModel::AxisType::XAxis1||am->type()==AxisModel::AxisType::XAxis2){
                qreal l = am->lower();
                qreal u = am->upper();
                qreal l2 = l+(u-l)*scale1;
                qreal u2 = l+(u-l)*scale2;
                if(u2-l2<0){
                    continue;
                }
                am->setRangeNoUpdateAndSave(l2,u2);
            }else{
                qreal l = am->lower();
                qreal u = am->upper();
                qreal l2 = l+(u-l)*scale3;
                qreal u2 = l+(u-l)*scale4;
                if(u2-l2<0){
                    continue;
                }
                am->setRangeNoUpdateAndSave(l2,u2);
            }
        }
    }
}

void AxisListModel::save()
{
    foreach(AxisModel* am,modelList){
        am->save();
    }
}

void AxisListModel::recover()
{
    foreach(AxisModel* am,modelList){
        am->recover();
    }
}

void AxisListModel::goback()
{
    foreach(AxisModel* am,modelList){
        am->goback();
    }
}

void AxisListModel::record()
{
    foreach(AxisModel* am,modelList){
        am->record();
    }
}

void AxisListModel::moveBySacle(QPointF p1,QPointF p2,QRect &rect, int mode)
{
    if(mode==0){    //横向
        qreal scale = (p1.x()-p2.x())/rect.width();
        foreach(AxisModel* am,modelList){
            if(am->type()==AxisModel::AxisType::XAxis1||am->type()==AxisModel::AxisType::XAxis2){
                am->moveBySacle(scale);
            }
        }
    }else if(mode==1){  //纵向
        qreal scale = -(p1.y()-p2.y())/rect.height();
        foreach(AxisModel* am,modelList){
            if(am->type()==AxisModel::AxisType::YAxis1||am->type()==AxisModel::AxisType::YAxis2){
                am->moveBySacle(scale);
            }
        }
    }else{          //双向
        qreal scale1 = (p1.x()-p2.x())/rect.width();
        qreal scale2 = -(p1.y()-p2.y())/rect.height();
        foreach(AxisModel* am,modelList){
            if(am->type()==AxisModel::AxisType::XAxis1||am->type()==AxisModel::AxisType::XAxis2){
                am->moveBySacle(scale1);
            }else{
                am->moveBySacle(scale2);
            }
        }
    }
}

void AxisListModel::setItemColor(QColor color)
{
    foreach (AxisModel* am, modelList) {
        am->setLineColor(color);
        am->setFontColor(color);
        am->setLineColor(color);
        am->setFontColor(color);
    }
}

QHash<int, QByteArray> AxisListModel::roleNames() const
{
    return m_roles;
}

AxisModel* AxisListModel::getFirstX()
{
    foreach (AxisModel* am,modelList) {
        if(am->type()==AxisModel::AxisType::XAxis1||am->type()==AxisModel::AxisType::XAxis2){
            return am;
        }
    }
    return nullptr;
}

AxisModel *AxisListModel::getFirstY()
{
    foreach (AxisModel* am,modelList) {
        if(am->type()==AxisModel::AxisType::YAxis1||am->type()==AxisModel::AxisType::YAxis2){
            return am;
        }
    }
    return nullptr;
}

QPointF AxisListModel::getLableRange(QPainter *painter)
{
    qreal w = 0;
    qreal h = 0;
    foreach (AxisModel* am, modelList) {
        if(am->type()==AxisModel::AxisType::YAxis1||am->type()==AxisModel::AxisType::YAxis2){
            qreal wTemp = am->labelWidth(painter);
            if(!am->title().isEmpty()){
                wTemp+=am->labelHeight(painter)+10;
            }
            w = qMax(w,wTemp);
        }else{
            qreal hTemp = am->labelHeight(painter);
            if(!am->title().isEmpty()){
                hTemp+=hTemp+10;
            }
            h = qMax(h,hTemp);
        }
    }
    return QPointF(w,h);
}

