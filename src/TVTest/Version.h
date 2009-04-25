#ifndef VERSION_H
#define VERSION_H


#define VERSION_MAJOR	0
#define VERSION_MINOR	5
#define VERSION_BUILD	32

#define VERSION_TEXT_A	"0.5.32"
#define VERSION_TEXT_W	L"0.5.32"
#ifndef UNICODE
#define VERSION_TEXT VERSION_TEXT_A
#else
#define VERSION_TEXT VERSION_TEXT_W
#endif


#endif
