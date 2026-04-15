#include "spectrumview.h"

SpectrumView::SpectrumView(QQuickItem *parent) : QQuickPaintedItem(parent)
{
    scale = 640.0/480.0;
}

void SpectrumView::paint(QPainter *painter)
{
    QRect rect;//640, 480
    qreal w = this->width();
    qreal h = this->height();
    if(w==0||h==0){
        return;
    }
    if(w/h>scale){
        rect.setX((this->width()-h*scale)/2);
        rect.setY((this->height()-h)/2);
        rect.setWidth(h*scale);
        rect.setHeight(h);
    }else{
        rect.setX((this->width()-w)/2);
        rect.setY((this->height()-w/scale)/2);
        rect.setWidth(w);
        rect.setHeight(w/scale);
    }
    QImage image = jsonToImageByte(m_param);

    if(!image.isNull()){
        painter->setRenderHint(QPainter::SmoothPixmapTransform,true);
        painter->drawImage(rect,image);
        QFont font;
        font.setPixelSize(20);
        painter->setFont(font);
        painter->drawText(QRect(0,0,w,20),Qt::AlignHCenter|Qt::AlignBottom,m_param.find("label").value().toString());
    }
}

QImage SpectrumView::jsonToImageByte(QJsonObject &json)
{
    QImage image = QImage(640, 480, QImage::Format_RGB32);
    QJsonArray array = json.find("pic").value().toArray();
    for(int y = 0; y < array.size(); y++){
        QJsonArray array2 = array.at(y).toArray();
        for(int x = 0 ; x < array2.size(); x++){
            QJsonArray array3 = array2.at(x).toArray();
            if(array3.size()>=3){
                image.setPixel(x,y,qRgb(array3[0].toInt(),array3[1].toInt(),array3[2].toInt()));
            }
        }
    }
    return image;
}

QJsonObject SpectrumView::param() const
{
    return m_param;
}

void SpectrumView::setParam(const QJsonObject &param)
{
    m_param = param;
    update();
    qDebug()<<param;
    emit paramChanged();
}
