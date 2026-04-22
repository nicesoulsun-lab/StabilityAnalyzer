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
#include <QStringList>
#include <QPointF>
#include <QVariantMap>
#include <QVector>
#include <QHash>
#include <QSet>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QtMath>
#include <limits>
#include <algorithm>

// 引入 sqlite_orm 头文件（仅在此文件中包含模板代码）
// 注意：sqlite_orm 是头文件库，模板实例化在此文件中完成，避免在其他文件中重复编译
#include "sqlite_orm/sqlite_orm.h"
#include "qt_sqlite_orm.h"

using namespace sqlite_orm;

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
    int deleted_flag;          ///< 删除标记：0-正常，1-已删除
    QString deleted_at;        ///< 删除时间
    QString purge_after;       ///< 计划物理删除时间
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
    int scan_id;                   ///< 扫描 ID
    int scan_elapsed_ms;           ///< 距离实验开始的毫秒数
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
                                       make_column("created_at", &Experiment::created_at),
                                       make_column("deleted_flag", &Experiment::deleted_flag),
                                       make_column("deleted_at", &Experiment::deleted_at),
                                       make_column("purge_after", &Experiment::purge_after)
                                       ),
                            // ExperimentData 表 - 存储实验测量数据
                            make_table("experiment_data",
                                       make_column("id", &ExperimentData::id, primary_key().autoincrement()),
                                       make_column("experiment_id", &ExperimentData::experiment_id),
                                       make_column("timestamp", &ExperimentData::timestamp),
                                       make_column("scan_id", &ExperimentData::scan_id),
                                       make_column("scan_elapsed_ms", &ExperimentData::scan_elapsed_ms),
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

namespace {

QSqlDatabase openReadOnlyDb(const QString& dbPath, const QString& connectionName, QString* errorMessage = nullptr) {
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::removeDatabase(connectionName);
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(dbPath);
    if (!db.open() && errorMessage) {
        *errorMessage = db.lastError().text();
    }
    return db;
}

void closeReadOnlyDb(const QString& connectionName) {
    if (!QSqlDatabase::contains(connectionName)) {
        return;
    }
    QSqlDatabase db = QSqlDatabase::database(connectionName, false);
    if (db.isValid()) {
        db.close();
    }
}

QVariantMap readExtendedExperimentDataRow(const QSqlQuery& query) {
    QVariantMap data;
    data["id"] = query.value("id");
    data["experiment_id"] = query.value("experiment_id");
    data["timestamp"] = query.value("timestamp");
    data["scan_id"] = query.value("scan_id");
    data["scan_elapsed_ms"] = query.value("scan_elapsed_ms");
    data["height"] = query.value("height");
    data["backscatter_intensity"] = query.value("backscatter_intensity");
    data["transmission_intensity"] = query.value("transmission_intensity");
    return data;
}

QVariantMap experimentToVariantMap(const Experiment &experiment)
{
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
    experimentData["deleted_flag"] = experiment.deleted_flag;
    experimentData["deleted_at"] = experiment.deleted_at;
    experimentData["purge_after"] = experiment.purge_after;
    return experimentData;
}

struct LightCurveRowEx {
    double heightMm = 0.0;
    double backscatter = 0.0;
    double transmission = 0.0;
};

struct InstabilityRowEx {
    double heightMm = 0.0;
    double backscatter = 0.0;
    double transmission = 0.0;
};

struct SeparationRowEx {
    double heightMm = 0.0;
    double backscatter = 0.0;
    double transmission = 0.0;
};

QVariantList makePointList(const QVector<QPointF> &points)
{
    QVariantList result;
    result.reserve(points.size());
    for (const QPointF &point : points) {
        QVariantMap map;
        map["x"] = point.x();
        map["y"] = point.y();
        result.append(map);
    }
    return result;
}

QVariantList downsampleCurvePoints(const QVector<LightCurveRowEx> &rows, bool useTransmission, int maxPoints)
{
    if (rows.isEmpty()) {
        return QVariantList();
    }

    if (maxPoints <= 2 || rows.size() <= maxPoints) {
        QVector<QPointF> points;
        points.reserve(rows.size());
        for (const LightCurveRowEx &row : rows) {
            points.append(QPointF(row.heightMm, useTransmission ? row.transmission : row.backscatter));
        }
        return makePointList(points);
    }

    const int bucketCount = qMax(1, maxPoints / 2);
    QVector<QPointF> sampled;
    sampled.reserve(bucketCount * 2 + 2);
    sampled.append(QPointF(rows.first().heightMm, useTransmission ? rows.first().transmission : rows.first().backscatter));

    const int innerCount = rows.size() - 2;
    for (int bucket = 0; bucket < bucketCount; ++bucket) {
        const int startIndex = 1 + bucket * innerCount / bucketCount;
        const int endIndex = 1 + (bucket + 1) * innerCount / bucketCount;
        if (startIndex >= rows.size() - 1) {
            break;
        }

        const int safeEnd = qMax(startIndex + 1, qMin(endIndex, rows.size() - 1));
        int minIndex = startIndex;
        int maxIndex = startIndex;
        double minValue = useTransmission ? rows.at(startIndex).transmission : rows.at(startIndex).backscatter;
        double maxValue = minValue;

        for (int i = startIndex + 1; i < safeEnd; ++i) {
            const double value = useTransmission ? rows.at(i).transmission : rows.at(i).backscatter;
            if (value < minValue) {
                minValue = value;
                minIndex = i;
            }
            if (value > maxValue) {
                maxValue = value;
                maxIndex = i;
            }
        }

        if (minIndex <= maxIndex) {
            sampled.append(QPointF(rows.at(minIndex).heightMm, useTransmission ? rows.at(minIndex).transmission : rows.at(minIndex).backscatter));
            if (maxIndex != minIndex) {
                sampled.append(QPointF(rows.at(maxIndex).heightMm, useTransmission ? rows.at(maxIndex).transmission : rows.at(maxIndex).backscatter));
            }
        } else {
            sampled.append(QPointF(rows.at(maxIndex).heightMm, useTransmission ? rows.at(maxIndex).transmission : rows.at(maxIndex).backscatter));
            sampled.append(QPointF(rows.at(minIndex).heightMm, useTransmission ? rows.at(minIndex).transmission : rows.at(minIndex).backscatter));
        }
    }

    sampled.append(QPointF(rows.last().heightMm, useTransmission ? rows.last().transmission : rows.last().backscatter));
    std::sort(sampled.begin(), sampled.end(), [](const QPointF &a, const QPointF &b) {
        if (qFuzzyCompare(a.x(), b.x())) {
            return a.y() < b.y();
        }
        return a.x() < b.x();
    });
    return makePointList(sampled);
}

QVariantList downsamplePointSeries(const QVector<QPointF> &points, int maxPoints)
{
    if (points.isEmpty()) {
        return QVariantList();
    }

    if (maxPoints <= 2 || points.size() <= maxPoints) {
        return makePointList(points);
    }

    const int bucketCount = qMax(1, maxPoints / 2);
    QVector<QPointF> sampled;
    sampled.reserve(bucketCount * 2 + 2);
    sampled.append(points.first());

    const int innerCount = points.size() - 2;
    for (int bucket = 0; bucket < bucketCount; ++bucket) {
        const int startIndex = 1 + bucket * innerCount / bucketCount;
        const int endIndex = 1 + (bucket + 1) * innerCount / bucketCount;
        if (startIndex >= points.size() - 1) {
            break;
        }

        const int safeEnd = qMax(startIndex + 1, qMin(endIndex, points.size() - 1));
        int minIndex = startIndex;
        int maxIndex = startIndex;
        double minValue = points.at(startIndex).y();
        double maxValue = minValue;

        for (int i = startIndex + 1; i < safeEnd; ++i) {
            const double value = points.at(i).y();
            if (value < minValue) {
                minValue = value;
                minIndex = i;
            }
            if (value > maxValue) {
                maxValue = value;
                maxIndex = i;
            }
        }

        if (minIndex <= maxIndex) {
            sampled.append(points.at(minIndex));
            if (maxIndex != minIndex) {
                sampled.append(points.at(maxIndex));
            }
        } else {
            sampled.append(points.at(maxIndex));
            sampled.append(points.at(minIndex));
        }
    }

    sampled.append(points.last());
    std::sort(sampled.begin(), sampled.end(), [](const QPointF &a, const QPointF &b) {
        if (qFuzzyCompare(a.x(), b.x())) {
            return a.y() < b.y();
        }
        return a.x() < b.x();
    });
    return makePointList(sampled);
}

double lightCurveValueAtHeight(const QVector<LightCurveRowEx> &rows, bool useTransmission, double heightMm)
{
    if (rows.isEmpty()) {
        return 0.0;
    }
    if (heightMm <= rows.first().heightMm) {
        return useTransmission ? rows.first().transmission : rows.first().backscatter;
    }
    if (heightMm >= rows.last().heightMm) {
        return useTransmission ? rows.last().transmission : rows.last().backscatter;
    }

    for (int i = 1; i < rows.size(); ++i) {
        const LightCurveRowEx &left = rows.at(i - 1);
        const LightCurveRowEx &right = rows.at(i);
        if (heightMm > right.heightMm) {
            continue;
        }

        const double x0 = left.heightMm;
        const double x1 = right.heightMm;
        const double y0 = useTransmission ? left.transmission : left.backscatter;
        const double y1 = useTransmission ? right.transmission : right.backscatter;
        if (qFuzzyCompare(x0, x1)) {
            return y1;
        }
        const double ratio = (heightMm - x0) / (x1 - x0);
        return y0 + (y1 - y0) * ratio;
    }

    return useTransmission ? rows.last().transmission : rows.last().backscatter;
}

QVector<LightCurveRowEx> filterLightCurveRowsByHeight(const QVector<LightCurveRowEx> &rows, double lowerMm, double upperMm)
{
    QVector<LightCurveRowEx> filtered;
    filtered.reserve(rows.size());
    for (const LightCurveRowEx &row : rows) {
        if (row.heightMm >= lowerMm && row.heightMm <= upperMm) {
            filtered.append(row);
        }
    }
    return filtered;
}

double valueAtHeight(const QVector<InstabilityRowEx> &rows, bool useTransmission, double heightMm)
{
    if (rows.isEmpty()) {
        return 0.0;
    }
    if (heightMm <= rows.first().heightMm) {
        return useTransmission ? rows.first().transmission : rows.first().backscatter;
    }
    if (heightMm >= rows.last().heightMm) {
        return useTransmission ? rows.last().transmission : rows.last().backscatter;
    }

    for (int i = 1; i < rows.size(); ++i) {
        const InstabilityRowEx &left = rows.at(i - 1);
        const InstabilityRowEx &right = rows.at(i);
        if (heightMm > right.heightMm) {
            continue;
        }

        const double x0 = left.heightMm;
        const double x1 = right.heightMm;
        const double y0 = useTransmission ? left.transmission : left.backscatter;
        const double y1 = useTransmission ? right.transmission : right.backscatter;
        if (qFuzzyCompare(x0, x1)) {
            return y1;
        }
        const double ratio = (heightMm - x0) / (x1 - x0);
        return y0 + (y1 - y0) * ratio;
    }

    return useTransmission ? rows.last().transmission : rows.last().backscatter;
}

double averageTransmissionValue(const QVector<InstabilityRowEx> &rows)
{
    if (rows.isEmpty()) {
        return 0.0;
    }

    double sum = 0.0;
    for (const InstabilityRowEx &row : rows) {
        sum += row.transmission;
    }
    return sum / rows.size();
}

double averageTransmissionValue(const QVector<SeparationRowEx> &rows)
{
    if (rows.isEmpty()) {
        return 0.0;
    }

    double sum = 0.0;
    for (const SeparationRowEx &row : rows) {
        sum += row.transmission;
    }
    return sum / rows.size();
}

QVector<double> smoothSeries(const QVector<double> &values, int windowSize)
{
    if (values.isEmpty() || windowSize <= 1) {
        return values;
    }

    const int halfWindow = windowSize / 2;
    QVector<double> smoothed;
    smoothed.reserve(values.size());

    for (int i = 0; i < values.size(); ++i) {
        const int start = qMax(0, i - halfWindow);
        const int end = qMin(values.size() - 1, i + halfWindow);
        double sum = 0.0;
        int count = 0;
        for (int j = start; j <= end; ++j) {
            sum += values.at(j);
            ++count;
        }
        smoothed.append(count > 0 ? sum / count : values.at(i));
    }

    return smoothed;
}

double findBoundaryHeightForThreshold(const QVector<SeparationRowEx> &rows,
                                      const QVector<double> &proxyValues,
                                      double threshold,
                                      bool fromBottom)
{
    if (rows.size() < 2 || rows.size() != proxyValues.size()) {
        return rows.isEmpty() ? 0.0 : rows.first().heightMm;
    }

    int startIndex = fromBottom ? 1 : proxyValues.size() - 1;
    int endIndex = fromBottom ? proxyValues.size() : 0;
    int step = fromBottom ? 1 : -1;

    for (int i = startIndex; i != endIndex; i += step) {
        const int prevIndex = i - step;
        const double leftProxy = proxyValues.at(prevIndex);
        const double rightProxy = proxyValues.at(i);
        const bool crossed = fromBottom
            ? ((leftProxy >= threshold && rightProxy <= threshold) || (leftProxy <= threshold && rightProxy >= threshold))
            : ((leftProxy <= threshold && rightProxy >= threshold) || (leftProxy >= threshold && rightProxy <= threshold));
        if (!crossed) {
            continue;
        }

        const double leftHeight = rows.at(prevIndex).heightMm;
        const double rightHeight = rows.at(i).heightMm;
        if (qFuzzyCompare(leftProxy, rightProxy)) {
            return rightHeight;
        }

        const double ratio = (threshold - leftProxy) / (rightProxy - leftProxy);
        return leftHeight + ratio * (rightHeight - leftHeight);
    }

    return fromBottom ? rows.last().heightMm : rows.first().heightMm;
}

bool ensureSeparationLayerResultTable(QSqlDatabase &db, QString *errorMessage = nullptr)
{
    if (!db.isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("database is not open");
        }
        return false;
    }

    QSqlQuery query(db);
    const QString sql =
        "CREATE TABLE IF NOT EXISTS separation_layer_data ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "experiment_id INTEGER NOT NULL, "
        "scan_id INTEGER DEFAULT 0, "
        "scan_elapsed_ms INTEGER DEFAULT 0, "
        "channel_used TEXT, "
        "clarification_boundary_mm REAL DEFAULT 0, "
        "sediment_boundary_mm REAL DEFAULT 0, "
        "clarification_thickness_mm REAL DEFAULT 0, "
        "concentrated_phase_thickness_mm REAL DEFAULT 0, "
        "sediment_thickness_mm REAL DEFAULT 0, "
        "confidence REAL DEFAULT 0, "
        "created_at TEXT)";
    const bool ok = query.exec(sql);
    if (!ok && errorMessage) {
        *errorMessage = query.lastError().text();
    }
    return ok;
}

double computeInstabilityIntegral(const QVector<InstabilityRowEx> &referenceRows,
                                  const QVector<InstabilityRowEx> &currentRows,
                                  bool useTransmission)
{
    // 以首帧为参考，沿高度方向积分当前帧和参考帧的差值，
    // 得到不稳定性曲线里每个时间点的 Ius 值。
    if (referenceRows.size() < 2 || currentRows.size() < 2) {
        return 0.0;
    }

    const double overlapStart = qMax(referenceRows.first().heightMm, currentRows.first().heightMm);
    const double overlapEnd = qMin(referenceRows.last().heightMm, currentRows.last().heightMm);
    if (overlapEnd <= overlapStart) {
        return 0.0;
    }

    QVector<double> sampleHeights;
    sampleHeights.reserve(referenceRows.size() + currentRows.size() + 2);
    sampleHeights.append(overlapStart);
    for (const InstabilityRowEx &row : referenceRows) {
        if (row.heightMm > overlapStart && row.heightMm < overlapEnd) {
            sampleHeights.append(row.heightMm);
        }
    }
    for (const InstabilityRowEx &row : currentRows) {
        if (row.heightMm > overlapStart && row.heightMm < overlapEnd) {
            sampleHeights.append(row.heightMm);
        }
    }
    sampleHeights.append(overlapEnd);
    std::sort(sampleHeights.begin(), sampleHeights.end());
    sampleHeights.erase(std::unique(sampleHeights.begin(), sampleHeights.end(), [](double a, double b) {
        return qAbs(a - b) < 1e-9;
    }), sampleHeights.end());

    double accumulated = 0.0;
    for (int i = 1; i < sampleHeights.size(); ++i) {
        const double x0 = sampleHeights.at(i - 1);
        const double x1 = sampleHeights.at(i);
        const double diff0 = qAbs(valueAtHeight(referenceRows, useTransmission, x0) - valueAtHeight(currentRows, useTransmission, x0));
        const double diff1 = qAbs(valueAtHeight(referenceRows, useTransmission, x1) - valueAtHeight(currentRows, useTransmission, x1));
        accumulated += 0.5 * (diff0 + diff1) * (x1 - x0);
    }

    const double totalHeight = overlapEnd - overlapStart;
    return totalHeight > 0.0 ? accumulated / totalHeight : 0.0;
}

bool ensureInstabilityResultTable(QSqlDatabase &db, QString *errorMessage = nullptr)
{
    if (!db.isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("database is not open");
        }
        return false;
    }

    QSqlQuery query(db);
    const QString sql =
        "CREATE TABLE IF NOT EXISTS instability_curve_data ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "experiment_id INTEGER NOT NULL, "
        "scan_id INTEGER DEFAULT 0, "
        "scan_elapsed_ms INTEGER DEFAULT 0, "
        "channel_used TEXT, "
        "instability_value REAL DEFAULT 0, "
        "created_at TEXT)";
    const bool ok = query.exec(sql);
    if (!ok && errorMessage) {
        *errorMessage = query.lastError().text();
    }
    return ok;
}

