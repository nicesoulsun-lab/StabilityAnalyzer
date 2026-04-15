#include "inc/envelopeAnalysis/hilberttransform.h"

HilbertTransform::HilbertTransform(QObject *parent) : QObject(parent)
{
    m_pFilterBuf=0;
    m_FilterBufSize=0;
}
HilbertTransform::~HilbertTransform()
{
    if(m_pFilterBuf !=0)
        delete [] m_pFilterBuf;
}
double HilbertTransform::blackman(long n,long i)
{
    return 0.42-0.5*cos(2*i*PI/(n-1))+0.08*cos(4*i*PI/(n-1));
}

long HilbertTransform::Create(long length,long size)
{
    long temp=1;
    if(length>=size)
        return 0;
    if(length<=0)
        return 0;
    while(temp < size)
        temp=temp<<1;
    if(temp != size)
        return 0;
    //size必须为2的N次方。
    if( (length&1) == 0 )
        return 0;
    //length必须为奇数。

    float *pX=0,*pY=0;
    m_FilterBufSize=size;
    m_FilterLength=length;
    pX=new float[size];
    pY=new float[size];
    long i=0;
    for(i=0;i<size;i++)
        pX[i]=pY[i]=0;
    //
    long l2=length>>1;//63
    long n2=size>>1;//512

    //中心点在pBuf[n2]

    for(i=1;i<=l2;i+=2)
        pX[n2+i]=2.0/(PI*i)*sin(0.5*i*PI);

    int flag=0;
    for(i=1;i<=l2;i+=2)
    {
        if(flag)
            pX[n2+i]=-pX[n2+i];
        pX[n2+i]*=(float)blackman(length,l2-i);
        pX[n2-i]=-pX[n2+i];

        flag=!flag;
    }
    //
    FourierTransform::FFT(pX,pY,size,1);
    //
    delete[] pX;
    m_pFilterBuf=pY;
    return size;
}

bool HilbertTransform::Transform(float *pX, float *pY, long size)
{
    if(size != m_FilterBufSize)
        return 0;

    long i=0;
    long n2=size>>1;
    //圆周卷积
    FourierTransform::FFT(pX,pY,size,1);
    float Re,Im;
    for(i=0;i<=n2;i++)//针对实数信号
    {
        //complex::Multiply(pY[i],pX[i],m_pFilterBuf[i],Re2=0,pY[i],pX[i]);
        Re= -pY[i]*m_pFilterBuf[i];
        Im=  pX[i]*m_pFilterBuf[i];
        pX[i]=Re;
        pY[i]=Im;
    }
    for(i=1;i<n2;i++)//针对实数信号
    {
        pX[size-i]=  pX[i];
        pY[size-i]= -pY[i];
    }
    FourierTransform::FFT(pX,pY,size,-1);

    float swap=0;

    for(i=0;i<n2;i++)//针对实数信号
    {
        swap=pX[i];
        pX[i]=pX[i+n2];
        pX[i+n2]=swap;
    }

    //除去头尾
    long l2=m_FilterLength>>1;
    for(i=0;i<l2;i++)
    {
        pX[i]=pX[size-i-1]=0;
    }
    return 1;
}

