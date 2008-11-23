#include "stdafx.h"
#include "TVTest.h"
#include "PanelOptions.h"
#include "DialogUtil.h"
#include "resource.h"




CPanelOptions::CPanelOptions()
{
	m_fSnapAtMainWindow=true;
	m_SnapMargin=4;
	m_fAttachToMainWindow=true;
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
	return true;
}


bool CPanelOptions::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("PanelSnapAtMainWindow"),m_fSnapAtMainWindow);
	pSettings->Write(TEXT("PanelAttachToMainWindow"),m_fAttachToMainWindow);
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
