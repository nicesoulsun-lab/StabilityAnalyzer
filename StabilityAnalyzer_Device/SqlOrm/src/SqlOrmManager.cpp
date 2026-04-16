/**
 * @file SqlOrmManager.cpp
 * @brief SQLite ORM 数据库管理器实现文件
 * 
 * 本文件实现了基于 sqlite_orm 的数据库操作封装，提供用户、项目、实验及实验数据的管理功能。
 * 使用 sqlite_orm 模板库进行对象关系映射，将 C++ 结构体映射到 SQLite 数据库表。
 * 
 * @author StabilityAnalyzer Team
 * @date 2024
 */

#include "SqlOrmManager.h"
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariantMap>
#include <QVector>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QCoreApplication>

// 引入 sqlite_orm 头文件（仅在此文件中包含模板代码）
// 注意：sqlite_orm 是头文件库，模板实例化在此文件中完成，避免在其他文件中重复编译
#include "sqlite_orm/sqlite_orm.h"
#include "qt_sqlite_orm.h"

using namespace sqlite_orm;

namespace {
constexpr int kExperimentDataInsertChunkSize = 180;
}

// ============================================================================
// 数据模型定义（与 sqlManager 保持一致）
// ============================================================================

/**
 * @struct User
 * @brief 用户数据模型
 * 
 * 映射到数据库的 users 表，存储用户基本信息。
 */
struct User {
    int id;                    ///< 主键，自增
    QString username;          ///< 用户名，唯一
    QString password;          ///< 密码
    int role;                  ///< 权限：0-操作员，1-管理员
    QString created_at;        ///< 创建时间（字符串格式）
};

/**
 * @struct Project
 * @brief 工程项目数据模型
 * 
 * 映射到数据库的 projects 表，存储工程项目信息。
 */
struct Project {
    int id;                    ///< 主键，自增
    QString project_name;      ///< 工程名称
    QString description;       ///< 备注/描述
    int creator_id;            ///< 创建者 ID（关联用户表）
    QString created_at;        ///< 创建时间
    QString updated_at;        ///< 更新时间
};

/**
 * @struct Experiment
 * @brief 实验数据模型
 * 
 * 映射到数据库的 experiments 表，存储实验配置和状态信息。
 */
struct Experiment {
    int id;                    ///< 主键，自增
    int project_id;            ///< 所属工程 ID（外键）
    QString sample_name;       ///< 样品名称
    QString operator_name;     ///< 测试者
    QString description;       ///< 备注
    int creator_id;            ///< 创建者 ID（关联用户表）
    int duration;              ///< 持续时间（秒）
    int interval;              ///< 间隔时间（秒）
    int count;                 ///< 次数
    bool temperature_control;  ///< 是否控温
    double target_temp;        ///< 目标温度
    int scan_range_start;   ///< 扫描区间起始值
    int scan_range_end;     ///< 扫描区间结束值
    int scan_step;             ///< 扫描步长
    int status;                ///< 实验状态：0-未导入，1-已导入（PC 端导入）
    QString created_at;        ///< 创建时间
};

/**
 * @struct ExperimentData
 * @brief 实验测量数据模型
 * 
 * 映射到数据库的 experiment_data 表，存储实验过程中的测量数据。
 * 包含高度与背散射/透射光强的对应关系。
 */
struct ExperimentData {
    int id;                        ///< 主键，自增
    int experiment_id;             ///< 所属实验 ID（外键）
    int timestamp;                 ///< 时间戳（秒）
    double height;                 ///< 高度
    double backscatter_intensity;  ///< 背射光强
    double transmission_intensity; ///< 透射光强
};

/**
 * @struct OperationLog
 * @brief 操作日志数据模型
 * 
 * 映射到数据库的 operation_log 表，记录用户操作行为。
 */
struct OperationLog {
    int id;                ///< 主键，自增
    QString username;      ///< 操作用户名
    int user_id;           ///< 用户 ID
    QString operation;     ///< 操作类型（Login/Logout/CreateProject/StartExperiment 等）
    QString target;        ///< 操作对象（如工程名、实验ID等）
    QString detail;        ///< 操作详情（可选补充信息）
    QString created_at;    ///< 操作时间
};

// ============================================================================
// 数据库存储类
// ============================================================================

/**
 * @class DatabaseStorage
 * @brief 数据库存储结构定义类
 * 
 * 使用 sqlite_orm 的 make_storage 定义数据库表结构，包括：
 * - users: 用户表
 * - projects: 工程项目表
 * - experiments: 实验表
 * - experiment_data: 实验数据表
 * 
 * 该类负责定义表结构、字段约束（主键、唯一、自增等）和表之间的关系。
 */
