#include "curvemodel.h"

CurveModel::CurveModel(QObject *parent) : QObject(parent)
{
}

CurveModel::~CurveModel()
{
//    if(m_source&&!outSource){
//        delete m_source;
//    }
}

AxisModel *CurveModel::xAxis() const
{
    return m_xAxis;
}

void CurveModel::setXAxis(AxisModel *xAxis)
{
    if(m_xAxis!=nullptr){
        disconnect(m_xAxis,0,this,0);
    }
    m_xAxis = xAxis;
    emit xAxisChanged();
    emit sourceChanged();
}

AxisModel *CurveModel::yAxis() const
{
    return m_yAxis;
}

void CurveModel::setYAxis(AxisModel *yAxis)
{
    if(m_yAxis!=nullptr){
        disconnect(m_yAxis,0,this,0);
    }
    m_yAxis = yAxis;
    emit yAxisChanged();
    emit sourceChanged();
}

QColor CurveModel::color1() const
{
    return m_color1;
}

void CurveModel::setColor1(const QColor &color1)
{
    m_color1 = color1;
    emit color1Changed();
    emit styleChanged();
}

QColor CurveModel::color2() const
{
    return m_color2;
}

void CurveModel::setColor2(const QColor &color2)
{
    m_color2 = color2;
    emit color2Changed();
    emit styleChanged();
}

QColor CurveModel::lineColor() const
{
    return m_lineColor;
}

void CurveModel::setLineColor(const QColor &lineColor)
{
    m_lineColor = lineColor;
    emit lineColorChanged();
    emit styleChanged();
}

qreal CurveModel::lineWidth() const
{
    return m_lineWidth;
}

void CurveModel::setLineWidth(const qreal &lineWidth)
{
    m_lineWidth = lineWidth;
    emit styleChanged();
}

bool CurveModel::gradientFill() const
{
    return m_gradientFill;
}

void CurveModel::setGradientFill(bool gradientFill)
{
    m_gradientFill = gradientFill;
    emit gradientFillChanged();
    emit styleChanged();
}

void CurveModel::draw()
{
    int index = 0;
    QPointF pointTemp;
    QVector<QPointF> drawPoint;

    if(m_xAxis==nullptr||m_yAxis==nullptr||m_source.data()==nullptr){
        return;
    }
    if(m_xAxis->lower()>=m_xAxis->upper())
        return;

    QPixmap pix = QPixmap(m_width,m_height);
    pix.fill(Qt::transparent);
    if(!pix.paintEngine()){
        return;
    }
    QPainter painter(&pix);
    if(!painter.isActive())
        return;
    painter.setRenderHint(QPainter::Antialiasing,true);
    index = getPointByX(m_xAxis->lower());
    if(index<0){
        return;
    }
    index = index<5?0:index-5;
    while (runEnable) {
        if(index>m_source.data()->size()-1){
            break;
        }
        if(m_source.data()->at(index).x()<m_xAxis->lower()){
            pointTemp = m_source.data()->at(index);
            index++;
            continue;
        }
        QPointF point = transformCoords(m_source.data()->at(index));
        drawPoint.push_back(point);
        index++;
        if(point.x()>=m_width){
            break;
        }
    }
    drawPoint.push_front(transformCoords(pointTemp));


    QPen pen(m_lineColor,m_lineWidth);
    if(m_lineType==1){
        pen.setStyle(Qt::DashLine);
    }
    painter.setPen(pen);
    painter.drawPolyline(drawPoint);
    if(m_gradientFill){
        QLinearGradient linear(QPointF(0,0), QPointF(0, m_height));
        linear.setColorAt(0,color1());
        linear.setColorAt(1,color2());
        painter.setBrush(linear);
        painter.setPen(Qt::transparent);
        QPointF pTemp1 = drawPoint.at(0);
        QPointF pTemp2 = drawPoint.at(drawPoint.size()-1);
        drawPoint.push_back(QPointF(pTemp2.x(),m_height));
        drawPoint.push_back(QPointF(pTemp1.x(),m_height));
        painter.drawPolygon(drawPoint);
    }
//    drawAlarmLine(painter);
    canvas = pix;
}

