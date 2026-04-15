#ifndef QUAZIPWIDGET_H
#define QUAZIPWIDGET_H

#include <QWidget>

namespace Ui {
class QuaZipWidget;
}

class QuaZipWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QuaZipWidget(QWidget *parent = nullptr);
    ~QuaZipWidget();

private:
    Ui::QuaZipWidget *ui;
};

#endif // QUAZIPWIDGET_H
