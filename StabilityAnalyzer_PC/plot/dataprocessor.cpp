#include "dataprocessor.h"

DataProcessor::DataProcessor(QObject *parent) : QObject(parent)
{

}

DataProcessor::~DataProcessor()
{
}

QVector<QPointF> DataProcessor::readPoint(qreal lower, qreal upper)
{
	m_source = QVector<QPointF>();
    int index = getPointByX(lower);
    if(index==-1||m_data==nullptr)
        return m_source;
    index-=5;
	index = index < 0 ? 0 : index;
    while (index < m_data->size()) {
        m_source.push_back(m_data->at(index));
        if(m_data->at(index).x()>=upper){
            break;
        }else{
            index++;
        }
    }
    index+=5;
    if (index < m_data->size())
        m_source.push_back(m_data->at(index));
    return m_source;
}

int DataProcessor::getPointByX(qreal x)
{
    if(m_data==nullptr||m_data->isEmpty())
        return -1;

    int begin = 0;
    int end = m_data->size()-1;

    /* 二分法查询 */
    while (begin<end) {
        int mid = (end+begin)/2;
        if(m_data->at(mid).x() == x){
            return mid;
        }else if(m_data->at(mid).x()<x){
            begin = mid+1;
        }else{
            end = mid-1;
        }
    }

    if(m_data->at(begin).x()>x){
        if(begin==0)
            return 0;
        else
            return begin--;
    }else{
        return begin;
    }
}

QVector<QPointF> DataProcessor::read(qreal lower, qreal upper)
{
    return readPoint(lower, upper);
}

void DataProcessor::beginInitSource(LoopVector<QPointF>* array)
{
    m_data = array;
	flash();
}


void DataProcessor::flash()
{
	QPointF xRange, yRange;
    if(m_data==nullptr||m_data->size()==0)
        return;
    xRange.setX(m_data->at(0).x());
    xRange.setY(m_data->at(0).x());
    yRange.setX(m_data->at(0).y());
    yRange.setY(m_data->at(0).y());
    for(int i = 1; i<m_data->size(); i++){
        xRange.setX(qMin(xRange.x(), m_data->at(i).x()));
        xRange.setY(qMax(xRange.y(), m_data->at(i).x()));
        yRange.setX(qMin(yRange.x(), m_data->at(i).y()));
        yRange.setY(qMax(yRange.y(), m_data->at(i).y()));
    }
	emit dataChanged(xRange, yRange);
}

int DataProcessor::dataSum()
{
    return m_source.size();
}

void DataProcessor::getRange(QPointF &xRange, QPointF &yRange)
{
    if(m_data==nullptr||m_data->size() == 0)
        return;
    xRange.setX(m_data->at(0).x());
    xRange.setY(m_data->at(0).x());
    yRange.setX(m_data->at(0).y());
    yRange.setY(m_data->at(0).y());
    for(int i = 1; i<m_data->size(); i++){
        xRange.setX(qMin(xRange.x(), m_data->at(i).x()));
        xRange.setY(qMax(xRange.y(), m_data->at(i).x()));
        yRange.setX(qMin(yRange.x(), m_data->at(i).y()));
        yRange.setY(qMax(yRange.y(), m_data->at(i).y()));
    }
}
