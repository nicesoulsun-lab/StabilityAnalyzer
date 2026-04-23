#include "Analysis/LightCurveAnalysisEngine.h"

#include <QtMath>
#include <algorithm>
#include <limits>

// 解析一组 QVariant 点数据，转换成便于计算的 QPointF 序列。
QVector<QPointF> LightCurveAnalysisEngine::extractPoints(const QVariantList &points)
{
    QVector<QPointF> result;
    result.reserve(points.size());

    for (const QVariant &value : points) {
        const QVariantMap map = value.toMap();
        const double xValue = map.value("x").toDouble();
        const double yValue = map.value("y").toDouble();
        result.append(QPointF(xValue, yValue));
    }

    return result;
}

// 将处理后的点列重新封装成 QML 组件使用的 {x, y} 结构。
QVariantList LightCurveAnalysisEngine::toVariantPoints(const QVector<QPointF> &points)
{
    QVariantList result;
    result.reserve(points.size());

    for (const QPointF &point : points) {
        QVariantMap map;
        map["x"] = point.x();
        map["y"] = point.y();
        result.append(map);
    }

    return result;
}

QVector<LightCurveAnalysisCurve> LightCurveAnalysisEngine::fromVariantMaps(const QVector<QVariantMap> &curves)
{
    // 分析层尽量只处理强类型数据，避免算法过程中反复拆装 QVariant。
    QVector<LightCurveAnalysisCurve> result;
    result.reserve(curves.size());

    for (const QVariantMap &curveMap : curves) {
        LightCurveAnalysisCurve curve;
        curve.scanId = curveMap.value("scan_id").toInt();
        curve.timestamp = curveMap.value("timestamp").toInt();
        curve.elapsedMs = curveMap.value("scan_elapsed_ms").toInt();
        curve.pointCount = curveMap.value("point_count").toInt();
        curve.minHeightMm = curveMap.value("min_height_mm").toDouble();
        curve.maxHeightMm = curveMap.value("max_height_mm").toDouble();
        curve.minBackscatter = curveMap.value("min_backscatter").toDouble();
        curve.maxBackscatter = curveMap.value("max_backscatter").toDouble();
        curve.minTransmission = curveMap.value("min_transmission").toDouble();
        curve.maxTransmission = curveMap.value("max_transmission").toDouble();
        curve.backscatterPoints = extractPoints(curveMap.value("backscatter_points").toList());
        curve.transmissionPoints = extractPoints(curveMap.value("transmission_points").toList());
        result.append(curve);
    }

    return result;
}

QVector<QPointF> LightCurveAnalysisEngine::filterPointsByHeight(const QVector<QPointF> &points, double lowerMm, double upperMm)
{
    // 这里只做高度裁剪，不改原有点序，方便后续继续做参比或直接显示。
    QVector<QPointF> filtered;
    filtered.reserve(points.size());

    for (const QPointF &point : points) {
        if (point.x() >= lowerMm && point.x() <= upperMm) {
            filtered.append(point);
        }
    }

    return filtered;
}

double LightCurveAnalysisEngine::valueAtHeight(const QVector<QPointF> &points, double heightMm)
{
    // 当前曲线和参比曲线的采样高度不一定一致，因此这里用线性插值取参比值。
    if (points.isEmpty()) {
        return 0.0;
    }
    if (heightMm <= points.first().x()) {
        return points.first().y();
    }
    if (heightMm >= points.last().x()) {
        return points.last().y();
    }

    for (int i = 1; i < points.size(); ++i) {
        const QPointF &left = points.at(i - 1);
        const QPointF &right = points.at(i);
        if (heightMm > right.x()) {
            continue;
        }
        if (qFuzzyCompare(left.x(), right.x())) {
            return right.y();
        }

        const double ratio = (heightMm - left.x()) / (right.x() - left.x());
        return left.y() + (right.y() - left.y()) * ratio;
    }

    return points.last().y();
}