QPointF CurveModel::drawCursor(QPointF pos,bool &online)
{
    online = false;
    if(m_xAxis==nullptr||m_yAxis==nullptr||m_width==0||m_height==0){
        return QPointF();
    }
    if(m_xAxis->lower()>=m_xAxis->upper())
        return QPointF();

    QPixmap pix = QPixmap(m_width+20,m_height+20);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setBrush(m_lineColor);
    painter.setPen(m_lineColor);
    painter.translate(10,10);
    painter.setRenderHint(QPainter::Antialiasing,true);

    qreal scale = pos.x()/m_width;
    qreal x = m_xAxis->lower()+scale*(m_xAxis->upper()-m_xAxis->lower());
    QPointF p = getNearPointByX(x, pos, online);
    if(online){
        QPointF p2 = transformCoords(p);
        painter.drawRoundedRect(p2.x()-3,p2.y()-3,6,6,6,6);
    }
    cursorCanvas = pix;
    return p;
}

void CurveModel::drawAlarmLine(QPainter *painter,QRect &rect)
{
//    if(m_alarmValue.isEmpty()){
//        return;
//    }

//    QJsonObject hf = m_alarmValue.find("hf").value().toObject();
//    QJsonObject lf = m_alarmValue.find("lf").value().toObject();

//    if(!hf.isEmpty()){
//        qreal level1 = hf.find("lowThreshold1").value().toString().toDouble();
//        drawLine(painter,rect,level1,"高频预警");
////        qreal level2 = hf.find("lowThreshold2").value().toString().toDouble();
////        drawLine(painter,rect,level2,"高频预警2");
//        qreal level3 = hf.find("highThreshold1").value().toString().toDouble();
//        drawLine(painter,rect,level3,"高频告警");
////        qreal level4 = hf.find("highThreshold2").value().toString().toDouble();
////        drawLine(painter,rect,level4,"高频告警2");
//    }
//    if(!lf.isEmpty()){
//        qreal level1 = lf.find("lowThreshold1").value().toString().toDouble();
//        drawLine(painter,rect,level1,"低频预警");
////        qreal level2 = lf.find("lowThreshold2").value().toString().toDouble();
////        drawLine(painter,rect,level2,"低频预警2");
//        qreal level3 = lf.find("highThreshold1").value().toString().toDouble();
//        drawLine(painter,rect,level3,"低频告警");
////        qreal level4 = lf.find("highThreshold2").value().toString().toDouble();
////        drawLine(painter,rect,level4,"低频告警2");
//    }
}

void CurveModel::drawLine(QPainter *painter,QRect &rect, qreal val,QString text)
{
    QPointF p = transformCoords(QPointF(0,val));
    if(p.y()>0&&p.y()<m_height){
        QPen pen;
        pen.setColor(m_lineColor);
        pen.setDashPattern(QVector<qreal>()<<8<<3<<4<<3);
        painter->setPen(pen);
        painter->drawLine(rect.x(),rect.y()+p.y(),rect.x()+m_width,rect.y()+p.y());
        painter->setPen(m_lineColor);
        painter->setBrush(m_lineColor);
        painter->drawRect(rect.x()+m_width-70,rect.y()+p.y()-10,66,20);
        painter->setPen(Qt::white);
        painter->drawText(rect.x()+m_width-68,rect.y()+p.y()+6,text);
    }
}

void CurveModel::clearCursor()
{
    QPixmap pix = QPixmap(m_width,m_height);
    pix.fill(Qt::transparent);
    cursorCanvas = pix;
}

QPointF CurveModel::transformCoords(QPointF point)
{
    if(m_xAxis==nullptr||m_yAxis==nullptr)
        return QPoint(0,0);
    return QPointF((point.x()-m_xAxis->lower())*xUnitLen
                   ,(m_yAxis->upper()-point.y())*yUnitLen);
}


void CurveModel::setWidth(const qreal &width)
{
    m_width = width;
    if(m_xAxis!=nullptr)
        xUnitLen = m_width/(m_xAxis->upper()-m_xAxis->lower());
}

void CurveModel::setHeight(const qreal &height)
{
    m_height = height;
    if(m_yAxis!=nullptr)
        yUnitLen = m_height/(m_yAxis->upper()-m_yAxis->lower());
}

