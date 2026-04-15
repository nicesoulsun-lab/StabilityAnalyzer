#include "menumodel.h"

MenuModel::MenuModel(QObject *parent) : QAbstractListModel(parent)
{
    m_roles.insert(0,"menu");
}

MenuModel::~MenuModel()
{
    qDeleteAll(modelData);
}

int MenuModel::rowCount(const QModelIndex &parent) const
{
    return modelData.size();
}

QVariant MenuModel::data(const QModelIndex &index, int role) const
{
    int rowIndex = index.row();
    if(rowIndex<0||rowIndex>=modelData.size()){
        return QVariant();
    }
    return QVariant::fromValue<MenuInfo*>(modelData[rowIndex]);
}

QHash<int, QByteArray> MenuModel::roleNames() const
{
    return m_roles;
}

void MenuModel::add(MenuInfo *info)
{
    beginInsertRows(QModelIndex(),modelData.size(),modelData.size());
    modelData.push_back(info);
    endInsertRows();
}

MenuInfo::MenuInfo(MenuInfo *parent):QObject(parent)
{
    m_children = new MenuModel(this);
    if(parent!=nullptr){
        parent->addChild(this);
    }
}

void MenuInfo::addChild(MenuInfo *child)
{
    m_children->add(child);
    emit sizeChanged();
}

int MenuInfo::size()
{
    return m_children->rowCount();
}

QString MenuInfo::name() const
{
    return m_name;
}

void MenuInfo::setName(const QString &name)
{
    m_name = name;
    emit nameChanged();
}

bool MenuInfo::enabled() const
{
    return m_enabled;
}

void MenuInfo::setEnabled(bool enabled)
{
    m_enabled = enabled;
    emit enabledChanged();
}

MenuModel *MenuInfo::children() const
{
    return m_children;
}

void MenuInfo::click()
{
    emit clickSignal();
}

QPoint MenuInfo::pos() const
{
    return m_pos;
}

void MenuInfo::setPos(const QPoint &pos)
{
    m_pos = pos;
    emit posChanged();
}

bool MenuInfo::choose() const
{
    return m_choose;
}

void MenuInfo::setChoose(bool choose)
{
    m_choose = choose;
    emit chooseChanged();
}
