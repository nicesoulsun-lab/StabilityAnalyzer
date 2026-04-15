#include "QuaZipWidget.h"
#include "ui_QuaZipWidget.h"

QuaZipWidget::QuaZipWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QuaZipWidget)
{
    ui->setupUi(this);
}

QuaZipWidget::~QuaZipWidget()
{
    delete ui;
}
