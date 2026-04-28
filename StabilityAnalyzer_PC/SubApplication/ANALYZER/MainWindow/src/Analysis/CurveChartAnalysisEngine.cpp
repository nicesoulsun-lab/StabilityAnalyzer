#include "Analysis/CurveChartAnalysisEngine.h"

#include <QColor>
#include <QPointF>
#include <QVariantList>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {

// 从 QVariant 中安全提取数值，避免数据库空值或类型波动影响后续计算。
double toNumber(const QVariant& value, double fallback = 0.0)
{
    bool ok = false;
    const double number = value.toDouble(&ok);
    return ok ? number : fallback;
}

// 将内部使用的 QPointF 点列转换成 QML 易消费的 {x, y} 列表。
QVariantList toPointList(const QVector<QPointF>& points)
{
    QVariantList result;
    result.reserve(points.size());
    for (const QPointF& point : points) {
        QVariantMap map;
        map.insert("x", point.x());
        map.insert("y", point.y());
        result.append(map);
    }
    return result;
}

// 根据给定范围生成横轴刻度值列表，供 TrendChart 直接绘制时间轴。
QVariantList buildTimeTicks(double minValue, double maxValue, int desiredTickCount)
{
    QVariantList ticks;
    if (desiredTickCount <= 1 || maxValue <= minValue) {
        ticks.append(minValue);
        ticks.append(maxValue > minValue ? maxValue : minValue + 1.0);
        return ticks;
    }

    const double interval = (maxValue - minValue) / (desiredTickCount - 1);
    for (int i = 0; i < desiredTickCount; ++i) {
        ticks.append(minValue + interval * i);
    }
    return ticks;
}

// 生成纵轴标签文本，统一处理空数据、等值范围和小数位格式。
QVariantList makeAxisLabels(double minValue, double maxValue, int desiredTickCount, int precision)
{
    QVariantList labels;
    if (desiredTickCount <= 1 || !qIsFinite(minValue) || !qIsFinite(maxValue)) {
        labels.append(QString::number(0.0, 'f', precision));
        labels.append(QString::number(1.0, 'f', precision));
        return labels;
    }

    if (qFuzzyCompare(minValue, maxValue)) {
        maxValue = minValue + 1.0;
    }

    const double interval = (maxValue - minValue) / (desiredTickCount - 1);
    for (int i = desiredTickCount - 1; i >= 0; --i) {
        labels.append(QString::number(minValue + interval * i, 'f', precision));
    }
    return labels;
}

// 为图表下边界留出少量视觉余量，避免曲线贴边。
double paddedMin(double minValue, double maxValue, double fallbackSpan)
{
    if (!qIsFinite(minValue)) {
        return 0.0;
    }
    const double span = qMax(qAbs(maxValue - minValue), fallbackSpan);
    return minValue - span * 0.08;
}

// 为图表上边界留出少量视觉余量，避免最高点压在边框上。
double paddedMax(double maxValue, double minValue, double fallbackSpan)
{
    if (!qIsFinite(maxValue)) {
        return 1.0;
    }
    const double span = qMax(qAbs(maxValue - minValue), fallbackSpan);
    return maxValue + span * 0.08;
}

// 将经过的毫秒时间格式化成雷达图右侧色带使用的时间标签。
QString formatElapsedTime(qint64 elapsedMs)
{
    const qint64 totalSeconds = qMax<qint64>(0, qRound64(elapsedMs / 1000.0));
    const qint64 days = totalSeconds / (24 * 3600);
    const qint64 hours = (totalSeconds % (24 * 3600)) / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    return QString("%1d:%2h:%3m")
        .arg(days, 2, 10, QLatin1Char('0'))
        .arg(hours, 2, 10, QLatin1Char('0'))
        .arg(minutes, 2, 10, QLatin1Char('0'));
}

// 为空数据场景构造一个仍可正常渲染的时间序列图表结构。
QVariantMap buildEmptyTimeSeries(const QString& title, double lowerBoundMm, double upperBoundMm)
{
    QVariantMap result;
    result.insert("title", title);
    result.insert("rangeLabel", QString("%1 - %2 mm")
                                 .arg(QString::number(lowerBoundMm, 'f', 1))
                                 .arg(QString::number(upperBoundMm, 'f', 1)));
    result.insert("points", QVariantList());
    result.insert("chartMinX", 0.0);
    result.insert("chartMaxX", 1.0);
    result.insert("chartMinY", 0.0);
    result.insert("chartMaxY", 1.0);
    result.insert("xAxisTickValues", buildTimeTicks(0.0, 1.0, 2));
    result.insert("yAxisLabels", makeAxisLabels(0.0, 1.0, 6, 1));
    return result;
}

