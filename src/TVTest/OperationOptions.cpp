#include "stdafx.h"
#include "TVTest.h"
#include "OperationOptions.h"
#include "DialogUtil.h"
#include "resource.h"




COperationOptions::COperationOptions()
{
	m_WheelMode=WHEEL_VOLUME;
	m_WheelShiftMode=WHEEL_CHANNEL;
	m_fWheelChannelReverse=false;
	m_WheelChannelDelay=1000;
	m_VolumeStep=5;
	m_fDisplayDragMove=true;
}


COperationOptions::~COperationOptions()
{
}


bool COperationOptions::Read(CSettings *pSettings)
{
	int Value;

	if (pSettings->Read(TEXT("WheelMode"),&Value)
			&& Value>=WHEEL_FIRST && Value<=WHEEL_LAST)
		m_WheelMode=(WheelMode)Value;
	if (pSettings->Read(TEXT("WheelShiftMode"),&Value)
			&& Value>=WHEEL_FIRST && Value<=WHEEL_LAST)
		m_WheelShiftMode=(WheelMode)Value;
	pSettings->Read(TEXT("ReverseWheelChannel"),&m_fWheelChannelReverse);
	if (pSettings->Read(TEXT("WheelChannelDelay"),&Value)) {
		if (Value<WHEEL_CHANNEL_DELAY_MIN)
			Value=WHEEL_CHANNEL_DELAY_MIN;
		m_WheelChannelDelay=Value;
	}
	pSettings->Read(TEXT("VolumeStep"),&m_VolumeStep);
	pSettings->Read(TEXT("DisplayDragMove"),&m_fDisplayDragMove);
	return true;
}


bool COperationOptions::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("WheelMode"),(int)m_WheelMode);
	pSettings->Write(TEXT("WheelShiftMode"),(int)m_WheelShiftMode);
	pSettings->Write(TEXT("ReverseWheelChannel"),m_fWheelChannelReverse);
	pSettings->Write(TEXT("WheelChannelDelay"),m_WheelChannelDelay);
	pSettings->Write(TEXT("VolumeStep"),m_VolumeStep);
	pSettings->Write(TEXT("DisplayDragMove"),m_fDisplayDragMove);
	return true;
}


BOOL CALLBACK COperationOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			COperationOptions *pThis=static_cast<COperationOptions*>(OnInitDialog(hDlg,lParam));
			static const LPCTSTR pszWheelMode[] = {
				TEXT("‚È‚µ"),TEXT("‰¹—Ê"),TEXT("ƒ`ƒƒƒ“ƒlƒ‹")
			};
			int i;

			for (i=0;i<lengthof(pszWheelMode);i++)
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_WHEELMODE,pszWheelMode[i]);
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_WHEELMODE,pThis->m_WheelMode);
			for (i=0;i<lengthof(pszWheelMode);i++)
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_WHEELSHIFTMODE,pszWheelMode[i]);
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_WHEELSHIFTMODE,pThis->m_WheelShiftMode);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_WHEELCHANNELREVERSE,
												pThis->m_fWheelChannelReverse);
			::SetDlgItemInt(hDlg,IDC_OPTIONS_WHEELCHANNELDELAY,
											pThis->m_WheelChannelDelay,FALSE);
			::SetDlgItemInt(hDlg,IDC_OPTIONS_VOLUMESTEP,pThis->m_VolumeStep,TRUE);
			::SendDlgItemMessage(hDlg,IDC_OPTIONS_VOLUMESTEP_UD,UDM_SETRANGE32,1,100);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_DISPLAYDRAGMOVE,
							  pThis->m_fDisplayDragMove);
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				COperationOptions *pThis=GetThis(hDlg);

				pThis->m_WheelMode=(WheelMode)
					DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_WHEELMODE);
				pThis->m_WheelShiftMode=(WheelMode)
					DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_WHEELSHIFTMODE);
				pThis->m_fWheelChannelReverse=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_WHEELCHANNELREVERSE);
				pThis->m_WheelChannelDelay=
					::GetDlgItemInt(hDlg,IDC_OPTIONS_WHEELCHANNELDELAY,NULL,FALSE);
				if (pThis->m_WheelChannelDelay<WHEEL_CHANNEL_DELAY_MIN)
					pThis->m_WheelChannelDelay=WHEEL_CHANNEL_DELAY_MIN;
				pThis->m_VolumeStep=::GetDlgItemInt(hDlg,IDC_OPTIONS_VOLUMESTEP,NULL,TRUE);
				pThis->m_fDisplayDragMove=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_DISPLAYDRAGMOVE);
			}
			break;
		}
		break;

	case WM_DESTROY:
		{
			COperationOptions *pThis=GetThis(hDlg);

			pThis->OnDestroy();
		}
		return TRUE;
	}
	return FALSE;
}


COperationOptions *COperationOptions::GetThis(HWND hDlg)
{
	return static_cast<COperationOptions*>(GetOptions(hDlg));
}
