#include "cursormodel.h"

CursorModel::CursorModel(QObject *parent) : QObject(parent)
{
    m_cursorDataList = new CursorDataModel(this);
}

CursorModel::~CursorModel()
{
    if(m_xAxis!=nullptr)
        disconnect(m_xAxis,0,this,0);
}

void CursorModel::getPoints()
{
    m_cursorDataList->update(m_xVal);
}

QPointF CursorModel::getPointByX(qreal x, LoopVector<QPointF> *source)
{
    int begin = 0;
    int end = source->size()-1;
    if(source->isEmpty())
        return QPointF(0,0);
    /* 二分法查询 */
    while (begin<end) {
        int mid = (end+begin)/2;
        if(source->at(mid).x()== x){
            return source->at(mid);
        }else if(source->at(mid).x()<x){
            begin = mid+1;
        }else{
            end = mid-1;
        }
    }
    QPointF small,big;
    if(source->at(begin).x()>x){
        if(begin==0)
            return QPointF(x,source->at(begin).y());
        big = source->at(begin);
        small = source->at(begin-1);
    }else{
        if(begin==source->size()-1)
            return QPointF(x,source->at(begin).y());
        small = source->at(begin);
        big = source->at(begin+1);
    }
    if(big.x()-x>x-small.x())
        return small;
    else
        return big;
}

QPointF CursorModel::getMidPointByX(qreal x, LoopVector<QPointF> *source)
{
    int begin = 0;
    int end = source->size()-1;
    if(source->isEmpty())
        return QPointF(0,0);
    /* 二分法查询 */
    while (begin<end) {
        int mid = (end+begin)/2;
        if(source->at(mid).x()== x){
            return source->at(mid);
        }else if(source->at(mid).x()<x){
            begin = mid+1;
        }else{
            end = mid-1;
        }
    }
    QPointF small,big;
    if(source->at(begin).x()>x){
        if(begin==0)
            return QPointF(x,source->at(begin).y());
        big = source->at(begin);
        small = source->at(begin-1);
    }else{
        if(begin==source->size()-1)
            return QPointF(x,source->at(begin).y());
        small = source->at(begin);
        big = source->at(begin+1);
    }
    qreal scale =(x-small.x())
                  /(big.x()-small.x());

    qreal y = (big.y()-small.y())*scale+small.y();
    return QPointF(x,y);
}

QPointF CursorModel::getLeft(qreal x, LoopVector<QPointF> *source)
{
    int begin = 0;
    int end = source->size()-1;
    if(source==nullptr||source->isEmpty())
        return QPointF(0,0);
    /* 二分法查询 */
    while (begin<end) {
        int mid = (end+begin)/2;
        if(source->at(mid).x()== x){
            if(mid==0)
                return source->at(mid);
            else
                return source->at(mid-1);
        }else if(source->at(mid).x()<x){
            begin = mid+1;
        }else{
            end = mid-1;
        }
    }
    int small,big;
    if(source->at(begin).x()>x){
        if(begin==0)
            return source->at(begin);
        big = begin;
        small = begin-1;

    }else{
        if(begin==source->size()-1)
            return source->at(begin);
        small = begin;
        big = begin+1;
    }
    if(source->at(big).x()-x>x-source->at(small).x()){
        if(small==0)
            return source->at(small);
        else
            return source->at(small-1);
    }else{
        if(big==0)
            return source->at(big);
        else
            return source->at(big-1);
    }
}

QPointF CursorModel::getRight(qreal x, LoopVector<QPointF> *source)
{
    int begin = 0;
    int end = source->size()-1;
    if(source==nullptr||source->isEmpty())
        return QPointF(0,0);
    /* 二分法查询 */
    while (begin<end) {
        int mid = (end+begin)/2;
        if(source->at(mid).x()== x){
            if(mid==source->size()-1)
                return source->at(mid);
            else
                return source->at(mid+1);
        }else if(source->at(mid).x()<x){
            begin = mid+1;
        }else{
            end = mid-1;
        }
    }
    int small,big;
    if(source->at(begin).x()>x){
        if(begin==0)
            return source->at(begin);
        big = begin;
        small = begin-1;

    }else{
        if(begin==source->size()-1)
            return source->at(begin);
        small = begin;
        big = begin+1;
    }
    if(source->at(big).x()-x>x-source->at(small).x()){
        if(small==source->size()-1)
            return source->at(small);
        else
            return source->at(small+1);
    }else{
        if(big==source->size()-1)
            return source->at(big);
        else
            return source->at(big+1);
    }
}

