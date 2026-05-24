#include "CurveItem.h"
#include <QSGGeometry>
#include <QSGFlatColorMaterial>
#include <QQuickWindow>
#include <QThread>
#include <QTimer>
#include <algorithm>
#include <cstring>

CurveItem::CurveItem(QQuickItem *parent)
    : QQuickItem(parent)
    , m_lineColor(Qt::blue)
    , m_lineWidth(2.0)
    , m_maxPoints(1000)
    , m_autoScale(true)
    , m_minXValue(0.0)
    , m_maxXValue(55.0)
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

    m_updateTimer->setSingleShot(true);
    m_updateTimer->setInterval(16);

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

    QList<QPointF> newPoints;
    newPoints.reserve(points.size());
    for (const QVariant &point : points) {
        if (point.canConvert<QPointF>()) {
            newPoints.append(point.toPointF());
        } else if (point.canConvert<QVariantList>()) {
            const QVariantList pair = point.toList();
            if (pair.size() >= 2) {
                newPoints.append(QPointF(pair[0].toDouble(), pair[1].toDouble()));
            }
        }
    }

    if (newPoints.size() > m_maxPoints) {
        newPoints = newPoints.mid(newPoints.size() - m_maxPoints);
    }

    m_points = newPoints;
    m_pendingDataPoints.clear();
    calculateBounds();
    m_geometryDirty = true;
    emit dataPointsChanged();

    if (!m_updateTimer->isActive()) {
        update();
        m_updateTimer->start();
    } else {
        m_pendingUpdate = true;
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

qreal CurveItem::minXValue() const
{
    return m_minXValue;
}

void CurveItem::setMinXValue(qreal min)
{
    if (m_minXValue != min) {
        m_minXValue = min;
        emit minXValueChanged();
        update();
    }
}

qreal CurveItem::maxXValue() const
{
    return m_maxXValue;
}

void CurveItem::setMaxXValue(qreal max)
{
    if (m_maxXValue != max) {
        m_maxXValue = max;
        emit maxXValueChanged();
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

    m_points.append(QPointF(x, y));

    if (m_points.size() > m_maxPoints) {
        m_points.removeFirst();
    }

    if (m_autoScale) {
        if (wasEmpty) {
            m_actualMinY = y;
            m_actualMaxY = y;
        } else {
            if (y < m_actualMinY) m_actualMinY = y;
            if (y > m_actualMaxY) m_actualMaxY = y;
        }
    }

    m_geometryDirty = true;

    if (m_points.size() != m_lastPointCount) {
        emit dataPointsChanged();
    }

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

        if (!m_pendingDataPoints.isEmpty()) {
            m_updateTimer->start();
        }
    }
}

QSGNode *CurveItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    Q_UNUSED(data)

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

    if (isNewNode || needsReallocation) {
        if (!node) {
            node = new QSGGeometryNode;
            node->setFlag(QSGNode::OwnsGeometry);
            node->setFlag(QSGNode::OwnsMaterial);
        }

        QSGGeometry *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), currentPointCount);
        geometry->setLineWidth(m_lineWidth);
        geometry->setDrawingMode(QSGGeometry::DrawLineStrip);
        geometry->setVertexDataPattern(QSGGeometry::DynamicPattern);
        node->setGeometry(geometry);

        QSGFlatColorMaterial *material = new QSGFlatColorMaterial;
        material->setColor(m_lineColor);
        node->setMaterial(material);
    }

    QSGGeometry *geometry = node->geometry();
    if (geometry->vertexCount() == 0) {
        return node;
    }

    QSGGeometry::Point2D *vertices = geometry->vertexDataAsPoint2D();
    const qreal w = boundingRect().width();
    const qreal h = boundingRect().height();

    qreal minX = m_minXValue;
    qreal maxX = m_maxXValue;
    if (qFuzzyCompare(minX, maxX)) {
        minX = 0.0;
        maxX = 1.0;
    }
    const qreal xRange = std::max(maxX - minX, 1e-6);
    const qreal xScale = w / xRange;

    const qreal minY = m_autoScale ? m_actualMinY : m_minYValue;
    const qreal maxY = m_autoScale ? m_actualMaxY : m_maxYValue;
    const qreal yRange = std::max(maxY - minY, 1.0);
    const qreal yScale = h / yRange;

    const int vertexCountToUpdate = std::min(currentPointCount, m_maxPoints);

    const int startIndex = isNewNode || needsReallocation ? 0 : m_lastPointCount;

    for (int i = startIndex; i < vertexCountToUpdate; ++i) {
        const QPointF &p = m_points.at(i);

        const qreal relativeX = p.x() - minX;
        vertices[i].x = relativeX * xScale;
        vertices[i].y = h - ((p.y() - minY) * yScale);
    }

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
