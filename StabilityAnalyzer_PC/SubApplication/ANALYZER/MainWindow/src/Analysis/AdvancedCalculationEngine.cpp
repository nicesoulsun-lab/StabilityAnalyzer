#include "Analysis/AdvancedCalculationEngine.h"

#include <QtMath>
#include <QString>

namespace {

constexpr double kGravityAcceleration = 9.80665;
constexpr double kRichardsonZakiN = 4.65;
constexpr double kMillimeterPerHourToMeterPerSecond = 1.0 / 3600000.0;
constexpr double kMicrometerToMeter = 1.0e-6;
constexpr double kGramPerCubicCentimeterToKilogramPerCubicMeter = 1000.0;
constexpr double kCentipoiseToPascalSecond = 0.001;

// 后端入参都通过 QVariantMap 透传，这里统一做安全数值转换。
double toNumber(const QVariant& value, bool* ok = nullptr)
{
    bool localOk = false;
    const double result = value.toDouble(&localOk);
    if (ok) {
        *ok = localOk;
    }
    return localOk ? result : 0.0;
}

QVariantMap makeErrorResult(const QString& message,
                            const QString& targetKey = QString(),
                            const QString& unit = QString())
{
    QVariantMap result;
    result.insert("success", false);
    result.insert("targetKey", targetKey);
    result.insert("value", 0.0);
    result.insert("displayText", QString());
    result.insert("unit", unit);
    result.insert("message", message);
    return result;
}

QVariantMap makeSuccessResult(const QString& targetKey, double value, const QString& unit, int digits = 2)
{
    QVariantMap result;
    result.insert("success", true);
    result.insert("targetKey", targetKey);
    result.insert("value", value);
    result.insert("displayText", QString::number(value, 'f', digits));
    result.insert("unit", unit);
    result.insert("message", QString());
    return result;
}

// 高级计算页目前所有可编辑输入都要求为正值，这里集中复用校验逻辑。
bool requirePositive(const QVariantMap& params, const QString& key, double* outValue)
{
    bool ok = false;
    const double value = toNumber(params.value(key), &ok);
    if (!ok || value <= 0.0) {
        return false;
    }
    if (outValue) {
        *outValue = value;
    }
    return true;
}

// QML 传入的是 startDay/startHour... 这种扁平字段，后端在这里组装成小时数。
double totalHoursFromParams(const QVariantMap& params, const QString& prefix, bool* ok)
{
    bool dayOk = false;
    bool hourOk = false;
    bool minuteOk = false;
    bool secondOk = false;
    const double dayValue = toNumber(params.value(prefix + QStringLiteral("Day")), &dayOk);
    const double hourValue = toNumber(params.value(prefix + QStringLiteral("Hour")), &hourOk);
    const double minuteValue = toNumber(params.value(prefix + QStringLiteral("Minute")), &minuteOk);
    const double secondValue = toNumber(params.value(prefix + QStringLiteral("Second")), &secondOk);
    if (ok) {
        *ok = dayOk && hourOk && minuteOk && secondOk;
    }
    return dayValue * 24.0 + hourValue + minuteValue / 60.0 + secondValue / 3600.0;
}

QString fluidUnitForTarget(const QString& targetKey)
{
    if (targetKey == QStringLiteral("diameter")) {
        return QStringLiteral("μm");
    }
    if (targetKey == QStringLiteral("concentration")) {
        return QStringLiteral("%");
    }
    if (targetKey == QStringLiteral("continuousViscosity")) {
        return QStringLiteral("cP");
    }
    return QStringLiteral("g/cm³");
}

QString opticalUnitForTarget(const QString& targetKey)
{
    return targetKey == QStringLiteral("diameter") ? QStringLiteral("μm") : QStringLiteral("%");
}

int totalMillisecondsFromParams(const QVariantMap& params, const QString& prefix, bool* ok)
{
    bool dayOk = false;
    bool hourOk = false;
    bool minuteOk = false;
    bool secondOk = false;
    const int dayValue = params.value(prefix + QStringLiteral("Day")).toInt(&dayOk);
    const int hourValue = params.value(prefix + QStringLiteral("Hour")).toInt(&hourOk);
    const int minuteValue = params.value(prefix + QStringLiteral("Minute")).toInt(&minuteOk);
    const int secondValue = params.value(prefix + QStringLiteral("Second")).toInt(&secondOk);
    if (ok) {
        *ok = dayOk && hourOk && minuteOk && secondOk;
    }
    return (((dayValue * 24) + hourValue) * 60 + minuteValue) * 60 * 1000 + secondValue * 1000;
}

QVariantMap nearestRowByElapsedMs(const QVector<QVariantMap>& rows, int targetElapsedMs)
{
    if (rows.isEmpty()) {
        return QVariantMap();
    }

    QVariantMap nearest = rows.first();
    int nearestDistance = qAbs(rows.first().value(QStringLiteral("scan_elapsed_ms")).toInt() - targetElapsedMs);
    for (const QVariantMap& row : rows) {
        const int currentDistance = qAbs(row.value(QStringLiteral("scan_elapsed_ms")).toInt() - targetElapsedMs);
        if (currentDistance < nearestDistance) {
            nearest = row;
            nearestDistance = currentDistance;
        }
    }
    return nearest;
}

}

