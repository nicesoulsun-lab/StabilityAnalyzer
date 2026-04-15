#include "curvelistmodel.h"

QList<QColor> colorList = {
    QColor(	255,69,0)
    ,QColor(69,137,148)
    ,QColor(255,150,128)
    ,QColor(0,90,171)
    ,QColor(222,125,44)
    ,QColor(174,221,129)
    ,QColor(137,157,192)
    ,QColor(250,227,113)
    ,QColor(87,96,105)
    ,QColor(96,143,159)
};

CurveListModel::CurveListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_roles.insert(0,"curve");
}

CurveListModel::~CurveListModel()
{
    qDeleteAll(modelList);
}

int CurveListModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    // FIXME: Implement me!
    return modelList.size();
}

QVariant CurveListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if(role==0){
        return QVariant::fromValue<CurveModel*>(modelList[index.row()]);
    }
    return QVariant();
}
//TODO_liubo 画线颜色设置
CurveModel *CurveListModel::addCurve()
{
    beginInsertRows(QModelIndex(),modelList.size(),modelList.size());
    CurveModel* curve = new CurveModel();

    if(colorBuffer.isEmpty()){
        if(colorChooseIndex<colorList.size()){
            curve->setLineColor(colorList[colorChooseIndex]);
        }else{
            curve->setLineColor(QColor(qrand()%255,qrand()%255,qrand()%255));
        }
    }else{
        curve->setLineColor(colorBuffer.at(colorBuffer.size()-1));
        colorBuffer.pop_back();
    }
    colorChooseIndex++;
    curve->setIndex(modelList.size());
    modelList.push_back(curve);
    connect(curve,&CurveModel::styleChanged,[=](){
        emit styleChanged();
    });
    connect(curve,&CurveModel::visibleChanged,[=](){
        emit visibleChanged();
    });
    connect(curve,&CurveModel::sourceChanged,[=](){
        emit sourceChanged();
    });
    connect(curve,&CurveModel::visibleChanged,[=](){
        emit isSelectAllChanged();
    });
    emit modelChanged();
    addSignal(curve);
    endInsertRows();
    if(modelList.size()==1)
        emit firstCurveChanged();
    emit isSelectAllChanged();
    return curve;
}


void CurveListModel::remove(int i)
{
    beginRemoveRows(QModelIndex(),i,i);
    CurveModel* curve = modelList[i];
    curve->stop();
    colorBuffer.push_back(curve->lineColor());
    delete curve;
    modelList.remove(i);
    emit modelChanged();
    if(i==0)
        emit firstCurveChanged();
    endRemoveRows();
    updateCurveIndex();
}

void CurveListModel::removeAll()
{
    beginResetModel();
    foreach (CurveModel* c, modelList) {
        c->stop();
        delete c;
    }
    modelList.clear();
    emit modelChanged();
    endResetModel();
    emit firstCurveChanged();
}

CurveModel *CurveListModel::getModel(int index)
{
    if(index>modelList.size()){
        return nullptr;
    }
    return modelList.at(index);
}

bool CurveListModel::find(AxisModel *model)
{
    if(model->type()==AxisModel::AxisType::XAxis1||model->type()==AxisModel::AxisType::XAxis2){
        foreach(CurveModel* cm, modelList){
            if(cm->xAxis()==model)
                return true;
        }
    }else{
        foreach(CurveModel* cm, modelList){
            if(cm->yAxis()==model)
                return true;
        }
    }
    return false;
}

//void CurveListModel::move()
//{
//    foreach(CurveModel* cm, modelList){
//        cm->move();
//    }
//}

QHash<int, QByteArray> CurveListModel::roleNames() const
{
    return m_roles;
}

//void CurveListModel::getRange(QPointF &range, AxisModel *axis)
//{
//    if(axis == nullptr)
//        return;
//    QPointF xRange,yRange;
//    if(axis->type()==AxisModel::AxisType::XAxis1||axis->type()==AxisModel::AxisType::XAxis1){
//        for(int i = 0; i<modelList.size(); i++){
//            CurveModel *cm = modelList.at(i);
//            if(i==0){
//                range=
//            }else{
//                range.setX(qMin(range.x(),cm->xRange().x()));
//                range.setY(qMax(range.y(),cm->xRange().y()));
//            }
//        }
//    }else{
//        for(int i = 0; i<modelList.size(); i++){
//            CurveModel *cm = modelList.at(i);
//            if(i==0){
//                range=cm->yRange();
//            }else{
//                range.setX(qMin(range.x(),cm->yRange().x()));
//                range.setY(qMax(range.y(),cm->yRange().y()));
//            }
//        }
//    }
//}


CurveModel *CurveListModel::firstCurve()
{
    if(modelList.isEmpty()){
        return nullptr;
    }else{
        return modelList.at(0);
    }
}

void CurveListModel::move(int from, int to)
{
    beginResetModel();
    if(from>modelList.size()-1||to>modelList.size()-1||from==to){
        return;
    }
    CurveModel *curve = modelList.at(from);
    modelList.remove(from);
    modelList.insert(to,curve);
    emit sortChanged();
    endResetModel();
    if(from==0||to==0){
        emit firstCurveChanged();
    }
    updateCurveIndex();
}

int CurveListModel::curveSum()
{
    return modelList.size();
}

void CurveListModel::select(int index)
{
    if(index>=modelList.size()){
        return;
    }
    foreach(CurveModel* curve,modelList){
        curve->setVisible(false);
    }
    modelList.at(index)->setVisible(true);
    setSelectCurve(modelList.at(index));
    emit spectrumPlotChanged();
}

CurveModel *CurveListModel::selectCurve() const
{
    return m_selectCurve;
}

void CurveListModel::setSelectCurve(CurveModel *selectCurve)
{
    m_selectCurve = selectCurve;
    emit selectCurveChanged();
}

void CurveListModel::updateCurveIndex()
{
    int counter = 0;
    foreach(CurveModel* cm,modelList){
        cm->setIndex(counter++);
    }
}

bool CurveListModel::isSelectAll()
{
    foreach(CurveModel* cm,modelList){
        if(!cm->visible()){
            return false;
        }
    }
    return !modelList.isEmpty();
}

void CurveListModel::checkedAll(bool flag)
{
    foreach(CurveModel* cm,modelList){
        cm->setVisible(flag);
    }
}

void CurveListModel::calculate(QString jsonStr)
{
    QJsonDocument document = QJsonDocument::fromJson(jsonStr.toLocal8Bit().data());
    QJsonObject object = document.object();

//    foreach (CurveModel *cm, modelList) {
//        cm->calculate(object);
//    }
}



