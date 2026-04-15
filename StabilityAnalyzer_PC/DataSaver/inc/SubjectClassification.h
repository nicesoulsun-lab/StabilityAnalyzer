#include <QObject>
#include <atomic>
#include <memory>
#include <thread>
#include <QMutex>
#include <QMutexLocker>
#include "SubjectData.h"
#include "TaskInfo.h"
#include "datasaver_global.h"
#include <QMap>
/**
 * @brief The SubjectClassification class
 * 接收通信原始数据或者算法数据
 * 将各个模块数据发送至对应的存储模块
 */
class DATASAVER_EXPORT SubjectClassification : public QObject
{
    Q_OBJECT
public:
    explicit SubjectClassification(QObject *parent = nullptr);
    ~SubjectClassification();
    // 启动存储
    void start();
public:
    static SubjectClassification *subjectInstance();

private:

private slots:
    // 开始执行初始化 在子线程中创建资源
    void slot_subjecuhandle_start();

public slots:

signals:
    // 初始化信号
    void sig_subjecthandle_start();

private:
    QThread* runThread=nullptr;
    QMutex m_mutex;
};
