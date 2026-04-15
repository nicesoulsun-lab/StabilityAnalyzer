#include "CurveItem.h"
#include <QSGGeometry>
#include <QSGFlatColorMaterial>
#include <QQuickWindow>
#include <QThread>
#include <QTimer>
#include <algorithm>
#include <cstring>

/**
 * @brief CurveItem构造函数
 * @param parent 父对象指针
 *
 * 初始化曲线绘制组件：
 * - 设置默认线条颜色为蓝色
 * - 设置默认线条宽度为2.0
 * - 设置最大数据点数为1000
 * - 启用自动缩放模式
 * - 设置默认Y轴范围[0, 100]
 * - 启用Qt Quick内容渲染标志
 */
CurveItem::CurveItem(QQuickItem *parent)
    : QQuickItem(parent)
    , m_lineColor(Qt::blue)
    , m_lineWidth(2.0)
    , m_maxPoints(1000)
    , m_autoScale(true)
    , m_minYValue(0.0)
    , m_maxYValue(100.0)
    , m_actualMinY(0.0)
    , m_actualMaxY(100.0)
    , m_geometryDirty(true)
    , m_lastPointCount(0)
    , m_timeWindow(60.0)
    , m_updateTimer(new QTimer(this))
    , m_pendingUpdate(false)
    , m_pendingDataPoints()
{
    setFlag(QQuickItem::ItemHasContents, true);
    
    // 配置帧率限制定时器（60 FPS）
    m_updateTimer->setSingleShot(true);
    m_updateTimer->setInterval(16); // ~60 FPS
    
    // 连接定时器信号到更新槽
    connect(m_updateTimer, &QTimer::timeout, this, &CurveItem::performPendingUpdate);
}

CurveItem::~CurveItem()
{
}

QColor CurveItem::lineColor() const
{
    return m_lineColor;
}

void CurveItem::setLineColor(const QColor &color)
{
    if (m_lineColor != color) {
        m_lineColor = color;
        emit lineColorChanged();
        update();
    }
}

qreal CurveItem::lineWidth() const
{
    return m_lineWidth;
}

void CurveItem::setLineWidth(qreal width)
{
    if (m_lineWidth != width) {
        m_lineWidth = width;
        emit lineWidthChanged();
        update();
    }
}

QVariantList CurveItem::dataPoints() const
{
    QVariantList list;
    for (const QPointF &point : m_points) {
        list.append(QVariant(point));
    }
    return list;
}

void CurveItem::setDataPoints(const QVariantList &points)
{
    // 性能优化：避免不必要的清空和重建
    if (points.isEmpty()) {
        if (!m_points.isEmpty()) {
            m_points.clear();
            m_pendingDataPoints.clear();
            m_geometryDirty = true;
            m_pendingUpdate = false;
            m_updateTimer->stop();
            emit dataPointsChanged();
            update();
        }
        return;
    }

    // 性能优化：增量更新而不是完全重建
    const int oldSize = m_points.size();
    
    // 只添加新数据点，避免完全清空
    for (const QVariant &point : points) {
        if (point.canConvert<QPointF>()) {
            m_points.append(point.toPointF());
        }
    }

    // 如果超过最大点数，移除最早的数据点
    if (m_points.size() > m_maxPoints) {
        m_points = m_points.mid(m_points.size() - m_maxPoints);
    }

    // 性能优化：只在数据点数量变化时计算边界和发射信号
    if (m_points.size() != oldSize) {
        calculateBounds();
        m_geometryDirty = true;
        emit dataPointsChanged();
        
        // 使用帧率限制：如果定时器未运行，立即更新；否则标记为待更新
        if (!m_updateTimer->isActive()) {
            update();
            m_updateTimer->start();
        } else {
            m_pendingUpdate = true;
        }
    }
}

int CurveItem::maxPoints() const
{
    return m_maxPoints;
}

void CurveItem::setMaxPoints(int max)
{
    if (m_maxPoints != max && max > 0) {
        m_maxPoints = max;

        if (m_points.size() > m_maxPoints) {
            m_points = m_points.mid(m_points.size() - m_maxPoints);
            calculateBounds();
            m_geometryDirty = true;
            update();
        }

        emit maxPointsChanged();
    }
}

