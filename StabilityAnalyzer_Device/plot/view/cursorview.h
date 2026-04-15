#ifndef CURSORVIEW_H
#define CURSORVIEW_H

#include <QObject>
#include <QPainter>
class CursorView : public QObject
{
    Q_OBJECT
public:
    explicit CursorView(QObject *parent = nullptr);

    void init();

    qreal height() const;
    void setHeight(const qreal &height);

    QColor color() const;
    void setColor(const QColor &color);

    void paintCursor(QPixmap &pix,int type);

signals:

private:
    qreal m_width = 10;
    qreal m_height = 10;
    QColor m_color;
    int m_lineType = 0; //0:实线 1:虚线
    bool m_isMainCursor = false;

    QPixmap m_cursor1;  //普通
    QPixmap m_cursor2;  //普通 虚线
    QPixmap m_cursor3;  //主游标
    QPixmap m_cursor4;  //主游标 虚线
    QPixmap m_chooseRect;   //选中矩形
};

#endif // CURSORVIEW_H