// 通用双曲线图表构造器，用于均匀度和光强平均值这类 BS/T 成对曲线页面。
QVariantMap buildDualSeriesChartData(const QVector<QVariantMap>& inputRows,
                                     const QString& bsKey,
                                     const QString& tKey,
                                     bool fixedZeroOneRange)
{
    QVariantMap result;
    QVariantList rowList;
    rowList.reserve(inputRows.size());
    for (const QVariantMap& row : inputRows) {
        rowList.append(row);
    }
    result.insert("rows", rowList);

    if (inputRows.isEmpty()) {
        result.insert("bsPoints", QVariantList());
        result.insert("tPoints", QVariantList());
        result.insert("chartMinX", 0.0);
        result.insert("chartMaxX", 1.0);
        result.insert("chartMinY", 0.0);
        result.insert("chartMaxY", 1.0);
        result.insert("xAxisTickValues", buildTimeTicks(0.0, 1.0, 2));
        result.insert("yAxisLabels", makeAxisLabels(0.0, 1.0, 6, fixedZeroOneRange ? 2 : 1));
        return result;
    }

    QVector<QVariantMap> rows = inputRows;
    std::sort(rows.begin(), rows.end(), [](const QVariantMap& left, const QVariantMap& right) {
        return toNumber(left.value("scan_elapsed_ms")) < toNumber(right.value("scan_elapsed_ms"));
    });

    QVector<QPointF> bsPoints;
    QVector<QPointF> tPoints;
    bsPoints.reserve(rows.size());
    tPoints.reserve(rows.size());

    double minY = std::numeric_limits<double>::infinity();
    double maxY = -std::numeric_limits<double>::infinity();

    for (const QVariantMap& row : rows) {
        const double xValue = toNumber(row.value("scan_elapsed_ms")) / 60000.0;
        const double bsValue = toNumber(row.value(bsKey));
        const double tValue = toNumber(row.value(tKey));
        bsPoints.append(QPointF(xValue, bsValue));
        tPoints.append(QPointF(xValue, tValue));
        minY = qMin(minY, qMin(bsValue, tValue));
        maxY = qMax(maxY, qMax(bsValue, tValue));
    }

    const double chartMinX = bsPoints.isEmpty() ? 0.0 : bsPoints.first().x();
    double chartMaxX = bsPoints.size() > 1 ? bsPoints.last().x() : chartMinX + 1.0;
    if (chartMaxX <= chartMinX) {
        chartMaxX = chartMinX + 1.0;
    }

    const double chartMinY = fixedZeroOneRange ? 0.0 : paddedMin(minY, maxY, 1.0);
    const double chartMaxY = fixedZeroOneRange ? 1.0 : paddedMax(maxY, minY, 1.0);

    result.insert("rows", rowList);
    result.insert("bsPoints", toPointList(bsPoints));
    result.insert("tPoints", toPointList(tPoints));
    result.insert("chartMinX", chartMinX);
    result.insert("chartMaxX", chartMaxX);
    result.insert("chartMinY", chartMinY);
    result.insert("chartMaxY", chartMaxY);
    result.insert("xAxisTickValues", buildTimeTicks(chartMinX, chartMaxX, 6));
    result.insert("yAxisLabels", makeAxisLabels(chartMinY, chartMaxY, 6, fixedZeroOneRange ? 2 : 1));
    return result;
}

// 按当前模式选择使用被射光还是透射光信号。
double signalValue(const QVariantMap& row, int intensityMode)
{
    return intensityMode == 0
        ? toNumber(row.value("backscatter_intensity"))
        : toNumber(row.value("transmission_intensity"));
}

}

QVariantMap CurveChartAnalysisEngine::buildUniformityChartData(const QVector<QVariantMap>& rows)
{
    // 均匀度页面始终展示 BS/T 两条归一化曲线。
    return buildDualSeriesChartData(rows, "ui_backscatter", "ui_transmission", true);
}

