#ifndef SUBJECTSAVEWORKER_H
#define SUBJECTSAVEWORKER_H

#include <QObject>
#include "DataSaver_Base.h"

class SubjectSaveWorker : public QObject
{
    Q_OBJECT
public:
    explicit SubjectSaveWorker(QObject *parent = nullptr);

signals:
public:
};

/////////////////// ** 以下为各专业执行保存操作的类 **/////////////////////////////
/// 轨道几何
class WheelGeometry_Save : public DataSaver_Base
{
    Q_OBJECT
public:
    explicit WheelGeometry_Save(int subject = 0, QObject* parent = nullptr): DataSaver_Base(subject, parent) {
        m_tmpSqlTable = "track_trackdata_tmpsql";
    }
    ~WheelGeometry_Save(){}
protected:
    void createDataSqlList(QVector<QSharedPointer<SubjectData>>& dataList) override;
};
#endif // SUBJECTSAVEWORKER_H
