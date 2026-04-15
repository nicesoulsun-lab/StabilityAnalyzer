#include "inc/Controller/user_ctrl.h"
#include "SqlOrmManager.h"
#include <QDebug>

userCtrl::userCtrl(QObject *parent)
    : QObject(parent)
    , m_dbManager(SqlOrmManager::instance())
    , m_currentUserId(-1)
    , m_currentRole(-1)
    , m_isLoggedIn(false)
{
    qDebug() << "[userCtrl] 初始化完成";
    
    QVector<QVariantMap> users = m_dbManager->getAllUsers();
    if (users.isEmpty()) {
        qDebug() << "[userCtrl] 用户列表为空，添加默认管理员用户";
        
        QVariantMap userData;
        userData["username"] = "User";
        userData["password"] = "123456";
        userData["role"] = 1;
        
        bool success = m_dbManager->addUser(userData);
        if (success) {
            qDebug() << "[userCtrl] 默认管理员用户添加成功：User/123456";
        } else {
            qWarning() << "[userCtrl] 默认管理员用户添加失败";
        }
    }
}

userCtrl::~userCtrl()
{
    if (m_isLoggedIn) {
        logOperation("登出系统");
    }
    qDebug() << "[userCtrl] 析构";
}

void userCtrl::setCurrentUser(const QString& username, int userId, int role)
{
    m_currentUsername = username;
    m_currentUserId = userId;
    m_currentRole = role;
    m_isLoggedIn = true;
    emit currentUserChanged();
    qDebug() << "[userCtrl] 当前登录用户已设置:" << username << "角色:" << role;
}

void userCtrl::clearCurrentUser()
{
    QString prevUser = m_currentUsername;
    m_currentUsername.clear();
    m_currentUserId = -1;
    m_currentRole = -1;
    m_isLoggedIn = false;
    emit currentUserChanged();
    qDebug() << "[userCtrl] 用户已登出:" << prevUser;
}

bool userCtrl::login(const QString& username, const QString& password)
{
    if (username.isEmpty() || password.isEmpty()) {
        qWarning() << "[userCtrl] 登录失败：用户名或密码不能为空";
        return false;
    }

    QVariantMap user = m_dbManager->getUserByUsername(username);
    
    if (user.isEmpty()) {
        qWarning() << "[userCtrl] 登录失败：用户不存在 -" << username;
        emit operationFailed("用户不存在");
        return false;
    }

    QString storedPassword = user["password"].toString();
    if (storedPassword != password) {
        qWarning() << "[userCtrl] 登录失败：密码错误 -" << username;
        emit operationFailed("密码错误");
        return false;
    }

    int userId = user["id"].toInt();
    int role = user["role"].toInt();

    setCurrentUser(username, userId, role);

    logOperation("登录系统");

    emit loginSuccess(username, role);
    qDebug() << "[userCtrl] 登录成功:" << username << "角色:" << (role == 1 ? "管理员" : "操作员");
    return true;
}

void userCtrl::logout()
{
    if (!m_isLoggedIn) {
        qDebug() << "[userCtrl] 未登录状态，无需登出";
        return;
    }

    logOperation("登出系统");

    clearCurrentUser();
    emit logoutSuccess();
    qDebug() << "[userCtrl] 登出成功";
}

bool userCtrl::shouldMaskPassword() const
{
    if (!m_isLoggedIn) {
        return true;
    }
    return m_currentRole != 1;
}

bool userCtrl::logOperation(const QString& description)
{
    QVariantMap logData;
    logData["username"] = m_isLoggedIn ? m_currentUsername : "System";
    logData["user_id"] = m_currentUserId;
    logData["operation"] = description;

    bool ok = m_dbManager->addOperationLog(logData);
    if (!ok) {
        qWarning() << "[userCtrl] 操作日志记录失败:" << description;
    }
    return ok;
}

QVector<QVariantMap> userCtrl::getOperationLogs()
{
    return m_dbManager->getAllOperationLogs();
}

QVector<QVariantMap> userCtrl::getMyOperationLogs()
{
    if (!m_isLoggedIn || m_currentUserId <= 0) {
        return QVector<QVariantMap>();
    }
    return m_dbManager->getOperationLogsByUser(m_currentUserId);
}

