#ifndef CURSORLISTMODEL_H
#define CURSORLISTMODEL_H

#include <QAbstractListModel>
#include "cursormodel.h"
#include <QJsonArray>

//#if _MSC_VER >= 1600
//#pragma execution_character_set("utf-8")
//#endif

class CursorListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(bool init READ init WRITE setInit NOTIFY initChanged)
    Q_PROPERTY(qreal pos READ pos NOTIFY posChanged)
    Q_PROPERTY(QVariantMap info READ info  NOTIFY infoChanged)
public:
    explicit CursorListModel(QObject *parent = 0);
    ~CursorListModel();
    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void add(CursorModel *model);

    void pop();

    QHash<int, QByteArray> roleNames()const override;

    void update();
    void setCurveModel(CurveListModel *curveModel);

    bool visible() const;
    void setVisible(bool visible);

    bool init() const;
    void setInit(bool init);

    qreal pos() const;

    AxisModel *xAxis() const;
    void setXAxis(AxisModel *xAxis);

    CursorModel* getModel(int index);

    virtual QVariantMap info(){return QVariantMap();}

    Q_INVOKABLE void clear();


    Q_INVOKABLE void addLabel(QJsonArray &array);
    /* 取消选中 */
    Q_INVOKABLE void clearChecked();
    /* 全选 */
    Q_INVOKABLE void checkedAll();
    /* 删除选中游标标注 */
     Q_INVOKABLE void removeLabel();
    /* 删除全部游标标注 */
     Q_INVOKABLE void removeAllLabel();

signals:
    void visibleChanged();
    void initChanged();
    void posChanged();
    void infoChanged();
    void zoomChanged();
protected:
    bool m_init = false;
    QVector <CursorModel*> modelData;
    QHash<int, QByteArray> m_roles;
    CurveListModel* m_curveModel = nullptr;
    AxisModel *m_xAxis = nullptr;
    bool m_visible = false;
};

#endif // CURSORLISTMODEL_H
