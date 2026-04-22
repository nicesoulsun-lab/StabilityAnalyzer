#ifndef EXPERIMENT_LISTMODEL_H
#define EXPERIMENT_LISTMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QVariantMap>
#include "mainwindow_global.h"

/**
 * @brief 实验列表模型类
 * @class experiment_listmodel
 * 
 * 为 CustomListView_data.qml 提供实验数据模型，
 * 使用 SqlOrmManager 作为数据库后端。
 * 
 * 字段映射（与 CustomListView_data.qml 对应）：
 * - checked: 复选框选中状态（前端维护，不保存到数据库）
 * - projectName: 工程名称（数据库 project_name）
 * - expName: 实验名称（数据库 sample_name）
 * - status: 状态（0-未导入，1-已导入）
 * - expId: 实验 ID（数据库 id）
 * 
 * @par QML 使用示例：
 * @code
 * CustomListView_data {
 *     model: experimentListModel
 *     onRowSelected: {
 *         console.log("选中：", row, projectName, expName, status, expId)
 *     }
 * }
 * @endcode
 */
class MAINWINDOW_EXPORT experiment_listmodel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum ExperimentRoles {
        CheckedRole = Qt::UserRole + 1,
        SequenceRole,
        ProjectNameRole,
        ExpNameRole,
        StatusRole,
        StatusTextRole,
        StatusColorRole,
        ExpIdRole
    };

    explicit experiment_listmodel(QObject* parent = nullptr);
    void setDeletedOnly(bool deletedOnly);
    bool deletedOnly() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setExperiments(const QVector<QVariantMap> &experiments);

    /**
     * @brief 获取指定行和角色的数据
     * @param row 行索引
     * @param roleName 角色名称（"checked", "projectName", "expName", "status", "expId"）
     * @return 数据值
     */
    Q_INVOKABLE QVariant get(int row, const QString &roleName) const;
    Q_INVOKABLE QVariantMap getRow(int row) const;

    /**
     * @brief 设置复选框状态
     * @param row 行索引
     * @param checked 是否选中
     */
    Q_INVOKABLE void setChecked(int row, bool checked);

    /**
     * @brief 从数据库重新加载实验数据
     */
    Q_INVOKABLE void reloadFromDb();

    /**
     * @brief 获取实验数量
     * @return 实验数量
     */
    Q_INVOKABLE int count() const;

    /**
     * @brief 获取所有选中的实验ID
     * @return 选中的实验ID列表
     */
    Q_INVOKABLE QVariantList getCheckedExpIds() const;

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
    QVector<QVariantMap> m_experiments;
    QVector<bool> m_checkedStates;
    bool m_deletedOnly = false;
};

#endif // EXPERIMENT_LISTMODEL_H
