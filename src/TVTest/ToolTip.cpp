#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "ToolTip.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CToolTip::CToolTip()
	: m_hwndToolTips(NULL)
{
}


CToolTip::~CToolTip()
{
	Finalize();
}


bool CToolTip::Initialize(HWND hwnd)
{
	if (m_hwndToolTips!=NULL)
		return false;

	m_hwndToolTips=::CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS,NULL,
									WS_POPUP | TTS_NOPREFIX | TTS_BALLOON/* | TTS_CLOSE*/,
									0,0,0,0,
									NULL,NULL,GetAppClass().GetInstance(),NULL);
	if (m_hwndToolTips==NULL)
		return false;

	::SendMessage(m_hwndToolTips,TTM_SETMAXTIPWIDTH,0,320);

	TOOLINFO ti;

	::ZeroMemory(&ti,sizeof(ti));
	ti.cbSize=TTTOOLINFO_V1_SIZE;
	ti.uFlags=TTF_SUBCLASS | TTF_TRACK;
	ti.hwnd=hwnd;
	ti.uId=0;
	ti.hinst=NULL;
	ti.lpszText=TEXT("");
	::SendMessage(m_hwndToolTips,TTM_ADDTOOL,0,(LPARAM)&ti);

	m_hwndOwner=hwnd;

	return true;
}


void CToolTip::Finalize()
{
	if (m_hwndToolTips!=NULL) {
		::DestroyWindow(m_hwndToolTips);
		m_hwndToolTips=NULL;
	}
}


bool CToolTip::Show(LPCTSTR pszText,LPCTSTR pszTitle,const POINT *pPos,int Icon)
{
	if (m_hwndToolTips==NULL || pszText==NULL)
		return false;
	TOOLINFO ti;
	ti.cbSize=TTTOOLINFO_V1_SIZE;
	ti.hwnd=m_hwndOwner;
	ti.uId=0;
	ti.lpszText=const_cast<LPTSTR>(pszText);
	::SendMessage(m_hwndToolTips,TTM_UPDATETIPTEXT,0,(LPARAM)&ti);
	::SendMessage(m_hwndToolTips,TTM_SETTITLE,Icon,(LPARAM)(pszTitle!=NULL?pszTitle:TEXT("")));
	POINT pt;
	if (pPos!=NULL) {
		pt=*pPos;
	} else {
		RECT rc;
		::SystemParametersInfo(SPI_GETWORKAREA,0,&rc,0);
		pt.x=rc.right-32;
		pt.y=rc.bottom;
	}
	::SendMessage(m_hwndToolTips,TTM_TRACKPOSITION,0,MAKELPARAM(pt.x,pt.y));
	::SendMessage(m_hwndToolTips,TTM_TRACKACTIVATE,TRUE,(LPARAM)&ti);
	return true;
}


bool CToolTip::Hide()
{
	TOOLINFO ti;

	ti.cbSize=TTTOOLINFO_V1_SIZE;
	ti.hwnd=m_hwndOwner;
	ti.uId=0;
	::SendMessage(m_hwndToolTips,TTM_TRACKACTIVATE,FALSE,(LPARAM)&ti);
	return true;
}
