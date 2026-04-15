#include "inc/frequencyAnalysis/fouriertransform.h"

FourierTransform::FourierTransform(QObject *parent) : QObject(parent)
{

}
/**************************************************************/
//x:存放要变换数据的实部。
//y:存放要变换数据的虚部。
//a:存放变换结果的实部。
//b:存放变换结果的虚部。
//n:数据长度。
//sign:sign==1计算离散傅立叶正变换，sign==-1计算离散傅立叶反变换
/**************************************************************/
bool FourierTransform::DFT(float *x, float *y, float *a, float *b, int n, bool sign)
{
    int i,k;
    double d,q,w;
    q=6.28318530718/n;
    double c,s;

    if(sign!=-1)
        sign=1;

    for(k=0;k<n;k++)
    {
        w=k*q;
        a[k]=b[k]=0.0;
        for(i=0;i<n;i++)
        {
            d=i*w;
            c=cos(d);
            s=sin(d)*sign;
            a[k]+=(float)(c*x[i]+s*y[i]);
            b[k]+=(float)(c*y[i]-s*x[i]);
        }
    }
    if(sign==-1)
    {
        c=(float)(1.0/n);
        for(k=0;k<n;k++)
        {
            a[k]=(float)(a[k]*c);
            b[k]=(float)(b[k]*c);
        }
    }
    return true;
}
/**************************************************************/
//x:存放要变换数据的实部。
//y:存放要变换数据的虚部。
//n:数据长度。
//sign:sign==1计算离散傅立叶正变换，sign==-1计算离散傅立叶反变换
/**************************************************************/
bool FourierTransform::FFT(float *x, float *y, int n, bool sign)
{
    int i,j,k,l,m,n1,n2;
    float c,c1,e,s,s1,t,tr,ti;

    if(sign!=-1)
        sign=1;

    for(j=1,i=1;i<16;i++)
    {
        m=i;
        j=2*j;
        if(j==n)
            break;
        if(j>n)
            return false;
    }
    n1=n-1;
    for(j=0,i=0;i<n1;i++)
    {
        if(i<j)
        {
            tr=x[j];
            ti=y[j];
            x[j]=x[i];
            y[j]=y[i];
            x[i]=tr;
            y[i]=ti;
        }
        k=n/2;
        while(k<(j+1))
        {
            j=j-k;
            k=k/2;
        }
        j=j+k;
    }
    n1=1;
    for(l=1;l<=m;l++)
    {
        n1=2*n1;
        n2=n1/2;
        e=(float)(3.1415926/n2);
        c=1.0;
        s=0;
        c1=(float)cos(e);
        s1=(float)(-sign*sin(e));
        for(j=0;j<n2;j++)
        {
            for(i=j;i<n;i+=n1)
            {
                k=i+n2;
                tr=c*x[k]-s*y[k];
                ti=c*y[k]+s*x[k];
                x[k]=x[i]-tr;
                y[k]=y[i]-ti;
                x[i]=x[i]+tr;
                y[i]=y[i]+ti;
            }
            t=c;
            c=c*c1-s*s1;
            s=t*s1+s*c1;
        }
    }
    if(sign==-1)
    {
        for(i=0;i<n;i++)
        {
            x[i]/=n;
            y[i]/=n;
        }
    }
    return true;
}

bool FourierTransform::MyDFT(float *x, float *y, float *a, float *b, int n, bool sign,double bf,double ef)
{
    int i,k;
    double d,q,w;

    float c,s;
    if(ef<=bf)
        return false;
    double rate=22050.0/(ef-bf);
    double off=rate*n*bf/22050.0;

    q=3.141592653589793238/n/rate;
    if(sign==1)
    {
        for(k=0;k<n;k++)
        {
            w=(k+off)*q;
            a[k]=b[k]=0.0;
            for(i=0;i<n;i++)
            {
                d=i*w;
                c=(float)cos(d);
                s=(float)sin(d);
                a[k]+=c*x[i];
                b[k]+=-s*x[i];
            }
        }
    }
    if(sign==-1)
    {
        for(k=0;k<n;k++)
        {
            w=(k+off)*q;
            a[k]=b[k]=0.0;
            for(i=0;i<n;i++)
            {
                d=i*w;
                c=(float)cos(d);
                s=-(float)sin(d);
                a[k]+=c*x[i]+s*y[i];
                b[k]+=c*y[i]-s*x[i];
            }
        }
        c=(float)(1.0/n);
        for(k=0;k<n;k++)
        {
            a[k]=a[k]*c;
            b[k]=b[k]*c;
        }
    }
    return true;
}

//计算2的n次方
int FourierTransform::Power2(int n)
{
   int i,p=1;
   for(i=1;i<=n;i++)p=p*2;
   return p;
}

//求复数序列a的倒序
//L 复数序列a长度
void FourierTransform::Inverse(float *a[2],int L )
{
    int i,k,Num1,n1,N1;
    float t1;
    Num1 = 0;
    N1=L;
    for (i=1;i<=N1-1;i++){
        k = Num1;
        n1 = N1;
        while (n1/2<=k){
          n1=n1/2;
          k=k-n1;
        }
        k = k + n1 / 2;
        Num1 = k;

        if (Num1 > i) {     //Num1为i的反序，只有Num1>i时才交换顺序
            t1 = a[0][i];
            a[0][i] = a[0][Num1];
            a[0][Num1] = t1;
            t1 = a[1][i];
            a[1][i] = a[1][Num1];
            a[1][Num1] = t1;
        }
    }
}