QPixmap CurveModel::getCurve()
{
    if(!m_visible)
        return QPixmap();
    if(m_warning){
        int ms = QTime::currentTime().msec();
        bool flag = (ms/500)%2;
        if(flag){
            return QPixmap();
        }else{
            return canvas;
        }
    }
    return canvas;
}

QPixmap CurveModel::getCursor()
{
    return cursorCanvas;
}

QString CurveModel::title() const
{
    return m_title;
}

void CurveModel::setTitle(const QString &title)
{
    m_title = title;
    emit titleChanged();
}

void CurveModel::stop()
{
    runEnable = false;
}

bool CurveModel::tipsEnable() const
{
    return m_tipsEnable;
}

void CurveModel::setTipsEnable(bool tipsEnable)
{
    m_tipsEnable = tipsEnable;
    emit tipsEnableChanged();
}

int CurveModel::lineType() const
{
    return m_lineType;
}

void CurveModel::setLineType(int lineType)
{
    m_lineType = lineType;
    emit lineTypeChanged();
    emit styleChanged();
}

int CurveModel::sourceSize()
{
    if(m_source.data()==nullptr){
        return 0;
    }
    return m_source.data()->size();
}

bool CurveModel::visible() const
{
    return m_visible;
}

void CurveModel::setVisible(bool visible)
{
    m_visible = visible;
    emit visibleChanged();
}

void CurveModel::initSource(int cacheSum)
{
    m_source = QSharedPointer<LoopVector<QPointF>>(new LoopVector<QPointF>(cacheSum));
    emit sourceChanged();
}

void CurveModel::initSource(QSharedPointer<LoopVector<QPointF>> s)
{
    //outSource = true;
    m_source = s;
    emit sourceChanged();
}

QPointF CurveModel::getLabel(qreal x)
{
    if(m_xAxis==nullptr)
        return QPointF();
    if(x>=m_xAxis->lower()&&x<=m_xAxis->upper()){
        return getNearPointByX(x);
    }
    return QPointF(-1,-1);
}

int CurveModel::getPointByX(qreal x)
{
    if(m_source.data() == nullptr || m_source.data()->isEmpty())
        return -1;

    int begin = 0;
    int end = m_source.data()->size()-1;

    /* 二分法查询 */
    while (begin<end) {
        int mid = (end+begin)/2;
        if(m_source.data()->at(mid).x()== x){
            return mid;
        }else if(m_source.data()->at(mid).x()<x){
            begin = mid+1;
        }else{
            end = mid-1;
        }
    }

    if(m_source.data()->at(begin).x()>x){
        if(begin==0)
            return 0;
        else
            return begin--;
    }else{
        return begin;
    }
}

QPointF CurveModel::getIntersectionByX(qreal x)
{
    if(m_source.data() == nullptr || m_source.data()->isEmpty())
        return QPointF(0,0);
    int begin = 0;
    int end = m_source.data()->size()-1;
    /* 二分法查询 */
    while (begin<end) {
        int mid = (end+begin)/2;
        if(m_source.data()->at(mid).x()== x){
            return m_source.data()->at(mid);
        }else if(m_source.data()->at(mid).x()<x){
            begin = mid+1;
        }else{
            end = mid-1;
        }
    }

    QPointF small,big;
    if(m_source.data()->at(begin).x()>x){
        if(begin==0){
            return QPointF(x,m_source.data()->at(begin).y());
        }
        big = m_source.data()->at(begin);
        small = m_source.data()->at(begin-1);
    }else{
        if(begin==m_source.data()->size()-1){
            return QPointF(x,m_source.data()->at(begin).y());
        }
        small = m_source.data()->at(begin);
        big = m_source.data()->at(begin+1);
    }
    qreal scale =(x-small.x())/(big.x()-small.x());

    qreal y = (big.y()-small.y())*scale+small.y();
    return QPointF(x,y);
}

