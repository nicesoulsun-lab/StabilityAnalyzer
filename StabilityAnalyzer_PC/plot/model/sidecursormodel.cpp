#include "sidecursormodel.h"

SideCursorModel::SideCursorModel(QObject *parent):CursorListModel(parent)
{
    connect(this,&CursorListModel::zoomChanged,[=](){
        emit disChanged();
    });
}

void SideCursorModel::initSideFreqCursor(qreal pos, int sum)
{
    if(sum<0||m_curveModel==nullptr||m_xAxis==nullptr)
        return;
    if(sum>100)
        sum = 100;
    beginResetModel();
    qDeleteAll(modelData);
    modelData.clear();
    CursorModel *cm = new CursorModel(this);
    cm->setXAxis(m_xAxis);
    cm->setModel(m_curveModel);
    cm->setVisible(true);
    qreal xVal = cm->getNear(pos).x();
    cm->setXVal(xVal);
    cm->setPosFlag("中心");
    modelData.push_back(cm);
    setDis(0.4*(m_xAxis->upper()-m_xAxis->lower())/(sum*2));
    for(int i = 0; i<sum;i++){
        CursorModel *cm = new CursorModel(this);
        cm->setVisible(true);
        cm->setXAxis(m_xAxis);
        cm->setModel(m_curveModel);
        cm->setXVal(xVal-m_dis*(sum-i));
        cm->setPosFlag(QString::number(-sum+i));
        modelData.push_back(cm);
    }
    for(int i = 1; i<=sum;i++){
        CursorModel *cm = new CursorModel(this);
        cm->setVisible(true);
        cm->setXAxis(m_xAxis);
        cm->setModel(m_curveModel);
        cm->setXVal(xVal+m_dis*i);
        cm->setPosFlag(QString::number(i));
        modelData.push_front(cm);
    }
    endResetModel();

    setVisible(true);

    emit infoChanged();
}

void SideCursorModel::initSideFreqCursorByFm(qreal fm, int sum)
{
    beginResetModel();
    if(sum<0||m_curveModel==nullptr||m_xAxis==nullptr)
        return;
    if(sum>100)
        sum = 100;
    qDeleteAll(modelData);
    modelData.clear();

    CursorModel *cm = new CursorModel(this);
    cm->setXAxis(m_xAxis);
    cm->setModel(m_curveModel);
    cm->setXVal(fm);
    modelData.push_back(cm);

    setDis(0.4*(m_xAxis->upper()-m_xAxis->lower())/(sum*2));
    for(int i = 0; i<sum;i++){
        CursorModel *cm = new CursorModel(this);
        cm->setVisible(true);
        cm->setXAxis(m_xAxis);
        cm->setModel(m_curveModel);
        cm->setXVal(fm-m_dis*(sum-i));
        cm->setPosFlag(QString::number(sum-i));
        modelData.push_back(cm);
    }
    for(int i = 1; i<=sum;i++){
        CursorModel *cm = new CursorModel(this);
        cm->setVisible(true);
        cm->setXAxis(m_xAxis);
        cm->setModel(m_curveModel);
        cm->setXVal(fm+m_dis*i);
        cm->setPosFlag(QString::number(i));
        modelData.push_front(cm);
    }
    endResetModel();

    setVisible(true);

    emit infoChanged();
}

void SideCursorModel::adjustSideFreqCursor(int index, qreal pos)
{

    int sum = (modelData.size()-1)/2;

    if(index==sum||index>modelData.size()-1||modelData.size()<1)
        return;

    CursorModel cmTemp;
    cmTemp.setModel(m_curveModel);

    qreal xVal = cmTemp.getNear(pos).x();
    qreal mid = modelData.at(sum)->xVal();
    m_dis = (xVal-mid)/(index-sum);
    qreal first = mid - sum*m_dis;
    for(int i = 0; i<modelData.size(); i++){
        modelData.at(i)->setXVal(first+m_dis*i);
    }
    emit infoChanged();
}

