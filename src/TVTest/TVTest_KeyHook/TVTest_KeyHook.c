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
		static const struct {
			WORD KeyCode;
			WORD Modifier;
		} KeyList[] = {
			{VK_F17,	MK_SHIFT},				// Ch 1
			{VK_F18,	MK_SHIFT},				// Ch 2
			{VK_F19,	MK_SHIFT},				// Ch 3
			{VK_F20,	MK_SHIFT},				// Ch 4
			{VK_F21,	MK_SHIFT},				// Ch 5
			{VK_F22,	MK_SHIFT},				// Ch 6
			{VK_F23,	MK_SHIFT},				// Ch 7
			{VK_F24,	MK_SHIFT},				// Ch 8
			{VK_F13,	MK_CONTROL},			// Ch 9
			{VK_F16,	MK_SHIFT},				// Ch 10
			{VK_F14,	MK_CONTROL},			// Ch 11
			{VK_F15,	MK_CONTROL},			// Ch 12
			{VK_F13,	MK_SHIFT},				// 画面表示
			{VK_F14,	MK_SHIFT},				// 電源
			{VK_F15,	MK_SHIFT},				// 消音
			{VK_F16,	MK_CONTROL},			// メニュー
			{VK_F17,	MK_CONTROL},			// 全画面表示
			{VK_F18,	MK_CONTROL},			// 字幕
			{VK_F19,	MK_CONTROL},			// 音声切替
			{VK_F20,	MK_CONTROL},			// EPG
			{VK_F21,	MK_CONTROL},			// 戻る
			{VK_F22,	MK_CONTROL},			// 録画
			{VK_F23,	MK_CONTROL},			// メモ
			{VK_F24,	MK_CONTROL},			// 停止
			{VK_F13,	MK_CONTROL | MK_SHIFT},	// 再生
			{VK_F14,	MK_CONTROL | MK_SHIFT},	// 一時停止
			{VK_F15,	MK_CONTROL | MK_SHIFT},	// |<<
			{VK_F16,	MK_CONTROL | MK_SHIFT},	// <<
			{VK_F17,	MK_CONTROL | MK_SHIFT},	// >>
			{VK_F18,	MK_CONTROL | MK_SHIFT},	// >>|
			{VK_F19,	MK_CONTROL | MK_SHIFT},	// しおり
			{VK_F20,	MK_CONTROL | MK_SHIFT},	// ジャンプ
			{VK_F21,	MK_CONTROL | MK_SHIFT},	// A
			{VK_F22,	MK_CONTROL | MK_SHIFT},	// B
			{VK_F23,	MK_CONTROL | MK_SHIFT},	// C
			{VK_F24,	MK_CONTROL | MK_SHIFT},	// D
			/*
			{VK_UP,		MK_SHIFT},				// 音量 up
			{VK_DOWN,	MK_SHIFT},				// 音量 down
			{VK_UP,		MK_CONTROL},			// Ch up
			{VK_DOWN,	MK_CONTROL},			// Ch down
			*/
		};
		BOOL fPress=(lParam&0x80000000)==0;

		if (wParam==VK_CONTROL) {
			fCtrlPress=fPress;
		} else if (wParam==VK_SHIFT) {
			fShiftPress=fPress;
		} else if (wParam>=VK_F13 && wParam<=VK_F24) {
			if (fPress && (fCtrlPress || fShiftPress)) {
				WORD Modifier=0;
				int i;

				if (fCtrlPress)
					Modifier|=MK_CONTROL;
				if (fShiftPress)
					Modifier|=MK_SHIFT;
				for (i=0;i<sizeof(KeyList)/sizeof(KeyList[0]);i++) {
					if (KeyList[i].KeyCode==wParam
							&& KeyList[i].Modifier==Modifier) {
						PostMessage(hwndTarget,Message,wParam,
							// キーリピート回数が常に1になっている
							//(lParam&KEYHOOK_LPARAM_REPEATCOUNT) |
							((lParam&0x40000000)!=0?2:1) |
							(fCtrlPress?KEYHOOK_LPARAM_CONTROL:0) |
							(fShiftPress?KEYHOOK_LPARAM_SHIFT:0));
						return 1;
					}
				}
			}
		}
	}
	return CallNextHookEx(hHook,nCode,wParam,lParam);
}


__declspec(dllexport) BOOL WINAPI BeginHook(HWND hwnd,BOOL fLocal)
{
	if (hHook!=NULL)
		return TRUE;
	if (fLocal)
		hHook=SetWindowsHookEx(WH_KEYBOARD,KeyHookProc,NULL,GetWindowThreadProcessId(hwnd,NULL));
	else
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