bool ensureInstabilitySegmentResultTable(QSqlDatabase &db, QString *errorMessage = nullptr)
{
    // 分区不稳定性结果单独落库，避免整体/局部/自定义互相覆盖缓存。
    if (!db.isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("database is not open");
        }
        return false;
    }

    QSqlQuery query(db);
    const QString sql =
        "CREATE TABLE IF NOT EXISTS instability_segment_curve_data ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "experiment_id INTEGER NOT NULL, "
        "segment_key TEXT NOT NULL, "
        "height_lower_mm REAL DEFAULT 0, "
        "height_upper_mm REAL DEFAULT 0, "
        "scan_id INTEGER DEFAULT 0, "
        "scan_elapsed_ms INTEGER DEFAULT 0, "
        "channel_used TEXT, "
        "instability_value REAL DEFAULT 0, "
        "created_at TEXT)";
    const bool ok = query.exec(sql);
    if (!ok && errorMessage) {
        *errorMessage = query.lastError().text();
    }
    return ok;
}

bool ensureExperimentDataColumns(QSqlDatabase &db, QString *errorMessage = nullptr)
{
    if (!db.isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("database is not open");
        }
        return false;
    }

    QSet<QString> columns;
    QSqlQuery pragmaQuery(db);
    if (!pragmaQuery.exec("PRAGMA table_info(experiment_data)")) {
        if (errorMessage) {
            *errorMessage = pragmaQuery.lastError().text();
        }
        return false;
    }

    while (pragmaQuery.next()) {
        columns.insert(pragmaQuery.value("name").toString());
    }

    QSqlQuery alterQuery(db);
    if (!columns.contains("scan_id")) {
        if (!alterQuery.exec("ALTER TABLE experiment_data ADD COLUMN scan_id INTEGER DEFAULT 0")) {
            if (errorMessage) {
                *errorMessage = alterQuery.lastError().text();
            }
            return false;
        }
    }

    if (!columns.contains("scan_elapsed_ms")) {
        if (!alterQuery.exec("ALTER TABLE experiment_data ADD COLUMN scan_elapsed_ms INTEGER DEFAULT 0")) {
            if (errorMessage) {
                *errorMessage = alterQuery.lastError().text();
            }
            return false;
        }
    }

    return true;
}

bool ensureExperimentSoftDeleteColumns(QSqlDatabase &db, QString *errorMessage = nullptr)
{
    if (!db.isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("database is not open");
        }
        return false;
    }

    QSet<QString> columns;
    QSqlQuery pragmaQuery(db);
    if (!pragmaQuery.exec("PRAGMA table_info(experiments)")) {
        if (errorMessage) {
            *errorMessage = pragmaQuery.lastError().text();
        }
        return false;
    }

    while (pragmaQuery.next()) {
        columns.insert(pragmaQuery.value("name").toString());
    }

    QSqlQuery alterQuery(db);
    if (!columns.contains("deleted_flag")) {
        if (!alterQuery.exec("ALTER TABLE experiments ADD COLUMN deleted_flag INTEGER DEFAULT 0")) {
            if (errorMessage) {
                *errorMessage = alterQuery.lastError().text();
            }
            return false;
        }
    }

    if (!columns.contains("deleted_at")) {
        if (!alterQuery.exec("ALTER TABLE experiments ADD COLUMN deleted_at TEXT DEFAULT ''")) {
            if (errorMessage) {
                *errorMessage = alterQuery.lastError().text();
            }
            return false;
        }
    }

    if (!columns.contains("purge_after")) {
        if (!alterQuery.exec("ALTER TABLE experiments ADD COLUMN purge_after TEXT DEFAULT ''")) {
            if (errorMessage) {
                *errorMessage = alterQuery.lastError().text();
            }
            return false;
        }
    }

    return true;
}

