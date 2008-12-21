#include "stdafx.h"
#include "TVTest.h"
#include "RemoteController.h"
#include "TVTest_KeyHook/TVTest_KeyHook.h"
#include "resource.h"




CRemoteController::CRemoteController(HWND hwnd)
{
	m_hLib=NULL;
	m_fHook=false;
	m_Message=WM_NULL;
	m_hwnd=hwnd;
}


CRemoteController::~CRemoteController()
{
	if (m_hLib!=NULL) {
		EndHook();
		FreeLibrary(m_hLib);
	}
}


bool CRemoteController::BeginHook(bool fLocal)
{
	BeginHookFunc pBeginHook;

	if (m_hLib==NULL) {
		m_hLib=LoadLibrary(TEXT("TVTest_KeyHook.dll"));
		if (m_hLib==NULL)
			return false;
	}
	pBeginHook=(BeginHookFunc)GetProcAddress(m_hLib,"BeginHook");
	if (pBeginHook==NULL || !pBeginHook(m_hwnd,fLocal))
		return false;
	m_fHook=true;
	m_Message=RegisterWindowMessage(KEYHOOK_MESSAGE);

	// Vista ‚ÅŠÇ—ŽÒŒ ŒÀ‚ÅŽÀs‚³‚ê‚½Žž—p‚Ì‘Îô
#ifndef MSGFLT_ADD
#define MSGFLT_ADD 1
#endif
	typedef BOOL (WINAPI *ChangeWindowMessageFilterFunc)(UINT,DWORD);
	HMODULE hLib=LoadLibrary(TEXT("user32.dll"));
	ChangeWindowMessageFilterFunc pChangeFilter=(ChangeWindowMessageFilterFunc)
							GetProcAddress(hLib,"ChangeWindowMessageFilter");

	if (pChangeFilter!=NULL)
		pChangeFilter(m_Message,MSGFLT_ADD);
	FreeLibrary(hLib);
	return true;
}


bool CRemoteController::EndHook()
{
	if (m_fHook) {
		EndHookFunc pEndHook;

		pEndHook=(EndHookFunc)GetProcAddress(m_hLib,"EndHook");
		if (pEndHook==NULL)
			return false;
		pEndHook();
		m_fHook=false;
	}
	return true;
}


bool CRemoteController::TranslateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static const struct {
		WORD Key;
		WORD Modifier;
		int Command;
	} KeyMap[] = {
		{VK_F17,	MK_SHIFT,				CM_CHANNELNO_1},
		{VK_F18,	MK_SHIFT,				CM_CHANNELNO_2},
		{VK_F19,	MK_SHIFT,				CM_CHANNELNO_3},
		{VK_F20,	MK_SHIFT,				CM_CHANNELNO_4},
		{VK_F21,	MK_SHIFT,				CM_CHANNELNO_5},
		{VK_F22,	MK_SHIFT,				CM_CHANNELNO_6},
		{VK_F23,	MK_SHIFT,				CM_CHANNELNO_7},
		{VK_F24,	MK_SHIFT,				CM_CHANNELNO_8},
		{VK_F13,	MK_CONTROL,				CM_CHANNELNO_9},
		{VK_F16,	MK_SHIFT,				CM_CHANNELNO_10},
		{VK_F14,	MK_CONTROL,				CM_CHANNELNO_11},
		{VK_F15,	MK_CONTROL,				CM_CHANNELNO_12},
		{VK_F13,	MK_SHIFT,				CM_ASPECTRATIO},
		{VK_F14,	MK_SHIFT,				CM_CLOSE},
		{VK_F15,	MK_SHIFT,				CM_VOLUME_MUTE},
		{VK_F16,	MK_CONTROL,				CM_MENU},
		{VK_F17,	MK_CONTROL,				CM_FULLSCREEN},
		{VK_F19,	MK_CONTROL,				CM_SWITCHAUDIO},
		{VK_F20,	MK_CONTROL,				CM_PROGRAMGUIDE},
		{VK_F22,	MK_CONTROL,				CM_RECORD_START},
		{VK_F23,	MK_CONTROL,				CM_CAPTURE},
		{VK_F24,	MK_CONTROL,				CM_RECORD_STOP},
		{VK_F14,	MK_CONTROL | MK_SHIFT,	CM_RECORD_PAUSE},
		/*
		{VK_UP,		MK_SHIFT,	CM_VOLUME_UP},
		{VK_DOWN,	MK_SHIFT,	CM_VOLUME_DOWN},
		{VK_UP,		MK_CONTROL,	CM_CHANNEL_UP},
		{VK_DOWN,	MK_CONTROL,	CM_CHANNEL_DOWN},
		*/
	};
	int i;
	WORD Modifier;

	if (m_Message==WM_NULL || uMsg!=m_Message
			|| KEYHOOK_GET_REPEATCOUNT(lParam)>1)
		return false;
	Modifier=0;
	if (KEYHOOK_GET_CONTROL(lParam))
		Modifier|=MK_CONTROL;
	if (KEYHOOK_GET_SHIFT(lParam))
		Modifier|=MK_SHIFT;
	for (i=0;i<lengthof(KeyMap);i++) {
		if (KeyMap[i].Key==wParam && KeyMap[i].Modifier==Modifier) {
			PostMessage(m_hwnd,WM_COMMAND,KeyMap[i].Command,0);
			break;
		}
	}
	return true;
}
