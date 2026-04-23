#ifndef SQLORMMANAGER_H
#define SQLORMMANAGER_H

/**
 * @file SqlOrmManager.h
 * @brief SQLite ORM 数据库管理器头文件
 * 
 * 提供基于 sqlite_orm 的数据库操作接口，包括：
 * - 用户管理（增删改查、验证）
 * - 项目管理（增删改查）
 * - 实验管理（增删改查、状态更新）
 * - 实验数据管理（增删改查、批量操作）
 * - 事务管理（开始、提交、回滚）
 */

#include <QObject>
#include <QVariantMap>
#include <QVector>
#include <QString>
#include "sqlorm_global.h"

// 前向声明
class SqlOrmManagerPrivate;

/**
 * @class SqlOrmManager
 * @brief SQLite ORM 数据库管理器
 * 
 * 单例模式的数据库管理器，提供完整的 ORM 数据库操作接口。
 * 使用 sqlite_orm 库进行对象关系映射，支持用户、项目、实验和实验数据的管理。
 * 
 * @par 使用示例：
 * @code
 * // 初始化数据库
 * SQLORM->initialize();
 * 
 * // 添加用户
 * QVariantMap userData;
 * userData["username"] = "admin";
 * userData["password"] = "123456";
 * userData["role"] = 1;
 * SQLORM->addUser(userData);
 * 
 * // 使用事务批量插入数据
 * SQLORM->beginTransaction();
 * // ... 执行多个数据库操作
 * SQLORM->commitTransaction();
 * @endcode
 * 
 * @note 所有方法都是线程安全的
 * @see SqlOrmManager::instance()
 */
