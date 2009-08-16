#include "stdafx.h"
#include "TVTest.h"
#include "OperationOptions.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




COperationOptions::COperationOptions()
{
	m_pCommandList=NULL;
	m_WheelMode=WHEEL_VOLUME;
	m_WheelShiftMode=WHEEL_CHANNEL;
	m_WheelCtrlMode=WHEEL_AUDIO;
	m_fWheelChannelReverse=false;
	m_WheelChannelDelay=1000;
	m_VolumeStep=5;
	m_WheelZoomStep=5;
	m_fDisplayDragMove=true;
	m_LeftDoubleClickCommand=CM_FULLSCREEN;
	m_RightClickCommand=CM_MENU;
	m_MiddleClickCommand=0;
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
	if (pSettings->Read(TEXT("WheelCtrlMode"),&Value)
			&& Value>=WHEEL_FIRST && Value<=WHEEL_LAST)
		m_WheelCtrlMode=(WheelMode)Value;
	pSettings->Read(TEXT("ReverseWheelChannel"),&m_fWheelChannelReverse);
	if (pSettings->Read(TEXT("WheelChannelDelay"),&Value)) {
		if (Value<WHEEL_CHANNEL_DELAY_MIN)
			Value=WHEEL_CHANNEL_DELAY_MIN;
		m_WheelChannelDelay=Value;
	}
	pSettings->Read(TEXT("VolumeStep"),&m_VolumeStep);
	pSettings->Read(TEXT("WheelZoomStep"),&m_WheelZoomStep);
	pSettings->Read(TEXT("DisplayDragMove"),&m_fDisplayDragMove);
	return true;
}


inline LPCTSTR GetCommandText(const CCommandList *pCommandList,int Command)
{
	if (Command==0)
		return TEXT("");
	return pCommandList->GetCommandTextByID(Command);
}

bool COperationOptions::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("WheelMode"),(int)m_WheelMode);
	pSettings->Write(TEXT("WheelShiftMode"),(int)m_WheelShiftMode);
	pSettings->Write(TEXT("WheelCtrlMode"),(int)m_WheelCtrlMode);
	pSettings->Write(TEXT("ReverseWheelChannel"),m_fWheelChannelReverse);
	pSettings->Write(TEXT("WheelChannelDelay"),m_WheelChannelDelay);
	pSettings->Write(TEXT("VolumeStep"),m_VolumeStep);
	pSettings->Write(TEXT("WheelZoomStep"),m_WheelZoomStep);
	pSettings->Write(TEXT("DisplayDragMove"),m_fDisplayDragMove);
	if (m_pCommandList!=NULL) {
		pSettings->Write(TEXT("LeftDoubleClickCommand"),
			GetCommandText(m_pCommandList,m_LeftDoubleClickCommand));
		pSettings->Write(TEXT("RightClickCommand"),
			GetCommandText(m_pCommandList,m_RightClickCommand));
		pSettings->Write(TEXT("MiddleClickCommand"),
			GetCommandText(m_pCommandList,m_MiddleClickCommand));
	}
	return true;
}


bool COperationOptions::Load(LPCTSTR pszFileName)
{
	CSettings Settings;

	if (Settings.Open(pszFileName,TEXT("Settings"),CSettings::OPEN_READ)) {
		TCHAR szText[CCommandList::MAX_COMMAND_TEXT];

		if (Settings.Read(TEXT("LeftDoubleClickCommand"),szText,lengthof(szText))) {
			if (szText[0]=='\0')
				m_LeftDoubleClickCommand=0;
			else
				m_LeftDoubleClickCommand=m_pCommandList->ParseText(szText);
		}
		if (Settings.Read(TEXT("RightClickCommand"),szText,lengthof(szText))) {
			if (szText[0]=='\0')
				m_RightClickCommand=0;
			else
				m_RightClickCommand=m_pCommandList->ParseText(szText);
		}
		if (Settings.Read(TEXT("MiddleClickCommand"),szText,lengthof(szText))) {
			if (szText[0]=='\0')
				m_MiddleClickCommand=0;
			else
				m_MiddleClickCommand=m_pCommandList->ParseText(szText);
		}
	}
	return true;
}


bool COperationOptions::Initialize(LPCTSTR pszFileName,const CCommandList *pCommandList)
{
	m_pCommandList=pCommandList;
	Load(pszFileName);
	return true;
}


void COperationOptions::InitWheelModeList(HWND hDlg,int ID)
{
	static const LPCTSTR pszWheelMode[] = {
		TEXT("Ç»Çµ"),
		TEXT("âπó "),
		TEXT("É`ÉÉÉìÉlÉã"),
		TEXT("âπê∫"),
		TEXT("ï\é¶î{ó¶"),
		TEXT("î‰ó¶"),
	};
	int i;

	for (i=0;i<lengthof(pszWheelMode);i++)
		DlgComboBox_AddString(hDlg,ID,pszWheelMode[i]);
}


