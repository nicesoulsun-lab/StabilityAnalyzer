#include "multicursormodel.h"

MultiCursorModel::MultiCursorModel(QObject *parent):CursorListModel(parent)
{
    connect(this,&CursorListModel::zoomChanged,[=](){
        emit disChanged();
    });
}

void MultiCursorModel::initMultiFreqCursor(int index, qreal pos,int maxsum)
{
    if(pos<=0||m_curveModel==nullptr||m_xAxis==nullptr)
      return;

    CursorModel cm;
    cm.setModel(m_curveModel);
    qreal xVal = cm.getNear(pos).x()/(index+1);
    setDis(xVal);
    QPointF range = m_xAxis->maxRange();
    int sum = 0;
    if(!range.isNull()){
        sum = range.y()/xVal;
    }
    sum = sum>maxsum?maxsum:sum;
    m_maxsum = maxsum;
    if(sum<modelData.size()){
        int delSum = modelData.size()-sum;
        beginRemoveRows(QModelIndex(),sum,sum+delSum);
        for(int i = 0; i < delSum; i++){
            CursorModel *cm = modelData.at(modelData.size()-1);
            delete cm;
            modelData.pop_back();
        }
        for(int i = 0; i<modelData.size(); i++){
            qreal val = xVal*(i+1);
            modelData.at(i)->setXVal(val);
            modelData.at(i)->setPosFlag(QString::number(i+1));
        }
        endRemoveRows();
    }else{
        int addsum = sum-modelData.size();
        for(int i = 0; i < addsum; i++){
            CursorModel* cm = new CursorModel(this);
            cm->setModel(m_curveModel);
            cm->setXAxis(m_xAxis);
            cm->setVisible(true);
            add(cm);
        }
        for(int i = 0; i<modelData.size(); i++){
            qreal val = xVal*(i+1);
            modelData.at(i)->setXVal(val);
            modelData.at(i)->setPosFlag(QString::number(i+1));
        }
    }

    setVisible(true);
    emit infoChanged();
}

void MultiCursorModel::initMultiFreqCursor(int index, qreal pos)
{
    if(pos<=0||m_curveModel==nullptr||m_xAxis==nullptr)
      return;

    CursorModel cm;
    cm.setModel(m_curveModel);
    qreal xVal = cm.getNear(pos).x()/(index+1);
    setDis(xVal);
    QPointF range = m_xAxis->maxRange();
    int sum = 0;
    if(!range.isNull()){
        sum = range.y()/xVal;
    }

    sum = sum>m_maxsum?m_maxsum:sum;

    if(sum<modelData.size()){
        int delSum = modelData.size()-sum;
        beginRemoveRows(QModelIndex(),sum,sum+delSum);
        for(int i = 0; i < delSum; i++){
            CursorModel *cm = modelData.at(modelData.size()-1);
            delete cm;
            modelData.pop_back();
        }
        for(int i = 0; i<modelData.size(); i++){
            qreal val = xVal*(i+1);
            modelData.at(i)->setXVal(val);
            modelData.at(i)->setPosFlag(QString::number(i+1));
        }
        endRemoveRows();
    }else{
        int addsum = sum-modelData.size();
        for(int i = 0; i < addsum; i++){
            CursorModel* cm = new CursorModel(this);
            cm->setModel(m_curveModel);
            cm->setXAxis(m_xAxis);
            cm->setVisible(true);
            add(cm);
        }
        for(int i = 0; i<modelData.size(); i++){
            qreal val = xVal*(i+1);
            modelData.at(i)->setXVal(val);
            modelData.at(i)->setPosFlag(QString::number(i+1));
        }
    }

    setVisible(true);
    emit infoChanged();
}

