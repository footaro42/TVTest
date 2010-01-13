#include "stdafx.h"
#include "TVTest.h"
#include "OSDOptions.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "Util.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




COSDOptions::COSDOptions()
{
	m_fShowOSD=true;
	m_fPseudoOSD=false;
	m_TextColor=RGB(0,255,0);
	m_Opacity=80;
	m_FadeTime=3000;
	m_fEnableNotificationBar=true;
	m_NotificationBarDuration=3000;
	m_NotificationBarFlags=NOTIFY_EVENTNAME
#ifndef TVH264_FOR_1SEG
		 | NOTIFY_ECMERROR
#endif
		;
	NONCLIENTMETRICS ncm;
#if WINVER<0x0600
	ncm.cbSize=sizeof(ncm);
#else
	ncm.cbSize=offsetof(NONCLIENTMETRICS,iPaddedBorderWidth);
#endif
	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,ncm.cbSize,&ncm,0);
	m_NotificationBarFont=ncm.lfMessageFont;
	m_NotificationBarFont.lfHeight=
#ifndef TVH264_FOR_1SEG
		-14;
#else
		-12;
#endif

	m_DisplayMenuFont=ncm.lfMessageFont;
	m_fDisplayMenuFontAutoSize=true;
}


COSDOptions::~COSDOptions()
{
}


bool COSDOptions::Read(CSettings *pSettings)
{
	pSettings->Read(TEXT("UseOSD"),&m_fShowOSD);
	pSettings->Read(TEXT("PseudoOSD"),&m_fPseudoOSD);
	pSettings->ReadColor(TEXT("OSDTextColor"),&m_TextColor);
	pSettings->Read(TEXT("OSDOpacity"),&m_Opacity);
	pSettings->Read(TEXT("OSDFadeTime"),&m_FadeTime);
	pSettings->Read(TEXT("EnableNotificationBar"),&m_fEnableNotificationBar);
	pSettings->Read(TEXT("NotificationBarDuration"),&m_NotificationBarDuration);
	bool f;
	if (pSettings->Read(TEXT("NotifyEventName"),&f))
		EnableNotify(NOTIFY_EVENTNAME,f);
	if (pSettings->Read(TEXT("NotifyEcmError"),&f))
		EnableNotify(NOTIFY_ECMERROR,f);
	// Font
	TCHAR szFont[LF_FACESIZE];
	int Value;
	if (pSettings->Read(TEXT("NotificationBarFontName"),szFont,LF_FACESIZE)
			&& szFont[0]!='\0') {
		lstrcpy(m_NotificationBarFont.lfFaceName,szFont);
		m_NotificationBarFont.lfEscapement=0;
		m_NotificationBarFont.lfOrientation=0;
		m_NotificationBarFont.lfUnderline=0;
		m_NotificationBarFont.lfStrikeOut=0;
		m_NotificationBarFont.lfCharSet=DEFAULT_CHARSET;
		m_NotificationBarFont.lfOutPrecision=OUT_DEFAULT_PRECIS;
		m_NotificationBarFont.lfClipPrecision=CLIP_DEFAULT_PRECIS;
		m_NotificationBarFont.lfQuality=DRAFT_QUALITY;
		m_NotificationBarFont.lfPitchAndFamily=DEFAULT_PITCH | FF_DONTCARE;
	}
	if (pSettings->Read(TEXT("NotificationBarFontSize"),&Value)) {
		m_NotificationBarFont.lfHeight=Value;
		m_NotificationBarFont.lfWidth=0;
	}
	if (pSettings->Read(TEXT("NotificationBarFontWeight"),&Value))
		m_NotificationBarFont.lfWeight=Value;
	if (pSettings->Read(TEXT("NotificationBarFontItalic"),&Value))
		m_NotificationBarFont.lfItalic=Value;

	pSettings->Read(TEXT("DisplayMenuFont"),&m_DisplayMenuFont);
	pSettings->Read(TEXT("DisplayMenuFontAutoSize"),&m_fDisplayMenuFontAutoSize);
	return true;
}