QPointF CurveModel::getNearPointByX(qreal x)
{
    if(m_source.data() == nullptr || m_source.data()->isEmpty())
        return QPointF(0,0);
    int begin = 0;
    int end = m_source.data()->size()-1;
    /* 二分法查询 */
    while (begin<end) {
        int mid = (end+begin)/2;
        if(m_source.data()->at(mid).x()== x){
            return m_source.data()->at(mid);
        }else if(m_source.data()->at(mid).x()<x){
            begin = mid+1;
        }else{
            end = mid-1;
        }
    }

    QPointF small,big;
    if(m_source.data()->at(begin).x()>x){
        if(begin==0){
            return QPointF(x,m_source.data()->at(begin).y());
        }
        big = m_source.data()->at(begin);
        small = m_source.data()->at(begin-1);
    }else{
        if(begin==m_source.data()->size()-1){
            return QPointF(x,m_source.data()->at(begin).y());
        }
        small = m_source.data()->at(begin);
        big = m_source.data()->at(begin+1);
    }
    if(big.x()-x>x-small.x())
        return small;
    else
        return big;
}

QPointF CurveModel::getNearPointByX(qreal x,QPointF pos,bool &onLine)
{
    if(m_source.data() == nullptr || m_source.data()->isEmpty())
        return QPointF(0,0);
    int begin = 0;
    int end = m_source.data()->size()-1;
    onLine = false;
    /* 二分法查询 */
    while (begin<end) {
        int mid = (end+begin)/2;
        if(m_source.data()->at(mid).x()== x){
            onLine = true;
            return m_source.data()->at(mid);
        }else if(m_source.data()->at(mid).x()<x){
            begin = mid+1;
        }else{
            end = mid-1;
        }
    }

    QPointF small,big;
    if(m_source.data()->at(begin).x()>x){
        if(begin==0){
            return QPointF(x,m_source.data()->at(begin).y());
        }
        big = m_source.data()->at(begin);
        small = m_source.data()->at(begin-1);
    }else{
        if(begin==m_source.data()->size()-1){
            return QPointF(x,m_source.data()->at(begin).y());
        }
        small = m_source.data()->at(begin);
        big = m_source.data()->at(begin+1);
    }

    /* 取附近的20个点，获取离得最近的一个点 */
    int x1 = qMax(0,begin-10);
    int x2 = qMin(m_source.data()->size()-1,begin+10);
    QPointF p = m_source.data()->at(x1);
    qreal dis2 = getDis(p,pos);
    for(int i = x1+1;i<=x2;i++){
        qreal dis3 = getDis(transformCoords(m_source.data()->at(i)),pos);
        if(dis3<dis2){
            dis2 = dis3;
            p = m_source.data()->at(i);
        }
    }
    onLine = dis2<16;
    return p;
}

QPointF CurveModel::addLabel(qreal scale,QPoint pos,bool& flag)
{
    if(m_xAxis==nullptr){
        flag = false;
        return QPointF();
    }
    qreal x = m_xAxis->lower()+scale*(m_xAxis->upper()-m_xAxis->lower());
    return getNearPointByX(x,pos,flag);
}

QPointF CurveModel::addLabel(qreal x)
{
    if(m_xAxis==nullptr){
        return QPointF();
    }
    return getNearPointByX(x);
}


qreal CurveModel::getDis(QPointF p1, QPointF p2)
{
    return qSqrt((p1.x()-p2.x())*(p1.x()-p2.x())+(p1.y()-p2.y())*(p1.y()-p2.y()));
}

int CurveModel::index() const
{
    return m_index;
}

void CurveModel::setIndex(int index)
{
    m_index = index;
    emit indexChanged();
}

int CurveModel::dataType() const
{
    return m_dataType;
}

void CurveModel::setDataType(int dataType)
{
    m_dataType = dataType;
}

LoopVector<QPointF>* CurveModel::sourcePtr()
{
    if(m_source.data()==nullptr){
        m_source = QSharedPointer<LoopVector<QPointF>>(new LoopVector<QPointF>(1000));
    }
    return m_source.data();
}

