#include "stdafx.h"
#include "Options.h"




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


void COptions::OnDestroy()
{
	if (m_hDlg) {
		::RemoveProp(m_hDlg,TEXT("This"));
		m_hDlg=NULL;
	}
}