void HilbertTransform::Convolution(float *datain,int dataLen)
{
    float *h;
    //Hilbert变换器的单位抽样响应
    for (auto i = 0; i <= dataLen; i++)
    {
        if(i%2==0)
        {
            h[i]=0;
        }
        else
        {
            h[i]=2/(i*PI);
        }
    }
//    int Scala=64;
//    double Delay = Scala / 2.0;
//    int fs=1000;
//    double *ImpactData;//1/(PI*t)
//    double AngFreq = 2.0 * PI * (lFreq / Freq);
//    float *RawData;
//    for (auto i = 0; i <= dataLen; i++) {
//        double s = i - Delay;
//        ImpactData[i] = (sin(PI * s) - sin(AngFreq * s)) / (PI * s);
//        ImpactData[Scala - i] = ImpactData[i];
//    }
//    if(1 == Mid)
//        ImpactData[Scala / 2] = 1.0 - AngFreq / PI;

//    RawData = new float[static_cast<uint>(dataLen)];
//    OutHTData = new double[static_cast<uint>(dataLen)];
//    memcpy(RawData, datain, static_cast<uint>(dataLen) * sizeof (float));
//    if(dataLen > Scala) {
//        for (auto i = 0; i < dataLen; i++) {
//            OutHTData[i] = 0;
//            double sum = 0;
//            if(i < Scala) {
//                for (auto j = 0; j <= i; j++) {
//                    sum = sum + static_cast<double>(RawData[j]) * ImpactData[i - j];
//                }
//            }
//            if(i >= Scala && i < dataLen) {
//                int temp = Scala - 1;
//                for (auto j = i - Scala; j < i; j++) {
//                    sum = sum + static_cast<double>(RawData[j]) * ImpactData[temp];
//                    --temp;
//                }
//            }
//            if(i > dataLen) {
//                int temp = dataLen - 1;
//                for (auto j = 0; j < Scala; j++) {
//                    sum = sum + static_cast<double>(RawData[temp]) * ImpactData[j];
//                    --temp;
//                }
//            }
//            OutHTData[i] = sum;
//        }
//    }
//    else {
//    }
}

/*包络分析*/
void HilbertTransform::DataEnvelopmentAnalysis(float *datain,int dataLen)
{
//    Convolution(datain,dataLen);
//    for(int i=0;i<dataLen;i++)
//    {
//        OutEnvelopmentData[i]=sqrt(datain[i]*datain[i]+OutHTData[i]*OutHTData[i]);//信号的包络
//    }
    int i,j;
    float h[100];
    float R[501],I[501];
    float zheji[600]={0.0};
    float dt=0.004,dth=0.336;

    for(i=0;i<501;i++)
        R[i]=sin(pi*i*dt);
    for(i=1;i<51;i++)
    {  h[i+49]=1/(pi*i*dth);
       h[i-1]=1/(pi*(i-51)*dth);
    }
    for(j=0;j<600;j++)
    {  for(i=0;i<100;i++)
       if((j-i)>=0&(j-i)<501)
       zheji[j]+=h[i]*R[j-i];
    }

    for(i=0;i<501;i++)
        I[i]=zheji[i+50];
    for(i=0;i<501;i++)
    {
        OutEnvelopmentData[i]=sqrt(R[i]*R[i]+I[i]*I[i]);//信号的包络
    }
}

/*-----------------------------------------------
           FFT函数
    data:指向数据序列地指针
    a   :指向data的DFT序列的指针
    L   :2的L次方为FFT的点数
--------------------------------------------------*/
int HilbertTransform::fft(float *data,complex <double> *a,int L)
{
    complex <double> u;
    complex <double> w;
    complex <double> t;
    unsigned n=1,nv2,nm1,k,le,lei,ip;
    int i,j,m,number;
    double tmp;
    n<<=L;
    for(number = 0; number<n; number++)
    {
        a[number] = complex <double> (data[number],0);
    }
    nv2=n>>1;
    nm1=n-1;
    j=0;
    for(i=0;i<nm1;i++)
    {
        if(i<j)
        {
            t=a[j];
            a[j]=a[i];
            a[i]=t;
        }
        k=nv2;
        while(k<=j)
        {
            j-=k;
            k>>=1;
        }
        j+=k;
    }
    le=1;
    for(m=1;m<=L;m++)
    {
        lei=le;
        le<<=1;
        u=complex<double>(1,0);
        tmp=PI/lei;
        w=complex<double>(cos(tmp),-sin(tmp));
        for(j=0;j<lei;j++)
        {
            for(i=j;i<n;i+=le)
            {
                ip=i+lei;
                t=a[ip]*u;
                a[ip]=a[i]-t;
                a[i]+=t;
            }
            u*=w;
        }
    }

    return 0;
}
/*-----------------------------------------------
           IFFT函数
    data:指向数据序列地指针
    a   :指向data的DFT序列的指针
    L   :2的L次方为FFT的点数
--------------------------------------------------*/

