#include "axismodel.h"
#include <QDebug>
AxisModel::AxisModel(QObject *parent) : QObject(parent)
{
}

AxisModel::~AxisModel()
{
}

void AxisModel::copyStyle(AxisModel *axis)
{
    m_type = axis->type();
    /* 坐标轴量程 */
    m_autoRange = axis->autoRange();   //自动量程
    m_lower = axis->lower();          //最小值
    m_upper = axis->upper();          //最大值

    /* 样式 */
    m_mainTickSum = axis->mainTickSum();      //主刻度个数
    m_subTickSum = axis->subTickSum();       //子刻度个数
    m_mainTickLen = axis->mainTickLen();    //主刻度线长度
    m_subTickLen = axis->subTickLen();     //子刻度线长度
    m_mainTickVisible = axis->mainTickVisible(); //主刻度可见
    m_subTickVisible = axis->subTickVisible();  //子刻度可见
    m_labelVisible = axis->labelVisible();    //坐标值可见
    m_axisLineVisible = axis->axisLineVisible(); //轴线可见
    m_titleVisible = axis->titleVisible();    //标题可见
    m_lineColor = axis->lineColor();     //线颜色
    m_lineWidth = axis->lineWidth();      //线宽
    m_title = axis->title();        //坐标轴标题
}

bool AxisModel::autoRange() const
{
    return m_autoRange;
}

void AxisModel::setAutoRange(bool autoRange)
{
    m_autoRange = autoRange;
    emit autoRangeChanged();
}

qreal AxisModel::lower() const
{
    return m_lower;
}

void AxisModel::setLower(const qreal &lower)
{
    if(lower>m_upper)
        return;
    m_lower = lower;
    emit lowerChanged();
    emit rangeChanged();
}

qreal AxisModel::upper() const
{
    return m_upper;
}

void AxisModel::setUpper(const qreal &upper)
{
    if(upper<m_lower)
        return;
    m_upper = upper;
    emit upperChanged();
    emit rangeChanged();
}

void AxisModel::setRange(const qreal &lower, const qreal &upper)
{
    if(lower>=upper)
        return;
    m_lower = lower;
    m_upper = upper;
    emit lowerChanged();
    emit upperChanged();
    emit rangeChanged();
    emit moveChanged();
}

void AxisModel::setRangeNoUpdate(const qreal &lower, const qreal &upper)
{
    if(lower>=upper)
        return;
    m_lower = lower;
    m_upper = upper;
    emit zoomChanged();
}

void AxisModel::setRangeNoUpdateAndSave(const qreal &lower, const qreal &upper)
{
    if(lower>=upper)
        return;
    if((m_maxRange.y()-m_maxRange.x())/(upper-lower)>500){
        return;
    }
    rangeTemp.push_back(QPointF(m_lower,m_upper));
    m_lower = lower;
    m_upper = upper;
    emit zoomChanged();
}

int AxisModel::mainTickSum() const
{
    return m_mainTickSum;
}
//网格线
void AxisModel::setMainTickSum(int mainTickSum)
{
    if(mainTickSum<=0)
        return;
    m_mainTickSum = mainTickSum;
    emit mainTickSumChanged();
    emit styleChanged();
}

int AxisModel::subTickSum() const
{
    return m_subTickSum;
}

void AxisModel::setSubTickSum(int subTickSum)
{
    if(subTickSum<=0)
        return;
    m_subTickSum = subTickSum;
    emit subTickSumChanged();
    emit styleChanged();
}

qreal AxisModel::mainTickLen() const
{
    return m_mainTickLen;
}

void AxisModel::setMainTickLen(const qreal &mainTickLen)
{
    m_mainTickLen = mainTickLen;
    emit mainTickLenChanged();
    emit styleChanged();
}

qreal AxisModel::subTickLen() const
{
    return m_subTickLen;
}

void AxisModel::setSubTickLen(const qreal &subTickLen)
{
    m_subTickLen = subTickLen;
    emit subTickLenChanged();
    emit styleChanged();
}

bool AxisModel::mainTickVisible() const
{
    return m_mainTickVisible;
}