bool CurveItem::autoScale() const
{
    return m_autoScale;
}

void CurveItem::setAutoScale(bool autoScale)
{
    if (m_autoScale != autoScale) {
        m_autoScale = autoScale;
        emit autoScaleChanged();
        update();
    }
}

qreal CurveItem::minYValue() const
{
    return m_minYValue;
}

void CurveItem::setMinYValue(qreal min)
{
    if (m_minYValue != min) {
        m_minYValue = min;
        emit minYValueChanged();
        update();
    }
}

qreal CurveItem::maxYValue() const
{
    return m_maxYValue;
}

void CurveItem::setMaxYValue(qreal max)
{
    if (m_maxYValue != max) {
        m_maxYValue = max;
        emit maxYValueChanged();
        update();
    }
}

void CurveItem::addDataPoint(qreal x, qreal y)
{
    const bool wasEmpty = m_points.isEmpty();
    
    // 添加新数据点
    m_points.append(QPointF(x, y));

    // 如果超过最大点数，移除最早的数据点
    if (m_points.size() > m_maxPoints) {
        m_points.removeFirst();
    }

    // 自动缩放时更新Y轴范围
    if (m_autoScale) {
        if (wasEmpty) {
            // 第一个数据点，直接设置范围
            m_actualMinY = y;
            m_actualMaxY = y;
        } else {
            // 增量更新范围
            if (y < m_actualMinY) m_actualMinY = y;
            if (y > m_actualMaxY) m_actualMaxY = y;
        }
    }

    // 标记几何数据需要更新
    m_geometryDirty = true;
    
    // 只在数据点数量变化时发射信号
    if (m_points.size() != m_lastPointCount) {
        emit dataPointsChanged();
    }
    
    // 使用帧率限制：如果定时器未运行，立即更新；否则标记为待更新
    if (!m_updateTimer->isActive()) {
        update();
        m_updateTimer->start();
    } else {
        m_pendingUpdate = true;
    }
}

void CurveItem::clearData()
{
    m_points.clear();
    m_pendingDataPoints.clear();
    m_actualMinY = 0.0;
    m_actualMaxY = 100.0;
    m_geometryDirty = true;
    m_pendingUpdate = false;
    m_updateTimer->stop();
    emit dataPointsChanged();
    update();
}

void CurveItem::updateCurve()
{
    m_geometryDirty = true;
    
    // 使用帧率限制：如果定时器未运行，立即更新；否则标记为待更新
    if (!m_updateTimer->isActive()) {
        update();
        m_updateTimer->start();
    } else {
        m_pendingUpdate = true;
    }
}

void CurveItem::performPendingUpdate()
{
    if (m_pendingUpdate) {
        m_pendingUpdate = false;
        update();
        
        // 如果还有待处理的数据点，继续处理
        if (!m_pendingDataPoints.isEmpty()) {
            m_updateTimer->start();
        }
    }
}

