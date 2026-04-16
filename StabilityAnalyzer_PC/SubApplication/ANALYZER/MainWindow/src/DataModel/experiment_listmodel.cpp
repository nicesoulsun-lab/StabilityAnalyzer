#include "DataModel/experiment_listmodel.h"
#include "SqlOrmManager.h"
#include <QDebug>

namespace {
QString statusTextForValue(const QVariant &statusValue)
{
    return statusValue.toInt() == 1 ? QStringLiteral("已导入")
                                    : QStringLiteral("未导入");
}

QString statusColorForValue(const QVariant &statusValue)
{
    return statusValue.toInt() == 1 ? QStringLiteral("#333333")
                                    : QStringLiteral("#FF4D4F");
}
}

experiment_listmodel::experiment_listmodel(QObject* parent)
    : QAbstractTableModel(parent)
{
    reloadFromDb();
}

int experiment_listmodel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_experiments.size();
}

int experiment_listmodel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 4;
}

QVariant experiment_listmodel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_experiments.size())
        return QVariant();

    const QVariantMap &experiment = m_experiments[index.row()];

    if (role == CheckedRole) return m_checkedStates[index.row()];
    if (role == SequenceRole) return index.row() + 1;
    if (role == ProjectNameRole) return experiment["project_name"];
    if (role == ExpNameRole) return experiment["sample_name"];
    if (role == StatusRole) return experiment["status"];
    if (role == StatusTextRole) return statusTextForValue(experiment["status"]);
    if (role == StatusColorRole) return statusColorForValue(experiment["status"]);
    if (role == ExpIdRole) return experiment["id"];

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case 0: return m_checkedStates[index.row()];
        case 1: return index.row() + 1;
        case 2: return experiment["project_name"];
        case 3: return experiment["sample_name"];
        default: return QVariant();
        }
    }
    return QVariant();
}

QVariant experiment_listmodel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return QStringLiteral("选择");
        case 1: return QStringLiteral("工程名称");
        case 2: return QStringLiteral("实验名称");
        case 3: return QStringLiteral("状态");
        default: return QVariant();
        }
    }
    return QVariant();
}

QHash<int, QByteArray> experiment_listmodel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[CheckedRole] = "checked";
    roles[SequenceRole] = "sequence";
    roles[ProjectNameRole] = "projectName";
    roles[ExpNameRole] = "expName";
    roles[StatusRole] = "status";
    roles[StatusTextRole] = "statusText";
    roles[StatusColorRole] = "statusColor";
    roles[ExpIdRole] = "expId";
    return roles;
}

void experiment_listmodel::setExperiments(const QVector<QVariantMap> &experiments)
{
    beginResetModel();
    m_experiments = experiments;
    m_checkedStates = QVector<bool>(experiments.size(), false);
    endResetModel();
}

QVariant experiment_listmodel::get(int row, const QString &roleName) const
{
    if (row < 0 || row >= m_experiments.size()) return QVariant();
    const QVariantMap &experiment = m_experiments[row];
    if (roleName == "checked") return m_checkedStates[row];
    if (roleName == "sequence") return row + 1;
    if (roleName == "projectName") return experiment["project_name"];
    if (roleName == "expName") return experiment["sample_name"];
    if (roleName == "status") return experiment["status"];
    if (roleName == "statusText") return statusTextForValue(experiment["status"]);
    if (roleName == "statusColor") return statusColorForValue(experiment["status"]);
    if (roleName == "expId") return experiment["id"];
    return experiment.value(roleName);
}

QVariantMap experiment_listmodel::getRow(int row) const
{
    if (row < 0 || row >= m_experiments.size()) return QVariantMap();

    QVariantMap experiment = m_experiments[row];
    experiment["checked"] = m_checkedStates.value(row, false);
    experiment["sequence"] = row + 1;
    experiment["statusText"] = statusTextForValue(experiment["status"]);
    experiment["statusColor"] = statusColorForValue(experiment["status"]);
    return experiment;
}

void experiment_listmodel::setChecked(int row, bool checked)
{
    if (row < 0 || row >= m_checkedStates.size()) return;
    m_checkedStates[row] = checked;
    QModelIndex index = createIndex(row, 0);
    emit dataChanged(index, index, {CheckedRole});
}

void experiment_listmodel::reloadFromDb()
{
    try {
        auto experiments = SQLORM->getAllExperiments();

        qDebug()<<"实验数据："<<experiments;

        setExperiments(experiments);
        emit success();
    } catch (...) {
        setExperiments({});
        emit error("加载实验数据失败");
    }
}

int experiment_listmodel::count() const
{
    return m_experiments.size();
}

QVariantList experiment_listmodel::getCheckedExpIds() const
{
    QVariantList result;
    for (int i = 0; i < m_experiments.size(); ++i) {
        if (m_checkedStates[i]) {
            result.append(m_experiments[i]["id"]);
        }
    }
    return result;
}
