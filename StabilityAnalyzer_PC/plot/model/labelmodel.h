#ifndef LABELMODEL_H
#define LABELMODEL_H

#include <QObject>
#include <QPointF>

class LabelModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString content READ content WRITE setContent NOTIFY contentChanged)
    Q_PROPERTY(QPointF pos READ screenPos NOTIFY posChanged)
    Q_PROPERTY(bool choosed READ choosed WRITE setChoosed NOTIFY choosedChanged)
public:
    explicit LabelModel(QObject *parent = 0);

    QString content() const;
    void setContent(const QString &content);

    QPointF pos() const;
    void setPos(const QPointF &pos);

    void setScreenPos(const QPointF &screenPos);

    QPointF screenPos() const;


    bool choosed() const;
    void setChoosed(bool choosed);

signals:
    void posChanged();
    void contentChanged();
    void choosedChanged();

public slots:

private:
    QPointF m_pos;      //位置
    QPointF m_screenPos;      //屏幕位置
    QString m_content;  //内容
    bool m_choosed = false;
};

#endif // LABELMODEL_H