class DatabaseStorage {
public:
    /**
     * @brief 初始化数据库存储
     * @param dbPath 数据库文件路径
     * @return 数据库存储对象
     * 
     * 创建数据库连接并定义所有表结构。如果数据库文件不存在，会自动创建。
     */
    static auto initStorage(const QString& dbPath) {
        return make_storage(dbPath.toStdString(),
                            // User 表 - 存储用户信息
                            make_table("users",
                                       make_column("id", &User::id, primary_key().autoincrement()),
                                       make_column("username", &User::username, unique()),
                                       make_column("password", &User::password),
                                       make_column("role", &User::role),
                                       make_column("created_at", &User::created_at)
                                       ),
                            // Project 表 - 存储工程项目信息
                            make_table("projects",
                                       make_column("id", &Project::id, primary_key().autoincrement()),
                                       make_column("project_name", &Project::project_name),
                                       make_column("description", &Project::description),
                                       make_column("creator_id", &Project::creator_id),
                                       make_column("created_at", &Project::created_at),
                                       make_column("updated_at", &Project::updated_at)
                                       ),
                            // Experiment 表 - 存储实验配置和状态
                            make_table("experiments",
                                       make_column("id", &Experiment::id, primary_key().autoincrement()),
                                       make_column("project_id", &Experiment::project_id),
                                       make_column("sample_name", &Experiment::sample_name),
                                       make_column("operator_name", &Experiment::operator_name),
                                       make_column("description", &Experiment::description),
                                       make_column("creator_id", &Experiment::creator_id),
                                       make_column("duration", &Experiment::duration),
                                       make_column("interval", &Experiment::interval),
                                       make_column("count", &Experiment::count),
                                       make_column("temperature_control", &Experiment::temperature_control),
                                       make_column("target_temp", &Experiment::target_temp),
                                       make_column("scan_range_start", &Experiment::scan_range_start),
                                       make_column("scan_range_end", &Experiment::scan_range_end),
                                       make_column("scan_step", &Experiment::scan_step),
                                       make_column("status", &Experiment::status),
                                       make_column("created_at", &Experiment::created_at)
                                       ),
                            // ExperimentData 表 - 存储实验测量数据
                            make_table("experiment_data",
                                       make_column("id", &ExperimentData::id, primary_key().autoincrement()),
                                       make_column("experiment_id", &ExperimentData::experiment_id),
                                       make_column("timestamp", &ExperimentData::timestamp),
                                       make_column("height", &ExperimentData::height),
                                       make_column("backscatter_intensity", &ExperimentData::backscatter_intensity),
                                       make_column("transmission_intensity", &ExperimentData::transmission_intensity)
                                       ),
                            // OperationLog 表 - 存储用户操作日志
                            make_table("operation_log",
                                       make_column("id", &OperationLog::id, primary_key().autoincrement()),
                                       make_column("username", &OperationLog::username),
                                       make_column("user_id", &OperationLog::user_id),
                                       make_column("operation", &OperationLog::operation),
                                       make_column("target", &OperationLog::target),
                                       make_column("detail", &OperationLog::detail),
                                       make_column("created_at", &OperationLog::created_at)
                                       )
                            );
    }
};

// ============================================================================
// 私有实现类
// ============================================================================

/**
 * @class SqlOrmManagerPrivate
 * @brief SqlOrmManager 的私有实现类（Pimpl 模式）
 * 
 * 使用 Pimpl（Pointer to Implementation）模式隐藏实现细节，减少编译依赖。
 * 包含数据库存储对象、路径、初始化状态和事务状态等私有成员。
 */
class SqlOrmManagerPrivate {
public:
    SqlOrmManagerPrivate(SqlOrmManager *q) : q_ptr(q) {}
    
    /// 数据库存储类型（使用 decltype 推导 sqlite_orm 的复杂模板类型）
    using StorageType = decltype(DatabaseStorage::initStorage(QString()));
    
    std::unique_ptr<StorageType> storage;  ///< 数据库存储对象
    QString dbPath;                         ///< 数据库文件路径
    bool initialized = false;               ///< 初始化状态标志
    bool inTransaction = false;             ///< 事务状态标志
    
    SqlOrmManager *q_ptr;                   ///< 指向公共类的指针
    Q_DECLARE_PUBLIC(SqlOrmManager)
};

// ============================================================================
// 单例模式实现
// ============================================================================

// 单例实例和互斥锁（线程安全）
static SqlOrmManager* s_instance = nullptr;
static QMutex s_instanceMutex;

/**
 * @brief 获取单例实例（线程安全的双重检查锁定）
 * @return SqlOrmManager 单例指针
 * 
 * 使用双重检查锁定（Double-Checked Locking）模式确保线程安全：
 * 1. 第一次检查：避免不必要的锁操作，提高性能
 * 2. 加锁：确保线程安全
 * 3. 第二次检查：防止多线程竞争条件下的重复创建
 */
SqlOrmManager* SqlOrmManager::instance() {
    if (s_instance) {
        return s_instance;
    }
    
    QMutexLocker locker(&s_instanceMutex);
    
    if (!s_instance) {
        s_instance = new SqlOrmManager();
    }
    
    return s_instance;
}

/**
 * @brief 构造函数
 * @param parent 父对象
 * 
 * 初始化私有数据成员、设置数据库路径，并自动初始化数据库。
 */
SqlOrmManager::SqlOrmManager(QObject *parent)
    : QObject(parent)
    , d_ptr(new SqlOrmManagerPrivate(this))
{
    Q_D(SqlOrmManager);
    
    // 设置数据库路径 - 使用应用程序目录的绝对路径
    QString appDir = QCoreApplication::applicationDirPath();
    d->dbPath = appDir + "/data/app_database.db";
    
    qDebug() << "[SqlOrmManager] 数据库路径：" << d->dbPath;
    
    // 自动初始化数据库
    initialize();
}

/**
 * @brief 析构函数
 * 
 * 清理资源，如果正在事务中则回滚事务。
 */
SqlOrmManager::~SqlOrmManager() {
    Q_D(SqlOrmManager);
    
    if (d->inTransaction) {
        rollbackTransaction();
    }
    
    delete d;
}

/**
 * @brief 初始化数据库
 * 
 * 创建数据库目录（如果不存在），然后创建数据库连接并同步表结构。
 * 如果数据库文件不存在会自动创建。该方法只需调用一次，重复调用会被跳过。
 * 
 * 主要操作：
 * 1. 创建数据库目录（如果不存在）
 * 2. 创建数据库存储对象
 * 3. 同步数据库架构（自动创建/更新表结构）
 * 4. 设置初始化标志
 */
