#ifndef COMMONDATA_H
#define COMMONDATA_H

#include <QObject>
#include <QtWidgets/QWidget>
#include <QRect>
#include <QPoint>
#include <QEvent>
#include "commondata_global.h"

class COMMONDATA_EXPORT CommonData : public QObject
{
    Q_OBJECT
public:
    explicit CommonData(QObject *parent = nullptr);
    static CommonData* instance();
    /**
     * @brief setMainWindow 设置主窗口信息
     * @param w
     * @param rect
     */
    void setMainWindow(QWidget* w, QRect rect);
    /**
     * @brief mainWidow 获取主窗口
     * @return
     */
    QWidget* mainWidow();
    /**
     * @brief moveWindow 移动主窗口到指定位置
     * @param x
     * @param y
     */
    Q_INVOKABLE void moveWindow(int x, int y);
    Q_INVOKABLE void moveWindow(const QPoint& point);
    Q_INVOKABLE void showMin();
    Q_INVOKABLE void showMax();
    Q_INVOKABLE void closeApp();
    /**
     * 添加监听事件 由app主窗口过滤事件
     */
    Q_INVOKABLE void addListenerToWindow(QObject* obj);
    /**
     * 添加监听事件 由CommonData实例主窗口过滤事件
     */
    Q_INVOKABLE void addListenerToCommon(QObject* obj);

    /* 大小端转换 */
    static void BLEndianUint64(double *value);
    static void BLEndianUint64(uint64_t *value);
    static void BLEndianUint32(unsigned int *value);
    static void BLEndianUint16(uint16_t *value);

    /* 逆序memcpy */
    static void reversememcpy(void * des, const void * src, size_t len);

protected:
    bool eventFilter(QObject *watched, QEvent *event);
signals:

public slots:
private:
    QWidget* m_mainWindow = nullptr;
    // 主屏幕分辨率
    QRect m_mainScreenRect;
    /**
     * @brief m_widgetScale 屏幕缩放比例
    */
    float m_widgetScale;
};
COMMONDATA_EXPORT CommonData* getCommonData();
#define  COMMONDATA getCommonData()
//#define COMMONDATA CommonData::instance()
#endif // COMMONDATA_H
