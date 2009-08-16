#include "stdafx.h"
#include "Options.h"



COptionFrame *COptions::m_pFrame=NULL;
DWORD COptions::m_GeneralUpdateFlags=0;


COptions::COptions()
{
	m_hDlg=NULL;
	m_UpdateFlags=0;
}


COptions::~COptions()
{
}


COptions *COptions::OnInitDialog(HWND hDlg,LPARAM lParam)
{
	COptions *pThis=reinterpret_cast<COptions*>(lParam);

	::SetProp(hDlg,TEXT("This"),pThis);
	pThis->m_hDlg=hDlg;
	pThis->m_UpdateFlags=0;
	return pThis;
}


COptions *COptions::GetOptions(HWND hDlg)
{
	return static_cast<COptions*>(::GetProp(hDlg,TEXT("This")));
}


void COptions::OnDestroy()
{
	if (m_hDlg) {
		::RemoveProp(m_hDlg,TEXT("This"));
		m_hDlg=NULL;
	}
}


void COptions::SettingError()
{
	if (m_pFrame!=NULL)
		m_pFrame->OnSettingError(this);
}


DWORD COptions::SetUpdateFlag(DWORD Flag)
{
	m_UpdateFlags|=Flag;
	return m_UpdateFlags;
}


DWORD COptions::SetGeneralUpdateFlag(DWORD Flag)
{
	m_GeneralUpdateFlags|=Flag;
	return m_GeneralUpdateFlags;
}