void MultiCursorModel::initMultiFreqCursorByFm(qreal fm)
{
    if(m_curveModel==nullptr||m_xAxis==nullptr)
      return;

    int sum = 0;
    setDis(fm);
    QPointF range = m_xAxis->maxRange();
    if(!range.isNull()){
        sum = range.y()/fm;
    }
    if(sum>10)
      sum = 10;
    if(sum<modelData.size()){
      int delSum = modelData.size()-sum;
      beginRemoveRows(QModelIndex(),sum,sum+delSum);
      for(int i = 0; i < delSum; i++){
          CursorModel *cm = modelData.at(modelData.size()-1);
          delete cm;
          modelData.pop_back();
      }
      for(int i = 0; i<modelData.size(); i++){
          qreal val = fm*(i+1);
          modelData.at(i)->setXVal(val);
          modelData.at(i)->setPosFlag(QString::number(i+1));
      }
      endRemoveRows();
    }else{
      int addsum = sum-modelData.size();
      for(int i = 0; i < addsum; i++){
          CursorModel* cm = new CursorModel(this);
          cm->setModel(m_curveModel);
          cm->setXAxis(m_xAxis);
          cm->setVisible(true);
          add(cm);
      }
      for(int i = 0; i<modelData.size(); i++){
          qreal val = fm*(i+1);
          modelData.at(i)->setXVal(val);
          modelData.at(i)->setPosFlag(QString::number(i+1));
      }
    }

    setVisible(true);
    emit infoChanged();
}

void MultiCursorModel::multiFreqCursorRightJump(int index)
{
    qreal xVal = modelData.at(index)->rightJump()/(index+1);
    initMultiFreqCursorByFm(xVal);
}

void MultiCursorModel::multiFreqCursorLeftJump(int index)
{
    qreal xVal = modelData.at(index)->leftJump()/(index+1);
    initMultiFreqCursorByFm(xVal);
}

qreal MultiCursorModel::dis() const
{
    if(m_xAxis!=nullptr)
        return m_dis/(m_xAxis->upper()-m_xAxis->lower());
    else
        return m_dis;
}

void MultiCursorModel::setDis(const qreal &dis)
{
    m_dis = dis;
    emit disChanged();
}

bool MultiCursorModel::checked() const
{
    return m_checked;
}

void MultiCursorModel::setChecked(bool checked)
{
    m_checked = checked;
    emit checkedChanged();
}

MultiCursorListModel::MultiCursorListModel(QObject *parent):QAbstractListModel(parent)
{

}

int MultiCursorListModel::rowCount(const QModelIndex &parent) const
{
    return modelData.size();
}

QVariant MultiCursorListModel::data(const QModelIndex &index, int role) const
{
    return QVariant::fromValue<MultiCursorModel*>(modelData.at(index.row()));
}

QHash<int, QByteArray> MultiCursorListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(0,"cModel");
    return roles;
}

void MultiCursorListModel::initMultiCursor(int index,qreal pos,int sum)
{
    beginInsertRows(QModelIndex(),modelData.size(),modelData.size());
    MultiCursorModel* scm = new MultiCursorModel(this);
    scm->setXAxis(m_xAxis);
    scm->setCurveModel(m_clm);
    scm->initMultiFreqCursor(index,pos,sum);
    modelData.push_back(scm);
    endInsertRows();
}

void MultiCursorListModel::delCursor(int index)
{
    MultiCursorModel* temp = modelData.at(index);
    beginRemoveRows(QModelIndex(),index,index);
    modelData.remove(index);
    endRemoveRows();
    delete temp;
}

void MultiCursorListModel::setCurveModel(CurveListModel *clm)
{
    m_clm = clm;
    foreach(MultiCursorModel* model,modelData){
        model->setCurveModel(clm);
    }
}

void MultiCursorListModel::setXAxisModel(AxisModel *am)
{
    m_xAxis = am;
    foreach(MultiCursorModel* model,modelData){
        model->setXAxis(am);
    }
}

void MultiCursorListModel::update()
{
    foreach(MultiCursorModel* model,modelData){
        model->update();
    }
}

void MultiCursorListModel::cancelChoose()
{
    foreach(MultiCursorModel* model,modelData){
        model->setChecked(false);
    }
}
