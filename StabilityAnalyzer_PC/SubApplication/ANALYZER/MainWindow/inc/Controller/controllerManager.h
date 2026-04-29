#ifndef CONTROLLERMANAGER_H
#define CONTROLLERMANAGER_H

#include <QObject>
#include <QQmlContext>
#include "datatransmitcontroller.h"
#include "systemsetting_ctrl.h"
#include "user_ctrl.h"
#include "data_ctrl.h"
#include "detail_ctrl.h"
#include "experiment_ctrl.h"
#include "realtime_ctrl.h"
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
        , m_detailCtrl(new detailCtrl(this))
        , m_experimentCtrl(new ExperimentCtrl(this))
        , m_realtimeCtrl(new realtimeCtrl(this))
        , m_userListmodel(new user_sql_listmodel(this))
        , m_experimentListmodel(new experiment_listmodel(this))
        , m_recycleExperimentListmodel(new experiment_listmodel(this))
    {
        m_recycleExperimentListmodel->setDeletedOnly(true);
        m_experimentCtrl->setDataTransmitController(m_dataTransmitCtrl);
        m_dataCtrl->setDataTransmitController(m_dataTransmitCtrl);
        m_realtimeCtrl->setDataTransmitController(m_dataTransmitCtrl);
        QObject::connect(m_experimentCtrl, &ExperimentCtrl::experimentStopped,
                         m_realtimeCtrl, &realtimeCtrl::handleExperimentStopped);
    }

    void bindToQmlContext(QQmlContext *context) const
    {
        if (!context) {
            return;
        }

        context->setContextProperty("data_transmit_ctrl", m_dataTransmitCtrl);
        context->setContextProperty("system_ctrl", m_sysSettingCtrl);
        context->setContextProperty("user_ctrl", m_userCtrl);
        context->setContextProperty("data_ctrl", m_dataCtrl);
        context->setContextProperty("detail_ctrl", m_detailCtrl);
        context->setContextProperty("experiment_ctrl", m_experimentCtrl);
        context->setContextProperty("realtime_ctrl", m_realtimeCtrl);
        context->setContextProperty("user_list_model", m_userListmodel);
        context->setContextProperty("experiment_list_model", m_experimentListmodel);
        context->setContextProperty("recycle_experiment_list_model", m_recycleExperimentListmodel);
    }

    DataTransmitController* getDataTransmitCtrl() const { return m_dataTransmitCtrl; }
    systemSettingCtrl* getSystemSettingCtrl() const { return m_sysSettingCtrl; }
    userCtrl* getUserCtrl() const { return m_userCtrl; }
    dataCtrl* getDataCtrl() const { return m_dataCtrl; }
    detailCtrl* getDetailCtrl() const { return m_detailCtrl; }
    ExperimentCtrl* getExperimentCtrl() const { return m_experimentCtrl; }
    realtimeCtrl* getRealtimeCtrl() const { return m_realtimeCtrl; }

    user_sql_listmodel * getUserListmodel() const { return m_userListmodel; }
    experiment_listmodel * getExperimentListmodel() const { return m_experimentListmodel; }
    experiment_listmodel * getRecycleExperimentListmodel() const { return m_recycleExperimentListmodel; }

private:
    DataTransmitController* m_dataTransmitCtrl = nullptr;
    systemSettingCtrl* m_sysSettingCtrl = nullptr;
    userCtrl* m_userCtrl = nullptr;
    dataCtrl* m_dataCtrl = nullptr;
    detailCtrl* m_detailCtrl = nullptr;
    ExperimentCtrl* m_experimentCtrl = nullptr;
    realtimeCtrl* m_realtimeCtrl = nullptr;
    user_sql_listmodel* m_userListmodel = nullptr;
    experiment_listmodel* m_experimentListmodel = nullptr;
    experiment_listmodel* m_recycleExperimentListmodel = nullptr;
};

#endif // CONTROLLERMANAGER_H