void CurveModel::getRange(QPointF &xRange, QPointF &yRange)
{
    xRange = QPointF(0,0);
    yRange = QPointF(0,0);
    if(m_source.data()==nullptr||m_source.data()->isEmpty()||m_xAxis==nullptr){
        return;
    }

    yRange.setX(m_source.data()->at(0).y());
    yRange.setY(m_source.data()->at(0).y());
    xRange.setX(m_source.data()->at(0).x());
    xRange.setY(m_source.data()->at(0).x());
    if(m_xAxis->autoRange()){
        for(int i = 0; i<m_source.data()->size(); i++){
            xRange.setX(qMin(m_source.data()->at(i).x(),xRange.x()));
            xRange.setY(qMax(m_source.data()->at(i).x(),xRange.y()));
            yRange.setX(qMin(m_source.data()->at(i).y(),yRange.x()));
            yRange.setY(qMax(m_source.data()->at(i).y(),yRange.y()));
        }
    }else{
       int i = getPointByX(m_xAxis->lower());
       i-=3 ;
       i = i<0?0:i;
       yRange.setX(m_source.data()->at(i).y());
       yRange.setY(m_source.data()->at(i).y());
       xRange.setX(m_source.data()->at(i).x());
       xRange.setY(m_source.data()->at(i).x());
       for(;i<m_source.data()->size();i++){
           xRange.setX(qMin(m_source.data()->at(i).x(),xRange.x()));
           xRange.setY(qMax(m_source.data()->at(i).x(),xRange.y()));
           yRange.setX(qMin(m_source.data()->at(i).y(),yRange.x()));
           yRange.setY(qMax(m_source.data()->at(i).y(),yRange.y()));
       }
    }
    yRange.setX(yRange.x());
    yRange.setY(yRange.y());
//    if(m_source==nullptr||m_source->isEmpty()){
//        return;
//    }
//    yRange.setX(m_source->at(0).y());
//    yRange.setY(m_source->at(0).y());
//    xRange.setX(m_source->at(0).x());
//    xRange.setY(m_source->at(0).x());
//    for(int i = 0; i<m_source->size(); i++){
//        xRange.setX(qMin(m_source->at(i).x(),xRange.x()));
//        xRange.setY(qMax(m_source->at(i).x(),xRange.y()));
//        yRange.setX(qMin(m_source->at(i).y(),yRange.x()));
//        yRange.setY(qMax(m_source->at(i).y(),yRange.y()));
//    }
}

CurveLabelModel::CurveLabelModel(CurveModel *parent):QObject(parent)
{
    if(parent==nullptr)
        return;
    m_parent = parent;
    connect(m_parent,&CurveModel::lineColorChanged,this,[=](){
        emit colorChanged();
    });
}

QString CurveLabelModel::label() const
{
    return m_label;
}

void CurveLabelModel::setLabel(const QString &label)
{
    m_label = label;
    emit labelChanged();
}

QPointF CurveLabelModel::coord()
{
    if(m_parent!=nullptr){
        return m_parent->transformCoords(m_point);
    }else {
        return QPointF(-1,-1);
    }
}

void CurveLabelModel::update()
{
    emit coordChanged();
}

void CurveLabelModel::setPoint(const QPointF &point)
{
    m_point = point;
    m_realX = point.x();
    emit pointChanged();
    emit coordChanged();
}

int CurveLabelModel::position() const
{
    return m_position;
}

void CurveLabelModel::setPosition(int position)
{
    m_position = position;
    emit positionChanged();
}

QColor CurveLabelModel::color()
{
    if(m_parent!=nullptr){
        return m_parent->lineColor();
    }
    return QColor();
}

QString CurveLabelModel::x()
{
    if(m_parent!=nullptr){
        if(m_parent->dataType()==0||m_parent->dataType()==2){
            return QString::number(m_point.x());
        }else if(m_parent->dataType()==1){
            QDateTime date = QDateTime::fromTime_t(m_point.x());
            return date.toString("yyyy/MM/dd hh:mm:ss");
        }
    }
    return QString::number(m_point.x());
}

QString CurveLabelModel::y()
{
    return QString::number(m_point.y());
}

bool CurveLabelModel::checked() const
{
    return m_checked;
}

void CurveLabelModel::setChecked(bool checked)
{
    m_checked = checked;
    emit checkedChanged();
}

CurveModel *CurveLabelModel::curve()
{
    return m_parent;
}

QPointF CurveLabelModel::point()
{
    return m_point;
}

