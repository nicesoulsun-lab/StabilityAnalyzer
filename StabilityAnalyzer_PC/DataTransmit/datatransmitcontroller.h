#ifndef DATATRANSMITCONTROLLER_H
#define DATATRANSMITCONTROLLER_H

#include <QObject>

class DataTransmitController : public QObject
{
    Q_OBJECT
public:
    explicit DataTransmitController(QObject *parent = nullptr);

signals:

};

#endif // DATATRANSMITCONTROLLER_H