QPointF CursorModel::getNear(qreal pos)
{
    if(m_model==nullptr)
        return QPointF();
    QPointF near;
    for(int i = 0; i<m_model->rowCount();i++){
        CurveModel* cm = m_model->getModel(i);
        if(cm->xAxis()==nullptr||cm->yAxis()==nullptr||cm->sourcePtr()==nullptr)
            continue;
        qreal x = (cm->xAxis()->upper()-cm->xAxis()->lower())*pos+cm->xAxis()->lower();
        QPointF p = getPointByX(x,cm->sourcePtr());
        if(p.isNull())
            continue;
        near = p;
        break;
    }
    return near;
}

qreal CursorModel::leftJump()
{
    if(m_model==nullptr||m_model->rowCount()==0)
        return -1;
    QPointF p = getLeft(m_xVal,m_model->getModel(0)->sourcePtr());
    return p.x();
}

qreal CursorModel::rightJump()
{
    if(m_model==nullptr||m_model->rowCount()==0)
        return -1;
    QPointF p = getRight(m_xVal,m_model->getModel(0)->sourcePtr());
    return p.x();
}

qreal CursorModel::position() const
{
    if(m_xAxis==nullptr)
        return -1;
    if((m_xAxis->upper()-m_xAxis->lower())==0){
        return -1;
    }else{
        return (m_xVal-m_xAxis->lower())
                /(m_xAxis->upper()-m_xAxis->lower());
    }
}

bool CursorModel::visible() const
{
    if(m_visible&&m_xAxis!=nullptr&&m_xAxis->lower()<=m_xVal&&m_xVal<=m_xAxis->upper()){
        return true;
    }else{
        return false;
    }
}

void CursorModel::setVisible(const bool &visible)
{
    m_visible = visible;
    emit visibleChanged();
}

void CursorModel::setModel(CurveListModel *model)
{
    m_model = model;
    if(m_model==nullptr)
        return;
    updating = true;
    m_cursorDataList->clear();
    for(int i = 0; i<m_model->rowCount();i++){
        CursorData *data = new CursorData(this);
        CurveModel* cm = m_model->getModel(i);
        if(i==0)
            xType = cm->dataType();
        data->setCurve(cm);
        data->update(m_xVal);
        m_cursorDataList->add(data);
    }
    connect(m_model,&CurveListModel::addSignal,this,[=](CurveModel *curve){
        CursorData *data = new CursorData(this);
        data->setCurve(curve);
        data->update(m_xVal);
        m_cursorDataList->add(data);
    });
    emit dataChanged();
    updating = false;
}

void CursorModel::update()
{
    m_cursorDataList->update();
    emit positionChanged();
    emit visibleChanged();
}

bool CursorModel::init() const
{
    return m_init;
}

void CursorModel::setInit(bool init)
{
    m_init = init;
    emit initChanged();
}

AxisModel *CursorModel::xAxis() const
{
    return m_xAxis;
}

void CursorModel::setXAxis(AxisModel *xAxis)
{
    m_xAxis = xAxis;
    connect(m_xAxis,&AxisModel::zoomChanged,this,[=](){
        emit visibleChanged();
        emit positionChanged();
    });
    connect(m_xAxis,&AxisModel::moveChanged,this,[=](){
        emit visibleChanged();
        emit positionChanged();
    });
}

qreal CursorModel::xVal() const
{
    return m_xVal;
}

void CursorModel::setXVal(const qreal &xVal)
{
    m_xVal = xVal;
    getPoints();
    emit positionChanged();
    emit visibleChanged();
}