CurveLabelListModel::CurveLabelListModel(QObject *parent):QAbstractListModel(parent)
{
}

int CurveLabelListModel::rowCount(const QModelIndex &parent) const
{
    return modelData.size();
}

QVariant CurveLabelListModel::data(const QModelIndex &index, int role) const
{
    if(index.row()<modelData.size()){
        return QVariant::fromValue<CurveLabelModel*>(modelData.at(index.row()).label);
    }
    return  QVariant();
}

void CurveLabelListModel::add(qreal scale,QPoint pos,CurveModel *curve)
{
    if(curve==nullptr){
        return;
    }
    bool flag = false;
    QPointF p = curve->addLabel(scale,pos,flag);
    if(flag){
        beginInsertRows(QModelIndex(),modelData.size(),modelData.size());
        CurveLabelModel *clm = new CurveLabelModel(curve);
        clm->setPoint(p);
        modelData.push_back({curve,clm});
        connect(curve,&CurveModel::destroyed,this,&CurveLabelListModel::removeByCurve);
        endInsertRows();
    }
}

void CurveLabelListModel::add(CurveModel *curve, qreal x)
{
    beginInsertRows(QModelIndex(),modelData.size(),modelData.size());
    CurveLabelModel *clm = new CurveLabelModel(curve);
    QPointF p = curve->addLabel(x);
    clm->setPoint(p);
    modelData.push_back({curve,clm});
//    qDebug()<<x<<curve->title()<<clm->point();
    connect(curve,&CurveModel::destroyed,this,&CurveLabelListModel::removeByCurve);
    endInsertRows();
}

void CurveLabelListModel::add(CurveModel *curve, qreal x, QString node)
{
    beginInsertRows(QModelIndex(),modelData.size(),modelData.size());
    CurveLabelModel *clm = new CurveLabelModel(curve);
    QPointF p = curve->addLabel(x);
    clm->setPoint(p);
    QString str = node+QString::number(x,'f',2);
    clm->setLabel(str);
    modelData.push_back({curve,clm});
//    qDebug()<<x<<curve->title()<<clm->point();
    connect(curve,&CurveModel::destroyed,this,&CurveLabelListModel::removeByCurve);
    endInsertRows();
}

void CurveLabelListModel::removeByCurve(QObject *obj)
{
    beginResetModel();
    CurveModel *c = (CurveModel*)obj;
    for(int i = 0; i<modelData.size(); i++){
        int pos = modelData.size()-i-1;
        if(modelData.at(pos).curve==c){
//            qDebug()<<"remove"<<pos;
            modelData.remove(pos);
        }
    }
    endResetModel();
}

void CurveLabelListModel::moveToLast(int index)
{
    if(index<0||index>=modelData.size()){
        return;
    }
    if(beginMoveRows(QModelIndex(),index,index,QModelIndex(),modelData.size())){
        CurveLabel cl = modelData.at(index);
        modelData.remove(index);
        modelData.push_back(cl);
        endMoveRows();
    }
}

void CurveLabelListModel::removeAll()
{
    beginResetModel();
    int len = modelData.size();
    for(int i = 0; i < modelData.size();i++){
        delete  modelData.at(len-i-1).label;
    }
    modelData.clear();
    endResetModel();
}

void CurveLabelListModel::removeChoose()
{
    beginResetModel();
    int len = modelData.size();
    for(int i = 0; i < modelData.size();i++){
        if(modelData.at(len-i-1).label->checked()){
            delete  modelData.at(len-i-1).label;
            modelData.remove(len-i-1);
        }
    }
    endResetModel();
}

void CurveLabelListModel::removeByIndex(int index)
{
    beginRemoveRows(QModelIndex(),index,index);
    modelData.remove(index);
    endRemoveRows();
}

void CurveLabelListModel::cancelChoose()
{
    for(int i = 0; i < modelData.size();i++){
        if(modelData.at(i).label->checked()){
            modelData.at(i).label->setChecked(false);
        }
    }
}

void CurveLabelListModel::update()
{
    foreach(CurveLabel cl, modelData){
        cl.label->update();
    }
}

QHash<int, QByteArray> CurveLabelListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(0,"label");
    return roles;
}