bool purgeExpiredDeletedExperiments(QSqlDatabase &db, QString *errorMessage = nullptr)
{
    if (!db.isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("database is not open");
        }
        return false;
    }

    if (!ensureSeparationLayerResultTable(db, errorMessage)
        || !ensureInstabilityResultTable(db, errorMessage)
        || !ensureInstabilitySegmentResultTable(db, errorMessage)) {
        return false;
    }

    const QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QVector<int> expiredExperimentIds;

    QSqlQuery query(db);
    query.prepare(
        "SELECT id FROM experiments "
        "WHERE deleted_flag = 1 "
        "AND purge_after IS NOT NULL "
        "AND purge_after != '' "
        "AND purge_after <= ?");
    query.addBindValue(now);
    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    while (query.next()) {
        expiredExperimentIds.append(query.value(0).toInt());
    }

    if (expiredExperimentIds.isEmpty()) {
        return true;
    }

    if (!db.transaction()) {
        if (errorMessage) {
            *errorMessage = db.lastError().text();
        }
        return false;
    }

    QSqlQuery deleteExperimentDataQuery(db);
    deleteExperimentDataQuery.prepare("DELETE FROM experiment_data WHERE experiment_id = ?");
    QSqlQuery deleteSeparationQuery(db);
    deleteSeparationQuery.prepare("DELETE FROM separation_layer_data WHERE experiment_id = ?");
    QSqlQuery deleteInstabilityQuery(db);
    deleteInstabilityQuery.prepare("DELETE FROM instability_curve_data WHERE experiment_id = ?");
    QSqlQuery deleteInstabilitySegmentQuery(db);
    deleteInstabilitySegmentQuery.prepare("DELETE FROM instability_segment_curve_data WHERE experiment_id = ?");
    QSqlQuery deleteExperimentQuery(db);
    deleteExperimentQuery.prepare("DELETE FROM experiments WHERE id = ?");

    for (int experimentId : expiredExperimentIds) {
        deleteExperimentDataQuery.bindValue(0, experimentId);
        if (!deleteExperimentDataQuery.exec()) {
            if (errorMessage) {
                *errorMessage = deleteExperimentDataQuery.lastError().text();
            }
            db.rollback();
            return false;
        }

        deleteSeparationQuery.bindValue(0, experimentId);
        if (!deleteSeparationQuery.exec()) {
            if (errorMessage) {
                *errorMessage = deleteSeparationQuery.lastError().text();
            }
            db.rollback();
            return false;
        }

        deleteInstabilityQuery.bindValue(0, experimentId);
        if (!deleteInstabilityQuery.exec()) {
            if (errorMessage) {
                *errorMessage = deleteInstabilityQuery.lastError().text();
            }
            db.rollback();
            return false;
        }

        deleteInstabilitySegmentQuery.bindValue(0, experimentId);
        if (!deleteInstabilitySegmentQuery.exec()) {
            if (errorMessage) {
                *errorMessage = deleteInstabilitySegmentQuery.lastError().text();
            }
            db.rollback();
            return false;
        }

        deleteExperimentQuery.bindValue(0, experimentId);
        if (!deleteExperimentQuery.exec()) {
            if (errorMessage) {
                *errorMessage = deleteExperimentQuery.lastError().text();
            }
            db.rollback();
            return false;
        }
    }

    if (!db.commit()) {
        if (errorMessage) {
            *errorMessage = db.lastError().text();
        }
        return false;
    }

    qDebug() << "[SqlOrmManager] 已物理清理过期删除实验，数量:" << expiredExperimentIds.size();
    return true;
}

