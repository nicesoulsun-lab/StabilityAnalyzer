#ifndef COMMON_H
#define COMMON_H
#ifdef NDEBUG
 #define Assert(x)	if (!x)	throw;
#else			//否则用系统定义的函数assert()处理
 #include <cassert>
 #define Assert(x)	assert(x);
#endif			//NDEBUG

#include <iostream>
#include <complex>		//模板类complex的标准头文件
#include <valarray>	//模板类valarray的标准头文件
using namespace std;	//名字空间

#define FLOATERROR     (1.0e-6F)
#define DOUBLEERROR     (1.0e-15)
#define LONGDOUBLEERROR (1.0e-30)

#define GoldNo          (0.618033399)		//黄金分割常数(1.0-0.381966)
#define PI 3.1415926
#define PI2 6.2831853
#define MAINBUFSIZE 4096
#define DPI     (2.0 * PI)
//取x绝对值
template <class T>
long double Abs(const T& x);

//取x符号，+-或0
template <class T>
T Sgn(const T& x);

//比较两float浮点数相等
bool FloatEqual(float lhs, float rhs);
//比较两float浮点数不相等
bool FloatNotEqual(float lhs, float rhs);
//比较两double浮点数相等
bool FloatEqual(double lhs, double rhs);
//比较两double浮点数不相等
bool FloatNotEqual(double lhs, double rhs);
//比较两long double浮点数相等
bool FloatEqual(long double lhs, long double rhs);
//比较两long double浮点数不相等
bool FloatNotEqual(long double lhs, long double rhs);

//求x与y的最小值，返回小者
template <class T>
T Min(const T& x, const T& y);

//求x与y的最大值，返回大者
template <class T>
T Max(const T& x, const T& y);

//打印数组(向量)所有元素值
template <class T>
void ValarrayPrint(const valarray<T>& vOut);

//打印某个指定数组(向量)元素值
template <class T>
void ValarrayPrint(const valarray<T>& vOut, size_t sPosition);


#endif // COMMON_H