QVariantMap AdvancedCalculationEngine::calculateMigrationRate(const QVariantMap& params)
{
    Q_UNUSED(params)
    return makeErrorResult(QStringLiteral("缺少分层结果，无法计算颗粒迁移速率"),
                           QStringLiteral("migrationRate"),
                           QStringLiteral("mm/h"));
}

QVariantMap AdvancedCalculationEngine::calculateMigrationRate(const QVariantMap& params,
                                                              const QVector<QVariantMap>& separationRows)
{
    bool startOk = false;
    bool endOk = false;
    const int startElapsedMs = totalMillisecondsFromParams(params, QStringLiteral("start"), &startOk);
    const int endElapsedMs = totalMillisecondsFromParams(params, QStringLiteral("end"), &endOk);
    if (!startOk || !endOk) {
        return makeErrorResult(QStringLiteral("起止时间参数无效"), QStringLiteral("migrationRate"), QStringLiteral("mm/h"));
    }
    if (separationRows.size() < 2) {
        return makeErrorResult(QStringLiteral("分层结果不足，无法计算颗粒迁移速率"), QStringLiteral("migrationRate"), QStringLiteral("mm/h"));
    }

    const QVariantMap startRow = nearestRowByElapsedMs(separationRows, startElapsedMs);
    const QVariantMap endRow = nearestRowByElapsedMs(separationRows, endElapsedMs);
    if (startRow.isEmpty() || endRow.isEmpty()) {
        return makeErrorResult(QStringLiteral("未找到匹配的分层结果"), QStringLiteral("migrationRate"), QStringLiteral("mm/h"));
    }

    const int actualStartElapsedMs = startRow.value(QStringLiteral("scan_elapsed_ms")).toInt();
    const int actualEndElapsedMs = endRow.value(QStringLiteral("scan_elapsed_ms")).toInt();
    const int deltaElapsedMs = qAbs(actualEndElapsedMs - actualStartElapsedMs);
    if (deltaElapsedMs <= 0) {
        return makeErrorResult(QStringLiteral("起止时间落在同一帧，无法计算迁移速率"), QStringLiteral("migrationRate"), QStringLiteral("mm/h"));
    }

    // 第一版默认用沉降界面位移估算颗粒迁移速率，后续如有需要可开放切换澄清界面。
    const double startBoundaryMm = toNumber(startRow.value(QStringLiteral("sediment_boundary_mm")));
    const double endBoundaryMm = toNumber(endRow.value(QStringLiteral("sediment_boundary_mm")));
    const double deltaBoundaryMm = qAbs(endBoundaryMm - startBoundaryMm);
    const double deltaHours = static_cast<double>(deltaElapsedMs) / 3600000.0;
    const double migrationRate = deltaBoundaryMm / qMax(1e-9, deltaHours);

    return makeSuccessResult(QStringLiteral("migrationRate"), migrationRate, QStringLiteral("mm/h"));
}