QString CursorModel::getXVal()
{
    if(xType == 1){
        QDateTime date = QDateTime::fromTime_t(m_xVal);
        return date.toString("yyyy/MM/dd hh:mm:ss");
    }else{
        return QString::number(m_xVal);
    }
}

CursorDataModel *CursorModel::cursorDataList() const
{
    return m_cursorDataList;
}

void CursorModel::setCursorDataList(CursorDataModel *cursorDataList)
{
    m_cursorDataList = cursorDataList;
    emit cursorDataListChanged();
}

QString CursorModel::posFlag() const
{
    return m_posFlag;
}

void CursorModel::setPosFlag(QString posFlag)
{
    m_posFlag = posFlag;
    emit posFlagChanged();
}

void CursorModel::addLabel(QJsonArray &array)
{
    m_cursorDataList->addLabel(array);
}

void CursorModel::clearChecked()
{
    m_cursorDataList->clearChecked();
}

void CursorModel::checkedAll()
{
    m_cursorDataList->checkedAll();
}

void CursorModel::removeLabel()
{
    m_cursorDataList->removeLabel();
}

void CursorModel::removeAllLabel()
{
    m_cursorDataList->removeAllLabel();
}

CursorData::CursorData(QObject *parent)
{
    
}

QPointF CursorData::value() const
{
    return m_value;
}

void CursorData::setValue(const QPointF &value)
{
    m_value = value;
    if(m_label.isEmpty()){
        emit labelChanged();
    }
    emit valueChanged();
}

CurveModel *CursorData::curve() const
{
    return m_curve;
}

void CursorData::setCurve(CurveModel *curve)
{
    m_curve = curve;
    connect(m_curve,&CurveModel::destroyed,this,[=](){
        emit removeSignal(this);
    });
    emit curveChanged();
}

QPointF CursorData::coord() const
{
    return m_coord;
}

void CursorData::setCoord(const QPointF &coord)
{
    m_coord = coord;
    emit coordChanged();
}

bool CursorData::checked() const
{
    return m_checked;
}

void CursorData::setChecked(bool checked)
{
    m_checked = checked;
    emit checkedChanged();
}

QPoint CursorData::pos() const
{
    return m_pos;
}

void CursorData::setPos(const QPoint &pos)
{
    m_pos = pos;
    emit posChanged();
}

QString CursorData::label() const
{
    if(m_label.isEmpty()){
        double num = QString::number(m_value.x(),'f',2).toDouble();
        return QString::number(num);
    }
    return m_label;
}

void CursorData::setLabel(QString label)
{
    m_label = label;
    emit labelChanged();
}

bool CursorData::showLabel() const
{
    return m_showLabel;
}

void CursorData::setShowLabel(bool showLabel)
{
    m_showLabel = showLabel;
    emit showLabelChanged();
}

void CursorData::update(qreal &xVal)
{
    if(m_curve->xAxis()==nullptr||m_curve->yAxis()==nullptr)
        return;
    QPointF p = getMidPointByX(xVal,m_curve->sourcePtr());
    setValue(p);
    setCoord(m_curve->transformCoords(p));
    if(!p.isNull()&&xVal!=p.x()){
        xVal=p.x();
    }
}

void CursorData::update()
{
    setCoord(m_curve->transformCoords(m_value));
}

QPointF CursorData::getPointByX(qreal x, LoopVector<QPointF> *source)
{
    int begin = 0;
    int end = source->size()-1;
    if(source->isEmpty())
        return QPointF(0,0);
    /* 二分法查询 */
    while (begin<end) {
        int mid = (end+begin)/2;
        if(source->at(mid).x()== x){
            return source->at(mid);
        }else if(source->at(mid).x()<x){
            begin = mid+1;
        }else{
            end = mid-1;
        }
    }
    QPointF small,big;
    if(source->at(begin).x()>x){
        if(begin==0)
            return QPointF(x,0);
        big = source->at(begin);
        small = source->at(begin-1);
    }else{
        if(begin==source->size()-1)
            return QPointF(x,0);
        small = source->at(begin);
        big = source->at(begin+1);
    }
    if(big.x()-x>x-small.x())
        return small;
    else
        return big;
}

