#include "tickers.h"

Tickers::Tickers(QObject *parent) : QObject(parent)
{

}

void Tickers::getEffectiveValue(qreal num, qreal *val, int *pos)
{
    bool greaterZero = num>0? true : false;
    num = qAbs(num);
    int p;
    if(num>1){
        p = 0;
        while(num/=10.0){
            if(num>1){
                p++;
            }else{
                *val = num*10.0;
                break;
            }
        }
    }else{
        p = -1;
        while (num*=10.0) {
            if(num<1){
                p--;
            }else{
                *val = num;
                break;
            }
        }
    }
    *pos = p;
    if(!greaterZero)
        *val*=-1;
}

int Tickers::getNumByPos(qreal num, int pos)
{
    if(pos>=0){
        for(int i = 0;i<pos;i++){
            num /= 10;
        }
        return (int)num%10;
    }else{
        QString str = QString::number(num);
        int index = str.indexOf(".");
        if(index!=-1){
            str = QString(str.at(index-pos));
        }else{
            return 0;
        }
        return str.toInt();
    }
}

qreal Tickers::getStep(qreal min, qreal max, int sum)
{
    qreal val; int pos;
    qreal len = max-min;    //最大最小值之间的差值
    getEffectiveValue(len/sum,&val,&pos);  //转为科学计数法，val * 10 ^ pos

    /* 取两位小数 */
    QString str= QString::asprintf("%.2f", val);
    double f = str.toDouble();
    /* 获取小数点后一位的数字 */
    int num = getNumByPos(f,-1);
    /* 重新定义小数点后一位的值 */
    if(num<=3){
        f-=(num/10.0);
    }else if(num>=7){
        f+=((10-num)/10.0);
    }else{
        f+=(5-num)/10.0;
    }

    /* 截取两位有效数字 */
    QString str2 = QString::number(f,'f');
    int index = str2.indexOf(".");
    if(index != -1){
        str2 = str2.mid(0,index+2);
    }
    f = str2.toFloat();
    if(pos>0){
        for(int i = 0; i<pos; i++){
            f*=10.0;
        }
    }else{
        for(int i = pos; i<0; i++){
            f/=10.0;
        }
    }
    return f;
}

QList<qreal> Tickers::getTickNumber(AxisModel *axis, QList<QString> *textList)
{
    int sum = axis->mainTickSum();
    if (sum==0)
        return QList<qreal>();
    qreal lower = axis->lower();
    qreal upper = axis->upper();
    qreal step,begin;
    bool showTime = false;
    if(axis->labelType()==2){
        if(upper-lower<=24*3600){
            showTime = true;
        }
        step = (upper-lower)/axis->mainTickSum();
        axis->setStep(step);
        begin = lower;
    }else{
        step = getStep(lower,upper,sum);
        begin = qAbs(lower);
        if(step<0.0000001)
            step=0.0000001;
        axis->setStep(step);
        if(step==0){
            return QList<qreal>();
        }
        int flag = begin/step;
        begin = flag*step;
        if(lower<0){
            begin*=-1;
            begin-=step;
        }
    }

    QList<qreal> list;
    qreal saveNum = begin;
    /*是否显示毫秒*/
    bool showMS = (axis->upper()-axis->lower())*axis->sample()/axis->mainTickSum()<1000;
    for(qreal i = 0; saveNum<upper; i++){
        saveNum = begin+step*i;
        if(qAbs(saveNum-0)<0.0000001)
            saveNum = 0;
        QString str = QString("%1").arg(saveNum);
        list.push_back(saveNum);
        if(textList==nullptr){
        }else if(axis->labelType()==0){
            textList->push_back(QString("%1").arg(saveNum));
        }else if(axis->labelType()==1){
            int ms = saveNum*axis->sample();
            int s = ms/1000;
            int m = s/60;
            int rs = s-m*60;
            int rms = ms-rs*1000;
            QString str = "";
            if(m!=0){
                str.append(QString::number(m));
                str.append(":");
            }
            if(rs<10&&m!=0){
                str.append("0");
            }
            str.append(QString::number(rs));
            if(showMS){
                QString str2 = QString::number(rms);
                if(str2!="")
                    str.append("."+str2);

            }
            textList->push_back(str);
        }else if(axis->labelType()==2){
            int timeStamp = saveNum;
            QDateTime date = QDateTime::fromTime_t(timeStamp);
            if(showTime){
                textList->push_back(date.toString("hh:mm:ss"));
            }else{
                textList->push_back(date.toString("yyyy/MM/dd"));
            }
        }

    }
    return list;
}