void SideCursorModel::moveSideFreqCursor(qreal pos)
{
    if(modelData.size()<1)
        return;

    CursorModel cmTemp;
    cmTemp.setModel(m_curveModel);
    qreal xVal = cmTemp.getNear(pos).x();

    int sum = (modelData.size()-1)/2;

    qreal offset = xVal - modelData.at(sum)->xVal();
    for(int i = 0; i<modelData.size(); i++){
        modelData.at(i)->setXVal(modelData.at(i)->xVal()+offset);
    }

    emit infoChanged();
}

void SideCursorModel::sideFreqCursorRightJump()
{
    if(modelData.size()<1)
        return;

    int sum = (modelData.size()-1)/2;
    qreal xVal = modelData.at(sum)->rightJump();

    qreal offset = xVal - modelData.at(sum)->xVal();
    for(int i = 0; i<modelData.size(); i++){
        modelData.at(i)->setXVal(modelData.at(i)->xVal()+offset);
    }
    emit infoChanged();
}

void SideCursorModel::sideFreqCursorLeftJump()
{
    if(modelData.size()<1)
        return;
    int sum = (modelData.size()-1)/2;
    qreal xVal = modelData.at(sum)->leftJump();

    qreal offset = xVal - modelData.at(sum)->xVal();
    for(int i = 0; i<modelData.size(); i++){
        modelData.at(i)->setXVal(modelData.at(i)->xVal()+offset);
    }
    emit infoChanged();
}

qreal SideCursorModel::dis() const
{
    if(m_xAxis!=nullptr)
        return m_dis/(m_xAxis->upper()-m_xAxis->lower());
    else
        return m_dis;
}

void SideCursorModel::setDis(const qreal &dis)
{
    m_dis = dis;
    emit disChanged();
}

bool SideCursorModel::checked() const
{
    return m_checked;
}

void SideCursorModel::setChecked(bool checked)
{
    m_checked = checked;
    emit checkedChanged();
}

qreal SideCursorModel::bandWidth() const
{
    return QString::number(m_dis,'f',2).toDouble();
}


SideCursorListModel::SideCursorListModel(QObject *parent):QAbstractListModel(parent)
{

}

int SideCursorListModel::rowCount(const QModelIndex &parent) const
{
    return modelData.size();
}

QVariant SideCursorListModel::data(const QModelIndex &index, int role) const
{
    return QVariant::fromValue<SideCursorModel*>(modelData.at(index.row()));
}

QHash<int, QByteArray> SideCursorListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(0,"cModel");
    return roles;
}

void SideCursorListModel::initSideFreqCursor(qreal pos,int sum)
{
    beginInsertRows(QModelIndex(),modelData.size(),modelData.size());
    SideCursorModel* scm = new SideCursorModel(this);
    scm->setXAxis(m_xAxis);
    scm->setCurveModel(m_clm);
    scm->initSideFreqCursor(pos,sum);
    modelData.push_back(scm);
    endInsertRows();
}

void SideCursorListModel::delCursor(int index)
{
    SideCursorModel* temp = modelData.at(index);
    beginRemoveRows(QModelIndex(),index,index);
    modelData.remove(index);
    endRemoveRows();
    delete temp;
}

void SideCursorListModel::setCurveModel(CurveListModel *clm)
{
    m_clm = clm;
    foreach(SideCursorModel* model,modelData){
        model->setCurveModel(clm);
    }
}

void SideCursorListModel::setXAxisModel(AxisModel *am)
{
    m_xAxis = am;
    foreach(SideCursorModel* model,modelData){
        model->setXAxis(am);
    }
}

void SideCursorListModel::update()
{
    foreach(SideCursorModel* model,modelData){
        model->update();
    }
}

void SideCursorListModel::cancelChoose()
{
    foreach(SideCursorModel* model,modelData){
        model->setChecked(false);
    }
}