int HilbertTransform::ifft(complex <double> *a,float *data,int L)
{
    complex <double> u;
    complex <double> w;
    complex <double> t;
    unsigned n=1,nv2,nm1,k,le,lei,ip;
    int i,j,m,number;
    double tmp;
    n<<=L;
    nv2=n>>1;
    nm1=n-1;
    j=0;
    for(i=0;i<nm1;i++)
    {
        if(i<j)
        {
            t=a[j];
            a[j]=a[i];
            a[i]=t;
        }
        k=nv2;
        while(k<=j)
        {
            j-=k;
            k>>=1;
        }
        j+=k;
    }
    le=1;
    for(m=1;m<=L;m++)
    {
        lei=le;
        le<<=1;
        u=complex<double>(1,0);
        tmp=PI/lei;
        w=complex<double>(cos(tmp),sin(tmp));
        for(j=0;j<lei;j++)
        {
            for(i=j;i<n;i+=le)
            {
                ip=i+lei;
                t=a[ip]*u;
                a[ip]=a[i]-t;
                a[i]+=t;
            }
            u*=w;
        }
    }
    for(number = 0; number<n; number++)
    {
        data[number] = ceil(a[number].real())/n;
        a[number] = a[number]/complex<double>(n,0);
    }
    return 0;
}

/*--------------------------------------------------------------
                     Hilbert变换函数
    data：指向信号序列的指针
    filterdata：指向包络序列的指针
    dn：信号序列的点数
----------------------------------------------------------------*/
int HilbertTransform:: hilbert(float * data , float *filterdata,int dn)
{
    int i = 0,l = 0,N = 0;
    complex<double> *zk;
    //float *ldata;
    l = (int)(log(dn)/log(2))+1;
    N =(int) pow(2,l);
    zk = (complex<double>*)malloc(N*sizeof(complex<double>));
    ldata = (float *)malloc(N*sizeof(float));
    memcpy(ldata,data,dn*sizeof(float));
    for(i=dn;i<N;i++)
    {
        ldata[i] = 0;
    }
    fft(ldata,zk,l);//zk为fft
    for(i=0;i<N;i++)
    {
        if(i>=1 && i<=N/2-1)
        {
            zk[i] = complex<double>(2,0)*zk[i];
        }
        if(i>=N/2 && i<=N-1)
        {
            zk[i]= complex<double> (0,0);
        }
    }
    ifft(zk,ldata,l);
    for(i = 0 ;i<dn;i++)
    {
        filterdata[i] = (float)sqrt(pow(zk[i].imag(),2)+pow(zk[i].real(),2));
    }
    free(zk);
    free(ldata);
    return 0;
}

int HilbertTransform:: conv(int *h,int *data,int *result,int hn,int dn)
{
//    int l,i,j,k,N;
//    complex<double> *hk,*datak;
//    l = (int)(log(hn+dn-1)/log(2))+1;
//    N =(int) pow(2,l);
//    int *lh,*ldata;
//    lh =(int*)malloc(N*sizeof(int));
//    ldata =(int*)malloc(N*sizeof(int));
//    memcpy(lh,h,hn*sizeof(int));
//    memcpy(ldata,data,dn*sizeof(int));
//    for(i=hn;i<N;i++)
//    {
//        lh[i] = 0;
//    }
//    for(j=dn;j<N;j++)
//    {
//        ldata[j] = 0;
//    }
//    hk = (complex <double> *) malloc(N*sizeof(complex<double>));
//    datak = (complex <double> *) malloc(N*sizeof(complex<double>));
//    fft(lh,hk,l);
//    fft(ldata,datak,l);
//    for(k=0;k<N;k++)
//    {
//        datak[k] = datak[k]*hk[k];
//    }
//    ifft(datak,result,l);

//    free(lh);
//    free(ldata);
//    free(hk);
//    free(datak);
    return 0;
}