void SqlOrmManager::initialize() {
    Q_D(SqlOrmManager);
    
    if (d->initialized) {
        qDebug() << "[SqlOrmManager] 已经初始化过，跳过";
        return;
    }
    
    try {
        // 创建数据库目录（如果不存在）
        QFileInfo fileInfo(d->dbPath);
        QDir dir = fileInfo.absoluteDir();
        if (!dir.exists()) {
            qDebug() << "[SqlOrmManager] 创建数据库目录：" << dir.absolutePath();
            if (!dir.mkpath(".")) {
                qCritical() << "[SqlOrmManager] 无法创建数据库目录：" << dir.absolutePath();
                emit errorOccurred("无法创建数据库目录");
                return;
            }
        }
        
        // 检查数据库文件是否存在
        bool dbFileExists = QFile::exists(d->dbPath);
        qDebug() << "[SqlOrmManager] 数据库文件是否存在：" << dbFileExists;
        
        // 初始化数据库存储
        d->storage = std::make_unique<SqlOrmManagerPrivate::StorageType>(
                    DatabaseStorage::initStorage(d->dbPath)
                    );
        
        // 如果数据库文件已存在，先读取一下看看有多少用户
        if (dbFileExists) {
            try {
                auto existingUsers = d->storage->get_all<User>();
                qDebug() << "[SqlOrmManager] 现有用户数量：" << existingUsers.size();
                for (const auto& u : existingUsers) {
                    qDebug() << "  - 用户：" << u.id << u.username << u.role;
                }
            } catch (const std::exception& e) {
                qWarning() << "[SqlOrmManager] 读取现有用户失败（可能表不存在）：" << QString::fromStdString(e.what());
                // 表不存在，需要 sync_schema
                dbFileExists = false;
            }
        }
        
        // 仅在数据库文件不存在时才调用 sync_schema
        // 如果数据库文件已存在，sync_schema 会清空数据！
        if (!dbFileExists) {
            qDebug() << "[SqlOrmManager] 数据库文件不存在，执行 sync_schema 创建表结构";
            d->storage->sync_schema();
        } else {
            qDebug() << "[SqlOrmManager] 数据库文件已存在，跳过 sync_schema 避免数据丢失";
        }
        
        // 检查最终用户数量
        try {
            auto finalUsers = d->storage->get_all<User>();
            qDebug() << "[SqlOrmManager] 最终用户数量：" << finalUsers.size();
            for (const auto& u : finalUsers) {
                qDebug() << "  - 用户：" << u.id << u.username << u.role;
            }
        } catch (const std::exception& e) {
            qWarning() << "[SqlOrmManager] 读取用户失败：" << QString::fromStdString(e.what());
        }
        
        d->initialized = true;
        qDebug() << "[SqlOrmManager] 数据库初始化成功，路径：" << d->dbPath;
    } catch (const std::exception& e) {
        qCritical() << "[SqlOrmManager] 数据库初始化失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

// ============================================================================
// 事务管理
// ============================================================================

/**
 * @brief 开始事务
 * @return 成功返回 true，失败返回 false
 * 
 * 开始一个数据库事务，后续的数据库操作将在事务中执行，
 * 直到调用 commitTransaction() 提交或 rollbackTransaction() 回滚。
 * 
 * 使用场景：
 * - 批量数据插入/更新
 * - 需要保证原子性的多个操作
 * - 数据一致性要求高的场景
 */
bool SqlOrmManager::beginTransaction() {
    Q_D(SqlOrmManager);
    
    if (!d->initialized || d->inTransaction) {
        return false;
    }
    
    try {
        d->storage->begin_transaction();
        d->inTransaction = true;
        emit transactionStarted();
        qDebug() << "[SqlOrmManager] 事务开始";
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 开始事务失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

/**
 * @brief 提交事务
 * @return 成功返回 true，失败返回 false
 * 
 * 提交当前事务，将所有更改永久保存到数据库。
 */
bool SqlOrmManager::commitTransaction() {
    Q_D(SqlOrmManager);
    
    if (!d->initialized || !d->inTransaction) {
        return false;
    }
    
    try {
        d->storage->commit();
        d->inTransaction = false;
        emit transactionCommitted();
        qDebug() << "[SqlOrmManager] 事务提交";
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 提交事务失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

/**
 * @brief 回滚事务
 * @return 成功返回 true，失败返回 false
 * 
 * 回滚当前事务，撤销所有未提交的更改。
 */
bool SqlOrmManager::rollbackTransaction() {
    Q_D(SqlOrmManager);
    
    if (!d->initialized || !d->inTransaction) {
        return false;
    }
    
    try {
        d->storage->rollback();
        d->inTransaction = false;
        emit transactionRolledBack();
        qDebug() << "[SqlOrmManager] 事务回滚";
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 回滚事务失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

/**
 * @brief 检查是否在事务中
 * @return 在事务中返回 true，否则返回 false
 */
bool SqlOrmManager::isInTransaction() const {
    Q_D(const SqlOrmManager);
    return d->inTransaction;
}

// ============================================================================
// 用户管理
// ============================================================================

/**
 * @brief 添加用户
 * @param userData 用户数据，包含 username、password、role 字段
 * @return 成功返回 true，失败返回 false
 * 
 * 向数据库插入新用户。用户名必须唯一。
 */
bool SqlOrmManager::addUser(const QVariantMap& userData) {
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return false;
    
    try {
        User user;
        user.username = userData.value("username", "").toString();
        user.password = userData.value("password", "").toString();
        user.role = userData.value("role", 0).toInt();
        user.created_at = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        
        d->storage->insert(user);
        qDebug() << "[SqlOrmManager] 用户添加成功：" << user.username;
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 添加用户失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

/**
 * @brief 更新用户信息
 * @param userId 用户 ID
 * @param userData 要更新的用户数据
 * @return 成功返回 true，失败返回 false
 */
bool SqlOrmManager::updateUser(int userId, const QVariantMap& userData) {
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return false;
    
    try {
        auto users = d->storage->get_all<User>(
                    where(c(&User::id) == userId)
                    );
        
        if (users.empty()) {
            qWarning() << "[SqlOrmManager] 用户不存在：" << userId;
            return false;
        }
        
        User& user = users[0];
        
        if (userData.contains("username"))
            user.username = userData.value("username").toString();
        if (userData.contains("password"))
            user.password = userData.value("password").toString();
        if (userData.contains("role"))
            user.role = userData.value("role").toInt();
        
        d->storage->update(user);
        qDebug() << "[SqlOrmManager] 用户更新成功：" << userId;
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 更新用户失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

QVariantMap SqlOrmManager::getUserById(int userId) {
    QVariantMap result;
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return result;
    
    try {
        auto users = d->storage->get_all<User>(
                    where(c(&User::id) == userId)
                    );
        
        if (!users.empty()) {
            const User& user = users[0];
            result["id"] = user.id;
            result["username"] = user.username;
            result["password"] = user.password;
            result["role"] = user.role;
            result["created_at"] = user.created_at;
        }
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 查询用户失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }
    
    return result;
}

QVariantMap SqlOrmManager::getUserByUsername(const QString& username) {
    QVariantMap result;
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return result;
    
    try {
        auto users = d->storage->get_all<User>(
                    where(c(&User::username) == username.toStdString())
                    );
        
        if (!users.empty()) {
            const User& user = users[0];
            result["id"] = user.id;
            result["username"] = user.username;
            result["password"] = user.password;
            result["role"] = user.role;
            result["createdAt"] = user.created_at;
        }
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 查询用户失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }
    
    return result;
}

QVector<QVariantMap> SqlOrmManager::getAllUsers() {
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return result;
    
    try {
        auto users = d->storage->get_all<User>();
        
        for (const auto& user : users) {
            QVariantMap userData;
            userData["id"] = user.id;
            userData["username"] = user.username;
            userData["password"] = user.password;
            userData["role"] = user.role;
            userData["created_at"] = user.created_at;
            result.append(userData);
        }
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 查询所有用户失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }
    
    return result;
}

bool SqlOrmManager::deleteUser(int userId) {
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return false;
    
    try {
        d->storage->remove_all<User>(
                    where(c(&User::id) == userId)
                    );
        qDebug() << "[SqlOrmManager] 用户删除成功：" << userId;
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 删除用户失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

bool SqlOrmManager::validateUser(const QString& username, const QString& password) {
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return false;
    
    try {
        auto users = d->storage->get_all<User>(
                    where(
                        c(&User::username) == username.toStdString()
                        and c(&User::password) == password.toStdString()
                        )
                    );
        
        bool valid = !users.empty();
        qDebug() << "[SqlOrmManager] 用户验证：" << username << (valid ? "成功" : "失败");
        return valid;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 验证用户失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

// ==================== 项目管理 ====================

bool SqlOrmManager::addProject(const QVariantMap& projectData) {
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return false;
    
    try {
        Project project;
        project.project_name = projectData.value("project_name", "").toString();
        project.description = projectData.value("description", "").toString();
        project.creator_id = projectData.value("creator_id", 0).toInt();
        project.created_at = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        project.updated_at = projectData.value("updated_at", "").toString();
        
        d->storage->insert(project);
        qDebug() << "[SqlOrmManager] 项目添加成功：" << project.project_name;
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 添加项目失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

bool SqlOrmManager::updateProject(int projectId, const QVariantMap& projectData) {
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return false;
    
    try {
        auto projects = d->storage->get_all<Project>(
                    where(c(&Project::id) == projectId)
                    );
        
        if (projects.empty()) {
            qWarning() << "[SqlOrmManager] 项目不存在：" << projectId;
            return false;
        }
        
        Project& project = projects[0];
        
        if (projectData.contains("project_name"))
            project.project_name = projectData.value("project_name").toString();
        if (projectData.contains("description"))
            project.description = projectData.value("description").toString();
        if (projectData.contains("creator_id"))
            project.creator_id = projectData.value("creator_id").toInt();
        if (projectData.contains("updated_at"))
            project.updated_at = projectData.value("updated_at").toString();
        
        d->storage->update(project);
        qDebug() << "[SqlOrmManager] 项目更新成功：" << projectId;
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 更新项目失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

QVariantMap SqlOrmManager::getProjectById(int projectId) {
    QVariantMap result;
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return result;
    
    try {
        auto projects = d->storage->get_all<Project>(
                    where(c(&Project::id) == projectId)
                    );
        
        if (!projects.empty()) {
            const Project& project = projects[0];
            result["id"] = project.id;
            result["project_name"] = project.project_name;
            result["description"] = project.description;
            result["creator_id"] = project.creator_id;
            result["created_at"] = project.created_at;
            result["updated_at"] = project.updated_at;
        }
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 查询项目失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }
    
    return result;
}

QVector<QVariantMap> SqlOrmManager::getAllProjects() {
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return result;
    
    try {
        auto projects = d->storage->get_all<Project>();
        
        for (const auto& project : projects) {
            QVariantMap projectData;
            projectData["id"] = project.id;
            projectData["project_name"] = project.project_name;
            projectData["description"] = project.description;
            projectData["creator_id"] = project.creator_id;
            projectData["created_at"] = project.created_at;
            projectData["updated_at"] = project.updated_at;
            result.append(projectData);
        }
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 查询所有项目失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }
    
    return result;
}

QVector<QVariantMap> SqlOrmManager::getProjectsByCreator(int creatorId) {
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return result;
    
    try {
        auto projects = d->storage->get_all<Project>(
                    where(c(&Project::creator_id) == creatorId)
                    );
        
        for (const auto& project : projects) {
            QVariantMap projectData;
            projectData["id"] = project.id;
            projectData["name"] = project.project_name;
            projectData["description"] = project.description;
            projectData["creatorId"] = project.creator_id;
            projectData["createdAt"] = project.created_at;
            result.append(projectData);
        }
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 查询创建者的项目失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }
    
    return result;
}

bool SqlOrmManager::deleteProject(int projectId) {
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return false;
    
    try {
        d->storage->remove_all<Project>(
                    where(c(&Project::id) == projectId)
                    );
        qDebug() << "[SqlOrmManager] 项目删除成功：" << projectId;
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 删除项目失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

// ==================== 实验管理 ====================

bool SqlOrmManager::addExperiment(const QVariantMap& experimentData) {
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return false;
    
    try {
        Experiment experiment;
        experiment.project_id = experimentData.value("project_id", 0).toInt();
        experiment.sample_name = experimentData.value("sample_name", "").toString();
        experiment.operator_name = experimentData.value("operator_name", "").toString();
        experiment.description = experimentData.value("description", "").toString();
        experiment.creator_id = experimentData.value("creator_id", 0).toInt();
        experiment.duration = experimentData.value("duration", 0).toInt();
        experiment.interval = experimentData.value("interval", 0).toInt();
        experiment.count = experimentData.value("count", 0).toInt();
        experiment.temperature_control = experimentData.value("temperature_control", false).toBool();
        experiment.target_temp = experimentData.value("target_temp", 0.0).toDouble();
        experiment.scan_range_start = experimentData.value("scan_range_start", 0.0).toInt();
        experiment.scan_range_end = experimentData.value("scan_range_end", 0.0).toInt();
        experiment.scan_step = experimentData.value("scan_step", 0.0).toInt();
        experiment.status = experimentData.value("status", 0).toInt();
        experiment.created_at = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        
        d->storage->insert(experiment);
        qDebug() << "[SqlOrmManager] 实验添加成功：" << experiment.sample_name;
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 添加实验失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

bool SqlOrmManager::updateExperiment(int experimentId, const QVariantMap& experimentData) {
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return false;
    
    try {
        auto experiments = d->storage->get_all<Experiment>(
                    where(c(&Experiment::id) == experimentId)
                    );
        
        if (experiments.empty()) {
            qWarning() << "[SqlOrmManager] 实验不存在：" << experimentId;
            return false;
        }
        
        Experiment& experiment = experiments[0];
        
        if (experimentData.contains("project_id"))
            experiment.project_id = experimentData.value("project_id").toInt();
        if (experimentData.contains("sample_name"))
            experiment.sample_name = experimentData.value("sample_name").toString();
        if (experimentData.contains("operator_name"))
            experiment.operator_name = experimentData.value("operator_name").toString();
        if (experimentData.contains("description"))
            experiment.description = experimentData.value("description").toString();
        if (experimentData.contains("creator_id"))
            experiment.creator_id = experimentData.value("creator_id").toInt();
        if (experimentData.contains("duration"))
            experiment.duration = experimentData.value("duration").toInt();
        if (experimentData.contains("interval"))
            experiment.interval = experimentData.value("interval").toInt();
        if (experimentData.contains("count"))
            experiment.count = experimentData.value("count").toInt();
        if (experimentData.contains("temperature_control"))
            experiment.temperature_control = experimentData.value("temperature_control").toBool();
        if (experimentData.contains("target_temp"))
            experiment.target_temp = experimentData.value("target_temp").toDouble();
        if (experimentData.contains("scan_range_start"))
            experiment.scan_range_start = experimentData.value("scan_range_start").toInt();
        if (experimentData.contains("scan_range_end"))
            experiment.scan_range_end = experimentData.value("scan_range_end").toInt();
        if (experimentData.contains("scan_step"))
            experiment.scan_step = experimentData.value("scan_step").toInt();
        if (experimentData.contains("status")) {
            experiment.status = experimentData.value("status").toInt();
            if (experiment.status == 2) {  // 状态为 2 表示完成
                // 不需要 completed_at 字段
            }
        }
        
        d->storage->update(experiment);
        qDebug() << "[SqlOrmManager] 实验更新成功：" << experimentId;
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 更新实验失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

bool SqlOrmManager::updateExperimentStatus(int experimentId, int status) {
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return false;
    
    try {
        auto experiments = d->storage->get_all<Experiment>(
                    where(c(&Experiment::id) == experimentId)
                    );
        
        if (experiments.empty()) {
            qWarning() << "[SqlOrmManager] 实验不存在：" << experimentId;
            return false;
        }
        
        Experiment& experiment = experiments[0];
        experiment.status = status;
        
        d->storage->update(experiment);
        qDebug() << "[SqlOrmManager] 实验状态更新成功：" << experimentId << "状态:" << status;
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 更新实验状态失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

QVariantMap SqlOrmManager::getExperimentById(int experimentId) {
    QVariantMap result;
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return result;
    
    try {
        auto experiments = d->storage->get_all<Experiment>(
                    where(c(&Experiment::id) == experimentId)
                    );
        
        if (!experiments.empty()) {
            const Experiment& experiment = experiments[0];
            result["id"] = experiment.id;
            result["project_id"] = experiment.project_id;
            result["sample_name"] = experiment.sample_name;
            result["operator_name"] = experiment.operator_name;
            result["description"] = experiment.description;
            result["creator_id"] = experiment.creator_id;
            result["duration"] = experiment.duration;
            result["interval"] = experiment.interval;
            result["count"] = experiment.count;
            result["temperature_control"] = experiment.temperature_control;
            result["target_temp"] = experiment.target_temp;
            result["scan_range_start"] = experiment.scan_range_start;
            result["scan_range_end"] = experiment.scan_range_end;
            result["scan_step"] = experiment.scan_step;
            result["status"] = experiment.status;
            result["created_at"] = experiment.created_at;
        }
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 查询实验失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }
    
    return result;
}

QVector<QVariantMap> SqlOrmManager::getAllExperiments() {
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return result;
    
    try {
        auto experiments = d->storage->get_all<Experiment>();
        
        for (const auto& experiment : experiments) {
            QVariantMap experimentData;
            experimentData["id"] = experiment.id;
            experimentData["project_id"] = experiment.project_id;
            experimentData["sample_name"] = experiment.sample_name;
            experimentData["operator_name"] = experiment.operator_name;
            experimentData["description"] = experiment.description;
            experimentData["creator_id"] = experiment.creator_id;
            experimentData["duration"] = experiment.duration;
            experimentData["interval"] = experiment.interval;
            experimentData["count"] = experiment.count;
            experimentData["temperature_control"] = experiment.temperature_control;
            experimentData["target_temp"] = experiment.target_temp;
            experimentData["scan_range_start"] = experiment.scan_range_start;
            experimentData["scan_range_end"] = experiment.scan_range_end;
            experimentData["scan_step"] = experiment.scan_step;
            experimentData["status"] = experiment.status;
            experimentData["created_at"] = experiment.created_at;
            
            // 通过 project_id 查询项目名称
            QVariantMap projectData = getProjectById(experiment.project_id);
            experimentData["project_name"] = projectData["project_name"].toString();
            
            result.append(experimentData);
        }
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 查询所有实验失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }
    
    return result;
}

QVector<QVariantMap> SqlOrmManager::getExperimentsByProject(int projectId) {
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return result;
    
    try {
        // 先获取项目名称
        QVariantMap projectData = getProjectById(projectId);
        QString projectName = projectData["project_name"].toString();
        
        auto experiments = d->storage->get_all<Experiment>(
                    where(c(&Experiment::project_id) == projectId)
                    );
        
        for (const auto& experiment : experiments) {
            QVariantMap experimentData;
            experimentData["id"] = experiment.id;
            experimentData["project_id"] = experiment.project_id;
            experimentData["sample_name"] = experiment.sample_name;
            experimentData["operator_name"] = experiment.operator_name;
            experimentData["description"] = experiment.description;
            experimentData["creator_id"] = experiment.creator_id;
            experimentData["duration"] = experiment.duration;
            experimentData["interval"] = experiment.interval;
            experimentData["count"] = experiment.count;
            experimentData["temperature_control"] = experiment.temperature_control;
            experimentData["target_temp"] = experiment.target_temp;
            experimentData["scan_range_start"] = experiment.scan_range_start;
            experimentData["scan_range_end"] = experiment.scan_range_end;
            experimentData["scan_step"] = experiment.scan_step;
            experimentData["status"] = experiment.status;
            experimentData["created_at"] = experiment.created_at;
            experimentData["project_name"] = projectName;
            result.append(experimentData);
        }
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 查询项目的实验失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }
    
    return result;
}

QVector<QVariantMap> SqlOrmManager::getExperimentsByUser(const QString& operatorName) {
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return result;
    
    try {
        auto experiments = d->storage->get_all<Experiment>(
                    where(c(&Experiment::operator_name) == operatorName.toStdString())
                    );
        
        for (const auto& experiment : experiments) {
            QVariantMap experimentData;
            experimentData["id"] = experiment.id;
            experimentData["project_id"] = experiment.project_id;
            experimentData["sample_name"] = experiment.sample_name;
            experimentData["operator_name"] = experiment.operator_name;
            experimentData["description"] = experiment.description;
            experimentData["creator_id"] = experiment.creator_id;
            experimentData["duration"] = experiment.duration;
            experimentData["interval"] = experiment.interval;
            experimentData["count"] = experiment.count;
            experimentData["temperature_control"] = experiment.temperature_control;
            experimentData["target_temp"] = experiment.target_temp;
            experimentData["scan_range_start"] = experiment.scan_range_start;
            experimentData["scan_range_end"] = experiment.scan_range_end;
            experimentData["scan_step"] = experiment.scan_step;
            experimentData["status"] = experiment.status;
            experimentData["created_at"] = experiment.created_at;
            
            // 通过 project_id 查询项目名称
            QVariantMap projectData = getProjectById(experiment.project_id);
            experimentData["project_name"] = projectData["project_name"].toString();
            
            result.append(experimentData);
        }
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 查询用户的实验失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }
    
    return result;
}

QVector<QVariantMap> SqlOrmManager::getExperimentsByCreator(int creatorId) {
    // 实验表没有 creatorId 字段，此方法返回空
    Q_UNUSED(creatorId)
    return QVector<QVariantMap>();
}

QVector<QVariantMap> SqlOrmManager::getExperimentsByStatus(int status) {
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return result;
    
    try {
        auto experiments = d->storage->get_all<Experiment>(
                    where(c(&Experiment::status) == status)
                    );
        
        for (const auto& experiment : experiments) {
            QVariantMap experimentData;
            experimentData["id"] = experiment.id;
            experimentData["project_id"] = experiment.project_id;
            experimentData["sample_name"] = experiment.sample_name;
            experimentData["operator_name"] = experiment.operator_name;
            experimentData["description"] = experiment.description;
            experimentData["creator_id"] = experiment.creator_id;
            experimentData["duration"] = experiment.duration;
            experimentData["interval"] = experiment.interval;
            experimentData["count"] = experiment.count;
            experimentData["temperature_control"] = experiment.temperature_control;
            experimentData["target_temp"] = experiment.target_temp;
            experimentData["scan_range_start"] = experiment.scan_range_start;
            experimentData["scan_range_end"] = experiment.scan_range_end;
            experimentData["scan_step"] = experiment.scan_step;
            experimentData["status"] = experiment.status;
            experimentData["created_at"] = experiment.created_at;
            
            // 通过 project_id 查询项目名称
            QVariantMap projectData = getProjectById(experiment.project_id);
            experimentData["project_name"] = projectData["project_name"].toString();
            
            result.append(experimentData);
        }
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 查询指定状态的实验失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }
    
    return result;
}

bool SqlOrmManager::deleteExperiment(int experimentId) {
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return false;
    
    try {
        d->storage->remove_all<Experiment>(
                    where(c(&Experiment::id) == experimentId)
                    );
        qDebug() << "[SqlOrmManager] 实验删除成功：" << experimentId;
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 删除实验失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

// ==================== 实验数据管理 ====================

bool SqlOrmManager::addExperimentData(const QVariantMap& data) {
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return false;
    
    try {
        ExperimentData expData;
        expData.experiment_id = data.value("experiment_id", 0).toInt();
        expData.timestamp = data.value("timestamp", 0).toInt();
        expData.height = data.value("height", 0.0).toDouble();
        expData.backscatter_intensity = data.value("backscatter_intensity", 0.0).toDouble();
        expData.transmission_intensity = data.value("transmission_intensity", 0.0).toDouble();
        
        d->storage->insert(expData);
        qDebug() << "[SqlOrmManager] 实验数据添加成功：" << expData.timestamp;
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 添加实验数据失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

bool SqlOrmManager::batchAddExperimentData(const QVector<QVariantMap>& dataList) {
    Q_D(SqlOrmManager);

    if (!d->initialized) return false;
    if (dataList.isEmpty()) return true;

    const bool alreadyInTransaction = d->inTransaction;
    bool transactionStarted = false;

    try {
        QVector<ExperimentData> rows;
        rows.reserve(dataList.size());

        for (const auto& data : dataList) {
            ExperimentData expData;
            expData.experiment_id = data.value("experiment_id", 0).toInt();
            expData.timestamp = data.value("timestamp", 0).toInt();
            expData.height = data.value("height", 0.0).toDouble();
            expData.backscatter_intensity = data.value("backscatter_intensity", 0.0).toDouble();
            expData.transmission_intensity = data.value("transmission_intensity", 0.0).toDouble();
            rows.append(expData);
        }

        transactionStarted = alreadyInTransaction ? false : beginTransaction();
        if (!alreadyInTransaction && !transactionStarted) {
            qWarning() << "[SqlOrmManager] batchAddExperimentData failed: unable to start transaction";
            return false;
        }

        for (int offset = 0; offset < rows.size(); offset += kExperimentDataInsertChunkSize) {
            const int chunkSize = qMin(kExperimentDataInsertChunkSize, rows.size() - offset);
            d->storage->insert_range(rows.constBegin() + offset, rows.constBegin() + offset + chunkSize);
        }

        if (transactionStarted && !commitTransaction()) {
            qWarning() << "[SqlOrmManager] batchAddExperimentData failed: commit transaction failed";
            rollbackTransaction();
            return false;
        }

        qDebug() << "[SqlOrmManager] 批量添加实验数据成功：" << rows.size()
                 << "条 chunks="
                 << ((rows.size() + kExperimentDataInsertChunkSize - 1) / kExperimentDataInsertChunkSize);
        return true;
    } catch (const std::exception& e) {
        if (transactionStarted && d->inTransaction) {
            rollbackTransaction();
        }
        qWarning() << "[SqlOrmManager] 批量添加实验数据失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

QVariantMap SqlOrmManager::getExperimentDataById(int dataId) {
    QVariantMap result;
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return result;
    
    try {
        auto dataList = d->storage->get_all<ExperimentData>(
                    where(c(&ExperimentData::id) == dataId)
                    );
        
        if (!dataList.empty()) {
            const ExperimentData& expData = dataList[0];
            result["id"] = expData.id;
            result["experiment_id"] = expData.experiment_id;
            result["timestamp"] = expData.timestamp;
            result["height"] = expData.height;
            result["backscatter_intensity"] = expData.backscatter_intensity;
            result["transmission_intensity"] = expData.transmission_intensity;
        }
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 查询实验数据失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }
    
    return result;
}

QVector<QVariantMap> SqlOrmManager::getExperimentDataByExperiment(int experimentId) {
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return result;
    
    try {
        auto dataList = d->storage->get_all<ExperimentData>(
                    where(c(&ExperimentData::experiment_id) == experimentId),
                    order_by(&ExperimentData::timestamp).asc()
                    );
        
        for (const auto& expData : dataList) {
            QVariantMap data;
            data["id"] = expData.id;
            data["experiment_id"] = expData.experiment_id;
            data["timestamp"] = expData.timestamp;
            data["height"] = expData.height;
            data["backscatter_intensity"] = expData.backscatter_intensity;
            data["transmission_intensity"] = expData.transmission_intensity;
            result.append(data);
        }
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 查询实验数据失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }
    
    return result;
}

QVector<QVariantMap> SqlOrmManager::getExperimentDataByRange(int experimentId, int startTimestamp, int endTimestamp) {
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return result;
    
    try {
        auto dataList = d->storage->get_all<ExperimentData>(
                    where(
                        c(&ExperimentData::experiment_id) == experimentId
                        and c(&ExperimentData::timestamp) >= startTimestamp
                        and c(&ExperimentData::timestamp) <= endTimestamp
                    ),
                    order_by(&ExperimentData::timestamp).asc()
                    );
        
        for (const auto& expData : dataList) {
            QVariantMap data;
            data["id"] = expData.id;
            data["experiment_id"] = expData.experiment_id;
            data["timestamp"] = expData.timestamp;
            data["height"] = expData.height;
            data["backscatter_intensity"] = expData.backscatter_intensity;
            data["transmission_intensity"] = expData.transmission_intensity;
            result.append(data);
        }
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 查询范围实验数据失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }
    
    return result;
}

QVector<QVariantMap> SqlOrmManager::getAllExperimentData() {
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return result;
    
    try {
        auto dataList = d->storage->get_all<ExperimentData>();
        
        for (const auto& expData : dataList) {
            QVariantMap data;
            data["id"] = expData.id;
            data["experiment_id"] = expData.experiment_id;
            data["timestamp"] = expData.timestamp;
            data["height"] = expData.height;
            data["backscatter_intensity"] = expData.backscatter_intensity;
            data["transmission_intensity"] = expData.transmission_intensity;
            result.append(data);
        }
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 查询所有实验数据失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }
    
    return result;
}

bool SqlOrmManager::updateExperimentData(int dataId, const QVariantMap& data) {
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return false;
    
    try {
        auto dataList = d->storage->get_all<ExperimentData>(
                    where(c(&ExperimentData::id) == dataId)
                    );
        
        if (dataList.empty()) {
            qWarning() << "[SqlOrmManager] 实验数据不存在：" << dataId;
            return false;
        }
        
        ExperimentData& expData = dataList[0];
        
        if (data.contains("timestamp"))
            expData.timestamp = data.value("timestamp").toInt();
        if (data.contains("height"))
            expData.height = data.value("height").toDouble();
        if (data.contains("backscatterIntensity"))
            expData.backscatter_intensity = data.value("backscatterIntensity").toDouble();
        if (data.contains("transmissionIntensity"))
            expData.transmission_intensity = data.value("transmissionIntensity").toDouble();
        
        d->storage->update(expData);
        qDebug() << "[SqlOrmManager] 实验数据更新成功：" << dataId;
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 更新实验数据失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

bool SqlOrmManager::deleteExperimentData(int dataId) {
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return false;
    
    try {
        d->storage->remove_all<ExperimentData>(
                    where(c(&ExperimentData::id) == dataId)
                    );
        qDebug() << "[SqlOrmManager] 实验数据删除成功：" << dataId;
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 删除实验数据失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

bool SqlOrmManager::deleteExperimentDataByExperiment(int experimentId) {
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return false;
    
    try {
        d->storage->remove_all<ExperimentData>(
                    where(c(&ExperimentData::experiment_id) == experimentId)
                    );
        qDebug() << "[SqlOrmManager] 实验数据批量删除成功：" << experimentId;
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 批量删除实验数据失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

// ============================================================================
// 操作日志管理
// ============================================================================

bool SqlOrmManager::addOperationLog(const QVariantMap& logData) {
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return false;
    
    try {
        OperationLog log;
        log.username = logData.value("username", "").toString();
        log.user_id = logData.value("user_id", -1).toInt();
        log.operation = logData.value("operation", "").toString();
        log.target = logData.value("target", "").toString();
        log.detail = logData.value("detail", "").toString();
        log.created_at = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        
        d->storage->insert(log);
        qDebug() << "[SqlOrmManager] 操作日志记录成功：" << log.username << "-" << log.operation;
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 添加操作日志失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

QVector<QVariantMap> SqlOrmManager::getAllOperationLogs() {
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return result;
    
    try {
        auto logs = d->storage->get_all<OperationLog>(
                    order_by(&OperationLog::id).desc()
                    );
        
        for (const auto& log : logs) {
            QVariantMap logData;
            logData["id"] = log.id;
            logData["username"] = log.username;
            logData["user_id"] = log.user_id;
            logData["operation"] = log.operation;
            logData["target"] = log.target;
            logData["detail"] = log.detail;
            logData["created_at"] = log.created_at;
            result.append(logData);
        }
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 查询操作日志失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }
    
    return result;
}

QVector<QVariantMap> SqlOrmManager::getOperationLogsByUser(int userId) {
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return result;
    
    try {
        auto logs = d->storage->get_all<OperationLog>(
                    where(c(&OperationLog::user_id) == userId),
                    order_by(&OperationLog::id).desc()
                    );
        
        for (const auto& log : logs) {
            QVariantMap logData;
            logData["id"] = log.id;
            logData["username"] = log.username;
            logData["user_id"] = log.user_id;
            logData["operation"] = log.operation;
            logData["target"] = log.target;
            logData["detail"] = log.detail;
            logData["created_at"] = log.created_at;
            result.append(logData);
        }
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 按用户查询操作日志失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }
    
    return result;
}

QVector<QVariantMap> SqlOrmManager::getOperationLogsByType(const QString& operation) {
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return result;
    
    try {
        auto logs = d->storage->get_all<OperationLog>(
                    where(c(&OperationLog::operation) == operation.toStdString()),
                    order_by(&OperationLog::id).desc()
                    );
        
        for (const auto& log : logs) {
            QVariantMap logData;
            logData["id"] = log.id;
            logData["username"] = log.username;
            logData["user_id"] = log.user_id;
            logData["operation"] = log.operation;
            logData["target"] = log.target;
            logData["detail"] = log.detail;
            logData["created_at"] = log.created_at;
            result.append(logData);
        }
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 按类型查询操作日志失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }
    
    return result;
}

QVector<QVariantMap> SqlOrmManager::getOperationLogsByTimeRange(const QString& startTime, const QString& endTime) {
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);
    
    if (!d->initialized) return result;
    
    try {
        auto logs = d->storage->get_all<OperationLog>(
                    where(
                        c(&OperationLog::created_at) >= startTime.toStdString()
                        and c(&OperationLog::created_at) <= endTime.toStdString()
                    ),
                    order_by(&OperationLog::id).desc()
                    );
        
        for (const auto& log : logs) {
            QVariantMap logData;
            logData["id"] = log.id;
            logData["username"] = log.username;
            logData["user_id"] = log.user_id;
            logData["operation"] = log.operation;
            logData["target"] = log.target;
            logData["detail"] = log.detail;
            logData["created_at"] = log.created_at;
            result.append(logData);
        }
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 按时间范围查询操作日志失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }
    
    return result;
}

int SqlOrmManager::deleteOldOperationLogs(const QString& beforeTime) {
    Q_D(SqlOrmManager);

    if (!d->initialized) return 0;

    try {
        // 1. 先计算将要删除的记录数量
        auto count = d->storage->count<OperationLog>(
                    where(c(&OperationLog::created_at) < beforeTime.toStdString())
                    );

        if (count > 0) {
            // 2. 执行删除操作 (remove_all 返回 void)
            d->storage->remove_all<OperationLog>(
                        where(c(&OperationLog::created_at) < beforeTime.toStdString())
                        );

            qDebug() << "[SqlOrmManager] 清理旧操作日志，删除" << count << "条记录";
            return static_cast<int>(count);
        } else {
            qDebug() << "[SqlOrmManager] 没有需要清理的旧操作日志";
            return 0;
        }

    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 清理旧操作日志失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return 0;
    }
}

void SqlOrmManager::close() {
    Q_D(SqlOrmManager);
    
    if (d->inTransaction) {
        rollbackTransaction();
    }
    
    d->storage.reset();
    d->initialized = false;
    
    qDebug() << "[SqlOrmManager] 数据库已关闭";
}

bool SqlOrmManager::isValid() const {
    Q_D(const SqlOrmManager);
    return d->initialized && d->storage != nullptr;
}
