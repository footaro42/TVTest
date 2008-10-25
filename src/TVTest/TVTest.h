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


#define WM_APP_SERVICEUPDATE	WM_APP
#define WM_APP_CHANNELCHANGE	(WM_APP+1)
#define WM_APP_IMAGESAVE		(WM_APP+2)
#define WM_APP_TRAYICON			(WM_APP+3)
#define WM_APP_EXECUTE			(WM_APP+4)
#define WM_APP_QUERYPORT		(WM_APP+5)
#define WM_APP_FILEWRITEERROR	(WM_APP+6)


#endif
