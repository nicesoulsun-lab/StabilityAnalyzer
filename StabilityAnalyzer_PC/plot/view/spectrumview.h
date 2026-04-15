#ifndef SPECTRUMVIEW_H
#define SPECTRUMVIEW_H

#include <QQuickPaintedItem>
#include <QJsonObject>
#include <QJsonArray>
#include <QPainter>
#include "model/curvelistmodel.h"

class SpectrumView : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QJsonObject param READ param WRITE setParam NOTIFY paramChanged)
public:
    explicit SpectrumView(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;

    QImage jsonToImageByte(QJsonObject &json);

    QJsonObject param() const;
    void setParam(const QJsonObject &param);

signals:
    void paramChanged();
private:
    qreal scale;
    QJsonObject m_param;
};

#endif // SPECTRUMVIEW_H
