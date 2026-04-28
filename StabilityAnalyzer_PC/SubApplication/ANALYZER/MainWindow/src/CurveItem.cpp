#include "CurveItem.h"
#include <QDebug>
#include <QSGGeometry>
#include <QSGFlatColorMaterial>
#include <QVariantMap>
#include <algorithm>
#include <new>

namespace {
bool variantToPoint(const QVariant &value, QPointF &point)
{
    if (value.canConvert<QPointF>()) {
        point = value.toPointF();
        return true;
    }

    const QVariantList list = value.toList();
    if (list.size() >= 2) {
        bool xOk = false;
        bool yOk = false;
        const qreal x = list.at(0).toReal(&xOk);
        const qreal y = list.at(1).toReal(&yOk);
        if (xOk && yOk) {
            point = QPointF(x, y);
            return true;
        }
    }

    const QVariantMap map = value.toMap();
    if (map.isEmpty()) {
        return false;
    }

    bool xOk = false;
    bool yOk = false;
    const qreal x = map.value("x", map.value("timestamp")).toReal(&xOk);
    const qreal y = map.value("y", map.value("value")).toReal(&yOk);
    if (!xOk || !yOk) {
        return false;
    }

    point = QPointF(x, y);
    return true;
}
}

CurveItem::CurveItem(QQuickItem *parent)
    : QQuickItem(parent)
    , m_lineColor(Qt::blue)
    , m_lineWidth(2.0)
    , m_maxPoints(1000)
    , m_autoScale(true)
    , m_minXValue(0.0)
    , m_maxXValue(100.0)
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

CurveItem::~CurveItem() = default;

QColor CurveItem::lineColor() const { return m_lineColor; }

void CurveItem::setLineColor(const QColor &color)
{
    if (m_lineColor != color) {
        m_lineColor = color;
        emit lineColorChanged();
        update();
    }
}

qreal CurveItem::lineWidth() const { return m_lineWidth; }

void CurveItem::setLineWidth(qreal width)
{
    if (!qFuzzyCompare(m_lineWidth, width)) {
        m_lineWidth = width;
        m_geometryDirty = true;
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
            clearData();
        }
        return;
    }

    QList<QPointF> convertedPoints;
    const int maxRenderedPoints = m_maxPoints > 0 ? m_maxPoints : points.size();
    const int targetPointCount = std::min(points.size(), maxRenderedPoints);

    try {
        convertedPoints.reserve(targetPointCount);
        for (int i = 0; i < targetPointCount; ++i) {
            const int sourceIndex = targetPointCount == points.size()
                    ? i
                    : static_cast<int>(static_cast<double>(i) * (points.size() - 1) / std::max(targetPointCount - 1, 1) + 0.5);
            QPointF point;
            if (variantToPoint(points.at(sourceIndex), point)) {
                convertedPoints.append(point);
            }
        }
    } catch (const std::bad_alloc &) {
        qWarning() << "[CurveItem] insufficient memory while converting dataPoints"
                   << "inputSize=" << points.size()
                   << "maxRenderedPoints=" << maxRenderedPoints;
        return;
    }

    if (convertedPoints.isEmpty()) {
        clearData();
        return;
    }

    if (m_points == convertedPoints) {
        return;
    }

    m_points = convertedPoints;
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

int CurveItem::maxPoints() const { return m_maxPoints; }

void CurveItem::setMaxPoints(int max)
{
    if (m_maxPoints != max && max > 0) {
        m_maxPoints = max;
        emit maxPointsChanged();
    }
}

bool CurveItem::autoScale() const { return m_autoScale; }

void CurveItem::setAutoScale(bool autoScale)
{
    if (m_autoScale != autoScale) {
        m_autoScale = autoScale;
        m_geometryDirty = true;
        emit autoScaleChanged();
        update();
    }
}

qreal CurveItem::minXValue() const { return m_minXValue; }

