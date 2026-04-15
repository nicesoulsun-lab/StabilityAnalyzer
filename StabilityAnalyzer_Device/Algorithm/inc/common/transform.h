#ifndef TRANSFORM_H
#define TRANSFORM_H

/*
 * 数学变换头文件
 */

#include <iostream>			//模板类输入输出流标准头文件
#include <valarray>			//模板类数组的标准头文件
#include "src/common/matrix.cpp"			//模板类矩阵头文件
#include "src/common/common.cpp"			//公共头文件
#include <stdio.h>
#include <stdlib.h>

using namespace std;

template <class T>
T getMax(const T &a, const T &b);

//傅里叶级数逼近
template <class T>
void FourierSeriesApproach(const valarray<T>& f, const valarray<T>& a, valarray<T>& b);

//快速傅里叶变换
template <class _Ty>
void FourierTransform(valarray<complex<_Ty> >& pp, valarray<complex<_Ty> >& ff, int l, int il);

//快速沃什变换
template <class _Ty>
void WalshTransform(valarray<_Ty>& p, valarray<_Ty>& x);

//五点三次平滑(曲线拟合)
template <class _Ty>
void Smooth5_3(valarray<_Ty>& y, valarray<_Ty>& yy);

//离散随机线性系统的卡尔曼滤波
template <class _Ty>
int SievingKalman(matrix<_Ty>& f, matrix<_Ty>& q, matrix<_Ty>& r,
                  matrix<_Ty>& h, matrix<_Ty>& y, matrix<_Ty>& x,
                  matrix<_Ty>& p, matrix<_Ty>& g);

//a-b-r滤波
template <class _Ty>
void SievingABR(valarray<_Ty>& x, _Ty t, _Ty a, _Ty b, _Ty r, valarray<_Ty>& y);



#endif // TRANSFORM_H
