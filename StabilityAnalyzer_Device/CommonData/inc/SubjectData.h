#ifndef SUBJECTDATA_H
#define SUBJECTDATA_H

#include <QObject>
#include <QMap>
#include <QVariant>
#include "commondata_global.h"
namespace Config{


///// 添加专业时需在TimeLocal之前添加(其他模块有根据值判断的便利)
namespace CSubject{
const int None =                0;
namespace WheelForce{
const int Force =           1;
const int Stationarity =    2;
}
}

// 轨道几何
namespace WheelGeometryParam{
const int LeftHL =      0;      //左高低
const int RightHL =     1;      //右高低
const int LeftDire =    2;      //左轨向
const int RightDire =   3;      //右轨向
const int HorAcc =      4;      //横向加速度
const int VerAcc =      5;      //垂向加速度
const int DisChange =   6;      //轨距变化
const int Level =       7;      //水平
const int SuperHigh =   8;      //超高
const int Dis =         9;      //轨距
const int TrianglePit = 10;     //三角坑
}
}

/**
 * @brief The SubjectData class
 * 专业检测数据 解析后的数据
 */
class COMMONDATA_EXPORT SubjectData : public QObject
{
    Q_OBJECT
public:
    explicit SubjectData(int id = 0, int subid = 0, QObject *parent = nullptr);
    ~SubjectData();

    qreal kilometer() const;
    void setKilometer(const qreal &kilometer);

    uint64_t time() const;
    void setTime(const uint64_t &time);

    QMap<int, qreal> data() const;

    void insert(int key, const double &value);

    double get(int key);

    QMap<int, QVariant> moreData() const;
    void insertMoreData(int key, const QVariant& value);
    QVariant getMoreData(int key);

    qreal runkilometer() const;
    void setRunkilometer(const qreal &runkilometer);

    QString uuid() const;
    void createUuid();

    qreal originKilometer() const;
    void setOriginKilometer(qreal newOriginKilometer);


    qreal speed() const;
    void setSpeed(const qreal &speed);

    int id() const;

signals:

public slots:

private:
    int m_id;                       //专业id
    int m_subId;                    //子专业id
    qreal m_kilometer = 0;          //里程标(时空同步后的)
    qreal m_runkilometer = 0;       //行驶里程
    qreal m_originKilometer = 0;    //原始里程数据
    qreal m_speed = 0;              //速度
    uint64_t m_time;
    QString m_uuid;             //数据uuid 每条解析数据都有一个独立的uuid 方便数据存储与发送操作
    // 解析后数据 字段序号(不是字节)-字段值 不包含时间、里程字段(有单独的字段)
    // double类型单独存储 画图时用
    QMap<int, double> m_data;
    // 解析后数据 字段序号-字段值 用来存储非double类型的其他数值 例如
    QMap<int, QVariant> m_moreData;
    static quint64 m_count;
};

#endif // SUBJECTDATA_H