void AxisModel::setMainTickVisible(bool mainTickVisible)
{
    m_mainTickVisible = mainTickVisible;
    emit mainTickVisibleChanged();
    emit styleChanged();
}

bool AxisModel::subTickVisible() const
{
    return m_subTickVisible;
}

void AxisModel::setSubTickVisible(bool subTickVisible)
{
    m_subTickVisible = subTickVisible;
    emit subTickVisibleChanged();
    emit styleChanged();
}

bool AxisModel::labelVisible() const
{
    return m_labelVisible;
}

void AxisModel::setLabelVisible(bool labelVisible)
{
    m_labelVisible = labelVisible;
    emit labelVisibleChanged();
    emit styleChanged();
}

void AxisModel::setLabelVisibleNoUpdate(bool labelVisible)
{
    m_labelVisible = labelVisible;
}

bool AxisModel::axisLineVisible() const
{
    return m_axisLineVisible;
}

void AxisModel::setAxisLineVisible(bool axisLineVisible)
{
    m_axisLineVisible = axisLineVisible;
    emit axisLineVisibleChanged();
    emit styleChanged();
}

QColor AxisModel::lineColor() const
{
    return m_lineColor;
}

void AxisModel::setLineColor(const QColor &lineColor)
{
    m_lineColor = lineColor;
    emit lineColorChanged();
    emit styleChanged();
}

qreal AxisModel::lineWidth() const
{
    return m_lineWidth;
}

void AxisModel::setLineWidth(const qreal &lineWidth)
{
    m_lineWidth = lineWidth;
    emit lineWidthChanged();
    emit styleChanged();
}

QColor AxisModel::fontColor() const
{
    return m_fontColor;
}

void AxisModel::setFontColor(const QColor &fontColor)
{
    m_fontColor = fontColor;
    emit fontColorChanged();
    emit styleChanged();
}

QFont AxisModel::font() const
{
    return m_font;
}

void AxisModel::setFont(const QFont &font)
{
    m_font = font;
    emit fontChanged();
    emit styleChanged();
}

qreal AxisModel::labelWidth(QPainter *painter)
{
    if(labelVisible()&&painter->isActive()){
        painter->save();
        painter->setPen(QPen(lineColor(),lineWidth()));
        painter->setFont(m_font);
        QFontMetrics fm = painter->fontMetrics();    //QFontMetrics 计算文本宽高的类

        QList<QString> list;
        Tickers ticker;
        ticker.getTickNumber(this,&list);//获取刻度文本数组
        qreal maxWidth = 0;
        /* 循环遍历得到最大文本宽度 */
        foreach(QString num,list){
            qreal tempWidth = fm.width(num);
            maxWidth = qMax(tempWidth,maxWidth);
        };
        painter->restore();
        return maxWidth;
    }
    return 0;
}

qreal AxisModel::labelHeight(QPainter *painter)
{

    if(labelVisible()&&painter->isActive()){
        painter->save();
        painter->setPen(QPen(lineColor(),lineWidth()));
        painter->setFont(m_font);
        QFontMetrics fm = painter->fontMetrics();    //QFontMetrics 计算文本宽高的类

        qreal textHeight = 0;   //文本字体高度
        textHeight = fm.height();
        painter->restore();
        return textHeight;
    }
    return 0;
}

int AxisModel::labelType() const
{
    return m_labelType;
}

void AxisModel::setLabelType(int labelType)
{
    m_labelType = labelType;
    emit labelTypeChanged();
    emit styleChanged();
}

int AxisModel::sample() const
{
    return m_sample;
}

void AxisModel::setSample(int sample)
{
    m_sample = sample;
    emit sampleChanged();
    emit styleChanged();
}

QString AxisModel::title() const
{
    return m_title;
}

void AxisModel::setTitle(const QString &title)
{
    m_title = title;
    emit titleChanged();
    emit styleChanged();
}

AxisModel::AxisType AxisModel::type() const
{
    return m_type;
}

void AxisModel::setType(const AxisType &type)
{
    m_type = type;
    emit typeChanged();
    emit styleChanged();
}

