#include "singlecursormodel.h"

SingleCursorModel::SingleCursorModel(QObject *parent) : CursorListModel(parent)
{
    connect(this,&CursorListModel::zoomChanged,[=](){
        update();
    });
}

void SingleCursorModel::initSingleCursor(qreal pos)
{
    beginResetModel();
    CursorModel *cm = new CursorModel(this);
    cm->setXAxis(m_xAxis);
    cm->setModel(m_curveModel);
    qreal p = cm->getNear(pos).x();
    cm->setXVal(p);
    cm->setVisible(true);
    setVisible(true);
    setInit(true);
    add(cm);
    endResetModel();
    emit infoChanged();
}

void SingleCursorModel::move(qreal pos)
{
    qreal val = modelData.at(0)->getNear(pos).x();
    modelData.at(0)->setXVal(val);
    emit infoChanged();
}

void SingleCursorModel::leftJump()
{
    qreal val = modelData.at(0)->leftJump();
    modelData.at(0)->setXVal(val);
    emit infoChanged();
}

void SingleCursorModel::rightJump()
{
    qreal val = modelData.at(0)->rightJump();
    modelData.at(0)->setXVal(val);
    emit infoChanged();
}


QVariantMap SingleCursorModel::info()
{
    QVariantList array;
    if(m_curveModel==nullptr)
        return QVariantMap();

    for(int i = 0; i<m_curveModel->rowCount(); i++){
        CurveModel *cm = m_curveModel->getModel(i);
        QVariantMap curveInfo;
        curveInfo.insert("curve",cm->title());
        curveInfo.insert("color",cm->lineColor());
        foreach (CursorModel *cursor, modelData) {
            CursorData *cd = cursor->cursorDataList()->findByCurve(cm);
            if(cd!=nullptr){
                curveInfo.insert("fm",QString::number(cd->value().x()));
                curveInfo.insert("value",QString::number(cd->value().y()));
            }
        }
        array.push_back(curveInfo);
    }
    QVariantMap map;
    map.insert("data",array);
    return map;
}

int SingleCursorModel::checked() const
{
    return m_checked;
}

void SingleCursorModel::setChecked(int checked)
{
    m_checked = checked;
    emit checkedChanged();
}

SingleCursorListModel::SingleCursorListModel(QObject *parent):QAbstractListModel(parent)
{
    
}

int SingleCursorListModel::rowCount(const QModelIndex &parent) const
{
    return modelData.size();
}

QVariant SingleCursorListModel::data(const QModelIndex &index, int role) const
{
    return QVariant::fromValue<SingleCursorModel*>(modelData.at(index.row()));
}

QHash<int, QByteArray> SingleCursorListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(0,"cModel");
    return roles;
}

void SingleCursorListModel::initSingleCursor(qreal pos)
{
    beginInsertRows(QModelIndex(),modelData.size(),modelData.size());
    SingleCursorModel* scm = new SingleCursorModel(this);
    scm->setXAxis(m_xAxis);
    scm->setCurveModel(m_clm);
    scm->initSingleCursor(pos);
    modelData.push_back(scm);
    endInsertRows();
}

void SingleCursorListModel::delCursor(int index)
{
    SingleCursorModel* temp = modelData.at(index);
    beginRemoveRows(QModelIndex(),index,index);
    modelData.remove(index);
    endRemoveRows();
    delete temp;
}

void SingleCursorListModel::setCurveModel(CurveListModel *clm)
{
    m_clm = clm;
    foreach(SingleCursorModel* model,modelData){
        model->setCurveModel(clm);
    }
}

void SingleCursorListModel::setXAxisModel(AxisModel *am)
{
    m_xAxis = am;
    foreach(SingleCursorModel* model,modelData){
        model->setXAxis(am);
    }
}

void SingleCursorListModel::update()
{
    foreach(SingleCursorModel* model,modelData){
        model->update();
    }
}

void SingleCursorListModel::cancelChoose()
{
    foreach(SingleCursorModel* model,modelData){
        model->setChecked(false);
    }
}