bool hardDeleteExperimentInternal(QSqlDatabase &db, int experimentId, QString *errorMessage = nullptr)
{
    if (!db.isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("database is not open");
        }
        return false;
    }

    if (!ensureSeparationLayerResultTable(db, errorMessage)
        || !ensureInstabilityResultTable(db, errorMessage)
        || !ensureInstabilitySegmentResultTable(db, errorMessage)) {
        return false;
    }

    if (!db.transaction()) {
        if (errorMessage) {
            *errorMessage = db.lastError().text();
        }
        return false;
    }

    QSqlQuery deleteExperimentDataQuery(db);
    deleteExperimentDataQuery.prepare("DELETE FROM experiment_data WHERE experiment_id = ?");
    deleteExperimentDataQuery.addBindValue(experimentId);
    if (!deleteExperimentDataQuery.exec()) {
        if (errorMessage) {
            *errorMessage = deleteExperimentDataQuery.lastError().text();
        }
        db.rollback();
        return false;
    }

    QSqlQuery deleteSeparationQuery(db);
    deleteSeparationQuery.prepare("DELETE FROM separation_layer_data WHERE experiment_id = ?");
    deleteSeparationQuery.addBindValue(experimentId);
    if (!deleteSeparationQuery.exec()) {
        if (errorMessage) {
            *errorMessage = deleteSeparationQuery.lastError().text();
        }
        db.rollback();
        return false;
    }

    QSqlQuery deleteInstabilityQuery(db);
    deleteInstabilityQuery.prepare("DELETE FROM instability_curve_data WHERE experiment_id = ?");
    deleteInstabilityQuery.addBindValue(experimentId);
    if (!deleteInstabilityQuery.exec()) {
        if (errorMessage) {
            *errorMessage = deleteInstabilityQuery.lastError().text();
        }
        db.rollback();
        return false;
    }

    QSqlQuery deleteInstabilitySegmentQuery(db);
    deleteInstabilitySegmentQuery.prepare("DELETE FROM instability_segment_curve_data WHERE experiment_id = ?");
    deleteInstabilitySegmentQuery.addBindValue(experimentId);
    if (!deleteInstabilitySegmentQuery.exec()) {
        if (errorMessage) {
            *errorMessage = deleteInstabilitySegmentQuery.lastError().text();
        }
        db.rollback();
        return false;
    }

    QSqlQuery deleteExperimentQuery(db);
    deleteExperimentQuery.prepare("DELETE FROM experiments WHERE id = ?");
    deleteExperimentQuery.addBindValue(experimentId);
    if (!deleteExperimentQuery.exec()) {
        if (errorMessage) {
            *errorMessage = deleteExperimentQuery.lastError().text();
        }
        db.rollback();
        return false;
    }

    if (!db.commit()) {
        if (errorMessage) {
            *errorMessage = db.lastError().text();
        }
        return false;
    }

    return true;
}

}

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

        QString migrationError;
        QSqlDatabase migrationDb = openReadOnlyDb(d->dbPath, QStringLiteral("sqlorm_migration"), &migrationError);
        if (!migrationDb.isOpen()) {
            qWarning() << "[SqlOrmManager] 打开迁移连接失败:" << migrationError;
        } else {
            if (!ensureExperimentDataColumns(migrationDb, &migrationError)) {
                qWarning() << "[SqlOrmManager] 补齐 experiment_data 列失败:" << migrationError;
            }
            if (!ensureExperimentSoftDeleteColumns(migrationDb, &migrationError)) {
                qWarning() << "[SqlOrmManager] 补齐 experiments 软删除列失败:" << migrationError;
            }
            if (!purgeExpiredDeletedExperiments(migrationDb, &migrationError)) {
                qWarning() << "[SqlOrmManager] 清理过期已删除实验失败:" << migrationError;
            }
            closeReadOnlyDb(QStringLiteral("sqlorm_migration"));
            QSqlDatabase::removeDatabase(QStringLiteral("sqlorm_migration"));
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
        experiment.deleted_flag = 0;
        experiment.deleted_at.clear();
        experiment.purge_after.clear();
        
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
                    where(c(&Experiment::id) == experimentId and c(&Experiment::deleted_flag) == 0)
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
                    where(c(&Experiment::id) == experimentId and c(&Experiment::deleted_flag) == 0)
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
        QString purgeError;
        QSqlDatabase purgeDb = openReadOnlyDb(d->dbPath,
                                              QString("SqlOrmGetExperimentById_%1").arg(reinterpret_cast<quintptr>(this)),
                                              &purgeError);
        if (purgeDb.isOpen()) {
            purgeExpiredDeletedExperiments(purgeDb, &purgeError);
            closeReadOnlyDb(QString("SqlOrmGetExperimentById_%1").arg(reinterpret_cast<quintptr>(this)));
        }

        auto experiments = d->storage->get_all<Experiment>(
                    where(c(&Experiment::id) == experimentId and c(&Experiment::deleted_flag) == 0)
                    );
        
        if (!experiments.empty()) {
            const Experiment& experiment = experiments[0];
            result = experimentToVariantMap(experiment);
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
        QString purgeError;
        const QString connectionName = QString("SqlOrmGetAllExperiments_%1").arg(reinterpret_cast<quintptr>(this));
        QSqlDatabase purgeDb = openReadOnlyDb(d->dbPath, connectionName, &purgeError);
        if (purgeDb.isOpen()) {
            purgeExpiredDeletedExperiments(purgeDb, &purgeError);
            closeReadOnlyDb(connectionName);
        }

        auto experiments = d->storage->get_all<Experiment>(
                    where(c(&Experiment::deleted_flag) == 0)
                    );
        
        for (const auto& experiment : experiments) {
            QVariantMap experimentData = experimentToVariantMap(experiment);
            
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
        QString purgeError;
        const QString connectionName = QString("SqlOrmGetExperimentsByProject_%1").arg(reinterpret_cast<quintptr>(this));
        QSqlDatabase purgeDb = openReadOnlyDb(d->dbPath, connectionName, &purgeError);
        if (purgeDb.isOpen()) {
            purgeExpiredDeletedExperiments(purgeDb, &purgeError);
            closeReadOnlyDb(connectionName);
        }

        // 先获取项目名称
        QVariantMap projectData = getProjectById(projectId);
        QString projectName = projectData["project_name"].toString();
        
        auto experiments = d->storage->get_all<Experiment>(
                    where(c(&Experiment::project_id) == projectId and c(&Experiment::deleted_flag) == 0)
                    );
        
        for (const auto& experiment : experiments) {
            QVariantMap experimentData = experimentToVariantMap(experiment);
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
                    where(c(&Experiment::operator_name) == operatorName.toStdString() and c(&Experiment::deleted_flag) == 0)
                    );
        
        for (const auto& experiment : experiments) {
            QVariantMap experimentData = experimentToVariantMap(experiment);
            
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
        QString purgeError;
        const QString connectionName = QString("SqlOrmGetExperimentsByStatus_%1").arg(reinterpret_cast<quintptr>(this));
        QSqlDatabase purgeDb = openReadOnlyDb(d->dbPath, connectionName, &purgeError);
        if (purgeDb.isOpen()) {
            purgeExpiredDeletedExperiments(purgeDb, &purgeError);
            closeReadOnlyDb(connectionName);
        }

        auto experiments = d->storage->get_all<Experiment>(
                    where(c(&Experiment::status) == status and c(&Experiment::deleted_flag) == 0)
                    );
        
        for (const auto& experiment : experiments) {
            QVariantMap experimentData = experimentToVariantMap(experiment);
            
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

QVector<QVariantMap> SqlOrmManager::getDeletedExperiments()
{
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);

    if (!d->initialized) return result;

    try {
        QString purgeError;
        const QString connectionName = QString("SqlOrmGetDeletedExperiments_%1").arg(reinterpret_cast<quintptr>(this));
        QSqlDatabase purgeDb = openReadOnlyDb(d->dbPath, connectionName, &purgeError);
        if (purgeDb.isOpen()) {
            purgeExpiredDeletedExperiments(purgeDb, &purgeError);
            closeReadOnlyDb(connectionName);
        }

        auto experiments = d->storage->get_all<Experiment>(
                    where(c(&Experiment::deleted_flag) == 1),
                    order_by(&Experiment::deleted_at).desc()
                    );

        for (const auto& experiment : experiments) {
            QVariantMap experimentData = experimentToVariantMap(experiment);

            QVariantMap projectData = getProjectById(experiment.project_id);
            experimentData["project_name"] = projectData["project_name"].toString();

            result.append(experimentData);
        }
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 查询已删除实验失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    }

    return result;
}

bool SqlOrmManager::deleteExperiment(int experimentId) {
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

        Experiment &experiment = experiments[0];
        if (experiment.deleted_flag == 1) {
            qDebug() << "[SqlOrmManager] 实验已在回收状态，跳过重复删除：" << experimentId;
            return true;
        }

        const QDateTime now = QDateTime::currentDateTime();
        experiment.deleted_flag = 1;
        experiment.deleted_at = now.toString("yyyy-MM-dd hh:mm:ss");
        experiment.purge_after = now.addDays(7).toString("yyyy-MM-dd hh:mm:ss");

        d->storage->update(experiment);
        qDebug() << "[SqlOrmManager] 实验已标记删除：" << experimentId << "计划清理时间:" << experiment.purge_after;
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 删除实验失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

bool SqlOrmManager::restoreExperiment(int experimentId)
{
    Q_D(SqlOrmManager);

    if (!d->initialized) return false;

    try {
        auto experiments = d->storage->get_all<Experiment>(
                    where(c(&Experiment::id) == experimentId and c(&Experiment::deleted_flag) == 1)
                    );
        if (experiments.empty()) {
            qWarning() << "[SqlOrmManager] 已删除实验不存在：" << experimentId;
            return false;
        }

        Experiment &experiment = experiments[0];
        experiment.deleted_flag = 0;
        experiment.deleted_at.clear();
        experiment.purge_after.clear();

        d->storage->update(experiment);
        qDebug() << "[SqlOrmManager] 实验已恢复：" << experimentId;
        return true;
    } catch (const std::exception& e) {
        qWarning() << "[SqlOrmManager] 恢复实验失败：" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
        return false;
    }
}

bool SqlOrmManager::hardDeleteExperiment(int experimentId)
{
    Q_D(SqlOrmManager);

    if (!d->initialized || experimentId <= 0) return false;

    const QString connectionName = QString("SqlOrmHardDeleteExperiment_%1").arg(reinterpret_cast<quintptr>(this));
    QString openError;
    QSqlDatabase db = openReadOnlyDb(d->dbPath, connectionName, &openError);
    if (!db.isOpen()) {
        emit errorOccurred(openError);
        closeReadOnlyDb(connectionName);
        return false;
    }

    QString errorMessage;
    const bool ok = hardDeleteExperimentInternal(db, experimentId, &errorMessage);
    if (!ok && !errorMessage.isEmpty()) {
        emit errorOccurred(errorMessage);
    }

    closeReadOnlyDb(connectionName);
    return ok;
}

// ==================== 实验数据管理 ====================

bool SqlOrmManager::addExperimentData(const QVariantMap& data) {
    Q_D(SqlOrmManager);

    if (!d->initialized) return false;

    try {
        ExperimentData expData;
        expData.experiment_id = data.value("experiment_id", 0).toInt();
        expData.timestamp = data.value("timestamp", 0).toInt();
        expData.scan_id = data.value("scan_id", 0).toInt();
        expData.scan_elapsed_ms = data.value("scan_elapsed_ms", 0).toInt();
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
    
    try {
        // 使用事务批量插入
        bool transactionStarted = beginTransaction();
        
        int successCount = 0;
        for (const auto& data : dataList) {
            if (addExperimentData(data)) {
                successCount++;
            }
        }
        
        if (transactionStarted) {
            commitTransaction();
        }
        
        qDebug() << "[SqlOrmManager] 批量添加实验数据成功：" << successCount << "条";
        return successCount == dataList.size();
    } catch (const std::exception& e) {
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
            result["scan_id"] = expData.scan_id;
            result["scan_elapsed_ms"] = expData.scan_elapsed_ms;
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
            data["scan_id"] = expData.scan_id;
            data["scan_elapsed_ms"] = expData.scan_elapsed_ms;
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
            data["scan_id"] = expData.scan_id;
            data["scan_elapsed_ms"] = expData.scan_elapsed_ms;
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
            data["scan_id"] = expData.scan_id;
            data["scan_elapsed_ms"] = expData.scan_elapsed_ms;
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

QVector<QVariantMap> SqlOrmManager::getExperimentDataPreviewByExperiment(int experimentId, int limit) {
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);

    if (!d->initialized || experimentId <= 0 || limit <= 0) return result;

    const QString connectionName = QString("SqlOrmPreview_%1").arg(reinterpret_cast<quintptr>(this));
    QString openError;
    QSqlDatabase db = openReadOnlyDb(d->dbPath, connectionName, &openError);
    if (!db.isOpen()) {
        emit errorOccurred(openError);
        closeReadOnlyDb(connectionName);
        return result;
    }

    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare(
        "SELECT id, experiment_id, timestamp, scan_id, scan_elapsed_ms, height, "
        "backscatter_intensity, transmission_intensity "
        "FROM experiment_data WHERE experiment_id = ? "
        "ORDER BY timestamp ASC, scan_id ASC, id ASC LIMIT ?");
    query.addBindValue(experimentId);
    query.addBindValue(limit);

    if (!query.exec()) {
        emit errorOccurred(query.lastError().text());
        closeReadOnlyDb(connectionName);
        return result;
    }

    while (query.next()) {
        result.append(readExtendedExperimentDataRow(query));
    }

    closeReadOnlyDb(connectionName);
    return result;
}

QVector<QVariantMap> SqlOrmManager::getLightIntensityAveragesByExperiment(int experimentId) {
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);

    if (!d->initialized || experimentId <= 0) return result;

    const QString connectionName = QString("SqlOrmLightAvg_%1").arg(reinterpret_cast<quintptr>(this));
    QString openError;
    QSqlDatabase db = openReadOnlyDb(d->dbPath, connectionName, &openError);
    if (!db.isOpen()) {
        emit errorOccurred(openError);
        closeReadOnlyDb(connectionName);
        return result;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT scan_id, MAX(scan_elapsed_ms) AS scan_elapsed_ms, "
        "AVG(backscatter_intensity) AS avg_backscatter, "
        "AVG(transmission_intensity) AS avg_transmission "
        "FROM experiment_data WHERE experiment_id = ? "
        "GROUP BY scan_id "
        "ORDER BY scan_elapsed_ms ASC, scan_id ASC");
    query.addBindValue(experimentId);

    if (!query.exec()) {
        emit errorOccurred(query.lastError().text());
        closeReadOnlyDb(connectionName);
        return result;
    }

    while (query.next()) {
        QVariantMap row;
        row["scan_id"] = query.value("scan_id");
        row["scan_elapsed_ms"] = query.value("scan_elapsed_ms");
        row["avg_backscatter"] = query.value("avg_backscatter");
        row["avg_transmission"] = query.value("avg_transmission");
        result.append(row);
    }

    closeReadOnlyDb(connectionName);
    return result;
}

QVector<QVariantMap> SqlOrmManager::getUniformityIndicesByExperiment(int experimentId) {
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);

    if (!d->initialized || experimentId <= 0) return result;

    const QString connectionName = QString("SqlOrmUniformity_%1").arg(reinterpret_cast<quintptr>(this));
    QString openError;
    QSqlDatabase db = openReadOnlyDb(d->dbPath, connectionName, &openError);
    if (!db.isOpen()) {
        emit errorOccurred(openError);
        closeReadOnlyDb(connectionName);
        return result;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT scan_id, MAX(scan_elapsed_ms) AS scan_elapsed_ms, "
        "AVG(backscatter_intensity) AS avg_bs, "
        "AVG(backscatter_intensity * backscatter_intensity) AS avg_bs_square, "
        "AVG(transmission_intensity) AS avg_t, "
        "AVG(transmission_intensity * transmission_intensity) AS avg_t_square "
        "FROM experiment_data WHERE experiment_id = ? "
        "GROUP BY scan_id "
        "ORDER BY scan_elapsed_ms ASC, scan_id ASC");
    query.addBindValue(experimentId);

    if (!query.exec()) {
        emit errorOccurred(query.lastError().text());
        closeReadOnlyDb(connectionName);
        return result;
    }

    while (query.next()) {
        const double avgBs = query.value("avg_bs").toDouble();
        const double avgBsSquare = query.value("avg_bs_square").toDouble();
        const double avgT = query.value("avg_t").toDouble();
        const double avgTSquare = query.value("avg_t_square").toDouble();
        const double bsStd = avgBs > 0.0 ? qSqrt(qMax(0.0, avgBsSquare - avgBs * avgBs)) : 0.0;
        const double tStd = avgT > 0.0 ? qSqrt(qMax(0.0, avgTSquare - avgT * avgT)) : 0.0;
        const double uiBs = avgBs > 0.0 ? qBound(0.0, 1.0 - bsStd / avgBs, 1.0) : 0.0;
        const double uiT = avgT > 0.0 ? qBound(0.0, 1.0 - tStd / avgT, 1.0) : 0.0;

        QVariantMap row;
        row["scan_id"] = query.value("scan_id");
        row["scan_elapsed_ms"] = query.value("scan_elapsed_ms");
        row["ui_backscatter"] = uiBs;
        row["ui_transmission"] = uiT;
        row["ui_combined"] = (uiBs + uiT) / 2.0;
        result.append(row);
    }

    closeReadOnlyDb(connectionName);
    return result;
}

QVector<QVariantMap> SqlOrmManager::getLightIntensityCurvesByExperiment(int experimentId, int pointsPerCurve) {
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);

    if (!d->initialized || experimentId <= 0) return result;

    const QString connectionName = QString("SqlOrmLightCurves_%1").arg(reinterpret_cast<quintptr>(this));
    QString openError;
    QSqlDatabase db = openReadOnlyDb(d->dbPath, connectionName, &openError);
    if (!db.isOpen()) {
        emit errorOccurred(openError);
        closeReadOnlyDb(connectionName);
        return result;
    }

    struct CurveSummary {
        int scanId = 0;
        int timestamp = 0;
        int elapsedMs = 0;
        int pointCount = 0;
        double minHeightMm = 0.0;
        double maxHeightMm = 0.0;
        double minBackscatter = 0.0;
        double maxBackscatter = 0.0;
        double minTransmission = 0.0;
        double maxTransmission = 0.0;
    };

    QSqlQuery summaryQuery(db);
    summaryQuery.prepare(
        "SELECT scan_id, MIN(timestamp) AS timestamp, MAX(scan_elapsed_ms) AS scan_elapsed_ms, "
        "COUNT(*) AS point_count, "
        "MIN(height) / 1000.0 AS min_height_mm, MAX(height) / 1000.0 AS max_height_mm, "
        "MIN(backscatter_intensity) AS min_backscatter, MAX(backscatter_intensity) AS max_backscatter, "
        "MIN(transmission_intensity) AS min_transmission, MAX(transmission_intensity) AS max_transmission "
        "FROM experiment_data "
        "WHERE experiment_id = ? "
        "GROUP BY scan_id "
        "ORDER BY scan_id ASC");
    summaryQuery.addBindValue(experimentId);

    if (!summaryQuery.exec()) {
        emit errorOccurred(summaryQuery.lastError().text());
        closeReadOnlyDb(connectionName);
        return result;
    }

    QVector<CurveSummary> summaries;
    QHash<int, int> summaryIndexByScanId;
    while (summaryQuery.next()) {
        CurveSummary summary;
        summary.scanId = summaryQuery.value("scan_id").toInt();
        summary.timestamp = summaryQuery.value("timestamp").toInt();
        summary.elapsedMs = summaryQuery.value("scan_elapsed_ms").toInt();
        summary.pointCount = summaryQuery.value("point_count").toInt();
        summary.minHeightMm = summaryQuery.value("min_height_mm").toDouble();
        summary.maxHeightMm = summaryQuery.value("max_height_mm").toDouble();
        summary.minBackscatter = summaryQuery.value("min_backscatter").toDouble();
        summary.maxBackscatter = summaryQuery.value("max_backscatter").toDouble();
        summary.minTransmission = summaryQuery.value("min_transmission").toDouble();
        summary.maxTransmission = summaryQuery.value("max_transmission").toDouble();
        summaryIndexByScanId.insert(summary.scanId, summaries.size());
        summaries.append(summary);
    }

    if (summaries.isEmpty()) {
        closeReadOnlyDb(connectionName);
        return result;
    }

    const int totalPointBudgetPerChannel = 12000;
    const int effectivePointsPerCurve = qMax(64, qMin(pointsPerCurve > 0 ? pointsPerCurve : 640,
                                                      qMax(64, totalPointBudgetPerChannel / qMax(1, summaries.size()))));

    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare(
        "SELECT scan_id, height, backscatter_intensity, transmission_intensity "
        "FROM experiment_data WHERE experiment_id = ? "
        "ORDER BY scan_id ASC, height ASC, id ASC");
    query.addBindValue(experimentId);

    if (!query.exec()) {
        emit errorOccurred(query.lastError().text());
        closeReadOnlyDb(connectionName);
        return result;
    }

    int currentScanId = std::numeric_limits<int>::min();
    QVector<LightCurveRowEx> currentRows;
    auto flushCurve = [&](int scanId) {
        if (!summaryIndexByScanId.contains(scanId) || currentRows.isEmpty()) {
            return;
        }
        const CurveSummary &summary = summaries.at(summaryIndexByScanId.value(scanId));
        QVariantMap curve;
        curve["scan_id"] = summary.scanId;
        curve["timestamp"] = summary.timestamp;
        curve["scan_elapsed_ms"] = summary.elapsedMs;
        curve["point_count"] = summary.pointCount;
        curve["min_height_mm"] = summary.minHeightMm;
        curve["max_height_mm"] = summary.maxHeightMm;
        curve["min_backscatter"] = summary.minBackscatter;
        curve["max_backscatter"] = summary.maxBackscatter;
        curve["min_transmission"] = summary.minTransmission;
        curve["max_transmission"] = summary.maxTransmission;
        curve["backscatter_points"] = downsampleCurvePoints(currentRows, false, effectivePointsPerCurve);
        curve["transmission_points"] = downsampleCurvePoints(currentRows, true, effectivePointsPerCurve);
        result.append(curve);
    };

    while (query.next()) {
        const int scanId = query.value("scan_id").toInt();
        if (currentScanId != std::numeric_limits<int>::min() && scanId != currentScanId) {
            flushCurve(currentScanId);
            currentRows.clear();
        }

        currentScanId = scanId;
        LightCurveRowEx row;
        row.heightMm = query.value("height").toDouble() / 1000.0;
        row.backscatter = query.value("backscatter_intensity").toDouble();
        row.transmission = query.value("transmission_intensity").toDouble();
        currentRows.append(row);
    }

    if (currentScanId != std::numeric_limits<int>::min()) {
        flushCurve(currentScanId);
    }

    closeReadOnlyDb(connectionName);
    return result;
}

QVector<QVariantMap> SqlOrmManager::getProcessedLightIntensityCurvesByExperiment(int experimentId, int pointsPerCurve, int referenceScanId,
                                                                                 double lowerMm, double upperMm, bool useReference)
{
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);

    if (!d->initialized || experimentId <= 0) return result;

    const QString connectionName = QString("SqlOrmProcessedLightCurves_%1").arg(reinterpret_cast<quintptr>(this));
    QString openError;
    QSqlDatabase db = openReadOnlyDb(d->dbPath, connectionName, &openError);
    if (!db.isOpen()) {
        emit errorOccurred(openError);
        closeReadOnlyDb(connectionName);
        return result;
    }

    struct CurveSummary {
        int scanId = 0;
        int timestamp = 0;
        int elapsedMs = 0;
        int pointCount = 0;
        double minHeightMm = 0.0;
        double maxHeightMm = 0.0;
    };

    QSqlQuery summaryQuery(db);
    summaryQuery.prepare(
        "SELECT scan_id, MIN(timestamp) AS timestamp, MAX(scan_elapsed_ms) AS scan_elapsed_ms, "
        "COUNT(*) AS point_count, MIN(height) / 1000.0 AS min_height_mm, MAX(height) / 1000.0 AS max_height_mm "
        "FROM experiment_data WHERE experiment_id = ? "
        "GROUP BY scan_id ORDER BY scan_id ASC");
    summaryQuery.addBindValue(experimentId);

    if (!summaryQuery.exec()) {
        emit errorOccurred(summaryQuery.lastError().text());
        closeReadOnlyDb(connectionName);
        return result;
    }

    QVector<CurveSummary> summaries;
    double globalMinHeight = std::numeric_limits<double>::max();
    double globalMaxHeight = std::numeric_limits<double>::lowest();
    while (summaryQuery.next()) {
        CurveSummary summary;
        summary.scanId = summaryQuery.value("scan_id").toInt();
        summary.timestamp = summaryQuery.value("timestamp").toInt();
        summary.elapsedMs = summaryQuery.value("scan_elapsed_ms").toInt();
        summary.pointCount = summaryQuery.value("point_count").toInt();
        summary.minHeightMm = summaryQuery.value("min_height_mm").toDouble();
        summary.maxHeightMm = summaryQuery.value("max_height_mm").toDouble();
        summaries.append(summary);
        globalMinHeight = qMin(globalMinHeight, summary.minHeightMm);
        globalMaxHeight = qMax(globalMaxHeight, summary.maxHeightMm);
    }

    if (summaries.isEmpty()) {
        closeReadOnlyDb(connectionName);
        return result;
    }

    const double safeLower = qMax(globalMinHeight, qMin(lowerMm, upperMm));
    const double safeUpper = qMin(globalMaxHeight, qMax(lowerMm, upperMm));
    if (safeUpper - safeLower <= 1e-6) {
        closeReadOnlyDb(connectionName);
        return result;
    }

    const int totalPointBudgetPerChannel = 12000;
    const int effectivePointsPerCurve = qMax(64, qMin(pointsPerCurve > 0 ? pointsPerCurve : 640,
                                                      qMax(64, totalPointBudgetPerChannel / qMax(1, summaries.size()))));

    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare(
        "SELECT scan_id, height, backscatter_intensity, transmission_intensity "
        "FROM experiment_data WHERE experiment_id = ? "
        "ORDER BY scan_id ASC, height ASC, id ASC");
    query.addBindValue(experimentId);

    if (!query.exec()) {
        emit errorOccurred(query.lastError().text());
        closeReadOnlyDb(connectionName);
        return result;
    }

    QHash<int, QVector<LightCurveRowEx>> rowsByScanId;
    while (query.next()) {
        const int scanId = query.value("scan_id").toInt();
        LightCurveRowEx row;
        row.heightMm = query.value("height").toDouble() / 1000.0;
        row.backscatter = query.value("backscatter_intensity").toDouble();
        row.transmission = query.value("transmission_intensity").toDouble();
        rowsByScanId[scanId].append(row);
    }

    const int safeReferenceScanId = rowsByScanId.contains(referenceScanId) ? referenceScanId : summaries.first().scanId;
    const QVector<LightCurveRowEx> referenceRows = filterLightCurveRowsByHeight(rowsByScanId.value(safeReferenceScanId), safeLower, safeUpper);

    for (const CurveSummary &summary : summaries) {
        const QVector<LightCurveRowEx> filteredRows = filterLightCurveRowsByHeight(rowsByScanId.value(summary.scanId), safeLower, safeUpper);
        if (filteredRows.isEmpty()) {
            continue;
        }

        QVector<QPointF> backscatterPoints;
        QVector<QPointF> transmissionPoints;
        backscatterPoints.reserve(filteredRows.size());
        transmissionPoints.reserve(filteredRows.size());

        double minBackscatter = std::numeric_limits<double>::max();
        double maxBackscatter = std::numeric_limits<double>::lowest();
        double minTransmission = std::numeric_limits<double>::max();
        double maxTransmission = std::numeric_limits<double>::lowest();

        for (const LightCurveRowEx &row : filteredRows) {
            double backscatterValue = row.backscatter;
            double transmissionValue = row.transmission;
            if (useReference && !referenceRows.isEmpty()) {
                backscatterValue -= lightCurveValueAtHeight(referenceRows, false, row.heightMm);
                transmissionValue -= lightCurveValueAtHeight(referenceRows, true, row.heightMm);
            }

            backscatterPoints.append(QPointF(row.heightMm, backscatterValue));
            transmissionPoints.append(QPointF(row.heightMm, transmissionValue));
            minBackscatter = qMin(minBackscatter, backscatterValue);
            maxBackscatter = qMax(maxBackscatter, backscatterValue);
            minTransmission = qMin(minTransmission, transmissionValue);
            maxTransmission = qMax(maxTransmission, transmissionValue);
        }

        QVariantMap curve;
        curve["scan_id"] = summary.scanId;
        curve["timestamp"] = summary.timestamp;
        curve["scan_elapsed_ms"] = summary.elapsedMs;
        curve["point_count"] = filteredRows.size();
        curve["min_height_mm"] = safeLower;
        curve["max_height_mm"] = safeUpper;
        curve["min_backscatter"] = minBackscatter;
        curve["max_backscatter"] = maxBackscatter;
        curve["min_transmission"] = minTransmission;
        curve["max_transmission"] = maxTransmission;
        curve["reference_scan_id"] = safeReferenceScanId;
        curve["use_reference"] = useReference;
        curve["backscatter_points"] = downsamplePointSeries(backscatterPoints, effectivePointsPerCurve);
        curve["transmission_points"] = downsamplePointSeries(transmissionPoints, effectivePointsPerCurve);
        result.append(curve);
    }

    closeReadOnlyDb(connectionName);
    return result;
}

QVector<QVariantMap> SqlOrmManager::getSeparationLayerDataByExperiment(int experimentId)
{
    // 分层厚度采用“按实验惰性计算并缓存”的策略：
    // 已有完整结果时直接读取，没有则从 experiment_data 重建三区厚度。
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);

    if (!d->initialized || experimentId <= 0) return result;

    const QString connectionName = QString("SqlOrmSeparationLayer_%1").arg(reinterpret_cast<quintptr>(this));
    QString openError;
    QSqlDatabase db = openReadOnlyDb(d->dbPath, connectionName, &openError);
    if (!db.isOpen()) {
        emit errorOccurred(openError);
        closeReadOnlyDb(connectionName);
        return result;
    }

    QString schemaError;
    if (!ensureSeparationLayerResultTable(db, &schemaError)) {
        emit errorOccurred(schemaError);
        closeReadOnlyDb(connectionName);
        return result;
    }

    int scanCount = 0;
    int maxElapsedMs = -1;
    {
        QSqlQuery statsQuery(db);
        statsQuery.prepare(
            "SELECT COUNT(DISTINCT scan_id) AS scan_count, "
            "COALESCE(MAX(scan_elapsed_ms), -1) AS max_elapsed_ms "
            "FROM experiment_data WHERE experiment_id = ?");
        statsQuery.addBindValue(experimentId);
        if (!statsQuery.exec() || !statsQuery.next()) {
            emit errorOccurred(statsQuery.lastError().text());
            closeReadOnlyDb(connectionName);
            return result;
        }
        scanCount = statsQuery.value("scan_count").toInt();
        maxElapsedMs = statsQuery.value("max_elapsed_ms").toInt();
    }

    if (scanCount <= 0) {
        closeReadOnlyDb(connectionName);
        return result;
    }

    bool reuseExisting = false;
    {
        QSqlQuery existingQuery(db);
        existingQuery.prepare(
            "SELECT COUNT(*) AS result_count, COALESCE(MAX(scan_elapsed_ms), -1) AS max_elapsed_ms "
            "FROM separation_layer_data WHERE experiment_id = ?");
        existingQuery.addBindValue(experimentId);
        if (existingQuery.exec() && existingQuery.next()) {
            reuseExisting = existingQuery.value("result_count").toInt() == scanCount
                         && existingQuery.value("max_elapsed_ms").toInt() == maxElapsedMs;
        }
    }

    if (reuseExisting) {
        QSqlQuery query(db);
        query.prepare(
            "SELECT id, experiment_id, scan_id, scan_elapsed_ms, channel_used, "
            "clarification_boundary_mm, sediment_boundary_mm, "
            "clarification_thickness_mm, concentrated_phase_thickness_mm, sediment_thickness_mm, "
            "confidence, created_at "
            "FROM separation_layer_data WHERE experiment_id = ? "
            "ORDER BY scan_elapsed_ms ASC, scan_id ASC, id ASC");
        query.addBindValue(experimentId);
        if (!query.exec()) {
            emit errorOccurred(query.lastError().text());
            closeReadOnlyDb(connectionName);
            return result;
        }

        while (query.next()) {
            QVariantMap row;
            row["id"] = query.value("id");
            row["experiment_id"] = query.value("experiment_id");
            row["scan_id"] = query.value("scan_id");
            row["scan_elapsed_ms"] = query.value("scan_elapsed_ms");
            row["channel_used"] = query.value("channel_used");
            row["clarification_boundary_mm"] = query.value("clarification_boundary_mm");
            row["sediment_boundary_mm"] = query.value("sediment_boundary_mm");
            row["clarification_thickness_mm"] = query.value("clarification_thickness_mm");
            row["concentrated_phase_thickness_mm"] = query.value("concentrated_phase_thickness_mm");
            row["sediment_thickness_mm"] = query.value("sediment_thickness_mm");
            row["confidence"] = query.value("confidence");
            row["created_at"] = query.value("created_at");
            result.append(row);
        }

        closeReadOnlyDb(connectionName);
        return result;
    }

    closeReadOnlyDb(connectionName);

    const QString computeConnectionName = QString("SqlOrmSeparationLayerCompute_%1").arg(reinterpret_cast<quintptr>(this));
    QSqlDatabase computeDb = openReadOnlyDb(d->dbPath, computeConnectionName, &openError);
    if (!computeDb.isOpen()) {
        emit errorOccurred(openError);
        closeReadOnlyDb(computeConnectionName);
        return result;
    }

    QSqlQuery query(computeDb);
    query.setForwardOnly(true);
    query.prepare(
        "SELECT scan_id, scan_elapsed_ms, height, backscatter_intensity, transmission_intensity "
        "FROM experiment_data WHERE experiment_id = ? "
        "ORDER BY scan_id ASC, height ASC, id ASC");
    query.addBindValue(experimentId);

    if (!query.exec()) {
        emit errorOccurred(query.lastError().text());
        closeReadOnlyDb(computeConnectionName);
        return result;
    }

    QVector<QVariantMap> computedRows;
    QVector<SeparationRowEx> currentRows;
    int currentScanId = std::numeric_limits<int>::min();
    int currentElapsedMs = 0;

    auto flushScan = [&]() {
        if (currentRows.isEmpty()) {
            return;
        }

        if (currentRows.size() < 4) {
            const double minHeight = currentRows.first().heightMm;
            const double maxHeight = currentRows.last().heightMm;
            QVariantMap coarseRow;
            coarseRow["scan_id"] = currentScanId;
            coarseRow["scan_elapsed_ms"] = currentElapsedMs;
            coarseRow["channel_used"] = QStringLiteral("BS");
            coarseRow["clarification_boundary_mm"] = maxHeight;
            coarseRow["sediment_boundary_mm"] = minHeight;
            coarseRow["clarification_thickness_mm"] = 0.0;
            coarseRow["concentrated_phase_thickness_mm"] = qMax(0.0, maxHeight - minHeight);
            coarseRow["sediment_thickness_mm"] = 0.0;
            coarseRow["confidence"] = 0.0;
            computedRows.append(coarseRow);
            return;
        }

        const bool useTransmission = averageTransmissionValue(currentRows) > 0.2;
        QVector<double> proxyValues;
        proxyValues.reserve(currentRows.size());

        double minSignal = std::numeric_limits<double>::max();
        double maxSignal = std::numeric_limits<double>::lowest();
        for (const SeparationRowEx &row : currentRows) {
            const double signal = useTransmission ? row.transmission : row.backscatter;
            minSignal = qMin(minSignal, signal);
            maxSignal = qMax(maxSignal, signal);
        }

        const double range = maxSignal - minSignal;
        if (range < 1e-6) {
            QVariantMap flatRow;
            flatRow["scan_id"] = currentScanId;
            flatRow["scan_elapsed_ms"] = currentElapsedMs;
            flatRow["channel_used"] = useTransmission ? QStringLiteral("T") : QStringLiteral("BS");
            flatRow["clarification_boundary_mm"] = currentRows.last().heightMm;
            flatRow["sediment_boundary_mm"] = currentRows.first().heightMm;
            flatRow["clarification_thickness_mm"] = 0.0;
            flatRow["concentrated_phase_thickness_mm"] = currentRows.last().heightMm - currentRows.first().heightMm;
            flatRow["sediment_thickness_mm"] = 0.0;
            flatRow["confidence"] = 0.0;
            computedRows.append(flatRow);
            return;
        }

        for (const SeparationRowEx &row : currentRows) {
            const double signal = useTransmission ? row.transmission : row.backscatter;
            double normalized = (signal - minSignal) / range;
            double proxy = useTransmission ? (1.0 - normalized) : normalized;
            proxyValues.append(qBound(0.0, proxy, 1.0));
        }

        proxyValues = smoothSeries(proxyValues, 5);
        const double sedimentBoundary = findBoundaryHeightForThreshold(currentRows, proxyValues, 0.8, true);
        const double clarificationBoundary = findBoundaryHeightForThreshold(currentRows, proxyValues, 0.2, true);
        const double minHeight = currentRows.first().heightMm;
        const double maxHeight = currentRows.last().heightMm;

        const double sedimentThickness = qBound(0.0, sedimentBoundary - minHeight, maxHeight - minHeight);
        const double clarificationThickness = qBound(0.0, maxHeight - clarificationBoundary, maxHeight - minHeight);
        const double concentratedThickness = qBound(0.0, clarificationBoundary - sedimentBoundary, maxHeight - minHeight);
        const double confidence = qBound(0.0, range / 30.0, 1.0);

        QVariantMap row;
        row["scan_id"] = currentScanId;
        row["scan_elapsed_ms"] = currentElapsedMs;
        row["channel_used"] = useTransmission ? QStringLiteral("T") : QStringLiteral("BS");
        row["clarification_boundary_mm"] = qBound(minHeight, clarificationBoundary, maxHeight);
        row["sediment_boundary_mm"] = qBound(minHeight, sedimentBoundary, maxHeight);
        row["clarification_thickness_mm"] = clarificationThickness;
        row["concentrated_phase_thickness_mm"] = concentratedThickness;
        row["sediment_thickness_mm"] = sedimentThickness;
        row["confidence"] = confidence;
        computedRows.append(row);
    };

    while (query.next()) {
        const int scanId = query.value("scan_id").toInt();
        if (currentScanId != std::numeric_limits<int>::min() && scanId != currentScanId) {
            flushScan();
            currentRows.clear();
        }

        currentScanId = scanId;
        currentElapsedMs = query.value("scan_elapsed_ms").toInt();

        SeparationRowEx row;
        row.heightMm = query.value("height").toDouble() / 1000.0;
        row.backscatter = query.value("backscatter_intensity").toDouble();
        row.transmission = query.value("transmission_intensity").toDouble();
        currentRows.append(row);
    }

    if (currentScanId != std::numeric_limits<int>::min()) {
        flushScan();
    }

    closeReadOnlyDb(computeConnectionName);

    const QString writeConnectionName = QString("SqlOrmSeparationLayerWrite_%1").arg(reinterpret_cast<quintptr>(this));
    QSqlDatabase writeDb = openReadOnlyDb(d->dbPath, writeConnectionName, &openError);
    if (!writeDb.isOpen()) {
        emit errorOccurred(openError);
        closeReadOnlyDb(writeConnectionName);
        return result;
    }

    if (!ensureSeparationLayerResultTable(writeDb, &schemaError)) {
        emit errorOccurred(schemaError);
        closeReadOnlyDb(writeConnectionName);
        return result;
    }

    if (!writeDb.transaction()) {
        emit errorOccurred(writeDb.lastError().text());
        closeReadOnlyDb(writeConnectionName);
        return result;
    }

    QSqlQuery deleteQuery(writeDb);
    deleteQuery.prepare("DELETE FROM separation_layer_data WHERE experiment_id = ?");
    deleteQuery.addBindValue(experimentId);
    if (!deleteQuery.exec()) {
        emit errorOccurred(deleteQuery.lastError().text());
        writeDb.rollback();
        closeReadOnlyDb(writeConnectionName);
        return result;
    }

    QSqlQuery insertQuery(writeDb);
    insertQuery.prepare(
        "INSERT INTO separation_layer_data "
        "(experiment_id, scan_id, scan_elapsed_ms, channel_used, "
        "clarification_boundary_mm, sediment_boundary_mm, "
        "clarification_thickness_mm, concentrated_phase_thickness_mm, sediment_thickness_mm, "
        "confidence, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    const QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    for (const QVariantMap &row : computedRows) {
        insertQuery.bindValue(0, experimentId);
        insertQuery.bindValue(1, row.value("scan_id", 0).toInt());
        insertQuery.bindValue(2, row.value("scan_elapsed_ms", 0).toInt());
        insertQuery.bindValue(3, row.value("channel_used").toString());
        insertQuery.bindValue(4, row.value("clarification_boundary_mm", 0.0).toDouble());
        insertQuery.bindValue(5, row.value("sediment_boundary_mm", 0.0).toDouble());
        insertQuery.bindValue(6, row.value("clarification_thickness_mm", 0.0).toDouble());
        insertQuery.bindValue(7, row.value("concentrated_phase_thickness_mm", 0.0).toDouble());
        insertQuery.bindValue(8, row.value("sediment_thickness_mm", 0.0).toDouble());
        insertQuery.bindValue(9, row.value("confidence", 0.0).toDouble());
        insertQuery.bindValue(10, row.value("created_at", now).toString());
        if (!insertQuery.exec()) {
            emit errorOccurred(insertQuery.lastError().text());
            writeDb.rollback();
            closeReadOnlyDb(writeConnectionName);
            return result;
        }
    }

    if (!writeDb.commit()) {
        emit errorOccurred(writeDb.lastError().text());
        closeReadOnlyDb(writeConnectionName);
        return result;
    }

    closeReadOnlyDb(writeConnectionName);
    return getSeparationLayerDataByExperiment(experimentId);
}

bool SqlOrmManager::replaceInstabilityCurveData(int experimentId, const QVector<QVariantMap>& curveList) {
    Q_D(SqlOrmManager);

    if (!d->initialized || experimentId <= 0) return false;

    const QString connectionName = QString("SqlOrmInstabilityCurveWrite_%1").arg(reinterpret_cast<quintptr>(this));
    QString openError;
    QSqlDatabase db = openReadOnlyDb(d->dbPath, connectionName, &openError);
    if (!db.isOpen()) {
        emit errorOccurred(openError);
        closeReadOnlyDb(connectionName);
        return false;
    }

    QString schemaError;
    if (!ensureInstabilityResultTable(db, &schemaError)) {
        emit errorOccurred(schemaError);
        closeReadOnlyDb(connectionName);
        return false;
    }

    if (!db.transaction()) {
        emit errorOccurred(db.lastError().text());
        closeReadOnlyDb(connectionName);
        return false;
    }

    QSqlQuery deleteQuery(db);
    deleteQuery.prepare("DELETE FROM instability_curve_data WHERE experiment_id = ?");
    deleteQuery.addBindValue(experimentId);
    if (!deleteQuery.exec()) {
        emit errorOccurred(deleteQuery.lastError().text());
        db.rollback();
        closeReadOnlyDb(connectionName);
        return false;
    }

    if (!curveList.isEmpty()) {
        QSqlQuery insertQuery(db);
        insertQuery.prepare(
            "INSERT INTO instability_curve_data "
            "(experiment_id, scan_id, scan_elapsed_ms, channel_used, instability_value, created_at) "
            "VALUES (?, ?, ?, ?, ?, ?)");
        const QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

        for (const QVariantMap &curve : curveList) {
            insertQuery.bindValue(0, experimentId);
            insertQuery.bindValue(1, curve.value("scan_id", 0).toInt());
            insertQuery.bindValue(2, curve.value("scan_elapsed_ms", 0).toInt());
            insertQuery.bindValue(3, curve.value("channel_used").toString());
            insertQuery.bindValue(4, curve.value("instability_value", 0.0).toDouble());
            insertQuery.bindValue(5, curve.value("created_at", now).toString());
            if (!insertQuery.exec()) {
                emit errorOccurred(insertQuery.lastError().text());
                db.rollback();
                closeReadOnlyDb(connectionName);
                return false;
            }
        }
    }

    if (!db.commit()) {
        emit errorOccurred(db.lastError().text());
        closeReadOnlyDb(connectionName);
        return false;
    }

    closeReadOnlyDb(connectionName);
    return true;
}

QVector<QVariantMap> SqlOrmManager::getInstabilityCurveDataByExperiment(int experimentId) {
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);

    if (!d->initialized || experimentId <= 0) return result;

    const QString connectionName = QString("SqlOrmInstabilityCurveRead_%1").arg(reinterpret_cast<quintptr>(this));
    QString openError;
    QSqlDatabase db = openReadOnlyDb(d->dbPath, connectionName, &openError);
    if (!db.isOpen()) {
        emit errorOccurred(openError);
        closeReadOnlyDb(connectionName);
        return result;
    }

    QString schemaError;
    if (!ensureInstabilityResultTable(db, &schemaError)) {
        emit errorOccurred(schemaError);
        closeReadOnlyDb(connectionName);
        return result;
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT id, experiment_id, scan_id, scan_elapsed_ms, channel_used, instability_value, created_at "
        "FROM instability_curve_data WHERE experiment_id = ? "
        "ORDER BY scan_elapsed_ms ASC, scan_id ASC, id ASC");
    query.addBindValue(experimentId);

    if (!query.exec()) {
        emit errorOccurred(query.lastError().text());
        closeReadOnlyDb(connectionName);
        return result;
    }

    while (query.next()) {
        QVariantMap row;
        row["id"] = query.value("id");
        row["experiment_id"] = query.value("experiment_id");
        row["scan_id"] = query.value("scan_id");
        row["scan_elapsed_ms"] = query.value("scan_elapsed_ms");
        row["channel_used"] = query.value("channel_used");
        row["instability_value"] = query.value("instability_value");
        row["created_at"] = query.value("created_at");
        result.append(row);
    }

    closeReadOnlyDb(connectionName);
    return result;
}

QVector<QVariantMap> SqlOrmManager::getOrComputeInstabilityCurveDataByExperiment(int experimentId) {
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);

    if (!d->initialized || experimentId <= 0) return result;

    const QString connectionName = QString("SqlOrmInstabilityCompute_%1").arg(reinterpret_cast<quintptr>(this));
    QString openError;
    QSqlDatabase db = openReadOnlyDb(d->dbPath, connectionName, &openError);
    if (!db.isOpen()) {
        emit errorOccurred(openError);
        closeReadOnlyDb(connectionName);
        return result;
    }

    QString schemaError;
    if (!ensureInstabilityResultTable(db, &schemaError)) {
        emit errorOccurred(schemaError);
        closeReadOnlyDb(connectionName);
        return result;
    }

    int scanCount = 0;
    int maxElapsedMs = -1;
    {
        QSqlQuery statsQuery(db);
        statsQuery.prepare(
            "SELECT COUNT(DISTINCT scan_id) AS scan_count, "
            "COALESCE(MAX(scan_elapsed_ms), -1) AS max_elapsed_ms "
            "FROM experiment_data WHERE experiment_id = ?");
        statsQuery.addBindValue(experimentId);
        if (!statsQuery.exec() || !statsQuery.next()) {
            emit errorOccurred(statsQuery.lastError().text());
            closeReadOnlyDb(connectionName);
            return result;
        }
        scanCount = statsQuery.value("scan_count").toInt();
        maxElapsedMs = statsQuery.value("max_elapsed_ms").toInt();
    }

    if (scanCount <= 0) {
        closeReadOnlyDb(connectionName);
        return result;
    }

    bool reuseExisting = false;
    {
        QSqlQuery existingQuery(db);
        existingQuery.prepare(
            "SELECT COUNT(*) AS result_count, COALESCE(MAX(scan_elapsed_ms), -1) AS max_elapsed_ms "
            "FROM instability_curve_data WHERE experiment_id = ?");
        existingQuery.addBindValue(experimentId);
        if (existingQuery.exec() && existingQuery.next()) {
            reuseExisting = existingQuery.value("result_count").toInt() == scanCount
                         && existingQuery.value("max_elapsed_ms").toInt() == maxElapsedMs;
        }
    }

    closeReadOnlyDb(connectionName);
    if (reuseExisting) {
        return getInstabilityCurveDataByExperiment(experimentId);
    }

    const QString computeConnectionName = QString("SqlOrmInstabilityComputeData_%1").arg(reinterpret_cast<quintptr>(this));
    QSqlDatabase computeDb = openReadOnlyDb(d->dbPath, computeConnectionName, &openError);
    if (!computeDb.isOpen()) {
        emit errorOccurred(openError);
        closeReadOnlyDb(computeConnectionName);
        return result;
    }

    QSqlQuery query(computeDb);
    query.setForwardOnly(true);
    query.prepare(
        "SELECT scan_id, scan_elapsed_ms, height, backscatter_intensity, transmission_intensity "
        "FROM experiment_data WHERE experiment_id = ? "
        "ORDER BY scan_id ASC, height ASC, id ASC");
    query.addBindValue(experimentId);

    if (!query.exec()) {
        emit errorOccurred(query.lastError().text());
        closeReadOnlyDb(computeConnectionName);
        return result;
    }

    QVector<QVariantMap> computedCurves;
    QVector<InstabilityRowEx> referenceRows;
    QVector<InstabilityRowEx> currentRows;
    int currentScanId = std::numeric_limits<int>::min();
    int currentElapsedMs = 0;

    auto flushScan = [&](bool isReference) {
        if (currentRows.isEmpty()) {
            return;
        }

        QVariantMap row;
        row["scan_id"] = currentScanId;
        row["scan_elapsed_ms"] = currentElapsedMs;
        if (isReference) {
            row["channel_used"] = QStringLiteral("T");
            row["instability_value"] = 0.0;
            referenceRows = currentRows;
        } else {
            const bool useTransmission = averageTransmissionValue(currentRows) > 0.2;
            row["channel_used"] = useTransmission ? QStringLiteral("T") : QStringLiteral("BS");
            row["instability_value"] = computeInstabilityIntegral(referenceRows, currentRows, useTransmission);
        }
        computedCurves.append(row);
    };

    while (query.next()) {
        const int scanId = query.value("scan_id").toInt();
        if (currentScanId != std::numeric_limits<int>::min() && scanId != currentScanId) {
            flushScan(referenceRows.isEmpty());
            currentRows.clear();
        }

        currentScanId = scanId;
        currentElapsedMs = query.value("scan_elapsed_ms").toInt();

        InstabilityRowEx row;
        row.heightMm = query.value("height").toDouble() / 1000.0;
        row.backscatter = query.value("backscatter_intensity").toDouble();
        row.transmission = query.value("transmission_intensity").toDouble();
        currentRows.append(row);
    }

    if (currentScanId != std::numeric_limits<int>::min()) {
        flushScan(referenceRows.isEmpty());
    }

    closeReadOnlyDb(computeConnectionName);

    if (!replaceInstabilityCurveData(experimentId, computedCurves)) {
        return QVector<QVariantMap>();
    }

    return getInstabilityCurveDataByExperiment(experimentId);
}