BOOL CALLBACK COperationOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			COperationOptions *pThis=static_cast<COperationOptions*>(OnInitDialog(hDlg,lParam));

			InitWheelModeList(hDlg,IDC_OPTIONS_WHEELMODE);
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_WHEELMODE,pThis->m_WheelMode);
			InitWheelModeList(hDlg,IDC_OPTIONS_WHEELSHIFTMODE);
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_WHEELSHIFTMODE,pThis->m_WheelShiftMode);
			InitWheelModeList(hDlg,IDC_OPTIONS_WHEELCTRLMODE);
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_WHEELCTRLMODE,pThis->m_WheelCtrlMode);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_WHEELCHANNELREVERSE,
												pThis->m_fWheelChannelReverse);
			::SetDlgItemInt(hDlg,IDC_OPTIONS_WHEELCHANNELDELAY,
											pThis->m_WheelChannelDelay,FALSE);
			::SetDlgItemInt(hDlg,IDC_OPTIONS_VOLUMESTEP,pThis->m_VolumeStep,TRUE);
			::SendDlgItemMessage(hDlg,IDC_OPTIONS_VOLUMESTEP_UD,UDM_SETRANGE32,1,100);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_DISPLAYDRAGMOVE,
							  pThis->m_fDisplayDragMove);

			int LeftDoubleClick=0,RightClick=0,MiddleClick=0;
			for (int i=IDC_OPTIONS_MOUSECOMMAND_FIRST;i<=IDC_OPTIONS_MOUSECOMMAND_LAST;i++) {
				DlgComboBox_AddString(hDlg,i,TEXT("Ç»Çµ"));
				DlgComboBox_SetItemData(hDlg,i,0,0);
			}
			int NumCommands=pThis->m_pCommandList->NumCommands();
			for (int i=0;i<NumCommands;i++) {
				TCHAR szText[CCommandList::MAX_COMMAND_NAME];
				int Command=pThis->m_pCommandList->GetCommandID(i);

				pThis->m_pCommandList->GetCommandName(i,szText,lengthof(szText));
				for (int j=IDC_OPTIONS_MOUSECOMMAND_FIRST;j<=IDC_OPTIONS_MOUSECOMMAND_LAST;j++) {
					int Index=DlgComboBox_AddString(hDlg,j,szText);
					DlgComboBox_SetItemData(hDlg,j,Index,Command);
				}
				if (Command==pThis->m_LeftDoubleClickCommand)
					LeftDoubleClick=i+1;
				if (Command==pThis->m_RightClickCommand)
					RightClick=i+1;
				if (Command==pThis->m_MiddleClickCommand)
					MiddleClick=i+1;
			}
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_LEFTDOUBLECLICKCOMMAND,LeftDoubleClick);
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_RIGHTCLICKCOMMAND,RightClick);
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_MIDDLECLICKCOMMAND,MiddleClick);
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
				pThis->m_WheelCtrlMode=(WheelMode)
					DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_WHEELCTRLMODE);
				pThis->m_fWheelChannelReverse=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_WHEELCHANNELREVERSE);
				pThis->m_WheelChannelDelay=
					::GetDlgItemInt(hDlg,IDC_OPTIONS_WHEELCHANNELDELAY,NULL,FALSE);
				if (pThis->m_WheelChannelDelay<WHEEL_CHANNEL_DELAY_MIN)
					pThis->m_WheelChannelDelay=WHEEL_CHANNEL_DELAY_MIN;
				pThis->m_VolumeStep=::GetDlgItemInt(hDlg,IDC_OPTIONS_VOLUMESTEP,NULL,TRUE);
				pThis->m_fDisplayDragMove=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_DISPLAYDRAGMOVE);

				pThis->m_LeftDoubleClickCommand=
					DlgComboBox_GetItemData(hDlg,IDC_OPTIONS_LEFTDOUBLECLICKCOMMAND,
						DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_LEFTDOUBLECLICKCOMMAND));
				pThis->m_RightClickCommand=
					DlgComboBox_GetItemData(hDlg,IDC_OPTIONS_RIGHTCLICKCOMMAND,
						DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_RIGHTCLICKCOMMAND));
				pThis->m_MiddleClickCommand=
					DlgComboBox_GetItemData(hDlg,IDC_OPTIONS_MIDDLECLICKCOMMAND,
						DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_MIDDLECLICKCOMMAND));
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
