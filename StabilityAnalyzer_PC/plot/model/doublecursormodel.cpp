#include "doublecursormodel.h"

DoubleCursorModel::DoubleCursorModel(QObject *parent) : CursorListModel(parent)
{
    connect(this,&CursorListModel::zoomChanged,[=](){
        emit disChanged();
    });
}

void DoubleCursorModel::initDoubleCursor(qreal pos)
{
    if(pos<=0||m_curveModel==nullptr)
        return;

    CursorModel* cm = new CursorModel(this);
    cm->setXAxis(m_xAxis);
    cm->setModel(m_curveModel);
    cm->setVisible(true);
    cm->setPosFlag("1");
    qreal xVal = cm->getNear(pos).x();
    cm->setXVal(xVal);
    add(cm);

    CursorModel* cm2 = new CursorModel(this);
    cm2->setXAxis(m_xAxis);
    cm2->setModel(m_curveModel);
    cm2->setVisible(true);
    cm2->setPosFlag("2");
    if(pos+0.1>1){
        qreal xVal = cm->getNear(pos-0.1).x();
        cm2->setXVal(xVal);
    }else{
        qreal xVal = cm->getNear(pos+0.1).x();
        cm2->setXVal(xVal);
    }
    add(cm2);
    setInit(true);
    setVisible(true);
    m_dis = modelData.at(0)->xVal()-modelData.at(1)->xVal();
    emit disChanged();
    emit infoChanged();
}

void DoubleCursorModel::doubleCursorMove(int index, qreal pos)
{
    if(modelData.size()<2||index>=2)
        return;
    qreal val1 = modelData.at(index)->getNear(pos).x();
    qreal val2;
    if(m_locked){

        QPointF range = m_xAxis->maxRange();
        if(index==0){
            val2 = val1 - m_dis;
            if(val2>range.y()){
                val2 = range.y();
                val1 =val2 + m_dis;
            }
            if(val2<range.x()){
                val2 = range.x();
                val1 =val2 + m_dis;
            }
        }else{
            val2 = val1;
            val1 = val2 + m_dis;
            if(val1>range.y()){
                val1 = range.y();
                val2 =val1 - m_dis;
            }
            if(val1<range.x()){
                val1 = range.x();
                val2 =val1 - m_dis;
            }
        }
        modelData.at(0)->setXVal(val1);
        modelData.at(1)->setXVal(val2);
    }else{
        modelData.at(index)->setXVal(val1);
        m_dis = modelData.at(0)->xVal()-modelData.at(1)->xVal();
        emit disChanged();
    }
    emit infoChanged();
}

void DoubleCursorModel::doubleCursorLeftJump(int index)
{
    qreal val1 = modelData.at(index)->leftJump();
    qreal val2;
    if(m_locked){

        QPointF range = m_xAxis->maxRange();
        if(index==0){
            val2 = val1 - m_dis;
            if(val2>range.y()){
                val2 = range.y();
                val1 =val2 + m_dis;
            }
            if(val2<range.x()){
                val2 = range.x();
                val1 =val2 + m_dis;
            }
        }else{
            val2 = val1;
            val1 = val2 + m_dis;
            if(val1>range.y()){
                val1 = range.y();
                val2 =val1 - m_dis;
            }
            if(val1<range.x()){
                val1 = range.x();
                val2 =val1 - m_dis;
            }
        }
        modelData.at(0)->setXVal(val1);
        modelData.at(1)->setXVal(val2);
    }else{
        modelData.at(index)->setXVal(val1);
        m_dis = modelData.at(0)->xVal()-modelData.at(1)->xVal();
        emit disChanged();
    }
    emit infoChanged();
}

