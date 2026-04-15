#ifndef SQLORM_GLOBAL_H
#define SQLORM_GLOBAL_H

#include <QtCore/qglobal.h>

#ifdef SQLORM_LIBRARY
#define SQLORM_EXPORT Q_DECL_EXPORT
#else
#define SQLORM_EXPORT Q_DECL_IMPORT
#endif

#endif // SQLORM_GLOBAL_H