void CurveItem::setMinXValue(qreal min)
{
    if (!qFuzzyCompare(m_minXValue, min)) {
        m_minXValue = min;
        m_geometryDirty = true;
        emit minXValueChanged();
        update();
    }
}

qreal CurveItem::maxXValue() const { return m_maxXValue; }

void CurveItem::setMaxXValue(qreal max)
{
    if (!qFuzzyCompare(m_maxXValue, max)) {
        m_maxXValue = max;
        m_geometryDirty = true;
        emit maxXValueChanged();
        update();
    }
}

qreal CurveItem::minYValue() const { return m_minYValue; }

void CurveItem::setMinYValue(qreal min)
{
    if (!qFuzzyCompare(m_minYValue, min)) {
        m_minYValue = min;
        m_geometryDirty = true;
        emit minYValueChanged();
        update();
    }
}

qreal CurveItem::maxYValue() const { return m_maxYValue; }

void CurveItem::setMaxYValue(qreal max)
{
    if (!qFuzzyCompare(m_maxYValue, max)) {
        m_maxYValue = max;
        m_geometryDirty = true;
        emit maxYValueChanged();
        update();
    }
}

void CurveItem::addDataPoint(qreal x, qreal y)
{
    m_points.append(QPointF(x, y));
    if (m_points.size() > m_maxPoints) {
        m_points.removeFirst();
    }
    calculateBounds();
    m_geometryDirty = true;
    emit dataPointsChanged();
    update();
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
    update();
}

void CurveItem::performPendingUpdate()
{
    if (m_pendingUpdate) {
        m_pendingUpdate = false;
        update();
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

    if (!node) {
        node = new QSGGeometryNode;
        node->setFlag(QSGNode::OwnsGeometry);
        node->setFlag(QSGNode::OwnsMaterial);
    }

    QSGGeometry *geometry = node->geometry();
    if (!geometry) {
        geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), currentPointCount);
        geometry->setDrawingMode(QSGGeometry::DrawLineStrip);
        geometry->setVertexDataPattern(QSGGeometry::DynamicPattern);
        node->setGeometry(geometry);
    } else if (geometry->vertexCount() != currentPointCount) {
        geometry->allocate(currentPointCount);
    }

    geometry->setLineWidth(m_lineWidth);
    geometry->setDrawingMode(QSGGeometry::DrawLineStrip);
    geometry->setVertexDataPattern(QSGGeometry::DynamicPattern);

    auto *material = static_cast<QSGFlatColorMaterial *>(node->material());
    if (!material) {
        material = new QSGFlatColorMaterial;
        node->setMaterial(material);
    }
    material->setColor(m_lineColor);

    QSGGeometry::Point2D *vertices = geometry->vertexDataAsPoint2D();
    const qreal w = boundingRect().width();
    const qreal h = boundingRect().height();

    qreal minX = m_minXValue;
    qreal maxX = m_maxXValue;
    if (qFuzzyCompare(minX, maxX)) {
        minX = m_points.first().x();
        maxX = m_points.last().x();
        if (qFuzzyCompare(minX, maxX)) {
            minX -= 1.0;
            maxX += 1.0;
        }
    }

    const qreal minY = m_autoScale ? m_actualMinY : m_minYValue;
    const qreal maxY = m_autoScale ? m_actualMaxY : m_maxYValue;
    const qreal xRange = std::max(maxX - minX, 1.0);
    const qreal yRange = std::max(maxY - minY, 1.0);

    for (int i = 0; i < currentPointCount; ++i) {
        const QPointF &p = m_points.at(i);
        vertices[i].x = (p.x() - minX) / xRange * w;
        vertices[i].y = h - ((p.y() - minY) / yRange * h);
    }

    geometry->markVertexDataDirty();
    node->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
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
        m_actualMinY = std::min(m_actualMinY, point.y());
        m_actualMaxY = std::max(m_actualMaxY, point.y());
    }

    if (qFuzzyCompare(m_actualMinY, m_actualMaxY)) {
        m_actualMinY -= 1.0;
        m_actualMaxY += 1.0;
    }
}
