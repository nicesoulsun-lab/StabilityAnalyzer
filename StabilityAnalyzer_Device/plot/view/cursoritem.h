#ifndef CURSORITEM_H
#define CURSORITEM_H

#include <QQuickPaintedItem>
#include <QPainter>

class CursorItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(bool isMainCursor READ isMainCursor WRITE setIsMainCursor NOTIFY isMainCursorChanged)
    Q_PROPERTY(int lineType READ lineType WRITE setLineType NOTIFY lineTypeChanged)
public:
    CursorItem(QQuickItem *parent = nullptr);

    void paint(QPainter *painter)override;

    QColor color() const;
    void setColor(const QColor &color);

    bool isMainCursor() const;
    void setIsMainCursor(bool isMainCursor);

    int lineType() const;
    void setLineType(int lineType);

signals:
    void colorChanged();
    void isMainCursorChanged();
    void lineTypeChanged();

public slots:

private:
    QColor m_color;
    int m_lineType = 0; //0:实线 1:虚线
    bool m_isMainCursor = false;
};

#endif // CURSORITEM_H