bool COSDOptions::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("UseOSD"),m_fShowOSD);
	pSettings->Write(TEXT("PseudoOSD"),m_fPseudoOSD);
	pSettings->WriteColor(TEXT("OSDTextColor"),m_TextColor);
	pSettings->Write(TEXT("OSDOpacity"),m_Opacity);
	pSettings->Write(TEXT("OSDFadeTime"),m_FadeTime);
	pSettings->Write(TEXT("EnableNotificationBar"),m_fEnableNotificationBar);
	pSettings->Write(TEXT("NotificationBarDuration"),m_NotificationBarDuration);
	pSettings->Write(TEXT("NotifyEventName"),(m_NotificationBarFlags&NOTIFY_EVENTNAME)!=0);
	pSettings->Write(TEXT("NotifyEcmError"),(m_NotificationBarFlags&NOTIFY_ECMERROR)!=0);
	// Font
	pSettings->Write(TEXT("NotificationBarFontName"),m_NotificationBarFont.lfFaceName);
	pSettings->Write(TEXT("NotificationBarFontSize"),(int)m_NotificationBarFont.lfHeight);
	pSettings->Write(TEXT("NotificationBarFontWeight"),(int)m_NotificationBarFont.lfWeight);
	pSettings->Write(TEXT("NotificationBarFontItalic"),(int)m_NotificationBarFont.lfItalic);

	pSettings->Write(TEXT("DisplayMenuFont"),&m_DisplayMenuFont);
	pSettings->Write(TEXT("DisplayMenuFontAutoSize"),m_fDisplayMenuFontAutoSize);
	return true;
}


bool COSDOptions::IsNotifyEnabled(unsigned int Type) const
{
	return m_fEnableNotificationBar && (m_NotificationBarFlags&Type)!=0;
}


void COSDOptions::EnableNotify(unsigned int Type,bool fEnabled)
{
	if (fEnabled)
		m_NotificationBarFlags|=Type;
	else
		m_NotificationBarFlags&=~Type;
}


COSDOptions *COSDOptions::GetThis(HWND hDlg)
{
	return static_cast<COSDOptions*>(GetOptions(hDlg));
}


static void SetFontInfo(HWND hDlg,int ID,const LOGFONT *plf)
{
	HDC hdc;
	TCHAR szText[LF_FACESIZE+16];

	hdc=GetDC(hDlg);
	if (hdc==NULL)
		return;
	wsprintf(szText,TEXT("%s, %d pt"),plf->lfFaceName,CalcFontPointHeight(hdc,plf));
	SetDlgItemText(hDlg,ID,szText);
	ReleaseDC(hDlg,hdc);
}

