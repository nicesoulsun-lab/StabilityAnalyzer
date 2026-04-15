#include "SubjectClassification.h"
#include <QtCore>
#include "BasePackData.h"
#include "SubjectSaveWorker.h"
#include "logmanager.h"

using namespace SubjectType;
using namespace Config;

SubjectClassification::SubjectClassification(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<SubTaskInfo>("SubTaskInfo");
    qRegisterMetaType<QVector<QSharedPointer<SubjectData>>>("QVector<QSharedPointer<SubjectData>>");
    // qDebug()<<"------------------threadId:"<<QThread::currentThreadId();

    runThread = new QThread;
    this->moveToThread(runThread);
    connect(this, SIGNAL(sig_subjecthandle_start()), this, SLOT(slot_subjecuhandle_start()));
    // m_subTaskInfo.init();
    runThread->start();

}

SubjectClassification::~SubjectClassification()
{

}

SubjectClassification *SubjectClassification::subjectInstance()
{
    static SubjectClassification *m_instance=nullptr;
    if(m_instance == nullptr)
    {
        m_instance = new SubjectClassification;
    }
    return m_instance;
}

void SubjectClassification::slot_subjecuhandle_start()
{
    // 1.创建各个存储模块实例对象，TODO这地方也可以是吧需要存储的模块先放到commondata的一个集合里面，然后在这个地方遍历读取创建每个模块的实例对象
    WheelGeometry_Save* wheelGeometry =new WheelGeometry_Save(SubjectType::WheelGeometry);
    connect(this, SIGNAL(sig_WheelGeometryData(SubTaskInfo,QVector<QSharedPointer<SubjectData>>)), wheelGeometry, SLOT(slot_datasaver_subjectData(SubTaskInfo,QVector<QSharedPointer<SubjectData>>)));
    wheelGeometry->start();
}

void SubjectClassification::start()
{
    emit sig_subjecthandle_start();
}





