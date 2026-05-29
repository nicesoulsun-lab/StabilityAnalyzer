#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <QObject>
#include <QMap>
#include <QSet>
#include <QTimer>
#include "mainwindow_global.h"

// LED指示灯控制器，根据实验状态驱动物理LED的颜色和模式
// 实验启停由Device端ExperimentCtrl直接控制，不依赖工控机runStatus
class MAINWINDOW_EXPORT LedController : public QObject
{
    Q_OBJECT

public:
    explicit LedController(QObject *parent = nullptr);
    ~LedController();

    // 实验启动：标记通道为实验中，切换LED为流水/闪烁
    Q_INVOKABLE void onExperimentStarted(int channel);
    // 实验停止：清除通道实验标记，恢复空闲LED
    Q_INVOKABLE void onExperimentStopped(int channel);
    // 应用错误状态（runStatus=3时由轮询触发）
    Q_INVOKABLE void applyChannelError(int channel, bool hasError);
    // 关闭所有LED
    Q_INVOKABLE void allOff();

private:
    // RGB颜色结构
    struct Color {
        int r = 0;
        int g = 0;
        int b = 0;
        Color() = default;
        Color(int red, int green, int blue) : r(red), g(green), b(blue) {}
        bool operator==(const Color &o) const { return r == o.r && g == o.g && b == o.b; }
        bool operator!=(const Color &o) const { return !(*this == o); }
    };

    // LED显示模式
    enum LedMode {
        Steady,  // 常亮
        Blink,   // 闪烁
        Flow     // 流水灯（单通道专用）
    };

    // LED状态：颜色 + 模式
    struct LedState {
        Color color;
        LedMode mode = Steady;
    };

    void initFromProfile();
    void setLedColor(int ledIndex, const Color &color);
    void writeChannel(int ledIndex, const QString &channel, int value);
    void updateBlinkPhase();

    // 根据通道实验/错误状态计算LED颜色和模式，并应用
    void updateChannelLeds(int channel);

    // 获取通道对应的LED索引列表
    QList<int> ledIndicesForChannel(int channel) const;

    bool m_isSingleTower = false;       // 是否单通道设备
    int m_ledCount = 4;                 // LED数量（单通道5个，四通道4个）
    QMap<int, LedState> m_states;       // 各LED当前状态
    QTimer *m_blinkTimer = nullptr;     // 闪烁/流水定时器
    bool m_blinkPhase = true;           // 闪烁相位（亮/灭）
    int m_flowIndex = 0;                // 流水灯当前帧索引

    QSet<int> m_runningChannels;        // 当前实验中的通道集合
    QSet<int> m_errorChannels;          // 当前错误状态的通道集合
    QMap<int, qint64> m_errorSinceMs;   // 错误消抖：通道首次检测到错误的时间戳（0=未追踪）

    static constexpr int kBlinkIntervalMs = 800;   // 闪烁周期（ms）
    static constexpr int kFlowIntervalMs = 400;    // 流水灯帧间隔（ms）
    static constexpr int kFlowTrailLength = 2;     // 流水灯拖尾长度（含头灯）
    static constexpr int kFlowBgValue = 192;       // 流水灯背景灰色值
    static constexpr int kBlinkOffValue = 30;      // 闪烁灭态暗灰值
    static constexpr int kErrorDebounceMs = 15000; // 错误进入消抖：必须持续15秒以上
    static const QString kSysfsBase;               // sysfs LED路径前缀
};

#endif