int HilbertTransform::fft_f(double *data,complex <double> *a,int L)
{
    complex <double> u;
    complex <double> w;
    complex <double> t;
    unsigned n=1,nv2,nm1,k,le,lei,ip;
    int i,j,m,number;
    double tmp;
    n<<=L;
    for(number = 0; number<n; number++)
    {
        a[number] = complex <double> (data[number],0);
    }
    nv2=n>>1;
    nm1=n-1;
    j=0;
    for(i=0;i<nm1;i++)
    {
        if(i<j)
        {
            t=a[j];
            a[j]=a[i];
            a[i]=t;
        }
        k=nv2;
        while(k<=j)
        {
            j-=k;
            k>>=1;
        }
        j+=k;
    }
    le=1;
    for(m=1;m<=L;m++)
    {
        lei=le;
        le<<=1;
        u=complex<double>(1,0);
        tmp=PI/lei;
        w=complex<double>(cos(tmp),-sin(tmp));
        for(j=0;j<lei;j++)
        {
            for(i=j;i<n;i+=le)
            {
                ip=i+lei;
                t=a[ip]*u;
                a[ip]=a[i]-t;
                a[i]+=t;
            }
            u*=w;
        }
    }

    return 0;
}

int HilbertTransform::conv_f(double *h,int *data,int *result,int hn,int dn)
{
//    int l,i,j,k,N;
//    complex<double> *hk,*datak;
//    l = (int)(log(hn+dn-1)/log(2))+1;
//    N =(int) pow(2,l);
//    int *ldata;
//    double *lh;
//    lh =(double*)malloc(N*sizeof(double));
//    ldata =(int*)malloc(N*sizeof(int));
//    memcpy(lh,h,hn*sizeof(double));
//    memcpy(ldata,data,dn*sizeof(int));
//    for(i=hn;i<N;i++)
//    {
//        lh[i] = 0;
//    }
//    for(j=dn;j<N;j++)
//    {
//        ldata[j] = 0;
//    }
//    hk = (complex <double> *) malloc(N*sizeof(complex<double>));
//    datak = (complex <double> *) malloc(N*sizeof(complex<double>));
//    fft_f(lh,hk,l);
//    fft(ldata,datak,l);
//    for(k=0;k<N;k++)
//    {
//        datak[k] = datak[k]*hk[k];
//    }
//    ifft(datak,result,l);
//    free(lh);
//    free(ldata);
//    free(hk);
//    free(datak);
    return 0;
}

