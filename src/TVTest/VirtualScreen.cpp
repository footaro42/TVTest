#include "stdafx.h"
#include "VirtualScreen.h"




CVirtualScreen::CVirtualScreen()
	: m_hdc(NULL)
	, m_hbm(NULL)
	, m_hbmOld(NULL)
	, m_Width(0)
	, m_Height(0)
{
}


CVirtualScreen::~CVirtualScreen()
{
	Destroy();
}


bool CVirtualScreen::Create(int Width,int Height,HDC hdc)
{
	if (Width<=0 || Height<=0)
		return false;
	Destroy();
	HDC hdcScreen;
	if (hdc==NULL) {
		hdcScreen=::GetDC(NULL);
		if (hdcScreen==NULL)
			return false;
		hdc=hdcScreen;
	} else {
		hdcScreen=NULL;
	}
	m_hdc=::CreateCompatibleDC(hdc);
	if (m_hdc==NULL) {
		if (hdcScreen!=NULL)
			::ReleaseDC(NULL,hdcScreen);
		return false;
	}
	m_hbm=::CreateCompatibleBitmap(hdc,Width,Height);
	if (hdcScreen!=NULL)
		::ReleaseDC(NULL,hdcScreen);
	if (m_hbm==NULL) {
		Destroy();
		return false;
	}
	m_hbmOld=static_cast<HBITMAP>(::SelectObject(m_hdc,m_hbm));
	m_Width=Width;
	m_Height=Height;
	return true;
}


void CVirtualScreen::Destroy()
{
	if (m_hbmOld!=NULL) {
		::SelectObject(m_hdc,m_hbmOld);
		m_hbmOld=NULL;
	}
	if (m_hdc!=NULL) {
		::DeleteDC(m_hdc);
		m_hdc=NULL;
	}
	if (m_hbm!=NULL) {
		::DeleteObject(m_hbm);
		m_hbm=NULL;
		m_Width=0;
		m_Height=0;
	}
}
