#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <QObject>
#include <QMap>
#include <QSet>
#include <QTimer>
#include "mainwindow_global.h"

// LED指示灯控制器
// 实验中：单塔橙色流水灯，多塔橙色闪烁；空闲：绿色常亮
class MAINWINDOW_EXPORT LedController : public QObject
{
    Q_OBJECT

public:
    explicit LedController(QObject *parent = nullptr);
    ~LedController();

    Q_INVOKABLE void onExperimentStarted(int channel);
    Q_INVOKABLE void onExperimentStopped(int channel);
    Q_INVOKABLE void allOff();

private:
    struct Color {
        int r = 0;
        int g = 0;
        int b = 0;
        Color() = default;
        Color(int red, int green, int blue) : r(red), g(green), b(blue) {}
        bool operator==(const Color &o) const { return r == o.r && g == o.g && b == o.b; }
        bool operator!=(const Color &o) const { return !(*this == o); }
    };

    enum LedMode {
        Steady,
        Blink,
        Flow
    };

    struct LedState {
        Color color;
        LedMode mode = Steady;
    };

    void initFromProfile();
    void setLedColor(int ledIndex, const Color &color);
    void writeChannel(int ledIndex, const QString &channel, int value);
    void updateBlinkPhase();
    void updateChannelLeds(int channel);
    QList<int> ledIndicesForChannel(int channel) const;

    bool m_isSingleTower = false;
    int m_ledCount = 4;
    QMap<int, LedState> m_states;
    QTimer *m_blinkTimer = nullptr;
    bool m_blinkPhase = true;
    int m_flowIndex = 0;

    QSet<int> m_runningChannels;

    static constexpr int kBlinkIntervalMs = 800;
    static constexpr int kFlowIntervalMs = 400;
    static constexpr int kFlowTrailLength = 2;
    static constexpr int kFlowBgValue = 192;
    static constexpr int kBlinkOffValue = 30;
    static const QString kSysfsBase;
};

#endif
