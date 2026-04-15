#pragma once

#include <QObject>
#include <QVector>
#include <QTimer>
#include "wheelforcedata.h"
#include "wheelstationaritydata.h"
#include "LoopVector.h"
#include "totalmanagemodel.h"
#include "databasehandler.h"
#include "typetransform.h"
#include "paramsetmodel.h"

class DataPipeControl : public QObject
{
	Q_OBJECT
    Q_PROPERTY(bool enable READ enable WRITE setEnable NOTIFY enableChanged)
    Q_PROPERTY(QString currentKilometer READ currentKilometer NOTIFY currentValueChanged)
    Q_PROPERTY(QString currentSpeed READ currentSpeed NOTIFY currentValueChanged) 
public:
	DataPipeControl(QObject *parent = nullptr);
	~DataPipeControl();
	
	static DataPipeControl* instance() {
		if (m_instance == nullptr) {
			m_instance = new DataPipeControl();
		}
		return m_instance;
	}

    static void release(){
        if(m_instance!=nullptr){
            delete m_instance;
        }
    }

	/* 添加轮轨力数据 */
    void pushWheelForceData(BasePackData *data);

	/* 添加平稳性数据 */
    void pushStationarityeData(BasePackData *data);

    /* 添加平稳性数据 */
    void pushWheelGeometryData(BasePackData *data);

    /* 添加时空同步数据 */
    void pushTimeLocationData(TSLData &data);

    /* 同步 */
    void synchronize();

    void setKRange();

    void out(int index,BasePackData* data);

    bool enable() const;
    void setEnable(bool enable);

    bool pushEnable();

    QString currentKilometer() const;

    QString currentSpeed() const;

    QVector<TSLData> tslData() const;

    bool kIncrease() const;
    void setKIncrease(bool kIncrease);

signals:
    void enableChanged();
    void currentValueChanged();
    void updatePlot();
private:
    bool m_enable = false;
    static DataPipeControl* m_instance;

	//轮轨力的10个通道:时间，速度，里程，左垂，左横，右垂，右横，脱轨系数，轮重减载率，轮轴横向力
    QVector<BasePackData*> wheelForce;
	//平稳性的7个通道：时间，速度，里程，垂1，横1，垂2，横2
    QVector<BasePackData*> stationarity;

    //轨道几何的11个通道：左高低 右高低 左轨向 右轨向 轨距 超高 水平 三角坑 横向加速度 垂向加速度 轨距变化率
    QVector<BasePackData*> wheelGeometry;
    //时空同步数据
    LoopVector<TSLData> tsl;

    QTimer syncTimer;
    int syncFlag = 0;

    QList<QVector<BasePackData*>*> list;

    bool m_kIncrease = true;
public:
    qreal m_currentSpeed = 0;
    qreal m_currentKilometer = 0;
    qreal m_startKilometer = 0;

    QPointF kRange;
};