QVariantMap CurveChartAnalysisEngine::buildLightIntensityAverageChartData(const QVector<QVariantMap>& rows)
{
    // 光强平均值直接复用双曲线图表结构，但保留真实数值范围。
    return buildDualSeriesChartData(rows, "avg_backscatter", "avg_transmission", false);
}

QVariantMap CurveChartAnalysisEngine::buildSeparationLayerChartData(const QVector<QVariantMap>& inputRows)
{
    // 将三层厚度结果整理成三条共享时间轴的曲线，便于页面直接切换显示。
    QVariantMap result;
    QVariantList rowList;
    rowList.reserve(inputRows.size());
    for (const QVariantMap& row : inputRows) {
        rowList.append(row);
    }
    result.insert("rows", rowList);

    if (inputRows.isEmpty()) {
        result.insert("clarificationPoints", QVariantList());
        result.insert("concentratedPoints", QVariantList());
        result.insert("sedimentPoints", QVariantList());
        result.insert("chartMinX", 0.0);
        result.insert("chartMaxX", 1.0);
        result.insert("chartMinY", 0.0);
        result.insert("chartMaxY", 1.0);
        result.insert("xAxisTickValues", buildTimeTicks(0.0, 1.0, 2));
        result.insert("yAxisLabels", makeAxisLabels(0.0, 1.0, 6, 1));
        return result;
    }

    QVector<QVariantMap> rows = inputRows;
    std::sort(rows.begin(), rows.end(), [](const QVariantMap& left, const QVariantMap& right) {
        const double elapsedDiff = toNumber(left.value("scan_elapsed_ms")) - toNumber(right.value("scan_elapsed_ms"));
        if (!qFuzzyIsNull(elapsedDiff)) {
            return elapsedDiff < 0.0;
        }
        return toNumber(left.value("scan_id")) < toNumber(right.value("scan_id"));
    });

    QVector<QPointF> clarificationPoints;
    QVector<QPointF> concentratedPoints;
    QVector<QPointF> sedimentPoints;
    clarificationPoints.reserve(rows.size());
    concentratedPoints.reserve(rows.size());
    sedimentPoints.reserve(rows.size());

    double minY = std::numeric_limits<double>::infinity();
    double maxY = -std::numeric_limits<double>::infinity();

    for (const QVariantMap& row : rows) {
        const double xValue = toNumber(row.value("scan_elapsed_ms")) / 60000.0;
        const double clarificationValue = toNumber(row.value("clarification_thickness_mm"));
        const double concentratedValue = toNumber(row.value("concentrated_phase_thickness_mm"));
        const double sedimentValue = toNumber(row.value("sediment_thickness_mm"));

        clarificationPoints.append(QPointF(xValue, clarificationValue));
        concentratedPoints.append(QPointF(xValue, concentratedValue));
        sedimentPoints.append(QPointF(xValue, sedimentValue));

        minY = qMin(minY, qMin(clarificationValue, qMin(concentratedValue, sedimentValue)));
        maxY = qMax(maxY, qMax(clarificationValue, qMax(concentratedValue, sedimentValue)));
    }

    const double chartMinX = clarificationPoints.isEmpty() ? 0.0 : clarificationPoints.first().x();
    double chartMaxX = clarificationPoints.size() > 1 ? clarificationPoints.last().x() : chartMinX + 1.0;
    if (chartMaxX <= chartMinX) {
        chartMaxX = chartMinX + 1.0;
    }
    const double chartMaxY = qIsFinite(maxY) ? paddedMax(maxY, minY, 1.0) : 1.0;

    result.insert("rows", rowList);
    result.insert("clarificationPoints", toPointList(clarificationPoints));
    result.insert("concentratedPoints", toPointList(concentratedPoints));
    result.insert("sedimentPoints", toPointList(sedimentPoints));
    result.insert("chartMinX", chartMinX);
    result.insert("chartMaxX", chartMaxX);
    result.insert("chartMinY", 0.0);
    result.insert("chartMaxY", chartMaxY);
    result.insert("xAxisTickValues", buildTimeTicks(chartMinX, chartMaxX, 6));
    result.insert("yAxisLabels", makeAxisLabels(0.0, chartMaxY, 6, 1));
    return result;
}