QSGNode *CurveItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    Q_UNUSED(data)

    /* 1. 没有数据时直接清理 */
    if (m_points.isEmpty()) {
        delete oldNode;
        m_lastPointCount = 0;
        return nullptr;
    }

    QSGGeometryNode *node = static_cast<QSGGeometryNode *>(oldNode);
    const int currentPointCount = m_points.size();
    const bool isNewNode = !node;
    const bool needsReallocation = m_geometryDirty || 
                                  (node && node->geometry() && 
                                   node->geometry()->vertexCount() != currentPointCount);

    /* 2. 第一次创建节点或需要重新分配 */
    if (isNewNode || needsReallocation) {
        if (!node) {
            node = new QSGGeometryNode;
            node->setFlag(QSGNode::OwnsGeometry);
            node->setFlag(QSGNode::OwnsMaterial);
        }

        // 预分配足够大的缓冲区，避免频繁重分配
        // 使用最大点数作为缓冲区大小，避免频繁重分配
        const int allocatedSize = m_maxPoints;
        QSGGeometry *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), allocatedSize);
        geometry->setLineWidth(m_lineWidth);
        geometry->setDrawingMode(QSGGeometry::DrawLineStrip);
        geometry->setVertexDataPattern(QSGGeometry::DynamicPattern);
        node->setGeometry(geometry);

        QSGFlatColorMaterial *material = new QSGFlatColorMaterial;
        material->setColor(m_lineColor);
        node->setMaterial(material);
    }

    /* 3. 更新顶点数据 - 增量更新优化 */
    QSGGeometry *geometry = node->geometry();
    if (geometry->vertexCount() == 0) {
        return node;
    }

    QSGGeometry::Point2D *vertices = geometry->vertexDataAsPoint2D();
    const qreal w = boundingRect().width();
    const qreal h = boundingRect().height();

    // 计算X轴范围（使用实际数据点的X坐标）
    qreal minX = 0.0;
    qreal maxX = 1.0;
    
    if (!m_points.isEmpty()) {
        minX = m_points.first().x();
        maxX = m_points.last().x();
        
        // 如果所有点X坐标相同，设置一个合理的范围
        if (qFuzzyCompare(minX, maxX)) {
            minX = minX - 1.0;
            maxX = maxX + 1.0;
        }
    }
    
    const qreal xRange = std::max(maxX - minX, 1.0); // 避免除零
    const qreal xScale = w / xRange;

    // 预计算Y轴参数
    const qreal minY = m_autoScale ? m_actualMinY : m_minYValue;
    const qreal maxY = m_autoScale ? m_actualMaxY : m_maxYValue;
    const qreal yRange = std::max(maxY - minY, 1.0); // 避免除零
    const qreal yScale = h / yRange;

    // 优化：只更新实际使用的顶点，而不是整个缓冲区
    const int vertexCountToUpdate = std::min(currentPointCount, m_maxPoints);
    
    // 增量更新：只更新新增的数据点
    const int startIndex = isNewNode || needsReallocation ? 0 : m_lastPointCount;
    
    // 优化循环：减少函数调用和计算
    for (int i = startIndex; i < vertexCountToUpdate; ++i) {
        const QPointF &p = m_points.at(i);
        
        // 使用实际数据点的X坐标，而不是索引
        const qreal relativeX = p.x() - minX;
        
        // 优化X坐标计算：使用预计算的缩放因子
        vertices[i].x = relativeX * xScale;
        
        // 优化Y坐标计算：避免重复除法
        vertices[i].y = h - ((p.y() - minY) * yScale);
    }

    // 如果数据点减少，需要清理多余顶点
    if (vertexCountToUpdate < m_lastPointCount && !needsReallocation) {
        // 使用memset清零更高效
        QSGGeometry::Point2D *endVertices = vertices + m_lastPointCount;
        QSGGeometry::Point2D *startVertices = vertices + vertexCountToUpdate;
        const size_t clearSize = (endVertices - startVertices) * sizeof(QSGGeometry::Point2D);
        memset(startVertices, 0, clearSize);
    }

    // 只标记实际使用的顶点数据为脏
    geometry->markVertexDataDirty();
    node->markDirty(QSGNode::DirtyGeometry);

    m_lastPointCount = currentPointCount;
    m_geometryDirty = false;
    
    return node;
}

void CurveItem::updateGeometry()
{
    calculateBounds();
}

void CurveItem::calculateBounds()
{
    if (m_points.isEmpty()) {
        m_actualMinY = 0.0;
        m_actualMaxY = 100.0;
        return;
    }

    m_actualMinY = m_points.first().y();
    m_actualMaxY = m_points.first().y();

    for (const QPointF &point : m_points) {
        if (point.y() < m_actualMinY) m_actualMinY = point.y();
        if (point.y() > m_actualMaxY) m_actualMaxY = point.y();
    }

    if (qFuzzyCompare(m_actualMinY, m_actualMaxY)) {
        m_actualMinY -= 1.0;
        m_actualMaxY += 1.0;
    }
}
