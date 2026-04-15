#include "CommonData.h"
#include <QDebug>
#include <QMouseEvent>
#include <QWidget>
#include <QApplication>

CommonData::CommonData(QObject *parent) : QObject(parent)
{

}

CommonData *CommonData::instance()
{
    static CommonData*c = nullptr;
    if(c == nullptr){
        c = new CommonData;
    }
    return c;
}

void CommonData::setMainWindow(QWidget *w, QRect rect)
{
    m_mainWindow = w;
    m_mainScreenRect = rect;
    //qDebug() << "set window "<< w <<this;
}

QWidget *CommonData::mainWidow()
{
    qDebug() << "get window "<< this;
    if(m_mainWindow)
        return m_mainWindow;
    return nullptr;
}

void CommonData::moveWindow(int x, int y)
{
    if(m_mainWindow)
        m_mainWindow->move(x, y);
}

void CommonData::moveWindow(const QPoint &point)
{
    qDebug() << "window move ++ " <<point.x() <<point.y();
    if(m_mainWindow)
        m_mainWindow->move(point);
}

void CommonData::showMin()
{
    if(m_mainWindow)
        m_mainWindow->showMinimized();
}

void CommonData::showMax()
{
    if(m_mainWindow){
        if(m_mainWindow->isMaximized())
            m_mainWindow->showNormal();
        else
            m_mainWindow->showMaximized();
    }
}

void CommonData::closeApp()
{
    qApp->quit();
}

void CommonData::addListenerToWindow(QObject *obj)
{
    if(obj){
        if(m_mainWindow)
            obj->installEventFilter(m_mainWindow);
    }
}

void CommonData::addListenerToCommon(QObject *obj)
{
    if(obj)
        obj->installEventFilter(this);
}

void CommonData::BLEndianUint64(double *value)
{
    unsigned int* val = (unsigned int *)value;

    BLEndianUint32(val);
    BLEndianUint32(val+1);

    unsigned int temp = val[0];
    val[0] = val[1];
    val[1] = temp;
}

void CommonData::BLEndianUint64(uint64_t *value)
{
    unsigned int* val = (unsigned int *)value;

    BLEndianUint32(val);
    BLEndianUint32(val+1);

    unsigned int temp = val[0];
    val[0] = val[1];
    val[1] = temp;
}

void CommonData::BLEndianUint32(unsigned int *value)
{
    unsigned int val = *value;
    *value = ((val & 0x000000FF) << 24) |  ((val & 0x0000FF00) << 8) |  ((val & 0x00FF0000) >> 8) | ((val & 0xFF000000) >> 24);
}

void CommonData::BLEndianUint16(uint16_t *value)
{
    uint16_t val = *value;
    *value = ((val & 0x00FF)<<8)|((val & 0xFF00)>>8);
}

void CommonData::reversememcpy(void *des, const void *src, size_t len)
{
    /* 4个字节copy，减少循环次数 */
    size_t size = len / 4;
    size_t mod = len % 4;

    char *destemp = (char *)des + len;
    char *srctemp = (char *)src;

    while (size--)
    {
        *--destemp = *srctemp++;
        *--destemp = *srctemp++;
        *--destemp = *srctemp++;
        *--destemp = *srctemp++;
    }
    for (int i = 0; i < mod; i++) {
        *--destemp = *srctemp++;
    }
}

bool CommonData::eventFilter(QObject *watched, QEvent *event)
{
    //QEvent::MouseButtonPress QEvent::MouseMove QEvent::MouseButtonRelease
    if(event->type() ==  QEvent::MouseMove || event->type() == QEvent::MouseButtonPress
            || event->type() == QEvent::MouseButtonRelease){
    }
    return QObject::eventFilter(watched, event);
}

CommonData *getCommonData()
{
    static CommonData*c = nullptr;
    if(c == nullptr){
        c = new CommonData;
    }
    return c;
}
