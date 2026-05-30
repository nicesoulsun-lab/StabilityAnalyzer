#include "Controller/led_controller.h"
#include "deviceprofile.h"
#include <QDebug>
#include <QFile>

const QString LedController::kSysfsBase = QStringLiteral("/sys/class/leds");
constexpr int LedController::kBlinkIntervalMs;
constexpr int LedController::kFlowIntervalMs;
constexpr int LedController::kFlowTrailLength;
constexpr int LedController::kFlowBgValue;
constexpr int LedController::kBlinkOffValue;

LedController::LedController(QObject *parent)
    : QObject(parent)
    , m_blinkTimer(new QTimer(this))
{
    initFromProfile();

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

void LedController::initFromProfile()
{
    const DeviceProfile &profile = deviceProfile();
    m_isSingleTower = (profile.channelCount <= 1);
    m_ledCount = m_isSingleTower ? 5 : profile.channelCount;
}

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

void LedController::onExperimentStarted(int channel)
{
    m_runningChannels.insert(channel);
    updateChannelLeds(channel);
}

void LedController::onExperimentStopped(int channel)
{
    m_runningChannels.remove(channel);
    updateChannelLeds(channel);
}

void LedController::updateChannelLeds(int channel)
{
    const bool running = m_runningChannels.contains(channel);
    const Color color = running ? Color(255, 165, 0) : Color(0, 255, 0);
    const LedMode mode = running ? (m_isSingleTower ? Flow : Blink) : Steady;

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

void LedController::allOff()
{
    for (int i = 0; i < m_ledCount; ++i) {
        setLedColor(i, Color(0, 0, 0));
        m_states.remove(i);
    }
}

void LedController::setLedColor(int ledIndex, const Color &color)
{
    writeChannel(ledIndex, QStringLiteral("b"), color.b);
    writeChannel(ledIndex, QStringLiteral("g"), color.g);
    writeChannel(ledIndex, QStringLiteral("r"), color.r);
}

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

void LedController::updateBlinkPhase()
{
    bool hasFlow = false;
    for (auto it = m_states.constBegin(); it != m_states.constEnd(); ++it) {
        if (it.value().mode == Flow) {
            hasFlow = true;
            break;
        }
    }

    m_blinkTimer->setInterval(hasFlow ? kFlowIntervalMs : kBlinkIntervalMs);

    if (hasFlow) {
        m_flowIndex = (m_flowIndex + 1) % m_ledCount;
    }

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
            bool lit = false;
            for (int t = 0; t < kFlowTrailLength; ++t) {
                int activeIdx = (m_flowIndex - t + m_ledCount) % m_ledCount;
                if (led == activeIdx) {
                    lit = true;
                    break;
                }
            }
            if (lit) {
                int dist = (m_flowIndex - led + m_ledCount) % m_ledCount;
                float brightness = 1.0f - dist * (0.5f / kFlowTrailLength);
                setLedColor(led, Color(int(state.color.r * brightness),
                                       int(state.color.g * brightness),
                                       int(state.color.b * brightness)));
            } else {
                setLedColor(led, Color(kFlowBgValue, kFlowBgValue, kFlowBgValue));
            }
            break;
        }
        }
    }
}