INT_PTR CALLBACK COSDOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			COSDOptions *pThis=static_cast<COSDOptions*>(OnInitDialog(hDlg,lParam));

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_USEOSD,pThis->m_fShowOSD);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_PSEUDOOSD,pThis->m_fPseudoOSD);
			pThis->m_CurTextColor=pThis->m_TextColor;
			::SetDlgItemInt(hDlg,IDC_OPTIONS_OSDFADETIME,pThis->m_FadeTime/1000,TRUE);
			::SendDlgItemMessage(hDlg,IDC_OPTIONS_OSDFADETIME_UD,UDM_SETRANGE,0,
													MAKELPARAM(UD_MAXVAL,1));
			EnableDlgItems(hDlg,IDC_OPTIONS_OSD_FIRST,IDC_OPTIONS_OSD_LAST,pThis->m_fShowOSD);

			DlgCheckBox_Check(hDlg,IDC_NOTIFICATIONBAR_ENABLE,pThis->m_fEnableNotificationBar);
			DlgCheckBox_Check(hDlg,IDC_NOTIFICATIONBAR_NOTIFYEVENTNAME,(pThis->m_NotificationBarFlags&NOTIFY_EVENTNAME)!=0);
			DlgCheckBox_Check(hDlg,IDC_NOTIFICATIONBAR_NOTIFYECMERROR,(pThis->m_NotificationBarFlags&NOTIFY_ECMERROR)!=0);
			::SetDlgItemInt(hDlg,IDC_NOTIFICATIONBAR_DURATION,pThis->m_NotificationBarDuration/1000,FALSE);
			DlgUpDown_SetRange(hDlg,IDC_NOTIFICATIONBAR_DURATION_UPDOWN,1,60);
			pThis->m_CurNotificationBarFont=pThis->m_NotificationBarFont;
			SetFontInfo(hDlg,IDC_NOTIFICATIONBAR_FONT_INFO,&pThis->m_CurNotificationBarFont);
			EnableDlgItems(hDlg,IDC_NOTIFICATIONBAR_FIRST,IDC_NOTIFICATIONBAR_LAST,pThis->m_fEnableNotificationBar);

			pThis->m_CurDisplayMenuFont=pThis->m_DisplayMenuFont;
			SetFontInfo(hDlg,IDC_DISPLAYMENU_FONT_INFO,&pThis->m_DisplayMenuFont);
			DlgCheckBox_Check(hDlg,IDC_DISPLAYMENU_AUTOFONTSIZE,pThis->m_fDisplayMenuFontAutoSize);
		}
		return TRUE;

	case WM_DRAWITEM:
		{
			COSDOptions *pThis=GetThis(hDlg);
			LPDRAWITEMSTRUCT pdis=reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
			RECT rc;

			rc=pdis->rcItem;
			DrawEdge(pdis->hDC,&rc,BDR_SUNKENOUTER,BF_RECT | BF_ADJUST);
			DrawUtil::Fill(pdis->hDC,&rc,pThis->m_CurTextColor);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OPTIONS_USEOSD:
			EnableDlgItems(hDlg,IDC_OPTIONS_OSD_FIRST,IDC_OPTIONS_OSD_LAST,
						   DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_USEOSD));
			return TRUE;

		case IDC_OPTIONS_OSDTEXTCOLOR:
			{
				COSDOptions *pThis=GetThis(hDlg);

				if (ChooseColorDialog(hDlg,&pThis->m_CurTextColor))
					InvalidateDlgItem(hDlg,IDC_OPTIONS_OSDTEXTCOLOR);
			}
			return TRUE;

		case IDC_NOTIFICATIONBAR_ENABLE:
			EnableDlgItems(hDlg,IDC_NOTIFICATIONBAR_FIRST,IDC_NOTIFICATIONBAR_LAST,
						   DlgCheckBox_IsChecked(hDlg,IDC_NOTIFICATIONBAR_ENABLE));
			return TRUE;

		case IDC_NOTIFICATIONBAR_FONT_CHOOSE:
			{
				COSDOptions *pThis=GetThis(hDlg);

				if (ChooseFontDialog(hDlg,&pThis->m_CurNotificationBarFont))
					SetFontInfo(hDlg,IDC_NOTIFICATIONBAR_FONT_INFO,&pThis->m_CurNotificationBarFont);
			}
			return TRUE;

		case IDC_DISPLAYMENU_FONT_CHOOSE:
			{
				COSDOptions *pThis=GetThis(hDlg);

				if (ChooseFontDialog(hDlg,&pThis->m_CurDisplayMenuFont))
					SetFontInfo(hDlg,IDC_DISPLAYMENU_FONT_INFO,&pThis->m_CurDisplayMenuFont);
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				COSDOptions *pThis=GetThis(hDlg);

				pThis->m_fShowOSD=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_USEOSD);
				pThis->m_fPseudoOSD=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_PSEUDOOSD);
				pThis->m_TextColor=pThis->m_CurTextColor;
				pThis->m_FadeTime=::GetDlgItemInt(hDlg,IDC_OPTIONS_OSDFADETIME,NULL,FALSE)*1000;

				pThis->m_fEnableNotificationBar=
					DlgCheckBox_IsChecked(hDlg,IDC_NOTIFICATIONBAR_ENABLE);
				pThis->EnableNotify(NOTIFY_EVENTNAME,
									DlgCheckBox_IsChecked(hDlg,IDC_NOTIFICATIONBAR_NOTIFYEVENTNAME));
				pThis->EnableNotify(NOTIFY_ECMERROR,
									DlgCheckBox_IsChecked(hDlg,IDC_NOTIFICATIONBAR_NOTIFYECMERROR));
				pThis->m_NotificationBarDuration=
					::GetDlgItemInt(hDlg,IDC_NOTIFICATIONBAR_DURATION,NULL,FALSE)*1000;
				pThis->m_NotificationBarFont=pThis->m_CurNotificationBarFont;

				pThis->m_DisplayMenuFont=pThis->m_CurDisplayMenuFont;
				pThis->m_fDisplayMenuFontAutoSize=DlgCheckBox_IsChecked(hDlg,IDC_DISPLAYMENU_AUTOFONTSIZE);
			}
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			COSDOptions *pThis=GetThis(hDlg);

			pThis->OnDestroy();
		}
		return TRUE;
	}
	return FALSE;
}
