#include "DataModel/user_sql_listmodel.h"
#include "SqlOrmManager.h"

user_sql_listmodel::user_sql_listmodel(QObject* parent)
    : QAbstractTableModel(parent)
{
    reloadFromDb();
}

int user_sql_listmodel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_users.size();
}

int user_sql_listmodel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3;
}

QVariant user_sql_listmodel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_users.size())
        return QVariant();

    const QVariantMap &user = m_users[index.row()];

    if (role == UsernameRole) return user["username"];
    if (role == PasswordRole) return user["password"];
    if (role == LvRole) return user["role"];

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case 0: return user["username"];
        case 1: return user["password"];
        case 2: return user["role"];
        default: return QVariant();
        }
    }
    return QVariant();
}

QVariant user_sql_listmodel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return QStringLiteral("用户名");
        case 1: return QStringLiteral("密码");
        case 2: return QStringLiteral("等级");
        default: return QVariant();
        }
    }
    return QVariant();
}

QHash<int, QByteArray> user_sql_listmodel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[UsernameRole] = "username";
    roles[PasswordRole] = "password";
    roles[LvRole] = "lv";
    return roles;
}

void user_sql_listmodel::setUsers(const QVector<QVariantMap> &users)
{
    beginResetModel();
    m_users = users;
    endResetModel();
}

QVariant user_sql_listmodel::get(int row, const QString &roleName) const
{
    if (row < 0 || row >= m_users.size()) return QVariant();
    const QVariantMap &user = m_users[row];
    if (roleName == "id") return user["id"];
    if (roleName == "username") return user["username"];
    if (roleName == "password") return user["password"];
    if (roleName == "lv") return user["role"];
    return QVariant();
}

void user_sql_listmodel::reloadFromDb()
{
    try {
        auto users = SQLORM->getAllUsers();
        setUsers(users);
        emit success();
    } catch (...) {
        setUsers({});
        emit error("加载用户数据失败");
    }
}

int user_sql_listmodel::count() const
{
    return m_users.size();
}

bool user_sql_listmodel::addUser(const QString &username, const QString &password, int lv)
{
    QVariantMap userData;
    userData["username"] = username;
    userData["password"] = password;
    userData["role"] = lv;

    if (SQLORM->addUser(userData)) {
        reloadFromDb();
        emit success();
        return true;
    } else {
        emit error("添加用户失败");
        return false;
    }
}

bool user_sql_listmodel::updateUser(int row, const QString &username, const QString &password, int lv)
{
    if (row < 0 || row >= m_users.size()) {
        emit error("无效的行索引");
        return false;
    }

    int userId = m_users[row]["id"].toInt();
    QVariantMap userData;
    userData["username"] = username;
    userData["password"] = password;
    userData["role"] = lv;

    if (SQLORM->updateUser(userId, userData)) {
        reloadFromDb();
        emit success();
        return true;
    } else {
        emit error("更新用户失败");
        return false;
    }
}

bool user_sql_listmodel::deleteUser(int row)
{
    if (row < 0 || row >= m_users.size()) {
        emit error("无效的行索引");
        return false;
    }

    int userId = m_users[row]["id"].toInt();

    if (SQLORM->deleteUser(userId)) {
        reloadFromDb();
        emit success();
        return true;
    } else {
        emit error("删除用户失败");
        return false;
    }
}
