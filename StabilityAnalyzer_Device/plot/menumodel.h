#ifndef MENUMODEL_H
#define MENUMODEL_H

#include <QAbstractListModel>
#include <QVariant>
#include <QPoint>

class MenuModel;

class MenuInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int size READ size NOTIFY sizeChanged)
    Q_PROPERTY(MenuModel* children READ children NOTIFY childrenChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QPoint pos READ pos  WRITE setPos NOTIFY posChanged)
    Q_PROPERTY(bool choose READ choose WRITE setChoose NOTIFY chooseChanged)
public:
    MenuInfo(MenuInfo *parent = nullptr);
    void addChild(MenuInfo *child);

    int size();

    QString name() const;
    void setName(const QString &name);

    bool enabled() const;
    void setEnabled(bool enabled);

    MenuModel *children() const;

    Q_INVOKABLE void click();

    QPoint pos() const;
    void setPos(const QPoint &pos);

    bool choose() const;
    void setChoose(bool choose);

signals:
    void clickSignal();
    void sizeChanged();
    void enabledChanged();
    void nameChanged();
    void childrenChanged();
    void posChanged();
    void chooseChanged();
private:
    bool m_enabled = true;
    bool m_choose = false;
    QString m_name;
    MenuModel* m_children;
    QPoint m_pos;
};

class MenuModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit MenuModel(QObject *parent = nullptr);
    ~MenuModel();
    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames()const override;

    void add(MenuInfo *info);

signals:

private:
    QVector<MenuInfo *> modelData;
    QHash<int, QByteArray> m_roles;
};

#endif // MENUMODEL_H