bool FourierTransform::MyFFT(float *x, float *y, float *a, float *b, int n,bool sign)
{
    int i,j,k,l,m,n1,n2;
    float c,c1,e,s,s1,t,tr,ti;

    if(sign!=-1)
        sign=1;

    for(j=1,i=1;i<16;i++)
    {
        m=i;
        j=2*j;
        if(j==n)
            break;
        if(j>n)
            return false;
    }
    n1=n-1;
    for(j=0,i=0;i<n1;i++)
    {
        if(i<j)
        {
            tr=x[j];
            ti=y[j];
            x[j]=x[i];
            y[j]=y[i];
            x[i]=tr;
            y[i]=ti;
        }
        k=n/2;
        while(k<(j+1))
        {
            j=j-k;
            k=k/2;
        }
        j=j+k;
    }
    n1=1;
    for(l=1;l<=m;l++)
    {
        n1=2*n1;
        n2=n1/2;
        e=(float)(3.1415926/n2);
        c=1.0;
        s=0;
        c1=(float)cos(e);
        s1=-(float)sign*sin(e);
        for(j=0;j<n2;j++)
        {
            for(i=j;i<n;i+=n1)
            {
                k=i+n2;
                tr=c*x[k]-s*y[k];
                ti=c*y[k]+s*x[k];
                x[k]=x[i]-tr;
                y[k]=y[i]-ti;
                x[i]=x[i]+tr;
                y[i]=y[i]+ti;
            }
            t=c;
            c=c*c1-s*s1;
            s=t*s1+s*c1;
        }
    }
    if(sign==-1)
    {
        for(i=0;i<n;i++)
        {
            x[i]/=n;
            y[i]/=n;
        }
    }
    return true;
}


bool FourierTransform::MyDFT(float *x, float n, double freq, double &Re, double &Im)
{
    int i=0,size=(long)n;
    double d=0,w=0;
    float c=0,s=0;
//	if(n-size >0.5)
//		size++;

    //正变换
    Re=0;
    Im=0;
    w=freq*3.141592653589793238/22050.0;

    for(i=0;i<size;i++)
    {
        d=i*w;
        c=(float)cos(d);
        s=(float)sin(d);
        Re+=c*x[i];
        Im+=-s*x[i];
    }

    d=(n-1)*w;
    c=n-size;
    s=1-c;
    Re+=cos(d)*c*(x[i-1]*s+x[i]*c);
    Im+=-sin(d)*c*(x[i-1]*s+x[i]*c);/**/


    return true;
}

bool FourierTransform::ShortTimeDFT(float *pS, float *pD, long size, double freq)
{
    double angle=0.0;
    double fWaveLength=44100.0/freq;
    long nWaveLength=1+(long)fWaveLength;
    if(nWaveLength%2 ==0 )
        nWaveLength++;//设置为奇数

    float *pLow=pS;
    float *pMiddle=pLow+(nWaveLength/2);
    float *pHigh=pLow+nWaveLength;

    long i=0;
    double d,q,w;
    float c,s;

    //正变换
    double Re=0;
    double Im=0;
    //w=off*q;
    w=freq*3.141592653589793/22050.0;

    for(i=0;i<nWaveLength;i++)
    {
        d=i*w;
        c=cos(d);
        s=sin(d);
        Re+=c*pLow[i];
        Im+=-s*pLow[i];
        //Im+=s*x[i];
    }
    return 1;
}

void FourierTransform::rfft(float *x,long n)
{
    int i,j,k,m,i1,i2,i3,i4,n1,n2,n4;
    double a,e,cc,ss,xt,t1,t2;
    for (j=1,i=1;i<16;i++)
    {
        m=i;
        j=2*j;
        if(j==n)
            break;
    }
    n1=n-1;
    for (j=0,i=0;i<n1;i++)
    {
        if(i<j)
        {
            xt=x[j];
            x[j]=x[i];
            x[i]=xt;
        }
        k=n/2;
        while(k<(j+1))
        {
            j=j-k;
            k=k/2;
        }
        j=j+k;
    }
    for(i=0;i<n;i+=2)
    {
        xt=x[i];
        x[i]=xt+x[i+1];
        x[i+1]=xt-x[i+1];
    }
    n2=1;
    for(k=2;k<=m;k++)
    {
        n4=n2;
        n2=2*n4;
        n1=2*n2;
        e=6.28318530718/n1;
        for(i=0;i<n;i+=n1)
        {
            xt=x[i];
            x[i]=xt+x[i+n2];
            x[i+n2]=xt-x[i+n2];
            x[i+n2+n4]=-x[i+n2+n4];
            a=e;
            for(j=1;j<=(n4-1);j++)
            {
                i1=i+j;
                i2=i-j+n2;
                i3=i+j+n2;
                i4=i-j+n1;
                cc=cos(a);
                ss=sin(a);
                a=a+e;
                t1=cc*x[i3]+ss*x[i4];
                t2=ss*x[i3]-cc*x[i4];
                x[i4]=x[i2]-t2;
                x[i3]=-x[i2]-t2;
                x[i2]=x[i1]-t1;
                x[i1]=x[i1]+t1;
            }
        }
    }
}
