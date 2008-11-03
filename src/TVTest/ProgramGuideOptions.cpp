#include "stdafx.h"
#include <commctrl.h>
#include "TVTest.h"
#include "ProgramGuideOptions.h"
#include "Settings.h"
#include "resource.h"




CProgramGuideOptions::CProgramGuideOptions(CProgramGuide *pProgramGuide)
{
	m_pProgramGuide=pProgramGuide;
	m_ViewHours=26;
	m_ItemWidth=m_pProgramGuide->GetItemWidth();
	m_LinesPerHour=m_pProgramGuide->GetLinesPerHour();
}


CProgramGuideOptions::~CProgramGuideOptions()
{
}


bool CProgramGuideOptions::Load(LPCTSTR pszFileName)
{
	CSettings Settings;
	int Value;

	if (!Settings.Open(pszFileName,TEXT("ProgramGuide"),CSettings::OPEN_READ))
		return false;
	if (Settings.Read(TEXT("ViewHours"),&Value)
			&& Value>=MIN_VIEW_HOURS && Value<=MAX_VIEW_HOURS)
		m_ViewHours=Value;
	if (Settings.Read(TEXT("ItemWidth"),&Value)
			&& Value>=CProgramGuide::MIN_ITEM_WIDTH
			&& Value<=CProgramGuide::MAX_ITEM_WIDTH)
		m_ItemWidth=Value;
	if (Settings.Read(TEXT("LinesPerHour"),&Value)
			&& Value>=CProgramGuide::MIN_LINES_PER_HOUR
			&& Value<=CProgramGuide::MAX_LINES_PER_HOUR)
		m_LinesPerHour=Value;
	m_pProgramGuide->SetUIOptions(m_LinesPerHour,m_ItemWidth);
	return true;
}


bool CProgramGuideOptions::Save(LPCTSTR pszFileName) const
{
	CSettings Settings;

	if (!Settings.Open(pszFileName,TEXT("ProgramGuide"),CSettings::OPEN_WRITE))
		return false;
	Settings.Write(TEXT("ViewHours"),m_ViewHours);
	Settings.Write(TEXT("ItemWidth"),m_ItemWidth);
	Settings.Write(TEXT("LinesPerHour"),m_LinesPerHour);
	return true;
}


bool CProgramGuideOptions::GetTimeRange(SYSTEMTIME *pstFirst,SYSTEMTIME *pstLast)
{
	SYSTEMTIME st;
	FILETIME ft;

	::GetLocalTime(&st);
	st.wMinute=0;
	st.wSecond=0;
	st.wMilliseconds=0;
	*pstFirst=st;
	::SystemTimeToFileTime(&st,&ft);
	ft+=(LONGLONG)m_ViewHours*60*60*FILETIME_SECOND;
	::FileTimeToSystemTime(&ft,pstLast);
	return true;
}


CProgramGuideOptions *CProgramGuideOptions::GetThis(HWND hDlg)
{
	return static_cast<CProgramGuideOptions*>(::GetProp(hDlg,TEXT("This")));
}


BOOL CALLBACK CProgramGuideOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CProgramGuideOptions *pThis=dynamic_cast<CProgramGuideOptions*>(OnInitDialog(hDlg,lParam));

			::SetDlgItemInt(hDlg,IDC_PROGRAMGUIDEOPTIONS_VIEWHOURS,pThis->m_ViewHours,TRUE);
			::SendDlgItemMessage(hDlg,IDC_PROGRAMGUIDEOPTIONS_VIEWHOURS_UD,
								 UDM_SETRANGE32,MIN_VIEW_HOURS,MAX_VIEW_HOURS);
			::SetDlgItemInt(hDlg,IDC_PROGRAMGUIDEOPTIONS_CHANNELWIDTH,pThis->m_ItemWidth,TRUE);
			::SendDlgItemMessage(hDlg,IDC_PROGRAMGUIDEOPTIONS_CHANNELWIDTH_UD,
				UDM_SETRANGE32,CProgramGuide::MIN_ITEM_WIDTH,CProgramGuide::MAX_ITEM_WIDTH);
			::SetDlgItemInt(hDlg,IDC_PROGRAMGUIDEOPTIONS_LINESPERHOUR,pThis->m_LinesPerHour,TRUE);
			::SendDlgItemMessage(hDlg,IDC_PROGRAMGUIDEOPTIONS_LINESPERHOUR_UD,
				UDM_SETRANGE32,CProgramGuide::MIN_LINES_PER_HOUR,CProgramGuide::MAX_LINES_PER_HOUR);
		}
		return TRUE;

	/*
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		}
		return TRUE;
	*/

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case PSN_APPLY:
			{
				CProgramGuideOptions *pThis=GetThis(hDlg);
				int Value;

				Value=::GetDlgItemInt(hDlg,IDC_PROGRAMGUIDEOPTIONS_VIEWHOURS,NULL,TRUE);
				Value=LimitRange(Value,(int)MIN_VIEW_HOURS,(int)MAX_VIEW_HOURS);
				if (pThis->m_ViewHours!=Value) {
					SYSTEMTIME stFirst,stLast;
					FILETIME ft;

					pThis->m_ViewHours=Value;
					pThis->m_pProgramGuide->GetTimeRange(&stFirst,NULL);
					::SystemTimeToFileTime(&stFirst,&ft);
					ft+=(LONGLONG)pThis->m_ViewHours*(FILETIME_SECOND*60*60);
					pThis->m_pProgramGuide->SetTimeRange(&stFirst,&stLast);
					pThis->m_pProgramGuide->UpdateProgramGuide();
				}
				Value=::GetDlgItemInt(hDlg,IDC_PROGRAMGUIDEOPTIONS_CHANNELWIDTH,NULL,TRUE);
				pThis->m_ItemWidth=LimitRange(Value,
					(int)CProgramGuide::MIN_ITEM_WIDTH,(int)CProgramGuide::MAX_ITEM_WIDTH);
				Value=::GetDlgItemInt(hDlg,IDC_PROGRAMGUIDEOPTIONS_LINESPERHOUR,NULL,TRUE);
				pThis->m_LinesPerHour=LimitRange(Value,
					(int)CProgramGuide::MIN_LINES_PER_HOUR,(int)CProgramGuide::MAX_LINES_PER_HOUR);
				pThis->m_pProgramGuide->SetUIOptions(pThis->m_LinesPerHour,pThis->m_ItemWidth);
			}
			break;
		}
		break;

	case WM_DESTROY:
		{
			CProgramGuideOptions *pThis=GetThis(hDlg);

			pThis->OnDestroy();
		}
		return TRUE;
	}
	return FALSE;
}