QVariantMap AdvancedCalculationEngine::calculateHydrodynamic(const QVariantMap& params)
{
    const QString targetKey = params.value(QStringLiteral("targetKey")).toString();
    if (targetKey.isEmpty()) {
        return makeErrorResult(QStringLiteral("缺少流体力学计算目标"), QString(), QString());
    }

    double migrationRate = 0.0;
    if (!requirePositive(params, QStringLiteral("migrationRate"), &migrationRate)) {
        return makeErrorResult(QStringLiteral("颗粒迁移速率必须大于 0"), targetKey, fluidUnitForTarget(targetKey));
    }

    double concentration = 0.0;
    double diameter = 0.0;
    double dispersedDensity = 0.0;
    double continuousViscosity = 0.0;
    double continuousDensity = 0.0;

    if (targetKey != QStringLiteral("concentration")
            && !requirePositive(params, QStringLiteral("concentration"), &concentration)) {
        return makeErrorResult(QStringLiteral("体积浓度必须大于 0"), targetKey, fluidUnitForTarget(targetKey));
    }
    if (targetKey != QStringLiteral("diameter")
            && !requirePositive(params, QStringLiteral("diameter"), &diameter)) {
        return makeErrorResult(QStringLiteral("平均颗粒粒径必须大于 0"), targetKey, fluidUnitForTarget(targetKey));
    }
    if (targetKey != QStringLiteral("dispersedDensity")
            && !requirePositive(params, QStringLiteral("dispersedDensity"), &dispersedDensity)) {
        return makeErrorResult(QStringLiteral("分散相密度必须大于 0"), targetKey, fluidUnitForTarget(targetKey));
    }
    if (targetKey != QStringLiteral("continuousViscosity")
            && !requirePositive(params, QStringLiteral("continuousViscosity"), &continuousViscosity)) {
        return makeErrorResult(QStringLiteral("连续相粘度必须大于 0"), targetKey, fluidUnitForTarget(targetKey));
    }
    if (targetKey != QStringLiteral("continuousDensity")
            && !requirePositive(params, QStringLiteral("continuousDensity"), &continuousDensity)) {
        return makeErrorResult(QStringLiteral("连续相密度必须大于 0"), targetKey, fluidUnitForTarget(targetKey));
    }

    // 第一版按 Richardson-Zaki 浓度修正模型：
    // v = [d^2 * (ρp - ρf) * g / (18μ)] * (1 - φ)^n
    // 其中界面输入的体积浓度使用百分数，这里先换算成 0~1。
    const double phi = concentration / 100.0;
    if (targetKey != QStringLiteral("concentration") && (phi <= 0.0 || phi >= 1.0)) {
        return makeErrorResult(QStringLiteral("体积浓度必须在 0 到 100 之间"), targetKey, fluidUnitForTarget(targetKey));
    }

    const double hinderedFactor = qPow(qMax(0.000001, 1.0 - phi), kRichardsonZakiN);
    const double migrationRateSi = migrationRate * kMillimeterPerHourToMeterPerSecond;
    const double diameterSi = diameter * kMicrometerToMeter;
    const double dispersedDensitySi = dispersedDensity * kGramPerCubicCentimeterToKilogramPerCubicMeter;
    const double continuousDensitySi = continuousDensity * kGramPerCubicCentimeterToKilogramPerCubicMeter;
    const double viscositySi = continuousViscosity * kCentipoiseToPascalSecond;
    const double densityDiffSi = dispersedDensitySi - continuousDensitySi;
    const double stokesNumerator = 18.0 * viscositySi * migrationRateSi;
    double result = 0.0;

    if (targetKey == QStringLiteral("diameter")) {
        if (densityDiffSi <= 0.0) {
            return makeErrorResult(QStringLiteral("分散相密度必须大于连续相密度"), targetKey, fluidUnitForTarget(targetKey));
        }
        const double diameterResultSi = qSqrt(stokesNumerator / qMax(1e-18, densityDiffSi * kGravityAcceleration * hinderedFactor));
        result = diameterResultSi / kMicrometerToMeter;
    } else if (targetKey == QStringLiteral("concentration")) {
        if (densityDiffSi <= 0.0) {
            return makeErrorResult(QStringLiteral("分散相密度必须大于连续相密度"), targetKey, fluidUnitForTarget(targetKey));
        }
        const double inner = stokesNumerator / qMax(1e-18, diameterSi * diameterSi * densityDiffSi * kGravityAcceleration);
        if (inner <= 0.0) {
            return makeErrorResult(QStringLiteral("输入参数无法反算体积浓度"), targetKey, fluidUnitForTarget(targetKey));
        }
        result = 1.0 - qPow(inner, 1.0 / kRichardsonZakiN);
        result = qBound(0.0, result, 0.999999) * 100.0;
    } else if (targetKey == QStringLiteral("dispersedDensity")) {
        const double dispersedDensityResultSi = continuousDensitySi + stokesNumerator / qMax(1e-18, diameterSi * diameterSi * kGravityAcceleration * hinderedFactor);
        result = dispersedDensityResultSi / kGramPerCubicCentimeterToKilogramPerCubicMeter;
    } else if (targetKey == QStringLiteral("continuousViscosity")) {
        if (densityDiffSi <= 0.0) {
            return makeErrorResult(QStringLiteral("分散相密度必须大于连续相密度"), targetKey, fluidUnitForTarget(targetKey));
        }
        const double viscosityResultSi = diameterSi * diameterSi * densityDiffSi * kGravityAcceleration * hinderedFactor
                                         / qMax(1e-18, 18.0 * migrationRateSi);
        result = viscosityResultSi / kCentipoiseToPascalSecond;
    } else if (targetKey == QStringLiteral("continuousDensity")) {
        const double continuousDensityResultSi = dispersedDensitySi - stokesNumerator / qMax(1e-18, diameterSi * diameterSi * kGravityAcceleration * hinderedFactor);
        result = continuousDensityResultSi / kGramPerCubicCentimeterToKilogramPerCubicMeter;
    } else {
        return makeErrorResult(QStringLiteral("不支持的流体力学计算目标"), targetKey, QString());
    }

    return makeSuccessResult(targetKey, result, fluidUnitForTarget(targetKey));
}

