#ifndef CURVECHARTANALYSISENGINE_H
#define CURVECHARTANALYSISENGINE_H

#include <QVariantMap>
#include <QVector>
#include <QString>

// 通用图表整理器。
// 负责把数据库结果整理成 QML 可直接绑定的图表结构。
class CurveChartAnalysisEngine
{
public:
    // 构建均匀度图表数据，输出 BS/T 曲线和坐标轴信息。
    static QVariantMap buildUniformityChartData(const QVector<QVariantMap>& rows);
    // 构建光强平均值图表数据，输出 BS/T 曲线和坐标轴信息。
    static QVariantMap buildLightIntensityAverageChartData(const QVector<QVariantMap>& rows);
    // 构建分层厚度图表数据，输出澄清层、浓相层、沉淀层三条曲线。
    static QVariantMap buildSeparationLayerChartData(const QVector<QVariantMap>& rows);
    // 构建指定高度区间的不稳定性趋势图数据。
    static QVariantMap buildInstabilitySeriesChartData(const QVector<QVariantMap>& rows,
                                                       const QString& title,
                                                       double lowerBoundMm,
                                                       double upperBoundMm);
    // 将整体、底部、中部、顶部四组不稳定性曲线合成为雷达图多边形数据。
    static QVariantMap buildInstabilityRadarChartData(const QVariantMap& overallSeries,
                                                      const QVariantMap& bottomSeries,
                                                      const QVariantMap& middleSeries,
                                                      const QVariantMap& topSeries);
    // 直接基于原始扫描行构建峰厚度趋势图数据。
    static QVariantMap buildPeakThicknessChartData(const QVector<QVariantMap>& rawRows,
                                                   int intensityMode,
                                                   double lowerBoundMm,
                                                   double upperBoundMm,
                                                   double thresholdValue);
};

#endif // CURVECHARTANALYSISENGINE_H