QVariantMap LightCurveAnalysisEngine::toVariantCurve(const LightCurveAnalysisCurve &curve, int referenceScanId, bool useReference)
{
    // 将分析层结果回填成页面层统一使用的曲线结构。
    QVariantMap result;
    result["scan_id"] = curve.scanId;
    result["timestamp"] = curve.timestamp;
    result["scan_elapsed_ms"] = curve.elapsedMs;
    result["point_count"] = curve.pointCount;
    result["min_height_mm"] = curve.minHeightMm;
    result["max_height_mm"] = curve.maxHeightMm;
    result["min_backscatter"] = curve.minBackscatter;
    result["max_backscatter"] = curve.maxBackscatter;
    result["min_transmission"] = curve.minTransmission;
    result["max_transmission"] = curve.maxTransmission;
    result["reference_scan_id"] = referenceScanId;
    result["use_reference"] = useReference;
    result["backscatter_points"] = toVariantPoints(curve.backscatterPoints);
    result["transmission_points"] = toVariantPoints(curve.transmissionPoints);
    return result;
}

QVector<QVariantMap> LightCurveAnalysisEngine::processCurves(const QVector<LightCurveAnalysisCurve> &curves,
                                                             int referenceScanId,
                                                             double lowerMm,
                                                             double upperMm,
                                                             bool useReference)
{
    // 光强处理主流程：
    // 1. 找到参比曲线
    // 2. 裁剪到目标高度区间
    // 3. 按需执行参比相减
    // 4. 重算 min/max 和点列，返回给页面层
    QVector<QVariantMap> result;
    if (curves.isEmpty()) {
        return result;
    }

    int safeReferenceIndex = 0;
    for (int i = 0; i < curves.size(); ++i) {
        if (curves.at(i).scanId == referenceScanId) {
            safeReferenceIndex = i;
            break;
        }
    }

    const int safeReferenceScanId = curves.at(safeReferenceIndex).scanId;
    const QVector<QPointF> referenceBackscatter = filterPointsByHeight(curves.at(safeReferenceIndex).backscatterPoints, lowerMm, upperMm);
    const QVector<QPointF> referenceTransmission = filterPointsByHeight(curves.at(safeReferenceIndex).transmissionPoints, lowerMm, upperMm);

    result.reserve(curves.size());
    for (const LightCurveAnalysisCurve &rawCurve : curves) {
        LightCurveAnalysisCurve processedCurve = rawCurve;
        processedCurve.backscatterPoints = filterPointsByHeight(rawCurve.backscatterPoints, lowerMm, upperMm);
        processedCurve.transmissionPoints = filterPointsByHeight(rawCurve.transmissionPoints, lowerMm, upperMm);
        if (processedCurve.backscatterPoints.isEmpty() || processedCurve.transmissionPoints.isEmpty()) {
            continue;
        }

        processedCurve.pointCount = qMin(processedCurve.backscatterPoints.size(), processedCurve.transmissionPoints.size());
        processedCurve.minHeightMm = lowerMm;
        processedCurve.maxHeightMm = upperMm;
        processedCurve.minBackscatter = std::numeric_limits<double>::max();
        processedCurve.maxBackscatter = std::numeric_limits<double>::lowest();
        processedCurve.minTransmission = std::numeric_limits<double>::max();
        processedCurve.maxTransmission = std::numeric_limits<double>::lowest();

        if (useReference) {
            for (QPointF &point : processedCurve.backscatterPoints) {
                point.setY(point.y() - valueAtHeight(referenceBackscatter, point.x()));
            }
            for (QPointF &point : processedCurve.transmissionPoints) {
                point.setY(point.y() - valueAtHeight(referenceTransmission, point.x()));
            }
        }

        for (const QPointF &point : processedCurve.backscatterPoints) {
            processedCurve.minBackscatter = qMin(processedCurve.minBackscatter, point.y());
            processedCurve.maxBackscatter = qMax(processedCurve.maxBackscatter, point.y());
        }
        for (const QPointF &point : processedCurve.transmissionPoints) {
            processedCurve.minTransmission = qMin(processedCurve.minTransmission, point.y());
            processedCurve.maxTransmission = qMax(processedCurve.maxTransmission, point.y());
        }

        result.append(toVariantCurve(processedCurve, safeReferenceScanId, useReference));
    }

    return result;
}
