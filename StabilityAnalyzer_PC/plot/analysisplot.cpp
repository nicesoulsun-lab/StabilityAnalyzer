#include "analysisplot.h"

AnalysisPlot::AnalysisPlot(QQuickItem *parent) : Oscilloscope(parent)
{
    xAxis = aModel->addXAxis();
    xAxis->setAutoRange(false);
    yAxis = aModel->addYAxis();
    gModel->setXAxis(xAxis);
    gModel->setYAxis(yAxis);
}

void AnalysisPlot::init(const QString &url, QVariantList paramList, int analysisType)
{
    QString url2 = qApp->applicationDirPath() +"/output/";
    if(!quit()){
        qDebug()<<"重置绘图线程失败";
        return;
    }
    paramList = {0};
    QFile file(url+"all.csv");
    file.open(QIODevice::ReadOnly);
    QByteArrayList list = file.readAll().split('\n');
    if(list.count() < 2)
    {
        return;
    }
    QByteArrayList variableName = list[1].split(',');
    //QByteArrayList variableName = {"变量1"};

    if(variableName.isEmpty()||paramList.last().toInt()>variableName.size()){
        qDebug()<<"参数索引 > all.csv";
        return;
    }

    /* 初始化图表 */
    cModel->removeAll();
    for(int i = 0; i<paramList.size(); i++){
        CurveModel *curve = cModel->addCurve();
        curve->setXAxis(xAxis);
        curve->setYAxis(yAxis);
        curve->initSource(1000000); //设置最大容量（100w个点）
        curve->setTitle(variableName.at(paramList.at(i).toInt()));
    }

    /* 初始化数据 */
    QList<QList<qreal>> source;

    /* 1.从压缩文件中读数据 */
    for(int i = 0; i<paramList.size(); i++){
       source.push_back(readByCSV(url2+QString::number(paramList.at(i).toInt()+1)+".csv"));
    }
    /* 2.将数据做fft */

    /* 设置x轴的坐标范围 */
    xAxis->setRange(0,102400);
    /* 3.放入图表中 */
    start();
    pushDatas(source);
}

QList<qreal> AnalysisPlot::readByCSV(const QString &url)
{

    QList<qreal> source;
    QFile file(url);
    file.open(QIODevice::ReadOnly);
    char c[50] = {0};
    int len = 0;
    while (true) {
        int l = file.read(c+len,1);
        len+=l;
        if(l>0&&len<50){
            if(c[len-1]==','){
                source.push_back(QByteArray(c,len-1).toDouble());
                len = 0;
            }
        }else{
            break;
        }
    }
    qDebug() <<__FUNCTION__<< source.length();
    return source;
}

