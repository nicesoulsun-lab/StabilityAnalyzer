#ifndef ENCRYPTIONALGORITHM_H
#define ENCRYPTIONALGORITHM_H

#include <QObject>
#include <QWidget>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QAction>
//#include <QNetworkInterface>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTime>
#include <QTextStream>
#include <QDebug>
#include <QDateTime>
#include <QProcess>
#include <QClipboard>
#include <QSettings>
#include <QPalette>
#include <QPainter>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QCryptographicHash>

class encryptionAlgorithm : public QObject
{
    Q_OBJECT
public:
    explicit encryptionAlgorithm(QObject *parent = nullptr);
    QByteArray encryption(QByteArray data);
    QByteArray decryption(QByteArray data);

signals:

};
encryptionAlgorithm *getCryptionInstance();
#endif // ENCRYPTIONALGORITHM_H
