#include "Controller/led_controller.h"
#include "deviceprofile.h"
#include <QDebug>
#include <QFile>

const QString LedController::kSysfsBase = QStringLiteral("/sys/class/leds");
constexpr int LedController::kErrorEnterThreshold;

LedController::LedController(QObject *parent)
    : QObject(parent)
    , m_blinkTimer(new QTimer(this))
{
    initFromProfile();

    // 初始化所有LED为空闲绿色常亮
    for (int i = 0; i < m_ledCount; ++i) {
        m_states[i] = {Color(0, 255, 0), Steady};
        setLedColor(i, Color(0, 255, 0));
    }

    m_blinkTimer->setInterval(kBlinkIntervalMs);
    connect(m_blinkTimer, &QTimer::timeout, this, &LedController::updateBlinkPhase);
    m_blinkTimer->start();
}

LedController::~LedController()
{
    allOff();
}

// 根据设备配置初始化LED数量和设备类型
void LedController::initFromProfile()
{
    const DeviceProfile &profile = deviceProfile();
    m_isSingleTower = (profile.channelCount <= 1);
    m_ledCount = m_isSingleTower ? 5 : profile.channelCount;
}

// 单通道设备：所有LED属于同一通道；四通道设备：每个LED对应一个通道
QList<int> LedController::ledIndicesForChannel(int channel) const
{
    if (m_isSingleTower) {
        QList<int> indices;
        for (int i = 0; i < m_ledCount; ++i)
            indices.append(i);
        return indices;
    }
    return QList<int>{qBound(0, channel, m_ledCount - 1)};
}

// 实验启动：标记通道为实验中，更新LED
void LedController::onExperimentStarted(int channel)
{
    m_runningChannels.insert(channel);
    m_errorChannels.remove(channel);
    m_errorDebounce.remove(channel);
    updateChannelLeds(channel);
}

// 实验停止：清除通道实验标记，恢复空闲LED
void LedController::onExperimentStopped(int channel)
{
    m_runningChannels.remove(channel);
    m_errorChannels.remove(channel);
    m_errorDebounce.remove(channel);
    updateChannelLeds(channel);
}

// 应用错误状态（由轮询检测到runStatus=3时调用）
// 非对称消抖：进入错误需连续kErrorEnterThreshold次（容忍MCU瞬态干扰），退出错误即时响应
void LedController::applyChannelError(int channel, bool hasError)
{
    if (m_runningChannels.contains(channel))
        return;

    int &counter = m_errorDebounce[channel];
    if (hasError) {
        counter = qMin(counter + 1, kErrorEnterThreshold);
        if (counter < kErrorEnterThreshold || m_errorChannels.contains(channel))
            return;
        m_errorChannels.insert(channel);
    } else {
        counter = 0;
        if (!m_errorChannels.contains(channel))
            return;
        m_errorChannels.remove(channel);
    }
    updateChannelLeds(channel);
}

// 根据通道的实验/错误状态计算LED颜色和模式，并应用到对应LED
// 优先级：实验中 > 错误 > 空闲
void LedController::updateChannelLeds(int channel)
{
    Color color;
    LedMode mode;

    if (m_runningChannels.contains(channel)) {
        // 实验中：单通道橙色流水，四通道橙色闪烁
        color = Color(255, 165, 0);
        mode = m_isSingleTower ? Flow : Blink;
    } else if (m_errorChannels.contains(channel)) {
        // 错误：红色闪烁
        color = Color(255, 0, 0);
        mode = Blink;
    } else {
        // 空闲：绿色常亮
        color = Color(0, 255, 0);
        mode = Steady;
    }

    const QList<int> indices = ledIndicesForChannel(channel);
    for (int led : indices) {
        if (m_states.value(led).color != color || m_states.value(led).mode != mode) {
            m_states[led] = {color, mode};
            if (mode == Steady) {
                setLedColor(led, color);
            }
        }
    }
}

// 关闭所有LED并清空状态
void LedController::allOff()
{
    for (int i = 0; i < m_ledCount; ++i) {
        setLedColor(i, Color(0, 0, 0));
        m_states.remove(i);
    }
}

// 设置单个LED的RGB颜色
// 按B→G→R顺序写入，蓝色先写（人眼最不敏感），红色最后写（最敏感）
// 避免sysfs逐通道写入时产生红/绿/青等突兀的中间色
void LedController::setLedColor(int ledIndex, const Color &color)
{
    writeChannel(ledIndex, QStringLiteral("b"), color.b);
    writeChannel(ledIndex, QStringLiteral("g"), color.g);
    writeChannel(ledIndex, QStringLiteral("r"), color.r);
}

// 写入sysfs LED亮度文件
void LedController::writeChannel(int ledIndex, const QString &channel, int value)
{
    const QString path = QStringLiteral("%1/sunxi_led%2%3/brightness")
                             .arg(kSysfsBase)
                             .arg(ledIndex)
                             .arg(channel);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Unbuffered)) {
        qWarning() << "[LedController] failed to open" << path << file.errorString();
        return;
    }
    file.write(QByteArray::number(qBound(0, value, 255)));
    file.close();
}

// 定时器回调：驱动闪烁和流水灯效果
// - 常亮：已写入正确颜色，不做重复刷新
// - 闪烁：按相位亮灭切换
// - 流水：逐帧推进，头灯最亮，拖尾灯渐暗，背景灯灰色
void LedController::updateBlinkPhase()
{
    bool hasFlow = false;
    bool hasBlink = false;

    for (auto it = m_states.constBegin(); it != m_states.constEnd(); ++it) {
        if (it.value().mode == Flow)
            hasFlow = true;
        if (it.value().mode == Blink)
            hasBlink = true;
    }

    // 流水灯优先使用更短的帧间隔
    m_blinkTimer->setInterval(hasFlow ? kFlowIntervalMs : kBlinkIntervalMs);

    // 推进流水帧索引
    if (hasFlow) {
        m_flowIndex = (m_flowIndex + 1) % m_ledCount;
    }

    // 翻转闪烁相位
    m_blinkPhase = !m_blinkPhase;

    for (auto it = m_states.begin(); it != m_states.end(); ++it) {
        const int led = it.key();
        const LedState &state = it.value();

        switch (state.mode) {
        case Steady:
            break;
        case Blink:
            setLedColor(led, m_blinkPhase ? state.color : Color(kBlinkOffValue, kBlinkOffValue, kBlinkOffValue));
            break;
        case Flow: {
            // 判断当前LED是否在流水亮区
            bool lit = false;
            for (int t = 0; t < kFlowTrailLength; ++t) {
                int activeIdx = (m_flowIndex - t + m_ledCount) % m_ledCount;
                if (led == activeIdx) {
                    lit = true;
                    break;
                }
            }
            if (lit) {
                // 头灯满亮度，拖尾灯按距离渐暗
                int dist = (m_flowIndex - led + m_ledCount) % m_ledCount;
                float brightness = 1.0f - dist * (0.5f / kFlowTrailLength);
                setLedColor(led, Color(int(state.color.r * brightness),
                                       int(state.color.g * brightness),
                                       int(state.color.b * brightness)));
            } else {
                // 背景灯灰色
                setLedColor(led, Color(kFlowBgValue, kFlowBgValue, kFlowBgValue));
            }
            break;
        }
        }
    }
}
