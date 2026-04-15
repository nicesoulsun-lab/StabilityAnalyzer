#include "DataPipeControl.h"

DataPipeControl* DataPipeControl::m_instance = nullptr;

DataPipeControl::DataPipeControl(QObject *parent)
    : QObject(parent)
{
    wheelForce.reserve(3000);
    stationarity.reserve(3000);
    wheelGeometry.reserve(3000);
    tsl.reserve(100000);
    syncTimer.setInterval(200);
    connect(&syncTimer,&QTimer::timeout,this,[=](){
        syncFlag%=5;
        if(syncFlag==1){
            setKRange();
        }else if(syncFlag==4){
            synchronize();
            emit updatePlot();
        }
        syncFlag++;
    });
    list<<&wheelForce<<&stationarity<<&wheelGeometry;
}

DataPipeControl::~DataPipeControl()
{
}

void DataPipeControl::pushWheelForceData(BasePackData * data)
{
    //qDebug()<<QDateTime::fromMSecsSinceEpoch(data->time());
    if(pushEnable())
        wheelForce.push_back(data);
    else
        data->painterFinish();
}

void DataPipeControl::pushStationarityeData(BasePackData * data)
{
    //qDebug()<<QDateTime::fromMSecsSinceEpoch(data->time());
    if(pushEnable())
        stationarity.push_back(data);
    else
        data->painterFinish();
}

void DataPipeControl::pushWheelGeometryData(BasePackData *data)
{
    if(pushEnable())
        wheelGeometry.push_back(data);
    else
        data->painterFinish();
}
//qreal kilometerTemp = 0;
void DataPipeControl::pushTimeLocationData(TSLData &data)
{
    m_currentSpeed = data.speed;
    m_currentKilometer = data.kilometer;
    /* 自动启停测试 */
//    if(data.kilometer==ParamSetModel::instance()->tlist().at(0).toInt()*100){
//        ParamSetModel::instance()->setCurrentLine(false);
//        setEnable(true);
//    }else if(data.kilometer==ParamSetModel::instance()->tlist().at(2).toInt()*100){
//        ParamSetModel::instance()->setCurrentLine(true);
//        setEnable(true);
//    }else if(data.kilometer==ParamSetModel::instance()->tlist().at(1).toInt()*100){
//        ParamSetModel::instance()->setCurrentLine(false);
//        setEnable(false);
//    }else if(data.kilometer==ParamSetModel::instance()->tlist().at(3).toInt()*100){
//        ParamSetModel::instance()->setCurrentLine(true);
//        setEnable(false);
//    }

    //qDebug()<<data.kilometer<<m_currentSpeed<<QDateTime::fromMSecsSinceEpoch(data.time);

    tsl.push_back(data);

    emit currentValueChanged();
}


void DataPipeControl::synchronize()
{
    int tslSize = tsl.size();
    int pipeIndex = 0;
    foreach(QVector<BasePackData*> *dvector,list){
        int dlen = dvector->size()-1;
        int tlen = tslSize-1;
        int index = dlen;
        /* 倒序检索 */
        for(int i = dlen; i >= 0; i--){
            for(int j = tlen; j>=1; j--){
                if(tsl.at(j).time >= dvector->at(i)->time()&&tsl.at(j-1).time <= dvector->at(i)->time()){
                    /* 里程取均值 */
                    double k1 = tsl.at(j-1).kilometer;
                    double k2 = tsl.at(j).kilometer;
                    double t1 = tsl.at(j-1).time;
                    double t2 = tsl.at(j).time;
                    double k = k1;

                    if(t1 != t2)
                        k = (k1-k2)*(dvector->at(i)->time()-t1)/(t1-t2)+k1;
                    dvector->at(i)->setKilometer(k);
                    tlen = j;
                    break;
                }else if(tsl.at(j).time < dvector->at(i)->time()&&j!=0){  //时空同步系统中还未获取到该时刻的里程信息
                    index--;
                    break;
                }else if(tsl.at(j).time < dvector->at(i)->time()){  //数据超出缓存，该条数据无法校验
                    qDebug()<<__FUNCTION__<<"lost";
                    dvector->at(i)->setKilometer(-1);
                }
            }
        }

        for(int i = 0; i<index; i++){
            out(pipeIndex,dvector->at(0));
            dvector->pop_front();
        }
        pipeIndex++;
    }
}