void DoubleCursorModel::doubleCursorRightJump(int index)
{
    qreal val1 = modelData.at(index)->rightJump();
    qreal val2;
    if(m_locked){

        QPointF range = m_xAxis->maxRange();
        if(index==0){
            val2 = val1 - m_dis;
            if(val2>range.y()){
                val2 = range.y();
                val1 =val2 + m_dis;
            }
            if(val2<range.x()){
                val2 = range.x();
                val1 =val2 + m_dis;
            }
        }else{
            val2 = val1;
            val1 = val2 + m_dis;
            if(val1>range.y()){
                val1 = range.y();
                val2 =val1 - m_dis;
            }
            if(val1<range.x()){
                val1 = range.x();
                val2 =val1 - m_dis;
            }
        }
        modelData.at(0)->setXVal(val1);
        modelData.at(1)->setXVal(val2);
    }else{
        modelData.at(index)->setXVal(val1);
        m_dis = modelData.at(0)->xVal()-modelData.at(1)->xVal();
        emit disChanged();
    }
    emit infoChanged();
}

bool DoubleCursorModel::locked() const
{
    return m_locked;
}

void DoubleCursorModel::setLocked(bool locked)
{
    m_locked = locked;
    emit lockedChanged();
}

qreal DoubleCursorModel::dis() const
{
    if(m_xAxis!=nullptr)
        return qAbs(m_dis)/(m_xAxis->upper()-m_xAxis->lower());
    else
        return qAbs(m_dis);
}

void DoubleCursorModel::setDis(const qreal &dis)
{
    m_dis = dis;
    emit disChanged();
}

QVariantMap DoubleCursorModel::info()
{
    if(m_curveModel==nullptr){
        return QVariantMap();
    }
    QVariantList list;
    QStringList title;
    for(int i = 0; i<m_curveModel->rowCount(); i++){
        CurveModel *cm = m_curveModel->getModel(i);
        QVariantMap curveInfo;
        curveInfo.insert("color",cm->lineColor());
        curveInfo.insert("curve",cm->title());
        foreach (CursorModel *cursor, modelData) {
            CursorData *cd = cursor->cursorDataList()->findByCurve(cm);
            title.push_back(cursor->posFlag());
            QString str;
            if(cd!=nullptr){
                str = QString::number(cd->value().y());
            }
            curveInfo.insert("pos"+cursor->posFlag(),str);
        }
        list.push_back(curveInfo);
    }
    QVariantMap map;
    map.insert("data",list);
    return map;
}

bool DoubleCursorModel::checked() const
{
    return m_checked;
}

void DoubleCursorModel::setChecked(bool checked)
{
    m_checked = checked;
    emit checkedChanged();
}


DoubleCursorListModel::DoubleCursorListModel(QObject *parent):QAbstractListModel(parent)
{

}

int DoubleCursorListModel::rowCount(const QModelIndex &parent) const
{
    return modelData.size();
}

QVariant DoubleCursorListModel::data(const QModelIndex &index, int role) const
{
    return QVariant::fromValue<DoubleCursorModel*>(modelData.at(index.row()));
}

QHash<int, QByteArray> DoubleCursorListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(0,"cModel");
    return roles;
}

void DoubleCursorListModel::initDoubleCursor(qreal pos)
{
    beginInsertRows(QModelIndex(),modelData.size(),modelData.size());
    DoubleCursorModel* scm = new DoubleCursorModel(this);
    scm->setXAxis(m_xAxis);
    scm->setCurveModel(m_clm);
    scm->initDoubleCursor(pos);
    modelData.push_back(scm);
    endInsertRows();
}

void DoubleCursorListModel::delCursor(int index)
{
    DoubleCursorModel* temp = modelData.at(index);
    beginRemoveRows(QModelIndex(),index,index);
    modelData.remove(index);
    endRemoveRows();
    delete temp;
}

void DoubleCursorListModel::setCurveModel(CurveListModel *clm)
{
    m_clm = clm;
    foreach(DoubleCursorModel* model,modelData){
        model->setCurveModel(clm);
    }
}

void DoubleCursorListModel::setXAxisModel(AxisModel *am)
{
    m_xAxis = am;
    foreach(DoubleCursorModel* model,modelData){
        model->setXAxis(am);
    }
}

void DoubleCursorListModel::update()
{
    foreach(DoubleCursorModel* model,modelData){
        model->update();
    }
}

void DoubleCursorListModel::cancelChoose()
{
    foreach(DoubleCursorModel* model,modelData){
        model->setChecked(false);
    }
}
