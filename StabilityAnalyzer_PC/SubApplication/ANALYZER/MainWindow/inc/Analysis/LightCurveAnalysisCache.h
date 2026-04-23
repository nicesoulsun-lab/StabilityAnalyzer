#ifndef LIGHTCURVEANALYSISCACHE_H
#define LIGHTCURVEANALYSISCACHE_H

#include <QHash>
#include <QVariantList>

struct LightCurveAnalysisCacheKey
{
    // 一条缓存由实验、采样点数、参比线和高度区间共同确定。
    int experimentId = 0;
    int pointsPerCurve = 0;
    int referenceScanId = 0;
    qint64 lowerMilli = 0;
    qint64 upperMilli = 0;
    bool useReference = false;

    bool operator==(const LightCurveAnalysisCacheKey &other) const;
};

uint qHash(const LightCurveAnalysisCacheKey &key, uint seed = 0);

class LightCurveAnalysisCache
{
public:
    // 轻量内存缓存，由 data_ctrl 协调使用，避免重复处理光强曲线。
    bool contains(const LightCurveAnalysisCacheKey &key) const;
    // 读取一组参数对应的缓存结果。
    QVariantList value(const LightCurveAnalysisCacheKey &key) const;
    // 写入一组参数对应的处理结果。
    void insert(const LightCurveAnalysisCacheKey &key, const QVariantList &curves);
    // 清空全部光强分析缓存。
    void clear();
    // 只清空指定实验对应的缓存项。
    void clearExperiment(int experimentId);

private:
    QHash<LightCurveAnalysisCacheKey, QVariantList> m_cache;
};

#endif // LIGHTCURVEANALYSISCACHE_H
