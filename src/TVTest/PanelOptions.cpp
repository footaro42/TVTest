#include "stdafx.h"
#include "TVTest.h"
#include "PanelOptions.h"
#include "DialogUtil.h"
#include "resource.h"




CPanelOptions::CPanelOptions(CPanelFrame *pPanelFrame)
{
	m_pPanelFrame=pPanelFrame;
	m_fSnapAtMainWindow=true;
	m_SnapMargin=4;
	m_fAttachToMainWindow=true;
	m_Opacity=100;
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
	pSettings->Read(TEXT("PanelSnapAtMainWindow"),&m_fSnapAtMainWindow);
	pSettings->Read(TEXT("PanelAttachToMainWindow"),&m_fAttachToMainWindow);
	if (pSettings->Read(TEXT("PanelOpacity"),&m_Opacity))
		m_pPanelFrame->SetOpacity(m_Opacity);
	return true;
}


bool CPanelOptions::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("PanelSnapAtMainWindow"),m_fSnapAtMainWindow);
	pSettings->Write(TEXT("PanelAttachToMainWindow"),m_fAttachToMainWindow);
	pSettings->Write(TEXT("PanelOpacity"),m_Opacity);
	return true;
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
