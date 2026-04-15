#include "cursorlistmodel.h"

CursorListModel::CursorListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_roles.insert(0,"cursor");
}

CursorListModel::~CursorListModel()
{
}

int CursorListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return modelData.size();
}

QVariant CursorListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    return QVariant::fromValue<CursorModel*>(modelData.at(index.row()));
}

void CursorListModel::add(CursorModel *model)
{
    beginInsertRows(QModelIndex(),modelData.size(),modelData.size());
    modelData.push_back(model);
    endInsertRows();
}

void CursorListModel::pop()
{
    beginRemoveRows(QModelIndex(),modelData.size(),modelData.size());
    delete modelData.at(modelData.size()-1);
    modelData.pop_back();
    endRemoveRows();
}

QHash<int, QByteArray> CursorListModel::roleNames() const
{
    return m_roles;
}

void CursorListModel::update()
{
    foreach (CursorModel* cm, modelData) {
        cm->update();
    }
}

void CursorListModel::setCurveModel(CurveListModel *curveModel)
{
    m_curveModel = curveModel;
    emit infoChanged();
}

bool CursorListModel::visible() const
{
    if(m_init&&m_visible){
        return true;
    }
    return false;
}

void CursorListModel::setVisible(bool visible)
{
    m_visible = visible;
    foreach (CursorModel* cm, modelData) {
        cm->setVisible(visible);
    }
    emit visibleChanged();
}

bool CursorListModel::init() const
{
    return m_init;
}

void CursorListModel::setInit(bool init)
{
    m_init = init;
    emit initChanged();
    emit visibleChanged();
}

qreal CursorListModel::pos() const
{
    if(modelData.isEmpty())
        return -1;
    else
        return modelData.at(0)->position();
}

AxisModel *CursorListModel::xAxis() const
{
    return m_xAxis;
}

void CursorListModel::setXAxis(AxisModel *xAxis)
{
    if(m_xAxis!=nullptr){
        disconnect(m_xAxis,0,this,0);
    }
    m_xAxis = xAxis;
    connect(m_xAxis,&AxisModel::zoomChanged,this,[=](){
        emit zoomChanged();
    });
}

CursorModel *CursorListModel::getModel(int index)
{
    return modelData.at(index);
}

void CursorListModel::clear()
{
    beginResetModel();
    qDeleteAll(modelData);
    modelData.clear();
    setInit(false);
    setVisible(false);
    emit infoChanged();
    endResetModel();
}

void CursorListModel::addLabel(QJsonArray &array)
{
    foreach (CursorModel* cm, modelData) {
        cm->addLabel(array);
    }
}

void CursorListModel::clearChecked()
{
    foreach (CursorModel* cm, modelData) {
        cm->clearChecked();
    }
}

void CursorListModel::checkedAll()
{
    foreach (CursorModel* cm, modelData) {
        cm->checkedAll();
    }
}

void CursorListModel::removeLabel()
{
    foreach (CursorModel* cm, modelData) {
        cm->removeLabel();
    }
}

void CursorListModel::removeAllLabel()
{
    foreach (CursorModel* cm, modelData) {
        cm->removeAllLabel();
    }
}


