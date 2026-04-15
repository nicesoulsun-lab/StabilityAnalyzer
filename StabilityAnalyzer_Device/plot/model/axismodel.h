#ifndef AXISMODEL_H
#define AXISMODEL_H

#include <QObject>
#include <QFont>
#include <QColor>
#include <QVector>
#include <QPainter>
#include "../drawer/axisdrawer.h"
#include "../drawer/tickers.h"

class AxisModel : public QObject
{
    Q_OBJECT
    Q_ENUMS(AxisType)
    Q_PROPERTY(AxisType type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(qreal lower READ lower WRITE setLower NOTIFY lowerChanged)
    Q_PROPERTY(qreal lineWidth READ lower WRITE setLineWidth NOTIFY lineWidthChanged)
    Q_PROPERTY(qreal upper READ upper WRITE setUpper NOTIFY upperChanged)
    Q_PROPERTY(qreal mainTickLen READ mainTickLen WRITE setMainTickLen NOTIFY mainTickLenChanged)
    Q_PROPERTY(qreal subTickLen READ subTickLen WRITE setSubTickLen NOTIFY subTickLenChanged)
    Q_PROPERTY(int mainTickSum READ mainTickSum WRITE setMainTickSum NOTIFY mainTickSumChanged)
    Q_PROPERTY(int subTickSum READ subTickSum WRITE setSubTickSum NOTIFY subTickSumChanged)
    Q_PROPERTY(bool mainTickVisible READ mainTickVisible WRITE setMainTickVisible NOTIFY mainTickVisibleChanged)
    Q_PROPERTY(bool subTickVisible READ subTickVisible WRITE setSubTickVisible NOTIFY subTickVisibleChanged)
    Q_PROPERTY(bool labelVisible READ labelVisible WRITE setLabelVisible NOTIFY labelVisibleChanged)
    Q_PROPERTY(QColor lineColor READ lineColor WRITE setLineColor NOTIFY lineColorChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(QColor fontColor READ fontColor WRITE setFontColor NOTIFY fontColorChanged)
    Q_PROPERTY(bool axisLineVisible READ axisLineVisible WRITE setAxisLineVisible NOTIFY axisLineVisibleChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
     Q_PROPERTY(bool autoRange READ autoRange WRITE setAutoRange NOTIFY autoRangeChanged)
    Q_PROPERTY(int labelType READ labelType WRITE setLabelType NOTIFY labelTypeChanged)
    Q_PROPERTY(qreal sample READ sample WRITE setSample NOTIFY sampleChanged)
    Q_PROPERTY(int number READ number WRITE setNumber NOTIFY numberChanged)
    Q_PROPERTY(bool titleVisible READ titleVisible WRITE setTitleVisible NOTIFY titleVisibleChanged)
    Q_PROPERTY(QPointF maxRange READ maxRange WRITE setMaxRange NOTIFY maxRangeChanged)
public:
    enum class AxisType{  //坐标轴位置
        XAxis1,     //下
        XAxis2,     //上
        YAxis1,     //左
        YAxis2,     //右
    };

    explicit AxisModel(QObject *parent = nullptr);
    ~AxisModel();

    void copyStyle(AxisModel* axis);

    bool autoRange() const;
    void setAutoRange(bool autoRange);

    bool initLower() const;
    void setInitLower(bool initLower);

    bool initUpper() const;
    void setInitUpper(bool initUpper);

    qreal lower() const;
    void setLower(const qreal &lower);

    qreal upper() const;
    void setUpper(const qreal &upper);

    void setRange(const qreal &lower,const qreal &upper);

    void setRangeNoUpdate(const qreal &lower,const qreal &upper);

    void setRangeNoUpdateAndSave(const qreal &lower,const qreal &upper);

    int mainTickSum() const;
    void setMainTickSum(int mainTickSum);

    int subTickSum() const;
    void setSubTickSum(int subTickSum);

    qreal mainTickLen() const;
    void setMainTickLen(const qreal &mainTickLen);

    qreal subTickLen() const;
    void setSubTickLen(const qreal &subTickLen);

    bool mainTickVisible() const;
    void setMainTickVisible(bool mainTickVisible);

    bool subTickVisible() const;
    void setSubTickVisible(bool subTickVisible);

    bool labelVisible() const;
    void setLabelVisible(bool labelVisible);
    void setLabelVisibleNoUpdate(bool labelVisible);

    bool axisLineVisible() const;
    void setAxisLineVisible(bool axisLineVisible);

    QColor lineColor() const;
    void setLineColor(const QColor &lineColor);

    qreal lineWidth() const;
    void setLineWidth(const qreal &lineWidth);

    QColor fontColor() const;
    void setFontColor(const QColor &fontColor);

    QFont font() const;
    void setFont(const QFont &font);

    qreal labelWidth(QPainter *painter);

    qreal labelHeight(QPainter *painter);

    int labelType() const;
    void setLabelType(int labelType);

    int sample() const;
    void setSample(int sample);

    QString title() const;
    void setTitle(const QString &title);

    AxisType type() const;
    void setType(const AxisType &type);

    void setStep(const qreal &step);

    qreal step() const;

    qreal getSpacing() const;

    void move(qreal offset);

    int number() const;
    void setNumber(int number);

    void save();
    void recover();
    void goback();

    void record();  //移动后记录下新的坐标轴位置
    void moveBySacle(qreal scale);

    bool titleVisible() const;
    void setTitleVisible(bool titleVisible);
    void setTitleVisibleNoUpdate(bool titleVisible);

    bool init() const;
    void setInit(bool init);

    QPointF maxRange() const;
    void setMaxRange(const QPointF &maxRange);


    void setBandWidth(qreal min,qreal max);

    bool isX();
    bool isY();
signals:
    void rangeChanged();
    void styleChanged();
    void typeChanged();
    void lowerChanged();
    void upperChanged();
    void scaleChanged();
    void mainTickSumChanged();
    void subTickSumChanged();
    void mainTickVisibleChanged();
    void subTickVisibleChanged();
    void labelVisibleChanged();
    void axisRectChanged();
    void subTickLenChanged();
    void mainTickLenChanged();
    void spaceBetweenTLChanged();
    void lineColorChanged();
    void fontChanged();
    void fontColorChanged();
    void axisLineVisibleChanged();
    void lineWidthChanged();
    void cursorChanged();
    void titleChanged();
    void autoRangeChanged();
    void labelTypeChanged();
    void sampleChanged();
    void numberChanged();
    void titleVisibleChanged();
    void maxRangeChanged();
    void moveChanged();
    void zoomChanged();

private:
    AxisType m_type = AxisType::XAxis1;        //坐标轴类型
    bool m_init = false;
    /* 坐标轴量程 */
    bool m_autoRange = true;   //自动量程
    qreal m_lower = 0;          //最小值
    qreal m_upper = 10;          //最大值
    qreal m_lowerTemp = 0;       //最小值缓存
    qreal m_upperTemp = 10;      //最大值缓存
    qreal m_step = 0;

    /* 样式 */
    int m_mainTickSum = 5;      //主刻度个数
    int m_subTickSum = 10;       //子刻度个数
    qreal m_mainTickLen = 6;    //主刻度线长度
    qreal m_subTickLen = 4;     //子刻度线长度
    bool m_mainTickVisible = true; //主刻度可见
    bool m_subTickVisible = true;  //子刻度可见
    bool m_labelVisible = true;    //坐标值可见
    bool m_axisLineVisible = true; //轴线可见
    bool m_titleVisible = true;    //标题可见
    qreal spacing = 5;      //文字和刻度线之间的间隙
    QColor m_lineColor = QColor(120,139,206);     //线颜色
    qreal m_lineWidth = 1;      //线宽
    QString m_title = "";        //坐标轴标题

    /* 标签配置 */
    QColor m_fontColor = QColor(120,139,206);     //字体颜色
    QFont m_font;           //字体样式
    int m_labelType = 0;        //标签类型: 0:数字 1:时间 2:时间戳
    int m_sample = 1000;           //采样频率,单位ms

    /* 属性 */
    qreal m_length;         //长度
    int m_number = 0;           //唯一性标志

    QPointF m_maxRange = QPointF(0,10);     //坐标轴的最大量程范围，x:最小值 y:最大值
    QVector<QPointF> rangeTemp;     //存储进行缩放操作后的坐标轴范围
};

#endif // AXISMODEL_H