QVariantMap CurveChartAnalysisEngine::buildInstabilitySeriesChartData(const QVector<QVariantMap>& inputRows,
                                                                      const QString& title,
                                                                      double lowerBoundMm,
                                                                      double upperBoundMm)
{
    // 针对单个高度区间输出标准趋势图结构；空数据时仍保留区间信息供页面展示。
    const double safeLower = qMin(lowerBoundMm, upperBoundMm);
    const double safeUpper = qMax(lowerBoundMm, upperBoundMm);
    QVariantMap result = buildEmptyTimeSeries(title, safeLower, safeUpper);

    if (inputRows.isEmpty() || safeUpper <= safeLower) {
        return result;
    }

    QVector<QVariantMap> rows = inputRows;
    std::sort(rows.begin(), rows.end(), [](const QVariantMap& left, const QVariantMap& right) {
        return toNumber(left.value("scan_elapsed_ms")) < toNumber(right.value("scan_elapsed_ms"));
    });

    QVector<QPointF> points;
    points.reserve(rows.size());
    double maxY = 0.0;
    for (const QVariantMap& row : rows) {
        const double xValue = toNumber(row.value("scan_elapsed_ms")) / 60000.0;
        const double yValue = toNumber(row.value("instability_value"));
        points.append(QPointF(xValue, yValue));
        maxY = qMax(maxY, yValue);
    }

    const double chartMinX = points.isEmpty() ? 0.0 : points.first().x();
    double chartMaxX = points.size() > 1 ? points.last().x() : chartMinX + 1.0;
    if (chartMaxX <= chartMinX) {
        chartMaxX = chartMinX + 1.0;
    }
    const double chartMaxY = qMax(1.0, maxY * 1.12);

    result.insert("points", toPointList(points));
    result.insert("chartMinX", chartMinX);
    result.insert("chartMaxX", chartMaxX);
    result.insert("chartMinY", 0.0);
    result.insert("chartMaxY", chartMaxY);
    result.insert("xAxisTickValues", buildTimeTicks(chartMinX, chartMaxX, 6));
    result.insert("yAxisLabels", makeAxisLabels(0.0, chartMaxY, 6, 1));
    return result;
}

QVariantMap CurveChartAnalysisEngine::buildInstabilityRadarChartData(const QVariantMap& overallSeries,
                                                                     const QVariantMap& bottomSeries,
                                                                     const QVariantMap& middleSeries,
                                                                     const QVariantMap& topSeries)
{
    // 将四条同步时间序列按同一时间索引重组为雷达图每一层多边形。
    auto pointListFromSeries = [](const QVariantMap& series) {
        return series.value("points").toList();
    };

    const QVariantList overallPoints = pointListFromSeries(overallSeries);
    const QVariantList bottomPoints = pointListFromSeries(bottomSeries);
    const QVariantList middlePoints = pointListFromSeries(middleSeries);
    const QVariantList topPoints = pointListFromSeries(topSeries);
    const int pointCount = qMin(qMin(overallPoints.size(), bottomPoints.size()),
                                qMin(middlePoints.size(), topPoints.size()));

    QVariantList polygons;
    double maxValue = 1.0;
    for (int i = 0; i < pointCount; ++i) {
        const QVariantMap overallPoint = overallPoints.at(i).toMap();
        const QVariantMap bottomPoint = bottomPoints.at(i).toMap();
        const QVariantMap middlePoint = middlePoints.at(i).toMap();
        const QVariantMap topPoint = topPoints.at(i).toMap();

        const double overallValue = toNumber(overallPoint.value("y"));
        const double bottomValue = toNumber(bottomPoint.value("y"));
        const double middleValue = toNumber(middlePoint.value("y"));
        const double topValue = toNumber(topPoint.value("y"));

        QVariantList values;
        values << overallValue << bottomValue << middleValue << topValue;
        maxValue = qMax(maxValue, qMax(qMax(overallValue, bottomValue), qMax(middleValue, topValue)));

        const double ratio = pointCount <= 1 ? 0.0 : static_cast<double>(i) / static_cast<double>(pointCount - 1);
        const QColor color = QColor::fromHslF(0.65 * (1.0 - ratio), 0.9, 0.48, 0.95);

        QVariantMap polygon;
        polygon.insert("values", values);
        polygon.insert("label", formatElapsedTime(qRound64(toNumber(overallPoint.value("x")) * 60000.0)));
        polygon.insert("color", color.name(QColor::HexRgb));
        polygons.append(polygon);
    }

    QVariantMap result;
    result.insert("polygons", polygons);
    result.insert("maxValue", qMax(1.0, std::ceil(maxValue)));
    return result;
}
