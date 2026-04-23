#ifndef ADVANCEDCALCULATIONENGINE_H
#define ADVANCEDCALCULATIONENGINE_H

#include <QVariantMap>
#include <QVector>

// 高级计算统一入口。
// QML 只负责收集参数和展示结果，具体公式都收敛到这里，便于后续统一校正口径。
class AdvancedCalculationEngine
{
public:
    // 颗粒迁移速度：基于分层结果中的沉降界面位移估算。
    static QVariantMap calculateMigrationRate(const QVariantMap& params);
    // 流体力学计算：根据 targetKey 反算五个目标量中的一个。
    static QVariantMap calculateHydrodynamic(const QVariantMap& params);
    // 光学计算：当前只支持在粒径和体积浓度之间互相反算。
    static QVariantMap calculateOptical(const QVariantMap& params);

    // data_ctrl 读取实验分层结果后，通过该重载把结果行传入分析层完成迁移速率计算。
    static QVariantMap calculateMigrationRate(const QVariantMap& params,
                                             const QVector<QVariantMap>& separationRows);
};

#endif // ADVANCEDCALCULATIONENGINE_H
