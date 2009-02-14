#include "stdafx.h"
#include "TVTest.h"
#include "OSDOptions.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "resource.h"




COSDOptions::COSDOptions()
{
	m_fShowOSD=true;
	m_fPseudoOSD=false;
	m_TextColor=RGB(0,255,0);
	m_Opacity=80;
	m_FadeTime=3000;
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
	return true;
}


bool COSDOptions::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("UseOSD"),m_fShowOSD);
	pSettings->Write(TEXT("PseudoOSD"),m_fPseudoOSD);
	pSettings->WriteColor(TEXT("OSDTextColor"),m_TextColor);
	pSettings->Write(TEXT("OSDOpacity"),m_Opacity);
	pSettings->Write(TEXT("OSDFadeTime"),m_FadeTime);
	return true;
}


COSDOptions *COSDOptions::GetThis(HWND hDlg)
{
	return static_cast<COSDOptions*>(GetOptions(hDlg));
}


BOOL CALLBACK COSDOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
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
