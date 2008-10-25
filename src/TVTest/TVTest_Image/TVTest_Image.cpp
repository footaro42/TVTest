#include <windows.h>
#include <tchar.h>
#include "TVTest_Image.h"
#include "BMP.h"
#include "JPEG.h"
#include "PNG.h"


static HINSTANCE hInst;




BOOL WINAPI DllMain(HINSTANCE hInstance,DWORD dwReason,LPVOID pvReserved)
{
	if (dwReason==DLL_PROCESS_ATTACH) {
		hInst=hInstance;
	}
	return TRUE;
}


extern "C" __declspec(dllexport) BOOL WINAPI SaveImage(const ImageSaveInfo *pInfo)
{
	BOOL fResult;

	if (lstrcmpi(pInfo->pszFormat,TEXT("BMP"))==0) {
		fResult=SaveBMPFile(pInfo);
	} else if (lstrcmpi(pInfo->pszFormat,TEXT("JPEG"))==0) {
		fResult=SaveJPEGFile(pInfo);
	} else if (lstrcmpi(pInfo->pszFormat,TEXT("PNG"))==0) {
		fResult=SavePNGFile(pInfo);
	} else
		return FALSE;
	return fResult;
}