QPointF CursorData::getMidPointByX(qreal x, LoopVector<QPointF> *source)
{
    int begin = 0;
    int end = source->size()-1;
    if(source==nullptr||source->isEmpty())
        return QPointF(0,0);
    /* 二分法查询 */
    while (begin<end) {
        int mid = (end+begin)/2;
        if(source->at(mid).x()== x){
            return source->at(mid);
        }else if(source->at(mid).x()<x){
            begin = mid+1;
        }else{
            end = mid-1;
        }
    }
    QPointF small,big;
    if(source->at(begin).x()>x){
        if(begin==0)
            return QPointF(x,0);
        big = source->at(begin);
        small = source->at(begin-1);
    }else{
        if(begin==source->size()-1)
            return QPointF(x,0);
        small = source->at(begin);
        big = source->at(begin+1);
    }
    qreal scale =(x-small.x())
                  /(big.x()-small.x());

    qreal y = (big.y()-small.y())*scale+small.y();
    return QPointF(x,y);
}

QString CursorData::x()
{
    if(m_curve!=nullptr&&m_curve->dataType()==2){
        QDateTime date = QDateTime::fromTime_t(m_value.x());
        return date.toString("yyyy/MM/dd hh:mm:ss");
    }else{
        return QString::number(m_value.x());
    }
}

QString CursorData::y()
{
    return QString::number(m_value.y());
}

CursorDataModel::CursorDataModel(QObject *parent):QAbstractListModel(parent)
{
    m_roles.insert(0,"cursorData");
}

CursorDataModel::~CursorDataModel()
{
    qDeleteAll(modelData);
}

int CursorDataModel::rowCount(const QModelIndex &parent) const
{
    return modelData.size();
}

QVariant CursorDataModel::data(const QModelIndex &index, int role) const
{
    int rowIndex = index.row();
    return QVariant::fromValue<CursorData*>(modelData.at(rowIndex));
}

QHash<int, QByteArray> CursorDataModel::roleNames() const
{
    return m_roles;
}

void CursorDataModel::add(CursorData *data)
{
    beginInsertRows(QModelIndex(),modelData.size(),modelData.size());
    modelData.push_back(data);
    connect(data,&CursorData::removeSignal,this,&CursorDataModel::removeByCursor);
    endInsertRows();
}

void CursorDataModel::clear()
{
    beginResetModel();
    qDeleteAll(modelData);
    modelData.clear();
    endResetModel();
}

void CursorDataModel::removeByCursor(CursorData *data)
{
    int counter = 0;
    foreach (CursorData * cd, modelData) {
        if(cd ==data){
            beginRemoveRows(QModelIndex(),counter,counter);
            delete cd;
            modelData.remove(counter);
            endRemoveRows();
            break;
        }
        counter++;
    }
}

CursorData *CursorDataModel::findByCurve(CurveModel *cm)
{
    foreach (CursorData * cd, modelData) {
        if(cd->curve()==cm)
            return cd;
    }
    return nullptr;
}

void CursorDataModel::update(qreal &xVal)
{
    foreach (CursorData * cd, modelData) {
        cd->update(xVal);
    }
}

void CursorDataModel::update()
{
    foreach (CursorData * cd, modelData) {
        cd->update();
    }
}

void CursorDataModel::addLabel(QJsonArray &array)
{
    foreach(QJsonValue value,array){
        int index = value.toInt();
        if(index<modelData.size()){
            modelData.at(index)->setShowLabel(true);
        }
    }
}

void CursorDataModel::clearChecked()
{
    foreach(CursorData * cd, modelData){
        cd->setChecked(false);
    }
}

void CursorDataModel::checkedAll()
{
    foreach(CursorData * cd, modelData){
        cd->setChecked(true);
    }
}

void CursorDataModel::removeLabel()
{
    foreach(CursorData * cd, modelData){
        if(cd->checked()){
            cd->setChecked(false);
            cd->setLabel("");
            cd->setShowLabel(false);
        }
    }
}

void CursorDataModel::removeAllLabel()
{
    foreach(CursorData * cd, modelData){
        cd->setChecked(false);
        cd->setLabel("");
        cd->setShowLabel(false);
    }
}