class SQLORM_EXPORT SqlOrmManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     * @return SqlOrmManager 单例指针
     * 
     * 使用双重检查锁定确保线程安全。
     * 
     * @par 使用示例：
     * @code
     * SqlOrmManager* manager = SqlOrmManager::instance();
     * // 或使用宏
     * SQLORM->initialize();
     * @endcode
     */
    static SqlOrmManager* instance();
    
    /**
     * @brief 初始化数据库
     * 
     * 创建数据库连接并同步表结构。
     * 该方法只需调用一次，应在程序启动时调用。
     * 
     * @note 数据库文件路径：./data/app_database.db
     */
    void initialize();
    
    // ========================================================================
    // 事务管理
    // ========================================================================
    
    /**
     * @brief 开始事务
     * @return 成功返回 true，失败返回 false
     * 
     * 开始一个数据库事务，后续操作将在事务中执行。
     * 
     * @see commitTransaction()
     * @see rollbackTransaction()
     */
    bool beginTransaction();
    
    /**
     * @brief 提交事务
     * @return 成功返回 true，失败返回 false
     * 
     * 提交当前事务，保存所有更改。
     */
    bool commitTransaction();
    
    /**
     * @brief 回滚事务
     * @return 成功返回 true，失败返回 false
     * 
     * 回滚当前事务，撤销所有未提交的更改。
     */
    bool rollbackTransaction();
    
    /**
     * @brief 检查是否在事务中
     * @return 在事务中返回 true，否则返回 false
     */
    
    // ========================================================================
    // 用户管理
    // ========================================================================
    
    /**
     * @brief 添加用户
     * @param userData 用户数据（username、password、role）
     * @return 成功返回 true，失败返回 false
     */
    bool addUser(const QVariantMap& userData);
    
    /**
     * @brief 更新用户信息
     * @param userId 用户 ID
     * @param userData 要更新的数据
     * @return 成功返回 true，失败返回 false
     */
    bool updateUser(int userId, const QVariantMap& userData);
    
    /**
     * @brief 根据 ID 查询用户
     * @param userId 用户 ID
     * @return 用户数据，不存在返回空 QVariantMap
     */
    QVariantMap getUserById(int userId);
    
    /**
     * @brief 根据用户名查询用户
     * @param username 用户名
     * @return 用户数据，不存在返回空 QVariantMap
     */
    QVariantMap getUserByUsername(const QString& username);
    
    /**
     * @brief 获取所有用户
     * @return 用户数据列表
     */
    QVector<QVariantMap> getAllUsers();
    
    /**
     * @brief 删除用户
     * @param userId 用户 ID
     * @return 成功返回 true，失败返回 false
     */
    bool deleteUser(int userId);
    
    /**
     * @brief 验证用户登录
     * @param username 用户名
     * @param password 密码
     * @return 验证成功返回 true，失败返回 false
     */
    bool validateUser(const QString& username, const QString& password);
    
    // ========================================================================
    // 项目管理
    // ========================================================================
    
    /**
     * @brief 添加项目
     * @param projectData 项目数据
     * @return 成功返回 true，失败返回 false
     */
    bool addProject(const QVariantMap& projectData);
    
    /**
     * @brief 更新项目信息
     * @param projectId 项目 ID
     * @param projectData 要更新的数据
     * @return 成功返回 true，失败返回 false
     */
    
    /**
     * @brief 根据 ID 查询项目
     * @param projectId 项目 ID
     * @return 项目数据，不存在返回空 QVariantMap
     */
    
    /**
     * @brief 获取所有项目
     * @return 项目数据列表
     */
    QVector<QVariantMap> getAllProjects();
    
    /**
     * @brief 根据创建者 ID 查询项目
     * @param creatorId 创建者 ID
     * @return 项目数据列表
     */
    
    /**
     * @brief 删除项目
     * @param projectId 项目 ID
     * @return 成功返回 true，失败返回 false
     */
    
    // ========================================================================
    // 实验管理
    // ========================================================================
    
    /**
     * @brief 添加实验
     * @param experimentData 实验数据
     * @return 成功返回 true，失败返回 false
     */
    bool addExperiment(const QVariantMap& experimentData);
    
    /**
     * @brief 更新实验信息
     * @param experimentId 实验 ID
     * @param experimentData 要更新的数据
     * @return 成功返回 true，失败返回 false
     */
    
    /**
     * @brief 更新实验状态
     * @param experimentId 实验 ID
     * @param status 状态值
     * @return 成功返回 true，失败返回 false
     */
    bool updateExperimentStatus(int experimentId, int status);
    
    /**
     * @brief 根据 ID 查询实验
     * @param experimentId 实验 ID
     * @return 实验数据，不存在返回空 QVariantMap
     */
    QVariantMap getExperimentById(int experimentId);
    
    /**
     * @brief 获取所有实验
     * @return 实验数据列表
     */
    QVector<QVariantMap> getAllExperiments();
    
    /**
     * @brief 根据操作者查询实验
     * @param operatorName 操作者姓名
     * @return 实验数据列表
     */
    
    /**
     * @brief 根据项目 ID 查询实验
     * @param projectId 项目 ID
     * @return 实验数据列表
     */
    
    /**
     * @brief 根据创建者 ID 查询实验
     * @param creatorId 创建者 ID
     * @return 实验数据列表
     */
    
    /**
     * @brief 根据状态查询实验
     * @param status 状态值
     * @return 实验数据列表
     */
    QVector<QVariantMap> getExperimentsByStatus(int status);
    QVector<QVariantMap> getDeletedExperiments();
    
    /**
     * @brief 删除实验
     * @param experimentId 实验 ID
     * @return 成功返回 true，失败返回 false
     */
    bool deleteExperiment(int experimentId);
    bool restoreExperiment(int experimentId);
    bool hardDeleteExperiment(int experimentId);
    
    // ========================================================================
    // 实验数据管理
    // ========================================================================
    
    /**
     * @brief 添加实验数据
     * @param data 实验数据
     * @return 成功返回 true，失败返回 false
     */
    bool addExperimentData(const QVariantMap& data);
    
    /**
     * @brief 批量添加实验数据
     * @param dataList 实验数据列表
     * @return 成功返回 true，失败返回 false
     * 
     * 使用事务包装，保证原子性。
     */
    bool batchAddExperimentData(const QVector<QVariantMap>& dataList);
    
    /**
     * @brief 根据 ID 查询实验数据
     * @param dataId 数据 ID
     * @return 实验数据，不存在返回空 QVariantMap
     */
    
    /**
     * @brief 根据实验 ID 查询实验数据
     * @param experimentId 实验 ID
     * @return 实验数据列表
     */
    QVector<QVariantMap> getExperimentDataByExperiment(int experimentId);
    // 为光强页返回按 scan 聚合且已降采样的整帧曲线。
    QVector<QVariantMap> getLightIntensityCurvesByExperiment(int experimentId, int pointsPerCurve);
    QVector<QVariantMap> getLightIntensityAveragesByExperiment(int experimentId);
    QVector<QVariantMap> getUniformityIndicesByExperiment(int experimentId);
    // 计算并缓存三区分层厚度结果。
    QVector<QVariantMap> getSeparationLayerDataByExperiment(int experimentId);
    QVector<QVariantMap> getOrComputeInstabilityCurveDataByExperiment(int experimentId);
    // 针对指定高度区间计算不稳定性曲线，用于局部/自定义模式懒加载。
    QVector<QVariantMap> getOrComputeInstabilityCurveDataByHeightRange(int experimentId, double lowerMm, double upperMm, const QString &segmentKey);
    // 删除整体和分区缓存，保证实验重算时不会混入旧结果。
    
    /**
     * @brief 根据时间范围查询实验数据
     * @param experimentId 实验 ID
     * @param startTimestamp 起始时间戳
     * @param endTimestamp 结束时间戳
     * @return 实验数据列表
     */
    QVector<QVariantMap> getExperimentDataByRange(int experimentId, int startTimestamp, int endTimestamp);
    
    /**
     * @brief 获取所有实验数据
     * @return 实验数据列表
     */
    QVector<QVariantMap> getAllExperimentData();
    
    /**
     * @brief 更新实验数据
     * @param dataId 数据 ID
     * @param data 要更新的数据
     * @return 成功返回 true，失败返回 false
     */
    
    /**
     * @brief 删除实验数据
     * @param dataId 数据 ID
     * @return 成功返回 true，失败返回 false
     */
    bool deleteExperimentData(int dataId);
    
    /**
     * @brief 根据实验 ID 删除实验数据
     * @param experimentId 实验 ID
     * @return 成功返回 true，失败返回 false
     */
    bool deleteExperimentDataByExperiment(int experimentId);
    
    // ========================================================================
    // 操作日志管理
    // ========================================================================
    
    /**
     * @brief 添加操作日志
     * @param logData 日志数据（username, user_id, operation, target, detail）
     * @return 成功返回 true，失败返回 false
     */
    bool addOperationLog(const QVariantMap& logData);
    
    /**
     * @brief 获取所有操作日志
     * @return 操作日志列表
     */
    QVector<QVariantMap> getAllOperationLogs();
    
    /**
     * @brief 根据用户 ID 查询操作日志
     * @param userId 用户 ID
     * @return 操作日志列表
     */
    QVector<QVariantMap> getOperationLogsByUser(int userId);
    
    /**
     * @brief 根据操作类型查询日志
     * @param operation 操作类型
     * @return 操作日志列表
     */
    
    /**
     * @brief 根据时间范围查询日志
     * @param startTime 起始时间
     * @param endTime 结束时间
     * @return 操作日志列表
     */
    
    /**
     * @brief 删除指定时间之前的旧日志
     * @param beforeTime 时间阈值
     * @return 删除的记录数
     */

    // ========================================================================
    // 数据库操作
    // ========================================================================
    
    /**
     * @brief 关闭数据库连接
     * 
     * @note 通常不需要手动调用，析构函数会自动清理
     */
    void close();
    
    /**
     * @brief 检查数据库是否有效
     * @return 数据库有效返回 true，否则返回 false
     */
    bool isValid() const;

