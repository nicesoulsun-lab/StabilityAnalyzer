#include "plotdatahandler.h"

PlotDataHandler::PlotDataHandler(QObject *parent) : QAbstractListModel(parent)
{
    m_roles.insert(0,"name");
    m_roles.insert(1,"enable");
}

void PlotDataHandler::initPlot()
{
    beginResetModel();
    for(int i = 0; i<100; i++){
        CurveInfo info;
        info.enable = (100%10==0);
        info.name = "参数"+QString::number(i);
        clist.push_back(info);
    }
    endResetModel();
    emit readyInit(clist);

    /* test */
    QTimer *timer = new QTimer(this);
    connect(timer,&QTimer::timeout,this,[=](){
        QList<qreal> list;
        for(int i = 0; i<100; i++){
            list.push_back(qrand()%100/10.0);
        }
        push(list);
    });

    timer->setInterval(50);
    timer->start();


    QTimer *timer2 = new QTimer(this);
    connect(timer2,&QTimer::timeout,this,[=](){
        emit readyUpdate();
    });

    timer2->setInterval(1000);
    timer2->start();
}


void PlotDataHandler::modifyCurve(int ind, bool flag)
{
    if(ind>clist.size()-1){
        qDebug()<<__FUNCTION__<<"out of range";
        return;
    }
    if(clist[ind].enable==flag){
        return;
    }
    clist[ind].enable = flag;
    emit readyModify(ind,flag);
    emit dataChanged(index(ind,0),index(ind,0),QVector<int>()<<1);
}

void PlotDataHandler::push(QList<qreal> &source, int begin, int step)
{
//    if(clist.isEmpty()){
//        qDebug()<<__FUNCTION__<<"not init";
//        return;
//    }

//    if(source.size()<clist.size()){
//        qDebug()<<__FUNCTION__<<"data not match";
//        return;
//    }
//    QList<qreal> beginList;
//    if(begin<0){
//        foreach (const CurveInfo &info, clist) {
//            if(info.source->isEmpty()){
//                beginList.push_back(0);
//            }else{
//                beginList.push_back(info.source->atLast().x());
//            }
//        }
//    }
//    int counter = 0;
//    foreach (const CurveInfo &info, clist) {
//        beginList[counter] = beginList.at(counter)+step;
//        info.source->push_back(QPointF(beginList.at(counter),source.at(counter)));
//        counter++;
//    }
//    emit readyUpdate();
}

void PlotDataHandler::push(QList<QList<qreal> > &source, int begin, int step)
{
//    if(clist.isEmpty()){
//        qDebug()<<__FUNCTION__<<"not init";
//    }

//    if(source.size()<=clist.size()){
//        qDebug()<<__FUNCTION__<<"data not match";
//    }
//    QList<qreal> beginList;
//    if(begin<0){
//        foreach (const CurveInfo &info, clist) {
//            if(info.source->isEmpty()){
//                beginList.push_back(0);
//            }else{
//                beginList.push_back(info.source->atLast().x());
//            }
//        }
//    }
//    int counter = 0;
//    foreach (const CurveInfo &info, clist) {
//        foreach (const qreal &value, source.at(counter)) {
//            beginList[counter] = beginList.at(counter)+step;
//            info.source->push_back(QPointF(beginList.at(counter),value));
//        }
//        counter++;
//    }
//    emit readyUpdate();
}

int PlotDataHandler::rowCount(const QModelIndex &parent) const
{
    return clist.size();
}

QVariant PlotDataHandler::data(const QModelIndex &index, int role) const
{
    if(role==0){
        return clist.at(index.row()).name;
    }else if(role==0){
        return clist.at(index.row()).enable;
    }
    return QVariant();
}

QHash<int, QByteArray> PlotDataHandler::roleNames() const
{
    return m_roles;
}
