#ifndef CURSORSETMODEL_H
#define CURSORSETMODEL_H

#include <QObject>
#include <QColor>
class CursorSetModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int sideSum READ sideSum WRITE setSideSum NOTIFY sideSumChanged)
    Q_PROPERTY(QColor multiCursorColor READ multiCursorColor WRITE setMultiCursorColor NOTIFY multiCursorColorChanged)
    Q_PROPERTY(QColor doubleCursorColor READ doubleCursorColor WRITE setDoubleCursorColor NOTIFY doubleCursorColorChanged)
    Q_PROPERTY(QColor singleCursorColor READ singleCursorColor WRITE setSingleCursorColor NOTIFY singleCursorColorChanged)
    Q_PROPERTY(QColor sideCursorColor READ sideCursorColor WRITE setSideCursorColor NOTIFY sideCursorColorChanged)
    Q_PROPERTY(int lineType READ lineType WRITE setLineType NOTIFY lineTypeChanged)
    Q_PROPERTY(int multiSum READ multiSum WRITE setMultiSum NOTIFY multiSumChanged)
public:
    explicit CursorSetModel(QObject *parent = 0);

    int sideSum() const;
    void setSideSum(int sideSum);

    QColor multiCursorColor() const;
    void setMultiCursorColor(const QColor &multiCursorColor);

    QColor sideCursorColor() const;
    void setSideCursorColor(const QColor &sideCursorColor);

    QColor doubleCursorColor() const;
    void setDoubleCursorColor(const QColor &doubleCursorColor);

    QColor singleCursorColor() const;
    void setSingleCursorColor(const QColor &singleCursorColor);

    int lineType() const;
    void setLineType(int lineType);

    int multiSum() const;
    void setMultiSum(int multiSum);

signals:
    void sideSumChanged();
    void multiCursorColorChanged();
    void doubleCursorColorChanged();
    void singleCursorColorChanged();
    void sideCursorColorChanged();
    void lineTypeChanged();
    void multiSumChanged();
public slots:

private:
    QColor m_singleCursorColor = QColor(170,5,5);     //单游标颜色
    QColor m_doubleCursorColor = QColor(140,15,140);     //双游标颜色
    QColor m_sideCursorColor = QColor(11,11,170);       //边频游标颜色
    QColor m_multiCursorColor = QColor(14,124,124);      //倍频游标颜色
    int m_sideSum = 4;                  //边频游标数
    int m_multiSum = 10;                  //边频游标数
    int m_lineType = 1;             //线型： 0:实线  1:虚线
};

#endif // CURSORSETMODEL_H
