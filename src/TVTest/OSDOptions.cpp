#include "stdafx.h"
#include "TVTest.h"
#include "OSDOptions.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "Util.h"
#include "Aero.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




COSDOptions::COSDOptions()
	: m_fShowOSD(true)
	, m_fPseudoOSD(false)
	, m_TextColor(RGB(0,255,0))
	, m_Opacity(80)
	, m_FadeTime(3000)
	, m_ChannelChangeType(CHANNELCHANGE_LOGOANDTEXT)

	, m_fLayeredWindow(true)
	, m_fCompositionEnabled(false)

	, m_fEnableNotificationBar(true)
	, m_NotificationBarDuration(3000)
	, m_NotificationBarFlags(NOTIFY_EVENTNAME
#ifndef TVH264_FOR_1SEG
		 | NOTIFY_ECMERROR
#endif
		)
{
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize=sizeof(osvi);
	::GetVersionEx(&osvi);
	if (osvi.dwMajorVersion>=6) {
		CAeroGlass Aero;
		if (Aero.IsEnabled())
			m_fCompositionEnabled=true;
	}

	LOGFONT lf;
	DrawUtil::GetSystemFont(DrawUtil::FONT_MESSAGE,&lf);
	m_NotificationBarFont=lf;
	m_NotificationBarFont.lfHeight=
#ifndef TVH264_FOR_1SEG
		-14;
#else
		-12;
#endif

	m_DisplayMenuFont=lf;
	m_fDisplayMenuFontAutoSize=true;
}


COSDOptions::~COSDOptions()
{
}


bool COSDOptions::Read(CSettings *pSettings)
{
	int Value;

	pSettings->Read(TEXT("UseOSD"),&m_fShowOSD);
	pSettings->Read(TEXT("PseudoOSD"),&m_fPseudoOSD);
	pSettings->ReadColor(TEXT("OSDTextColor"),&m_TextColor);
	pSettings->Read(TEXT("OSDOpacity"),&m_Opacity);
	pSettings->Read(TEXT("OSDFadeTime"),&m_FadeTime);
	if (pSettings->Read(TEXT("ChannelOSDType"),&Value)
			&& Value>=CHANNELCHANGE_FIRST && Value<=CHANNELCHANGE_LAST)
		m_ChannelChangeType=(ChannelChangeType)Value;

	pSettings->Read(TEXT("EnableNotificationBar"),&m_fEnableNotificationBar);
	pSettings->Read(TEXT("NotificationBarDuration"),&m_NotificationBarDuration);
	bool f;
	if (pSettings->Read(TEXT("NotifyEventName"),&f))
		EnableNotify(NOTIFY_EVENTNAME,f);
	if (pSettings->Read(TEXT("NotifyEcmError"),&f))
		EnableNotify(NOTIFY_ECMERROR,f);
	// Font
	TCHAR szFont[LF_FACESIZE];
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
	pSettings->Write(TEXT("ChannelOSDType"),(int)m_ChannelChangeType);

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


bool COSDOptions::GetLayeredWindow() const
{
	return m_fLayeredWindow && m_fCompositionEnabled;
}


void COSDOptions::OnDwmCompositionChanged()
{
	CAeroGlass Aero;

	m_fCompositionEnabled=Aero.IsEnabled();
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

			DlgCheckBox_Check(hDlg,IDC_OSDOPTIONS_SHOWOSD,pThis->m_fShowOSD);
			DlgCheckBox_Check(hDlg,IDC_OSDOPTIONS_PSEUDOOSD,pThis->m_fPseudoOSD);
			pThis->m_CurTextColor=pThis->m_TextColor;
			::SetDlgItemInt(hDlg,IDC_OSDOPTIONS_FADETIME,pThis->m_FadeTime/1000,TRUE);
			DlgUpDown_SetRange(hDlg,IDC_OSDOPTIONS_FADETIME_UD,1,UD_MAXVAL);
			static const LPCTSTR ChannelChangeModeText[] = {
				TEXT("ƒƒS‚Æƒ`ƒƒƒ“ƒlƒ‹–¼"),
				TEXT("ƒ`ƒƒƒ“ƒlƒ‹–¼‚Ì‚Ý"),
				TEXT("ƒƒS‚Ì‚Ý"),
			};
			SetComboBoxList(hDlg,IDC_OSDOPTIONS_CHANNELCHANGE,
							ChannelChangeModeText,lengthof(ChannelChangeModeText));
			DlgComboBox_SetCurSel(hDlg,IDC_OSDOPTIONS_CHANNELCHANGE,(int)pThis->m_ChannelChangeType);
			EnableDlgItems(hDlg,IDC_OSDOPTIONS_FIRST,IDC_OSDOPTIONS_LAST,pThis->m_fShowOSD);

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
		case IDC_OSDOPTIONS_SHOWOSD:
			EnableDlgItemsSyncCheckBox(hDlg,IDC_OSDOPTIONS_FIRST,IDC_OSDOPTIONS_LAST,
									   IDC_OSDOPTIONS_SHOWOSD);
			return TRUE;

		case IDC_OSDOPTIONS_TEXTCOLOR:
			{
				COSDOptions *pThis=GetThis(hDlg);

				if (ChooseColorDialog(hDlg,&pThis->m_CurTextColor))
					InvalidateDlgItem(hDlg,IDC_OSDOPTIONS_TEXTCOLOR);
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

				pThis->m_fShowOSD=DlgCheckBox_IsChecked(hDlg,IDC_OSDOPTIONS_SHOWOSD);
				pThis->m_fPseudoOSD=DlgCheckBox_IsChecked(hDlg,IDC_OSDOPTIONS_PSEUDOOSD);
				pThis->m_TextColor=pThis->m_CurTextColor;
				pThis->m_FadeTime=::GetDlgItemInt(hDlg,IDC_OSDOPTIONS_FADETIME,NULL,FALSE)*1000;
				pThis->m_ChannelChangeType=(ChannelChangeType)DlgComboBox_GetCurSel(hDlg,IDC_OSDOPTIONS_CHANNELCHANGE);

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
