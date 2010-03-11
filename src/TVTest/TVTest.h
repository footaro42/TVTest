#ifndef TVTEST_H
#define TVTEST_H


#ifndef TVH264

#define APP_NAME_A	"TVTest"
#define APP_NAME_W	L"TVTest"

#define VERSION_MAJOR	0
#define VERSION_MINOR	6
#define VERSION_BUILD	6

#define VERSION_TEXT_A	"0.6.6"
#define VERSION_TEXT_W	L"0.6.6"

#define ABOUT_TEXT	"TVTest ver.0.6.6\nby HDUSTestの中の人 && HDUSスレの皆さん"

#else

#define APP_NAME_A	"TVH264"
#define APP_NAME_W	L"TVH264"

#define VERSION_MAJOR	0
#define VERSION_MINOR	4
#define VERSION_BUILD	1

#define VERSION_TEXT_A	"0.4.1"
#define VERSION_TEXT_W	L"0.4.1"

#define ABOUT_TEXT	"TVH264 ver.0.4.1\nby HDUSTestの中の人 && HDUSスレの皆さん"

// ワンセグ向け
#define TVH264_FOR_1SEG

#endif	// TVH264

#ifndef UNICODE
#define APP_NAME		APP_NAME_A
#define VERSION_TEXT	VERSION_TEXT_A
#else
#define APP_NAME		APP_NAME_W
#define VERSION_TEXT	VERSION_TEXT_W
#endif

#ifdef BONTSENGINE_RADIO_SUPPORT
#define TVTEST_RADIO_SUPPORT
#endif


#ifndef RC_INVOKED


#include "Util.h"

#define lengthof(a) (sizeof(a)/sizeof(a[0]))

#ifndef CLAMP
#define CLAMP(val,min,max) \
	(((val)>(max))?(max):(((val)<(min))?(min):(val)))
#endif

#ifndef SAFE_DELETE
//#define SAFE_DELETE(p)		if (p) { delete p; (p)=NULL; }
//#define SAFE_DELETE_ARRAY(p)	if (p) { delete [] p; (p)=NULL; }
// delete NULL でもいいので...
#define SAFE_DELETE(p)			((void)(delete p,(p)=NULL))
#define SAFE_DELETE_ARRAY(p)	((void)(delete [] p,(p)=NULL))
#endif

#ifndef TVH264
#define CHANNEL_FILE_EXTENSION	TEXT(".ch2")
#else
#define CHANNEL_FILE_EXTENSION	TEXT(".ch1")
#endif


#endif	// ndef RC_INVOKED


#endif	// ndef TVTEST_H
