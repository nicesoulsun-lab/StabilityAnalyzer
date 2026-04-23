#include "Analysis/LightCurveAnalysisCache.h"

bool LightCurveAnalysisCacheKey::operator==(const LightCurveAnalysisCacheKey &other) const
{
    // 缓存键必须在所有分析参数上都一致，才能视为同一份结果。
    return experimentId == other.experimentId
        && pointsPerCurve == other.pointsPerCurve
        && referenceScanId == other.referenceScanId
        && lowerMilli == other.lowerMilli
        && upperMilli == other.upperMilli
        && useReference == other.useReference;
}

uint qHash(const LightCurveAnalysisCacheKey &key, uint seed)
{
    // 与 operator== 使用同一组字段，确保同参数请求命中同一个桶。
    seed ^= ::qHash(key.experimentId, seed);
    seed ^= ::qHash(key.pointsPerCurve, seed);
    seed ^= ::qHash(key.referenceScanId, seed);
    seed ^= ::qHash(key.lowerMilli, seed);
    seed ^= ::qHash(key.upperMilli, seed);
    seed ^= ::qHash(key.useReference, seed);
    return seed;
}

bool LightCurveAnalysisCache::contains(const LightCurveAnalysisCacheKey &key) const
{
    // 先判断是否命中，避免不必要的结果拷贝。
    return m_cache.contains(key);
}

QVariantList LightCurveAnalysisCache::value(const LightCurveAnalysisCacheKey &key) const
{
    // 直接返回缓存的图表结果，不再重复进入分析流程。
    return m_cache.value(key);
}

void LightCurveAnalysisCache::insert(const LightCurveAnalysisCacheKey &key, const QVariantList &curves)
{
    // 缓存粒度是一整个实验在一组参数下的完整处理结果。
    m_cache.insert(key, curves);
}

void LightCurveAnalysisCache::clear()
{
    // 全量清空适用于大范围数据变更后的保守失效策略。
    m_cache.clear();
}

void LightCurveAnalysisCache::clearExperiment(int experimentId)
{
    // 只清理被修改实验对应的缓存，避免无关实验一起失效。
    auto it = m_cache.begin();
    while (it != m_cache.end()) {
        if (it.key().experimentId == experimentId) {
            it = m_cache.erase(it);
        } else {
            ++it;
        }
    }
}
