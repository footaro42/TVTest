#include "stdafx.h"
#include "TVTest.h"
#include "PanelOptions.h"
#include "InfoPanel.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CPanelOptions::CPanelOptions(CPanelFrame *pPanelFrame)
{
	m_pPanelFrame=pPanelFrame;
	m_fSnapAtMainWindow=true;
	m_SnapMargin=4;
	m_fAttachToMainWindow=true;
	m_Opacity=100;
	::GetObject(::GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),&m_Font);
	m_FirstTab=-1;
	m_LastTab=0;
}


CPanelOptions::~CPanelOptions()
{
}


void CPanelOptions::SetSnapAtMainWindow(bool fSnap)
{
	m_fSnapAtMainWindow=fSnap;
}


bool CPanelOptions::SetSnapMargin(int Margin)
{
	if (Margin<1)
		return false;
	m_SnapMargin=Margin;
	return true;
}


void CPanelOptions::SetAttachToMainWindow(bool fAttach)
{
	m_fAttachToMainWindow=fAttach;
}


bool CPanelOptions::Read(CSettings *pSettings)
{
	int Value;

	if (pSettings->Read(TEXT("InfoCurTab"),&Value)
			&& Value>=PANEL_TAB_FIRST && Value<=PANEL_TAB_LAST)
		m_LastTab=Value;
	if (pSettings->Read(TEXT("PanelFirstTab"),&Value)
			&& Value>=-1 && Value<=PANEL_TAB_LAST)
		m_FirstTab=Value;
	pSettings->Read(TEXT("PanelSnapAtMainWindow"),&m_fSnapAtMainWindow);
	pSettings->Read(TEXT("PanelAttachToMainWindow"),&m_fAttachToMainWindow);
	if (pSettings->Read(TEXT("PanelOpacity"),&m_Opacity))
		m_pPanelFrame->SetOpacity(m_Opacity);

	// Font
	TCHAR szFont[LF_FACESIZE];
	if (pSettings->Read(TEXT("PanelFontName"),szFont,LF_FACESIZE) && szFont[0]!='\0') {
		::lstrcpy(m_Font.lfFaceName,szFont);
		m_Font.lfEscapement=0;
		m_Font.lfOrientation=0;
		m_Font.lfUnderline=0;
		m_Font.lfStrikeOut=0;
		m_Font.lfCharSet=DEFAULT_CHARSET;
		m_Font.lfOutPrecision=OUT_DEFAULT_PRECIS;
		m_Font.lfClipPrecision=CLIP_DEFAULT_PRECIS;
		m_Font.lfQuality=DRAFT_QUALITY;
		m_Font.lfPitchAndFamily=DEFAULT_PITCH | FF_DONTCARE;
	}
	if (pSettings->Read(TEXT("PanelFontSize"),&Value)) {
		m_Font.lfHeight=Value;
		m_Font.lfWidth=0;
	}
	if (pSettings->Read(TEXT("PanelFontWeight"),&Value))
		m_Font.lfWeight=Value;
	if (pSettings->Read(TEXT("PanelFontItalic"),&Value))
		m_Font.lfItalic=Value;
	/*
	CInfoPanel *pPanel=dynamic_cast<CInfoPanel*>(m_pPanelFrame->GetWindow());
	if (pPanel!=NULL) {
		pPanel->SetTabFont(&m_Font);
		pPanel->SetPageFont(&m_Font);
	}
	*/

	return true;
}


bool CPanelOptions::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("PanelFirstTab"),m_FirstTab);
	pSettings->Write(TEXT("PanelSnapAtMainWindow"),m_fSnapAtMainWindow);
	pSettings->Write(TEXT("PanelAttachToMainWindow"),m_fAttachToMainWindow);
	pSettings->Write(TEXT("PanelOpacity"),m_Opacity);
	// Font
	pSettings->Write(TEXT("PanelFontName"),m_Font.lfFaceName);
	pSettings->Write(TEXT("PanelFontSize"),(int)m_Font.lfHeight);
	pSettings->Write(TEXT("PanelFontWeight"),(int)m_Font.lfWeight);
	pSettings->Write(TEXT("PanelFontItalic"),(int)m_Font.lfItalic);
	return true;
}


static void SetFontInfo(HWND hDlg,const LOGFONT *plf)
{
	HDC hdc;
	TCHAR szText[LF_FACESIZE+16];

	hdc=GetDC(hDlg);
	if (hdc==NULL)
		return;
	wsprintf(szText,TEXT("%s, %d pt"),plf->lfFaceName,CalcFontPointHeight(hdc,plf));
	SetDlgItemText(hDlg,IDC_PANELOPTIONS_FONTINFO,szText);
	ReleaseDC(hDlg,hdc);
}


int CPanelOptions::GetFirstTab() const
{
	return m_FirstTab>=0?m_FirstTab:m_LastTab;
}


