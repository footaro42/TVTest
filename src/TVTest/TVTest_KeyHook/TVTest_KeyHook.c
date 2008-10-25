#include <windows.h>
#include "TVTest_KeyHook.h"


#pragma data_seg(".SHARE")
HHOOK hHook=NULL;
HWND hwndTarget=NULL;
#pragma data_seg()
HINSTANCE hInst;
UINT Message;
BOOL fShiftPress;
BOOL fCtrlPress;




BOOL WINAPI DllMain(HINSTANCE hInstance,DWORD dwReason,LPVOID pvReserved)
{
	if (dwReason==DLL_PROCESS_ATTACH) {
		hInst=hInstance;
		Message=RegisterWindowMessage(KEYHOOK_MESSAGE);
	}
	return TRUE;
}


LRESULT CALLBACK KeyHookProc(int nCode,WPARAM wParam,LPARAM lParam)
{
	if (nCode==HC_ACTION) {
		BOOL fPress=(lParam&0x80000000)==0;

		if (wParam==VK_CONTROL) {
			fCtrlPress=fPress;
		} else if (wParam==VK_SHIFT) {
			fShiftPress=fPress;
		} else if (wParam>=VK_F13 && wParam<=VK_F24) {
			if (fPress && (fCtrlPress || fShiftPress))
				PostMessage(hwndTarget,Message,wParam,
					// キーリピート回数が常に1になっている
					//(lParam&KEYHOOK_LPARAM_REPEATCOUNT) |
					((lParam&0x40000000)!=0?2:1) |
					(fCtrlPress?KEYHOOK_LPARAM_CONTROL:0) |
					(fShiftPress?KEYHOOK_LPARAM_SHIFT:0));
		}
	}
	return CallNextHookEx(hHook,nCode,wParam,lParam);
}


__declspec(dllexport) BOOL WINAPI BeginHook(HWND hwnd)
{
	if (hHook!=NULL)
		return TRUE;
	hHook=SetWindowsHookEx(WH_KEYBOARD,KeyHookProc,hInst,0);
	if (hHook==NULL)
		return FALSE;
	hwndTarget=hwnd;
	fShiftPress=FALSE;
	fCtrlPress=FALSE;
	return TRUE;
}


__declspec(dllexport) BOOL WINAPI EndHook(void)
{
	if (hHook!=NULL) {
		UnhookWindowsHookEx(hHook);
		hHook=NULL;
	}
	return TRUE;
}