void AxisModel::setStep(const qreal &step)
{
    m_step = step;
}

qreal AxisModel::step() const
{
    return m_step;
}

qreal AxisModel::getSpacing() const
{
    return spacing;
}

void AxisModel::move(qreal offset)
{
    m_lower += offset;
    m_upper += offset;
}

int AxisModel::number() const
{
    return m_number;
}

void AxisModel::setNumber(int number)
{
    m_number = number;
    emit numberChanged();
}

void AxisModel::save()
{
    m_lowerTemp = m_lower;
    m_upperTemp = m_upper;
}

void AxisModel::recover()
{
    qreal len = (m_maxRange.y()-m_maxRange.x())*0.05;
    if(m_type==AxisType::YAxis1||m_type==AxisType::YAxis2){
        m_lower = m_maxRange.x();
        m_upper = m_maxRange.y()+len;
        if(m_lower<0){
            m_lower = m_maxRange.x()-len;
        }

    }else{
        m_lower = m_maxRange.x();
        m_upper = m_maxRange.y();
    }
    rangeTemp.clear();
    rangeTemp.push_back(QPointF(m_lower,m_upper));
    emit zoomChanged();
}

void AxisModel::goback()
{
    if(rangeTemp.isEmpty()){
        return;
    }
    QPointF range = rangeTemp.at(rangeTemp.size()-1);
    m_lower = range.x();
    m_upper = range.y();
    rangeTemp.pop_back();
    emit zoomChanged();
}

void AxisModel::record()
{
    m_lowerTemp = m_lower;
    m_upperTemp = m_upper;
}

void AxisModel::moveBySacle(qreal scale)
{
    qreal dis = m_upperTemp-m_lowerTemp;
    m_lower = m_lowerTemp+dis*scale;
    m_upper = m_upperTemp+dis*scale;
    qreal len = (m_maxRange.y()-m_maxRange.x())*0.05;
    if(m_type==AxisType::YAxis1||m_type==AxisType::YAxis2){
        if(m_upper>(m_maxRange.y()+len)){
            m_upper = m_maxRange.y()+len;
            m_lower = m_upper - dis;
        }
        if(m_maxRange.x()<0){
            if(m_lower<m_maxRange.x()-len){
                m_lower = m_maxRange.x()-len;
                m_upper = m_lower + dis;
            }
        }
        else {
            if(m_lower<m_maxRange.x()){
                m_lower = m_maxRange.x();
                m_upper = m_lower + dis;
            }
        }
    }else{
        if(m_upper>m_maxRange.y()){
            m_upper = m_maxRange.y();
            m_lower = m_upper - dis;
        }
        if(m_lower<m_maxRange.x()){
            m_lower = m_maxRange.x();
            m_upper = m_lower + dis;
        }
    }
    emit moveChanged();
}

bool AxisModel::titleVisible() const
{
    return m_titleVisible;
}

void AxisModel::setTitleVisible(bool titleVisible)
{
    m_titleVisible = titleVisible;
    emit titleVisibleChanged();
    emit styleChanged();
}

void AxisModel::setTitleVisibleNoUpdate(bool titleVisible)
{
    m_titleVisible = titleVisible;
    emit titleVisibleChanged();
}

bool AxisModel::init() const
{
    return m_init;
}

void AxisModel::setInit(bool init)
{
    m_init = init;
}

QPointF AxisModel::maxRange() const
{
    return m_maxRange;
}

void AxisModel::setMaxRange(const QPointF &maxRange)
{
    m_maxRange = maxRange;
    emit maxRangeChanged();
}

void AxisModel::setBandWidth(qreal min, qreal max)
{
    if(min<m_maxRange.x()){
        min = m_maxRange.x();
    }
    if(max>m_maxRange.y()){
        max = m_maxRange.y();
    }
    m_lower = min;
    m_upper = max;
    emit rangeChanged();
}

bool AxisModel::isX()
{
    return (m_type==AxisType::XAxis1||m_type==AxisType::XAxis2);
}

bool AxisModel::isY()
{
    return (m_type==AxisType::YAxis1||m_type==AxisType::YAxis2);
}