void DataPipeControl::setKRange()
{
//    if(m_kIncrease){
//        qreal begin = 0;
//        qreal end = m_currentKilometer;
//        if(m_currentKilometer-GlobalParam::instance()->kInterval()>0){
//            begin = m_currentKilometer-GlobalParam::instance()->kInterval();
//        }else{
//            end = GlobalParam::instance()->kInterval();
//        }
//        TotalManageModel::instance()->setBegin(begin);
//        TotalManageModel::instance()->setEnd(end);
//    }else{
//        qreal begin = m_currentKilometer;
//        qreal end = m_currentKilometer+GlobalParam::instance()->kInterval();
//        TotalManageModel::instance()->setBegin(begin);
//        TotalManageModel::instance()->setEnd(end);
//    }
    if(m_kIncrease){
        if(m_currentKilometer-GlobalParam::instance()->kInterval()<m_startKilometer){

            kRange.setX(m_startKilometer);
            kRange.setY(m_startKilometer+GlobalParam::instance()->kInterval());
            TotalManageModel::instance()->setBegin(m_startKilometer);
            TotalManageModel::instance()->setEnd(kRange.y());
        }else{
            kRange.setX(m_currentKilometer-GlobalParam::instance()->kInterval());
            kRange.setY(m_currentKilometer);
            TotalManageModel::instance()->setBegin(kRange.x());
            TotalManageModel::instance()->setEnd(m_currentKilometer);
            kRange.setX(m_currentKilometer-GlobalParam::instance()->kInterval());
            kRange.setY(m_currentKilometer);
        }
    }else{
        TotalManageModel::instance()->setBegin(m_currentKilometer-GlobalParam::instance()->kInterval());
        TotalManageModel::instance()->setEnd(m_currentKilometer);
    }
}

void DataPipeControl::out(int index, BasePackData *data)
{

    switch (index) {
    case 0:
        DataBaseHandler::instance()->pushWheelData(data);
        break;
    }
    TotalManageModel::instance()->push(index,data);
}

bool DataPipeControl::enable() const
{
    return m_enable;
}

void DataPipeControl::setEnable(bool enable)
{
    if(enable==m_enable){
        return;
    }
    m_enable = enable;
    if(m_enable){
        syncFlag = 0;
        syncTimer.start();
        m_startKilometer = m_currentKilometer;
    }else{
        syncTimer.stop();
    }
    emit enableChanged();
}

bool DataPipeControl::pushEnable()
{
    /* 开始采集，并且列车时速超过7.2km/h(2m/s) */
    if(m_enable && qAbs(m_currentSpeed)>7.2){
        return true;
    }else{
        return false;
    }
}

QString DataPipeControl::currentKilometer() const
{
    int k = m_currentKilometer/1000;
    int other = m_currentKilometer-k*1000;
    return "K"+QString::number(k)+"+"+QString::number(other);
}


QString DataPipeControl::currentSpeed() const
{
    return QString::number(QString::number(m_currentSpeed,'f',2).toDouble())+"km/h";
}

QVector<TSLData> DataPipeControl::tslData() const
{
    QVector<TSLData> data;
    for(int i = tsl.size()-1; i>=0; i--){
        data.push_front(tsl.at(i));
        if((tsl.at(i).kilometer>kRange.x())^m_kIncrease){
            break;
        }
    }
    return data;
}

bool DataPipeControl::kIncrease() const
{
    return m_kIncrease;
}

void DataPipeControl::setKIncrease(bool kIncrease)
{
    m_kIncrease = kIncrease;
}