signals:
    /// 事务开始信号
    void transactionStarted();
    
    /// 事务提交信号
    void transactionCommitted();
    
    /// 事务回滚信号
    void transactionRolledBack();
    
    /// 错误发生信号
    void errorOccurred(const QString& error);

private:
    // 这几项只服务于 SqlOrmManager 内部的辅助链路，不再对外暴露。
    QVariantMap getProjectById(int projectId);
    bool replaceInstabilityCurveData(int experimentId, const QVector<QVariantMap>& curveList);
    QVector<QVariantMap> getInstabilityCurveDataByExperiment(int experimentId);
    bool deleteInstabilityCurveDataByExperiment(int experimentId);

    explicit SqlOrmManager(QObject *parent = nullptr);
    ~SqlOrmManager();
    
    Q_DISABLE_COPY(SqlOrmManager)
    
    SqlOrmManagerPrivate* d_ptr;
    
    Q_DECLARE_PRIVATE(SqlOrmManager)
};

/**
 * @def SQLORM
 * @brief SqlOrmManager 单例访问宏
 * 
 * 等同于 SqlOrmManager::instance()
 * 
 * @par 使用示例：
 * @code
 * SQLORM->initialize();
 * SQLORM->addUser(userData);
 * @endcode
 */
#define SQLORM SqlOrmManager::instance()

#endif // SQLORMMANAGER_H