QVector<QVariantMap> SqlOrmManager::getOrComputeInstabilityCurveDataByHeightRange(int experimentId, double lowerMm, double upperMm, const QString &segmentKey)
{
    // 这是不稳定性页卡顿优化的核心接口：
    // 只按当前需要的高度区间计算，并把结果缓存到独立结果表中。
    QVector<QVariantMap> result;
    Q_D(SqlOrmManager);

    if (!d->initialized || experimentId <= 0) return result;

    const double safeLower = qMin(lowerMm, upperMm);
    const double safeUpper = qMax(lowerMm, upperMm);
    if (safeUpper - safeLower <= 1e-6) {
        return result;
    }

    const QString normalizedSegmentKey = segmentKey.trimmed().isEmpty()
        ? QStringLiteral("custom")
        : segmentKey.trimmed();

    const QString readConnectionName = QString("SqlOrmInstabilitySegmentRead_%1").arg(reinterpret_cast<quintptr>(this));
    QString openError;
    QSqlDatabase readDb = openReadOnlyDb(d->dbPath, readConnectionName, &openError);
    if (!readDb.isOpen()) {
        emit errorOccurred(openError);
        closeReadOnlyDb(readConnectionName);
        return result;
    }

    QString schemaError;
    if (!ensureInstabilitySegmentResultTable(readDb, &schemaError)) {
        emit errorOccurred(schemaError);
        closeReadOnlyDb(readConnectionName);
        return result;
    }

    int scanCount = 0;
    int maxElapsedMs = -1;
    {
        QSqlQuery statsQuery(readDb);
        statsQuery.prepare(
            "SELECT COUNT(DISTINCT scan_id) AS scan_count, "
            "COALESCE(MAX(scan_elapsed_ms), -1) AS max_elapsed_ms "
            "FROM experiment_data WHERE experiment_id = ?");
        statsQuery.addBindValue(experimentId);
        if (!statsQuery.exec() || !statsQuery.next()) {
            emit errorOccurred(statsQuery.lastError().text());
            closeReadOnlyDb(readConnectionName);
            return result;
        }
        scanCount = statsQuery.value("scan_count").toInt();
        maxElapsedMs = statsQuery.value("max_elapsed_ms").toInt();
    }

    if (scanCount <= 0) {
        closeReadOnlyDb(readConnectionName);
        return result;
    }

    bool reuseExisting = false;
    {
        QSqlQuery existingQuery(readDb);
        existingQuery.prepare(
            "SELECT COUNT(*) AS result_count, COALESCE(MAX(scan_elapsed_ms), -1) AS max_elapsed_ms "
            "FROM instability_segment_curve_data "
            "WHERE experiment_id = ? AND segment_key = ? "
            "AND ABS(height_lower_mm - ?) < 0.000001 "
            "AND ABS(height_upper_mm - ?) < 0.000001");
        existingQuery.addBindValue(experimentId);
        existingQuery.addBindValue(normalizedSegmentKey);
        existingQuery.addBindValue(safeLower);
        existingQuery.addBindValue(safeUpper);
        if (existingQuery.exec() && existingQuery.next()) {
            reuseExisting = existingQuery.value("result_count").toInt() == scanCount
                         && existingQuery.value("max_elapsed_ms").toInt() == maxElapsedMs;
        }
    }

    auto readExistingRows = [&](QSqlDatabase &db) {
        // 读缓存时统一按时间顺序返回，前端不再重复排序。
        QVector<QVariantMap> rows;
        QSqlQuery query(db);
        query.prepare(
            "SELECT id, experiment_id, segment_key, height_lower_mm, height_upper_mm, "
            "scan_id, scan_elapsed_ms, channel_used, instability_value, created_at "
            "FROM instability_segment_curve_data "
            "WHERE experiment_id = ? AND segment_key = ? "
            "AND ABS(height_lower_mm - ?) < 0.000001 "
            "AND ABS(height_upper_mm - ?) < 0.000001 "
            "ORDER BY scan_elapsed_ms ASC, scan_id ASC, id ASC");
        query.addBindValue(experimentId);
        query.addBindValue(normalizedSegmentKey);
        query.addBindValue(safeLower);
        query.addBindValue(safeUpper);
        if (!query.exec()) {
            emit errorOccurred(query.lastError().text());
            return rows;
        }

        while (query.next()) {
            QVariantMap row;
            row["id"] = query.value("id");
            row["experiment_id"] = query.value("experiment_id");
            row["segment_key"] = query.value("segment_key");
            row["height_lower_mm"] = query.value("height_lower_mm");
            row["height_upper_mm"] = query.value("height_upper_mm");
            row["scan_id"] = query.value("scan_id");
            row["scan_elapsed_ms"] = query.value("scan_elapsed_ms");
            row["channel_used"] = query.value("channel_used");
            row["instability_value"] = query.value("instability_value");
            row["created_at"] = query.value("created_at");
            rows.append(row);
        }
        return rows;
    };

    if (reuseExisting) {
        result = readExistingRows(readDb);
        closeReadOnlyDb(readConnectionName);
        return result;
    }

    closeReadOnlyDb(readConnectionName);

    const QString computeConnectionName = QString("SqlOrmInstabilitySegmentCompute_%1").arg(reinterpret_cast<quintptr>(this));
    QSqlDatabase computeDb = openReadOnlyDb(d->dbPath, computeConnectionName, &openError);
    if (!computeDb.isOpen()) {
        emit errorOccurred(openError);
        closeReadOnlyDb(computeConnectionName);
        return result;
    }

    QSqlQuery query(computeDb);
    query.setForwardOnly(true);
    query.prepare(
        "SELECT scan_id, scan_elapsed_ms, height, backscatter_intensity, transmission_intensity "
        "FROM experiment_data WHERE experiment_id = ? "
        "ORDER BY scan_id ASC, height ASC, id ASC");
    query.addBindValue(experimentId);

    if (!query.exec()) {
        emit errorOccurred(query.lastError().text());
        closeReadOnlyDb(computeConnectionName);
        return result;
    }

    QVector<QVariantMap> computedCurves;
    QVector<InstabilityRowEx> referenceRows;
    QVector<InstabilityRowEx> currentRows;
    int currentScanId = std::numeric_limits<int>::min();
    int currentElapsedMs = 0;

    auto filterRowsByHeight = [&](const QVector<InstabilityRowEx> &rows) {
        QVector<InstabilityRowEx> filtered;
        filtered.reserve(rows.size());
        for (const InstabilityRowEx &row : rows) {
            if (row.heightMm >= safeLower && row.heightMm <= safeUpper) {
                filtered.append(row);
            }
        }
        return filtered;
    };

    auto flushScan = [&](bool isReference) {
        // 每凑齐一帧就立即计算一次当前区间的 Ius，
        // 首帧作为参考帧，后续各帧与它比较。
        if (currentRows.isEmpty()) {
            return;
        }

        const QVector<InstabilityRowEx> filteredRows = filterRowsByHeight(currentRows);
        if (filteredRows.size() < 2) {
            return;
        }

        QVariantMap row;
        row["segment_key"] = normalizedSegmentKey;
        row["height_lower_mm"] = safeLower;
        row["height_upper_mm"] = safeUpper;
        row["scan_id"] = currentScanId;
        row["scan_elapsed_ms"] = currentElapsedMs;
        if (isReference || referenceRows.isEmpty()) {
            row["channel_used"] = QStringLiteral("T");
            row["instability_value"] = 0.0;
            referenceRows = filteredRows;
        } else {
            const bool useTransmission = averageTransmissionValue(filteredRows) > 0.2;
            row["channel_used"] = useTransmission ? QStringLiteral("T") : QStringLiteral("BS");
            row["instability_value"] = computeInstabilityIntegral(referenceRows, filteredRows, useTransmission);
        }
        computedCurves.append(row);
    };

    while (query.next()) {
        const int scanId = query.value("scan_id").toInt();
        if (currentScanId != std::numeric_limits<int>::min() && scanId != currentScanId) {
            flushScan(referenceRows.isEmpty());
            currentRows.clear();
        }

        currentScanId = scanId;
        currentElapsedMs = query.value("scan_elapsed_ms").toInt();

        InstabilityRowEx row;
        row.heightMm = query.value("height").toDouble() / 1000.0;
        row.backscatter = query.value("backscatter_intensity").toDouble();
        row.transmission = query.value("transmission_intensity").toDouble();
        currentRows.append(row);
    }

    if (currentScanId != std::numeric_limits<int>::min()) {
        flushScan(referenceRows.isEmpty());
    }

    closeReadOnlyDb(computeConnectionName);

    const QString writeConnectionName = QString("SqlOrmInstabilitySegmentWrite_%1").arg(reinterpret_cast<quintptr>(this));
    QSqlDatabase writeDb = openReadOnlyDb(d->dbPath, writeConnectionName, &openError);
    if (!writeDb.isOpen()) {
        emit errorOccurred(openError);
        closeReadOnlyDb(writeConnectionName);
        return result;
    }

    if (!ensureInstabilitySegmentResultTable(writeDb, &schemaError)) {
        emit errorOccurred(schemaError);
        closeReadOnlyDb(writeConnectionName);
        return result;
    }

    if (!writeDb.transaction()) {
        emit errorOccurred(writeDb.lastError().text());
        closeReadOnlyDb(writeConnectionName);
        return result;
    }

    QSqlQuery deleteQuery(writeDb);
    deleteQuery.prepare(
        "DELETE FROM instability_segment_curve_data "
        "WHERE experiment_id = ? AND segment_key = ? "
        "AND ABS(height_lower_mm - ?) < 0.000001 "
        "AND ABS(height_upper_mm - ?) < 0.000001");
    deleteQuery.addBindValue(experimentId);
    deleteQuery.addBindValue(normalizedSegmentKey);
    deleteQuery.addBindValue(safeLower);
    deleteQuery.addBindValue(safeUpper);
    if (!deleteQuery.exec()) {
        emit errorOccurred(deleteQuery.lastError().text());
        writeDb.rollback();
        closeReadOnlyDb(writeConnectionName);
        return result;
    }

    if (!computedCurves.isEmpty()) {
        QSqlQuery insertQuery(writeDb);
        insertQuery.prepare(
            "INSERT INTO instability_segment_curve_data "
            "(experiment_id, segment_key, height_lower_mm, height_upper_mm, scan_id, scan_elapsed_ms, channel_used, instability_value, created_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
        const QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

        for (const QVariantMap &curve : computedCurves) {
            insertQuery.bindValue(0, experimentId);
            insertQuery.bindValue(1, curve.value("segment_key", normalizedSegmentKey).toString());
            insertQuery.bindValue(2, curve.value("height_lower_mm", safeLower).toDouble());
            insertQuery.bindValue(3, curve.value("height_upper_mm", safeUpper).toDouble());
            insertQuery.bindValue(4, curve.value("scan_id", 0).toInt());
            insertQuery.bindValue(5, curve.value("scan_elapsed_ms", 0).toInt());
            insertQuery.bindValue(6, curve.value("channel_used").toString());
            insertQuery.bindValue(7, curve.value("instability_value", 0.0).toDouble());
            insertQuery.bindValue(8, curve.value("created_at", now).toString());
            if (!insertQuery.exec()) {
                emit errorOccurred(insertQuery.lastError().text());
                writeDb.rollback();
                closeReadOnlyDb(writeConnectionName);
                return result;
            }
        }
    }

    if (!writeDb.commit()) {
        emit errorOccurred(writeDb.lastError().text());
        closeReadOnlyDb(writeConnectionName);
        return result;
    }

    result = readExistingRows(writeDb);
    closeReadOnlyDb(writeConnectionName);
    return result;
}

bool SqlOrmManager::deleteInstabilityCurveDataByExperiment(int experimentId) {
    Q_D(SqlOrmManager);

    // 整体结果和分区结果必须一起删，否则切换模式时可能读到旧缓存。
    if (!d->initialized || experimentId <= 0) return false;

    const QString connectionName = QString("SqlOrmInstabilityCurveDelete_%1").arg(reinterpret_cast<quintptr>(this));
    QString openError;
    QSqlDatabase db = openReadOnlyDb(d->dbPath, connectionName, &openError);
    if (!db.isOpen()) {
        emit errorOccurred(openError);
        closeReadOnlyDb(connectionName);
        return false;
    }

    QString schemaError;
    if (!ensureInstabilityResultTable(db, &schemaError) || !ensureInstabilitySegmentResultTable(db, &schemaError)) {
        emit errorOccurred(schemaError);
        closeReadOnlyDb(connectionName);
        return false;
    }

    if (!db.transaction()) {
        emit errorOccurred(db.lastError().text());
        closeReadOnlyDb(connectionName);
        return false;
    }

    QSqlQuery query(db);
    query.prepare("DELETE FROM instability_curve_data WHERE experiment_id = ?");
    query.addBindValue(experimentId);
    if (!query.exec()) {
        emit errorOccurred(query.lastError().text());
        db.rollback();
        closeReadOnlyDb(connectionName);
        return false;
    }

    QSqlQuery segmentQuery(db);
    segmentQuery.prepare("DELETE FROM instability_segment_curve_data WHERE experiment_id = ?");
    segmentQuery.addBindValue(experimentId);
    if (!segmentQuery.exec()) {
        emit errorOccurred(segmentQuery.lastError().text());
        db.rollback();
        closeReadOnlyDb(connectionName);
        return false;
    }

    const bool ok = db.commit();
    if (!ok) {
        emit errorOccurred(db.lastError().text());
    }

    closeReadOnlyDb(connectionName);
    return ok;
}

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
