#include "PlotWidget.h"

PlotWidget::PlotWidget(QWidget *parent)
	: QWidget(parent)
{
	connect(&plot, &MultifunctionPlot::plotChanged, this, [=]() {
		update();
	});
	AxisListModel* axisModel = new AxisListModel(this);
	CurveListModel* curveModel = new CurveListModel(this);
	GridModel *gridModel = new GridModel(this);

	plot.setAxisModel(axisModel);
	plot.setCurveModel(curveModel);
	plot.setGrid(gridModel);
	xAxis = axisModel->addXAxis();
	xAxis->setLower(0);
	xAxis->setUpper(1000);
	xAxis->setAutoRange(false);
	yAxis = axisModel->addYAxis();
	gridModel->setXAxis(xAxis);
	gridModel->setYAxis(yAxis);
	pipeControl = DataPipeControl::instance();
	foreach (CurveInfo info,pipeControl->curveInfo()) {
		CurveModel *curve = curveModel->addCurve();
		//curve->setDataType(1);
		//curve->setLineColor(info.color);
		curve->setXAxis(xAxis);
		curve->setYAxis(yAxis);
		curve->initSource(info.xSource,info.ySource);
	}
	timer.setInterval(100);
	plot.setStep(10000);
	connect(&timer,&QTimer::timeout,&plot,&MultifunctionPlot::goAhead);
	connect(&timer, &QTimer::timeout, curveModel, &CurveListModel::flash);
	timer.start();
}

PlotWidget::~PlotWidget()
{
}

void PlotWidget::paintEvent(QPaintEvent * e)
{
	QPainter painter(this);
	painter.setPen(Qt::white);
	painter.setBrush(Qt::white);
	painter.drawRect(0, 0, width(), height());
	plot.draw(&painter);
}

void PlotWidget::resizeEvent(QResizeEvent * e)
{
	plot.resize(width(), height());
}
