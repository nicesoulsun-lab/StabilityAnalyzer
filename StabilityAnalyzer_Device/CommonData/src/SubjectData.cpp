#include "SubjectData.h"
#include <QUuid>
#include <QDebug>
using namespace Config;
quint64 SubjectData::m_count = 0;
SubjectData::SubjectData(int id, int subid, QObject *parent) : QObject(parent)
{
    m_id = id;
    m_subId = subid;
    createUuid();
    //qDebug() << "subject create ++++++ " << m_uuid << ++m_count;
}

SubjectData::~SubjectData()
{
    //qDebug() << "subject release ------ "<< m_uuid << --m_count;
}

qreal SubjectData::kilometer() const
{
    return m_kilometer;
}

void SubjectData::setKilometer(const qreal &kilometer)
{
    m_kilometer = kilometer;
}

uint64_t SubjectData::time() const
{
    return m_time;
}

void SubjectData::setTime(const uint64_t &time)
{
    m_time = time;
}

QMap<int, qreal> SubjectData::data() const
{
    return m_data;
}

void SubjectData::insert(int key, const double &value)
{
    m_data.insert(key,value);
}

double SubjectData::get(int key)
{
    if(!m_data.contains(key)){
        return 0.0;
    }
    return m_data.value(key);
}

QMap<int, QVariant> SubjectData::moreData() const
{
    return m_moreData;
}

void SubjectData::insertMoreData(int key, const QVariant &value)
{
    m_moreData.insert(key, value);
}

QVariant SubjectData::getMoreData(int key)
{
    return m_moreData.value(key);
}

qreal SubjectData::runkilometer() const
{
    return m_runkilometer;
}

void SubjectData::setRunkilometer(const qreal &runkilometer)
{
    m_runkilometer = runkilometer;
}

QString SubjectData::uuid() const
{
    return m_uuid;
}

void SubjectData::createUuid()
{
    m_uuid = QUuid::createUuid().toString().remove('{').remove('}').remove('-');
}

qreal SubjectData::originKilometer() const
{
    return m_originKilometer;
}

void SubjectData::setOriginKilometer(qreal newOriginKilometer)
{
    m_originKilometer = newOriginKilometer;
}

qreal SubjectData::speed() const
{
    return m_speed;
}

void SubjectData::setSpeed(const qreal &speed)
{
    m_speed = speed;
}

int SubjectData::id() const
{
    return m_id;
}

