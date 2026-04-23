#ifndef CONTROLLERMANAGER_H
#define CONTROLLERMANAGER_H

#include <QObject>
#include "datatransmitcontroller.h"
#include "systemsetting_ctrl.h"
#include "user_ctrl.h"
#include "data_ctrl.h"
#include "experiment_ctrl.h"
#include "DataModel/user_sql_listmodel.h"
#include "DataModel/experiment_listmodel.h"

class MAINWINDOW_EXPORT CtrllerManager : public QObject
{
    Q_OBJECT

public:
    explicit CtrllerManager(QObject *parent = nullptr)
        : QObject(parent)
        , m_dataTransmitCtrl(new DataTransmitController(this))
        , m_sysSettingCtrl(new systemSettingCtrl(this))
        , m_userCtrl(new userCtrl(this))
        , m_dataCtrl(new dataCtrl(this))
        , m_experimentCtrl(new ExperimentCtrl(this))
        , m_userListmodel(new user_sql_listmodel(this))
        , m_experimentListmodel(new experiment_listmodel(this))
        , m_recycleExperimentListmodel(new experiment_listmodel(this))
    {
        m_recycleExperimentListmodel->setDeletedOnly(true);
    }

    // 获取各个 Controller 实例
    DataTransmitController* getDataTransmitCtrl() const { return m_dataTransmitCtrl; }
    systemSettingCtrl* getSystemSettingCtrl() const { return m_sysSettingCtrl; }
    userCtrl* getUserCtrl() const { return m_userCtrl; }
    dataCtrl* getDataCtrl() const { return m_dataCtrl; }
    ExperimentCtrl* getExperimentCtrl() const { return m_experimentCtrl; }

    user_sql_listmodel * getUserListmodel() const { return m_userListmodel; }
    experiment_listmodel * getExperimentListmodel() const { return m_experimentListmodel; }
    experiment_listmodel * getRecycleExperimentListmodel() const { return m_recycleExperimentListmodel; }

private:
    DataTransmitController* m_dataTransmitCtrl = nullptr;
    systemSettingCtrl* m_sysSettingCtrl = nullptr;
    userCtrl* m_userCtrl = nullptr;
    dataCtrl* m_dataCtrl = nullptr;
    ExperimentCtrl* m_experimentCtrl = nullptr;
    user_sql_listmodel* m_userListmodel = nullptr;
    experiment_listmodel* m_experimentListmodel = nullptr;
    experiment_listmodel* m_recycleExperimentListmodel = nullptr;
};

#endif // CONTROLLERMANAGER_H