CPanelOptions *CPanelOptions::GetThis(HWND hDlg)
{
	return static_cast<CPanelOptions*>(GetOptions(hDlg));
}


BOOL CALLBACK CPanelOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CPanelOptions *pThis=dynamic_cast<CPanelOptions*>(OnInitDialog(hDlg,lParam));

			DlgCheckBox_Check(hDlg,IDC_PANELOPTIONS_SNAPATMAINWINDOW,
												pThis->m_fSnapAtMainWindow);
			DlgCheckBox_Check(hDlg,IDC_PANELOPTIONS_ATTACHTOMAINWINDOW,
												pThis->m_fAttachToMainWindow);

			// Opacity
			::SendDlgItemMessage(hDlg,IDC_PANELOPTIONS_OPACITY_TB,
										TBM_SETRANGE,TRUE,MAKELPARAM(20,100));
			::SendDlgItemMessage(hDlg,IDC_PANELOPTIONS_OPACITY_TB,
										TBM_SETPOS,TRUE,pThis->m_Opacity);
			::SendDlgItemMessage(hDlg,IDC_PANELOPTIONS_OPACITY_TB,
										TBM_SETPAGESIZE,0,10);
			::SendDlgItemMessage(hDlg,IDC_PANELOPTIONS_OPACITY_TB,
										TBM_SETTICFREQ,10,0);
			::SetDlgItemInt(hDlg,IDC_PANELOPTIONS_OPACITY_EDIT,pThis->m_Opacity,TRUE);
			::SendDlgItemMessage(hDlg,IDC_PANELOPTIONS_OPACITY_UD,
										UDM_SETRANGE,0,MAKELPARAM(100,20));

			pThis->m_CurSettingFont=pThis->m_Font;
			SetFontInfo(hDlg,&pThis->m_Font);

			static const LPCTSTR pszTabList[] = {
				TEXT("最後に表示したタブ"),
				TEXT("情報"),
				TEXT("番組表"),
				TEXT("チャンネル"),
				TEXT("操作"),
			};
			for (int i=0;i<lengthof(pszTabList);i++)
				DlgComboBox_AddString(hDlg,IDC_PANELOPTIONS_FIRSTTAB,pszTabList[i]);
			DlgComboBox_SetCurSel(hDlg,IDC_PANELOPTIONS_FIRSTTAB,pThis->m_FirstTab+1);
		}
		return TRUE;

	case WM_HSCROLL:
		if (reinterpret_cast<HWND>(lParam)==
				::GetDlgItem(hDlg,IDC_PANELOPTIONS_OPACITY_TB)) {
			SyncEditWithTrackBar(hDlg,IDC_PANELOPTIONS_OPACITY_TB,
									  IDC_PANELOPTIONS_OPACITY_EDIT);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PANELOPTIONS_OPACITY_EDIT:
			if (HIWORD(wParam)==EN_CHANGE)
				SyncTrackBarWithEdit(hDlg,IDC_PANELOPTIONS_OPACITY_EDIT,
										  IDC_PANELOPTIONS_OPACITY_TB);
			return TRUE;

		case IDC_PANELOPTIONS_CHOOSEFONT:
			{
				CPanelOptions *pThis=GetThis(hDlg);

				if (ChooseFontDialog(hDlg,&pThis->m_CurSettingFont))
					SetFontInfo(hDlg,&pThis->m_CurSettingFont);
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CPanelOptions *pThis=GetThis(hDlg);

				pThis->m_fSnapAtMainWindow=
					DlgCheckBox_IsChecked(hDlg,IDC_PANELOPTIONS_SNAPATMAINWINDOW);
				pThis->m_fAttachToMainWindow=
					DlgCheckBox_IsChecked(hDlg,IDC_PANELOPTIONS_ATTACHTOMAINWINDOW);
				pThis->m_Opacity=::GetDlgItemInt(hDlg,IDC_PANELOPTIONS_OPACITY_EDIT,NULL,TRUE);
				pThis->m_pPanelFrame->SetOpacity(pThis->m_Opacity);
				if (!CompareLogFont(&pThis->m_Font,&pThis->m_CurSettingFont)) {
					pThis->m_Font=pThis->m_CurSettingFont;
					CInfoPanel *pPanel=dynamic_cast<CInfoPanel*>(pThis->m_pPanelFrame->GetWindow());
					if (pPanel!=NULL) {
						pPanel->SetTabFont(&pThis->m_Font);
						pPanel->SetPageFont(&pThis->m_Font);
					}
				}
				pThis->m_FirstTab=DlgComboBox_GetCurSel(hDlg,IDC_PANELOPTIONS_FIRSTTAB)-1;
			}
			break;
		}
		break;

	case WM_DESTROY:
		{
			CPanelOptions *pThis=GetThis(hDlg);

			pThis->OnDestroy();
		}
		return TRUE;
	}
	return FALSE;
}