/*int HilbertTransform:: firwin_e(int n,int band,int fl,int fh,int fs,int wn,int *h)
{
    int i,n2,mid;
    double s,wc1,wc2,beta,delay;
    beta=0.0;
    double fln = (double)fl / fs;
    double fhn = (double)fh / fs;
//	if(wn==7)
//	{
//		printf("input beta parameter of Kaiser window(3<beta<10)\n");
//		scanf("%lf",&beta);
//	}
    beta = 6;
    if((n%2)==0)
    {
        n2=n/2-1;
        mid=1;
    }
    else
    {
        n2=n/2;
        mid=0;
    }
    delay=n/2.0;
    wc1=2.0*PI*fln;
    if(band>=3) wc2=2.0*PI*fhn;
    switch(band)
    {
    case 1://低通
        {
            for (i=0;i<=n2;i++)
            {
                s=i-delay;
                *(h+i)=(int)((sin(wc1*s)/(PI*s))*window(wn,n+1,i,beta));
                *(h+n-i)=*(h+i);
            }
            if(mid==1) *(h+n/2)=(int)(wc1/PI);
            break;
        }

    case 2: //高通
        {
            for (i=0;i<=n2;i++)
            {
                s=i-delay;
                *(h+i)=(int)((sin(PI*s)-sin(wc1*s))/(PI*s));
                *(h+i)=*(h+i)*(int)(window(wn,n+1,i,beta));
                *(h+n-i)=*(h+i);
            }
            if(mid==1) *(h+n/2)=(int)(1.0-wc1/PI);
            break;
        }
    case 3: //带通
        {
            for (i=0;i<=n2;i++)
            {
                s=i-delay;
                *(h+i)=(int)((sin(wc2*s)-sin(wc1*s))/(PI*s));
                *(h+i)=*(h+i)*(int)(window(wn,n+1,i,beta));
                *(h+n-i)=*(h+i);
            }
            if(mid==1) *(h+n/2)=(int)((wc2-wc1)/PI);
            break;
        }
    case 4: //带阻
        {
            for (i=0;i<=n2;i++)
            {
                s=i-delay;
                *(h+i)=(int)((sin(wc1*s)+sin(PI*s)-sin(wc2*s))/(PI*s));
                *(h+i)=*(h+i)*(int)(window(wn,n+1,i,beta));
                *(h+n-i)=*(h+i);
            }
            if(mid==1) *(h+n/2)=(int)((wc1+PI-wc2)/PI);
            break;
        }
    }
    return 0;
}*/

double HilbertTransform::window(int type,int n,int i,double beta)
{
    int k;
    double w=1.0;
    switch(type)
    {
    case 1:
        {
            w=1.0;
            break;
        }
    case 2:
        {
            k=(n-2)/10;
            if(i<=k) w=0.5*(1.0-cos(i*PI/(k+1)));
            if(i>n-k-2) w=0.5*(1.0-cos((n-i-1)*PI/(k+1)));
            break;
        }
    case 3:
        {
            w=1.0-fabs(1.0-2*i/(n-1.0));
            break;
        }
    case 4:
        {
            w=0.5*(1.0-cos(2*i*PI/(n-1)));
            break;
        }
    case 5:
        {
            w=0.54-0.46*cos(2*i*PI/(n-1));
            break;
        }
    case 6:
        {
            w=0.42-0.5*cos(2*i*PI/(n-1))+0.08*cos(4*i*PI/(n-1));
            break;
        }
    case 7:
        {
            w=kaiser(i,n,beta);
            break;
        }
    }
    return(w);
}

double HilbertTransform::kaiser(int i,int n,double beta)
{
    double a,w,a2,b1,b2,beta1;
    b1=bessel0(beta);
    a=2.0*i/(double)(n-1)-1.0;
    a2=a*a;
    beta1=beta*sqrt(1.0-a2);
    b2=bessel0(beta1);
    w=b2/b1;
    return(w);
}

double HilbertTransform::bessel0(double x)
{
    int i;
    double d,y,d2,sum;
    y=x/2.0;
    d=1.0;
    for(i=1;i<=25;i++)
    {
        d=d*y/i;
        d2=d*d;
        sum=sum+d2;
        if(d2<sum*(1.0e-8)) break;
    }
    return(sum);
}


