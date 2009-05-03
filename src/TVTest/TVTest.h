#ifndef TVTEST_H
#define TVTEST_H


#include "Util.h"


#define APP_NAME_A	"TVTest"
#define APP_NAME_W	L"TVTest"
#ifndef UNICODE
#define APP_NAME	APP_NAME_A
#else
#define APP_NAME	APP_NAME_W
#endif


#define lengthof(a) (sizeof(a)/sizeof(a[0]))

#ifndef SAFE_DELETE
//#define SAFE_DELETE(p)		if (p) { delete p; (p)=NULL; }
//#define SAFE_DELETE_ARRAY(p)	if (p) { delete [] p; (p)=NULL; }
#define SAFE_DELETE(p)			((void)(delete p,(p)=NULL))
#define SAFE_DELETE_ARRAY(p)	((void)(delete [] p,(p)=NULL))
#endif


#endif
