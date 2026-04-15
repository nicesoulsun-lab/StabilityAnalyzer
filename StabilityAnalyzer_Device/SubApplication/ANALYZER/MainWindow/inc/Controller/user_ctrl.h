#ifndef USER_CTRL_H
#define USER_CTRL_H

#include <QObject>
#include <QVariantMap>
#include <QVector>
#include "mainwindow_global.h"

class SqlOrmManager;

class MAINWINDOW_EXPORT userCtrl : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString currentUsername READ currentUsername NOTIFY currentUserChanged)
    Q_PROPERTY(int currentRole READ currentRole NOTIFY currentUserChanged)
    Q_PROPERTY(int currentUserId READ currentUserId NOTIFY currentUserChanged)
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn NOTIFY currentUserChanged)
    Q_PROPERTY(bool isAdmin READ isAdmin NOTIFY currentUserChanged)

public:
    explicit userCtrl(QObject *parent = nullptr);
    ~userCtrl();

    QString currentUsername() const { return m_currentUsername; }
    int currentRole() const { return m_currentRole; }
    int currentUserId() const { return m_currentUserId; }
    bool isLoggedIn() const { return m_isLoggedIn; }
    bool isAdmin() const { return m_currentRole == 1; }

    // ==================== 用户管理 ====================
    Q_INVOKABLE bool addUser(const QString& username, const QString& password, int role);
    Q_INVOKABLE bool updateUser(int userId, const QString& username, const QString& password, int role);
    Q_INVOKABLE QVariantMap getUserById(int userId);
    Q_INVOKABLE QVariantMap getUserByUsername(const QString& username);
    Q_INVOKABLE QVector<QVariantMap> getAllUsers();
    Q_INVOKABLE bool validateUser(const QString& username, const QString& password);
    Q_INVOKABLE bool deleteUser(int userId);

    // ==================== 登录/登出 ====================
    Q_INVOKABLE bool login(const QString& username, const QString& password);
    Q_INVOKABLE void logout();

    // ==================== 密码显示控制 ====================
    Q_INVOKABLE bool shouldMaskPassword() const;

    // ==================== 操作日志 ====================
    Q_INVOKABLE bool logOperation(const QString& description);
    Q_INVOKABLE QVector<QVariantMap> getOperationLogs();
    Q_INVOKABLE QVector<QVariantMap> getMyOperationLogs();

signals:
    void userAdded(int userId, const QString& username);
    void userUpdated(int userId);
    void userDeleted(int userId);
    void operationFailed(const QString& message);
    void currentUserChanged();
    void loginSuccess(const QString& username, int role);
    void logoutSuccess();

private:
    void setCurrentUser(const QString& username, int userId, int role);
    void clearCurrentUser();

    SqlOrmManager* m_dbManager;
    QString m_currentUsername;
    int m_currentUserId = -1;
    int m_currentRole = -1;
    bool m_isLoggedIn = false;
};

#endif // USER_CTRL_H
