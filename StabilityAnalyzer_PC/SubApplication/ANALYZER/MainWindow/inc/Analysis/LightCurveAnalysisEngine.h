#ifndef LIGHTCURVEANALYSISENGINE_H
#define LIGHTCURVEANALYSISENGINE_H

#include <QPointF>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

struct LightCurveAnalysisCurve
{
    // 分析层使用的强类型中间曲线结构，避免反复拆装 QVariant。
    int scanId = 0;
    int timestamp = 0;
    int elapsedMs = 0;
    int pointCount = 0;
    double minHeightMm = 0.0;
    double maxHeightMm = 0.0;
    double minBackscatter = 0.0;
    double maxBackscatter = 0.0;
    double minTransmission = 0.0;
    double maxTransmission = 0.0;
    QVector<QPointF> backscatterPoints;
    QVector<QPointF> transmissionPoints;
};

class LightCurveAnalysisEngine
{
public:
    // 将数据层返回的 QVariantMap 曲线解析为分析层使用的强类型结构。
    static QVector<LightCurveAnalysisCurve> fromVariantMaps(const QVector<QVariantMap> &curves);
    // 对每条曲线执行高度裁剪，并在需要时做参比相减。
    static QVector<QVariantMap> processCurves(const QVector<LightCurveAnalysisCurve> &curves,
                                              int referenceScanId,
                                              double lowerMm,
                                              double upperMm,
                                              bool useReference);

private:
    // 将 QVariant 点列转换成 QPointF，便于后续数值计算。
    static QVector<QPointF> extractPoints(const QVariantList &points);
    // 将处理后的 QPointF 点列重新封装成 QML 使用的 {x, y} 结构。
    static QVariantList toVariantPoints(const QVector<QPointF> &points);
    // 将处理后的单条曲线回填成页面约定的字段结构。
    static QVariantMap toVariantCurve(const LightCurveAnalysisCurve &curve, int referenceScanId, bool useReference);
    // 只保留落在目标高度区间内的曲线点。
    static QVector<QPointF> filterPointsByHeight(const QVector<QPointF> &points, double lowerMm, double upperMm);
    // 在参比曲线上按给定高度做插值，供参比模式计算差值。
    static double valueAtHeight(const QVector<QPointF> &points, double heightMm);
};

#endif // LIGHTCURVEANALYSISENGINE_H