int HilbertTransform:: firwin_e(int n,int band,int fl,int fh,int fs,int wn, int *data,int *result,int dn)
{
    int i,n2,mid;
    double *h;
    double s,wc1,wc2,beta,delay;
    beta=0.0;
    double fln = (double)fl / fs;
    double fhn = (double)fh / fs;
//	if(wn==7)
//	{
//		printf("input beta parameter of Kaiser window(3<beta<10)\n");
//		scanf("%lf",&beta);
//	}
    h = (double *)malloc((n+1)*sizeof(double));
    beta = 6;
    if((n%2)==0)
    {
        n2=n/2-1;
        mid=1;
    }
    else
    {
        n2=n/2;
        mid=0;
    }
    delay=n/2.0;
    wc1=2.0*PI*fln;
//	FILE *fp;
    if(band>=3) wc2=2.0*PI*fhn;
    switch(band)
    {
    case 1://低通
        {
            for (i=0;i<=n2;i++)
            {
                s=i-delay;
                *(h+i)=(sin(wc1*s)/(PI*s))*window(wn,n+1,i,beta);
                *(h+n-i)=*(h+i);
            }
            if(mid==1) *(h+n/2)=wc1/PI;
            break;
        }

    case 2: //高通
        {
            for (i=0;i<=n2;i++)
            {
                s=i-delay;
                *(h+i)=(sin(PI*s)-sin(wc1*s))/(PI*s);
                *(h+i)=*(h+i)*window(wn,n+1,i,beta);
                *(h+n-i)=*(h+i);
            }
            if(mid==1) *(h+n/2)=1.0-wc1/PI;
            break;
        }
    case 3: //带通
        {
            for (i=0;i<=n2;i++)
            {
                s=i-delay;
                *(h+i)=(sin(wc2*s)-sin(wc1*s))/(PI*s);
                *(h+i)=*(h+i)*window(wn,n+1,i,beta);
                *(h+n-i)=*(h+i);
            }
            if(mid==1) *(h+n/2)=(wc2-wc1)/PI;
            break;
        }
    case 4: //带阻
        {
            for (i=0;i<=n2;i++)
            {
                s=i-delay;
                *(h+i)=(sin(wc1*s)+sin(PI*s)-sin(wc2*s))/(PI*s);
                *(h+i)=*(h+i)*window(wn,n+1,i,beta);
                *(h+n-i)=*(h+i);
            }
            if(mid==1) *(h+n/2)=(wc1+PI-wc2)/PI;
            break;
        }
    }
//	fp=fopen("h.dat","w");
//	for(int p= 0;p<n+1;p++)
//	{
//		fprintf(fp, "%f\n", h[p]);
//	}
//	fclose(fp);
    conv_f(h,data,result,n+1,dn);
    free(h);
    return 0;
}

/*
 Hilbert变换得到解析信号，然后对窄带信号进行解包络
*/
void HilbertTransform::hilbtf(double* x,int n)
{
    int i,n1,n2;
    double t;
    outdata = new double[static_cast<uint>(n)];
    double *indata = new double[static_cast<uint>(n)];
    for(i=0;i<n;i++)
    {
        indata[i]=x[i];
    }
    n1 = n/2;
    n2=n1+1;
    fht1(x,n);//快速哈特莱变换
    for(i=1;i<n1;i++)
    {
        t = x[i];
        x[i] = x[n-i];
        x[n-i] = t;
    }
    for(i=n2;i<n;i++)
        x[i] = -x[i];
    x[0] = x[n1] = 0.0;
    fht1(x,n);//快速哈特莱变换
    t=1.0/n;
    for(i=0;i<n;i++)
    {
        x[i] *= t;
    }
    for(i=0;i<n;i++)
    {
        outdata[i]=sqrt(indata[i]*indata[i]+x[i]*x[i]);//得到信号的包络
    }
}

