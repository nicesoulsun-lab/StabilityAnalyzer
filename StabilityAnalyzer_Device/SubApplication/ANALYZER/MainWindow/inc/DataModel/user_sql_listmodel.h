#ifndef USER_SQL_LISTVIEW_H
#define USER_SQL_LISTVIEW_H

#include <QAbstractTableModel>
#include <QVector>
#include <QVariantMap>
#include "mainwindow_global.h"

/**
 * @brief 用户列表模型类
 * @class user_sql_listview
 *
 * 为 CustomListView_user.qml 提供用户数据模型，
 * 使用 SqlOrmManager 作为数据库后端。
 *
 * 字段映射：
 * - username: 用户名
 * - password: 密码
 * - lv: 等级（对应数据库的 role 字段）
 *
 * @par QML 使用示例：
 * @code
 * CustomListView_user {
 *     model: userSqlListView
 *     onRowSelected: {
 *         console.log("选中：", col1, col2, col3)
 *     }
 * }
 * @endcode
 */
class MAINWINDOW_EXPORT user_sql_listmodel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum UserRoles {
        UsernameRole = Qt::UserRole + 1,
        PasswordRole,
        LvRole
    };

    explicit user_sql_listmodel(QObject* parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setUsers(const QVector<QVariantMap> &users);

    /**
     * @brief 获取指定行和角色的数据
     * @param row 行索引
     * @param roleName 角色名称（"id", "username", "password", "lv"）
     * @return 数据值
     */
    Q_INVOKABLE QVariant get(int row, const QString &roleName) const;

    /**
     * @brief 从数据库重新加载用户数据
     */
    Q_INVOKABLE void reloadFromDb();

    /**
     * @brief 获取用户数量
     * @return 用户数量
     */
    Q_INVOKABLE int count() const;

    /**
     * @brief 添加用户
     * @param username 用户名
     * @param password 密码
     * @param lv 等级（0-管理员，1-操作员）
     * @return 成功返回 true
     */
    Q_INVOKABLE bool addUser(const QString &username, const QString &password, int lv);

    /**
     * @brief 更新用户
     * @param row 行索引
     * @param username 用户名
     * @param password 密码
     * @param lv 等级
     * @return 成功返回 true
     */
    Q_INVOKABLE bool updateUser(int row, const QString &username, const QString &password, int lv);

    /**
     * @brief 删除用户
     * @param row 行索引
     * @return 成功返回 true
     */
    Q_INVOKABLE bool deleteUser(int row);

signals:
    /**
     * @brief 操作成功信号
     */
    void success();

    /**
     * @brief 操作失败信号
     * @param message 错误消息
     */
    void error(const QString &message);

private:
    QVector<QVariantMap> m_users;
};

#endif // USER_SQL_LISTVIEW_H
