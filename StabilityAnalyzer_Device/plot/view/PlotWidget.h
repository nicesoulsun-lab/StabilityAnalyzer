#pragma once

#include <QWidget>
#include "multifunctionplot.h"
#include "DataPipeControl.h"
class PlotWidget : public QWidget
{
	Q_OBJECT

public:
	PlotWidget(QWidget *parent);
	~PlotWidget();
	
	void paintEvent(QPaintEvent *e) override;

	void resizeEvent(QResizeEvent *e) override;
	
	MultifunctionPlot plot;
	AxisModel *xAxis;
	AxisModel *yAxis;
	DataPipeControl *pipeControl;
	QTimer timer;
};