/*快速哈特莱变换*/
void HilbertTransform::fht(double* x,int n)
{
    int i,j,k,m;
    int l1,l2,l3,l4;
    int n1,n2,n4;
    double a,e,c,s,t,t1,t2;
    for(j=1,i=1;i<16;i++)
    {
        m=i;
        j=2*j;
        if(j==n) break;
    }
    n1=n-1;
    for(j=0,i=0;i<n1;i++)
    {
        if(i<j)
        {
            t=x[j];
            x[j]=x[i];
            x[i]=t;
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
        t=x[i];
        x[i]=t+x[i+1];
        x[i+1]=t-x[i+1];
    }
    n2=1;
    for(k=2;k<=m;k++)
    {
        n4=n2;
        n2=n4+n4;
        n1=n2+n2;
        e=6.283185307179586/n1;
        for(j=0;j<n;j+=n1)
        {
            l2=j+n2;
            l3=j+n4;
            l4=l2+n4;
            t=x[j];
            x[j]=t+x[l2];
            x[l2]=t-x[l2];
            t=x[l3];
            x[l3]=t+x[l4];
            x[l4]=t-x[l4];
            a=e;
            for(i=1;i<n4;i++)
            {
                l1=j+i;
                l2=j-i+n2;
                l3=l1+n2;
                l4=l2+n2;
                c=cos(a);
                s=sin(a);
                t1=x[l3]*c+x[l4]*s;
                t2=x[l3]*s-x[l4]*c;
                a=(i+1)*e;
                t=x[l1];
                x[l1]=t+t1;
                x[l3]=t-t1;
                t=x[l2];
                x[l2]=t+t2;
                x[l4]=t-t2;
            }
        }
    }
}


/*===========================================================
哈特莱变换  data 为滤波后的数据  Log2N为阶数
=============================================================*/
void HilbertTransform::fht1(double *Data,int n)
{
    int Log2N = log2(n);
    int length,i,j,k,step,i0,i1,i2,i3;
    double ck,sk,temp,temp0,temp1,temp2,temp3;

    length = 1<<Log2N;
    for(i = 0; i < length; i += 2)
    {
        temp = Data[i];
        Data[i] = temp + Data[i+1];
        Data[i+1] = temp - Data[i+1];
    }

    for(i = 2; i <= Log2N; i++)
    {
        step = 1 << i;

        for(k = 0; k < length/step; k++)
        {
            i0=k * step;
            i1 = k * step + step/2;
            i2 = k * step + step/4;
            i3=k*step+step*3/4;

            temp0 = Data[i0] + Data[i1];
            temp1 = Data[i0] - Data[i1];
            temp2 = Data[i2] + Data[i3];
            temp3 = Data[i2] - Data[i3];
            Data[i0] = temp0;
            Data[i1] = temp1;
            Data[i2] = temp2;
            Data[i3] = temp3;
        }
        for(j = 1; j < step/4; j++)
        {
            ck = cos(2.0 * pi * j/step);
            sk = sin(2.0 * pi * j/step);
            for(k = 0;k < length/step;k++)
            {
                i0=k*step+j;i1=k*step+step/2+j;i2=k*step+step/2-j;i3=k*step+step-j;
                temp0 = Data[i0]+ck*Data[i1]+sk*Data[i3];
                temp1 = Data[i0]-ck*Data[i1]-sk*Data[i3];
                temp2 = Data[i2]+sk*Data[i1]-ck*Data[i3];
                temp3 = Data[i2]-sk*Data[i1]+ck*Data[i3];
                Data[i0] = temp0;
                Data[i1] = temp1;
                Data[i2] = temp2;
                Data[i3] = temp3;
            }
        }
    }
}

QVector<QPointF> HilbertTransform::hilbtf(QVector<QPointF> &source)
{
    int i,n1,n2;
    int n = source.size();
    double t;
    QVector<QPointF> outdata;
    double *x = new double[static_cast<uint>(n)];
    for(i=0;i<n;i++)
    {
        x[i]=source.at(i).y();
    }
    n1 = n/2;
    n2=n1+1;
    fht1(x,n);//快速哈特莱变换
    for(i=1;i<n1;i++)
    {
        t = x[i];
        x[i] = x[n-i];
        x[n-i] = t;
    }
    for(i=n2;i<n;i++)
        x[i] = -x[i];
    x[0] = x[n1] = 0.0;
    fht1(x,n);//快速哈特莱变换
    t=1.0/n;
    for(i=0;i<n;i++)
    {
        x[i] *= t;
    }
    for(i=0;i<n;i++)
    {
        outdata.push_back(QPointF(i
                ,sqrt(source[i].y()*source[i].y()+x[i]*x[i])));//得到信号的包络
    }
    delete[] x;
    return outdata;
}