QVariantMap AdvancedCalculationEngine::calculateOptical(const QVariantMap& params)
{
    const QString targetKey = params.value(QStringLiteral("targetKey")).toString();
    if (targetKey != QStringLiteral("diameter") && targetKey != QStringLiteral("concentration")) {
        return makeErrorResult(QStringLiteral("不支持的光学计算目标"), targetKey, QString());
    }

    double dispersedRefractive = 0.0;
    double dispersedAbsorption = 0.0;
    double continuousRefractive = 0.0;
    double continuousExtinction = 0.0;
    if (!requirePositive(params, QStringLiteral("dispersedRefractive"), &dispersedRefractive)
            || !requirePositive(params, QStringLiteral("dispersedAbsorption"), &dispersedAbsorption)
            || !requirePositive(params, QStringLiteral("continuousRefractive"), &continuousRefractive)
            || !requirePositive(params, QStringLiteral("continuousExtinction"), &continuousExtinction)) {
        return makeErrorResult(QStringLiteral("光学输入参数必须大于 0"), targetKey, opticalUnitForTarget(targetKey));
    }

    double concentration = 0.0;
    double diameter = 0.0;
    if (targetKey == QStringLiteral("diameter")) {
        if (!requirePositive(params, QStringLiteral("concentration"), &concentration)) {
            return makeErrorResult(QStringLiteral("体积浓度必须大于 0"), targetKey, opticalUnitForTarget(targetKey));
        }
    } else {
        if (!requirePositive(params, QStringLiteral("diameter"), &diameter)) {
            return makeErrorResult(QStringLiteral("粒径必须大于 0"), targetKey, opticalUnitForTarget(targetKey));
        }
    }

    const double refractiveDelta = qAbs(dispersedRefractive - continuousRefractive);
    if (refractiveDelta <= 0.0) {
        return makeErrorResult(QStringLiteral("分散相与连续相折射率不能相同"), targetKey, opticalUnitForTarget(targetKey));
    }

    // 光学计算当前是经验模型：固定四个光学参数，再在粒径和体积浓度之间做双向反算。
    const double baseSignal = dispersedAbsorption + continuousExtinction;
    double result = 0.0;
    if (targetKey == QStringLiteral("diameter")) {
        result = (baseSignal + concentration / 100.0) / refractiveDelta;
    } else {
        result = 100.0 * qMax(0.0, diameter * refractiveDelta - baseSignal);
    }

    return makeSuccessResult(targetKey, result, opticalUnitForTarget(targetKey));
}
