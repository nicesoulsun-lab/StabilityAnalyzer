#ifndef CURVEITEM_H
#define CURVEITEM_H

#include <QQuickItem>
#include <QSGGeometryNode>
#include <QSGFlatColorMaterial>
#include <QList>
#include <QPointF>
#include <QTimer>
#include "mainwindow_global.h"

/**
 * @brief The CurveItem class
 * @class CurveItem
 *
 * 自定义QQuickItem用于高性能实时曲线绘制
 *
 * 优点：
 * - 基于Qt Scene Graph的高性能渲染
 * - 支持动态数据点更新和实时绘制
 * - 自动缩放和手动缩放模式
 * - 数据点数量限制和内存管理
 * - 平滑的曲线渲染和动画效果
 *
 * 核心功能：
 * - 实时数据点添加和曲线更新
 * - 自动/手动Y轴范围调整
 * - 数据点缓存和性能优化
 * - 自定义线条颜色和宽度
 * - 数据清理和重置功能
 *
 * 渲染机制：
 * - 重写updatePaintNode()方法实现自定义渲染
 * - 使用QSGGeometryNode和QSGFlatColorMaterial
 * - 支持硬件加速的OpenGL渲染
 */
class MAINWINDOW_EXPORT CurveItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QColor lineColor READ lineColor WRITE setLineColor NOTIFY lineColorChanged)
    Q_PROPERTY(qreal lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)
    Q_PROPERTY(QVariantList dataPoints READ dataPoints WRITE setDataPoints NOTIFY dataPointsChanged)
    Q_PROPERTY(int maxPoints READ maxPoints WRITE setMaxPoints NOTIFY maxPointsChanged)
    Q_PROPERTY(bool autoScale READ autoScale WRITE setAutoScale NOTIFY autoScaleChanged)
    Q_PROPERTY(qreal minXValue READ minXValue WRITE setMinXValue NOTIFY minXValueChanged)
    Q_PROPERTY(qreal maxXValue READ maxXValue WRITE setMaxXValue NOTIFY maxXValueChanged)
    Q_PROPERTY(qreal minYValue READ minYValue WRITE setMinYValue NOTIFY minYValueChanged)
    Q_PROPERTY(qreal maxYValue READ maxYValue WRITE setMaxYValue NOTIFY maxYValueChanged)

public:
    explicit CurveItem(QQuickItem *parent = nullptr);
    ~CurveItem();

    // QML属性访问器
    QColor lineColor() const;
    void setLineColor(const QColor &color);
    qreal lineWidth() const;
    void setLineWidth(qreal width);
    QVariantList dataPoints() const;
    void setDataPoints(const QVariantList &points);
    int maxPoints() const;
    void setMaxPoints(int max);
    bool autoScale() const;
    void setAutoScale(bool autoScale);
    qreal minXValue() const;
    void setMinXValue(qreal min);
    qreal maxXValue() const;
    void setMaxXValue(qreal max);
    qreal minYValue() const;
    void setMinYValue(qreal min);
    qreal maxYValue() const;
    void setMaxYValue(qreal max);

    // 添加数据点的方法
    Q_INVOKABLE void addDataPoint(qreal x, qreal y);
    Q_INVOKABLE void clearData();
    Q_INVOKABLE void updateCurve();

signals:
    void lineColorChanged();
    void lineWidthChanged();
    void dataPointsChanged();
    void maxPointsChanged();
    void autoScaleChanged();
    void minXValueChanged();
    void maxXValueChanged();
    void minYValueChanged();
    void maxYValueChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;

private:
    void updateGeometry();
    void calculateBounds();
    void updateVertexData(QSGGeometryNode *node);
    QSGGeometryNode* createNewGeometryNode();
    void updateExistingGeometry(QSGGeometryNode *node);

private:
    void performPendingUpdate();
    
private:
    QColor m_lineColor;
    qreal m_lineWidth;
    QList<QPointF> m_points;
    int m_maxPoints;
    bool m_autoScale;
    qreal m_minXValue;
    qreal m_maxXValue;
    qreal m_minYValue;
    qreal m_maxYValue;
    qreal m_actualMinY;
    qreal m_actualMaxY;
    bool m_geometryDirty;
    int m_lastPointCount; // 上次绘制的点数，用于增量更新
    qreal m_timeWindow;   // 时间窗口大小（秒）
    
    // 帧率限制相关
    QTimer *m_updateTimer;
    bool m_pendingUpdate;
    QList<QPointF> m_pendingDataPoints;
};

#endif // CURVEITEM_H