bool userCtrl::addUser(const QString& username, const QString& password, int role)
{
    if (username.isEmpty() || password.isEmpty()) {
        qWarning() << "[userCtrl] 用户名或密码不能为空";
        emit operationFailed("用户名或密码不能为空");
        return false;
    }

    if (role != 1 && role != 0) {
        qWarning() << "[userCtrl] 用户权限错误" << role;
        emit operationFailed("请选择用户权限");
        return false;
    }

    QVariantMap userData;
    userData["username"] = username;
    userData["password"] = password;
    userData["role"] = role;

    bool success = m_dbManager->addUser(userData);

    if (success) {
        auto user = m_dbManager->getUserByUsername(username);
        if (!user.isEmpty()) {
            emit userAdded(user["id"].toInt(), username);
            logOperation(QString("新增用户 %1").arg(username));
            qDebug() << "[userCtrl] 添加用户成功:" << username;
        }
    } else {
        emit operationFailed("添加用户失败");
        qWarning() << "[userCtrl] 添加用户失败:" << username;
    }

    return success;
}

bool userCtrl::updateUser(int userId, const QString& username, const QString& password, int role)
{
    if (userId <= 0) {
        qWarning() << "[userCtrl] 用户 ID 无效";
        return false;
    }

    QVariantMap targetUser = m_dbManager->getUserById(userId);
    if (targetUser.isEmpty()) {
        qWarning() << "[userCtrl] 目标用户不存在，ID:" << userId;
        emit operationFailed("目标用户不存在");
        return false;
    }

    int targetRole = targetUser["role"].toInt();

    if (m_currentRole != 1) {
        qWarning() << "[userCtrl] 非管理员无权修改用户信息";
        emit operationFailed("仅管理员可修改用户信息");
        return false;
    }

    if (targetRole == 1 && userId != m_currentUserId) {
        qWarning() << "[userCtrl] 管理员不能修改其他管理员的信息";
        emit operationFailed("不能修改其他管理员的信息");
        return false;
    }

    if (userId == m_currentUserId && !password.isEmpty()) {
        logOperation("修改自身密码");
    } else if (userId != m_currentUserId) {
        logOperation(QString("修改用户 %1 的信息").arg(targetUser["username"].toString()));
    }

    QVariantMap updateData;
    if (!username.isEmpty()) {
        updateData["username"] = username;
    }
    if (!password.isEmpty()) {
        updateData["password"] = password;
    }
    updateData["role"] = role;

    bool success = m_dbManager->updateUser(userId, updateData);

    if (success) {
        if (m_currentUserId == userId && !username.isEmpty()) {
            m_currentUsername = username;
            emit currentUserChanged();
        }
        emit userUpdated(userId);
        qDebug() << "[userCtrl] 更新用户成功，ID:" << userId;
    } else {
        emit operationFailed("更新用户失败");
        qWarning() << "[userCtrl] 更新用户失败，ID:" << userId;
    }

    return success;
}

QVariantMap userCtrl::getUserById(int userId)
{
    if (userId <= 0) {
        qWarning() << "[userCtrl] 用户 ID 无效";
        return QVariantMap();
    }

    return m_dbManager->getUserById(userId);
}

QVariantMap userCtrl::getUserByUsername(const QString& username)
{
    if (username.isEmpty()) {
        qWarning() << "[userCtrl] 用户名不能为空";
        return QVariantMap();
    }

    return m_dbManager->getUserByUsername(username);
}

QVector<QVariantMap> userCtrl::getAllUsers()
{
    return m_dbManager->getAllUsers();
}

bool userCtrl::validateUser(const QString& username, const QString& password)
{
    if (username.isEmpty() || password.isEmpty()) {
        qWarning() << "[userCtrl] 用户名或密码不能为空";
        return false;
    }

    bool success = m_dbManager->validateUser(username, password);

    if (success) {
        qDebug() << "[userCtrl] 用户验证成功:" << username;
    } else {
        qWarning() << "[userCtrl] 用户验证失败:" << username;
    }

    return success;
}

bool userCtrl::deleteUser(int userId)
{
    if (userId <= 0) {
        qWarning() << "[userCtrl] 用户 ID 无效";
        return false;
    }

    if (userId == m_currentUserId) {
        qWarning() << "[userCtrl] 不能删除当前登录的用户";
        emit operationFailed("不能删除当前登录的用户");
        return false;
    }

    QVariantMap targetUser = m_dbManager->getUserById(userId);
    QString targetName = targetUser.value("username", "").toString();

    bool success = m_dbManager->deleteUser(userId);

    if (success) {
        emit userDeleted(userId);
        logOperation(QString("删除用户 %1").arg(targetName));
        qDebug() << "[userCtrl] 删除用户成功，ID:" << userId;
    } else {
        emit operationFailed("删除用户失败");
        qWarning() << "[userCtrl] 删除用户失败，ID:" << userId;
    }

    return success;
}
