#include "stdafx.h"
#include "Dialog.h"




CBasicDialog::CBasicDialog()
	: m_hDlg(NULL)
	, m_fModeless(false)
{
}


CBasicDialog::~CBasicDialog()
{
	Destroy();
}


int CBasicDialog::ShowDialog(HWND hwndOwner,HINSTANCE hinst,LPCTSTR pszTemplate)
{
	if (m_hDlg!=NULL)
		return -1;
	return ::DialogBoxParam(hinst,pszTemplate,hwndOwner,DialogProc,
							reinterpret_cast<LPARAM>(this));
}


bool CBasicDialog::CreateDialogWindow(HWND hwndOwner,HINSTANCE hinst,LPCTSTR pszTemplate)
{
	if (m_hDlg!=NULL)
		return false;
	if (::CreateDialogParam(hinst,pszTemplate,hwndOwner,DialogProc,
							reinterpret_cast<LPARAM>(this))==NULL)
		return false;
	m_fModeless=true;
	return true;
}


CBasicDialog *CBasicDialog::GetThis(HWND hDlg)
{
	return static_cast<CBasicDialog*>(::GetProp(hDlg,TEXT("This")));
}


INT_PTR CALLBACK CBasicDialog::DialogProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	CBasicDialog *pThis;

	if (uMsg==WM_INITDIALOG) {
		pThis=reinterpret_cast<CBasicDialog*>(lParam);
		pThis->m_hDlg=hDlg;
		::SetProp(hDlg,TEXT("This"),pThis);
	} else {
		pThis=GetThis(hDlg);
		if (uMsg==WM_NCDESTROY) {
			pThis->DlgProc(hDlg,uMsg,wParam,lParam);
			pThis->m_hDlg=NULL;
			::RemoveProp(hDlg,TEXT("This"));
			return TRUE;
		}
	}
	if (pThis!=NULL)
		return pThis->DlgProc(hDlg,uMsg,wParam,lParam);
	return FALSE;
}


bool CBasicDialog::IsCreated() const
{
	return m_hDlg!=NULL;
}


bool CBasicDialog::Destroy()
{
	if (m_hDlg==NULL)
		return false;
	if (m_fModeless)
		return ::DestroyWindow(m_hDlg)!=FALSE;
	::SendMessage(m_hDlg,WM_CLOSE,0,0);
	return true;
}


bool CBasicDialog::ProcessMessage(LPMSG pMsg)
{
	if (m_hDlg==NULL)
		return false;
	return ::IsDialogMessage(m_hDlg,pMsg)!=FALSE;
}


bool CBasicDialog::IsVisible() const
{
	return m_hDlg!=NULL && ::IsWindowVisible(m_hDlg);
}


bool CBasicDialog::SetVisible(bool fVisible)
{
	if (m_hDlg==NULL)
		return false;
	return ::ShowWindow(m_hDlg,fVisible?SW_SHOW:SW_HIDE)!=FALSE;
}




CResizableDialog::CResizableDialog()
	: m_hwndSizeGrip(NULL)
{
}


CResizableDialog::~CResizableDialog()
{
}


INT_PTR CResizableDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			RECT rc;

			::GetWindowRect(hDlg,&rc);
			m_MinSize.cx=rc.right-rc.left;
			m_MinSize.cy=rc.bottom-rc.top;
			::GetClientRect(hDlg,&rc);
			m_OriginalClientSize.cx=rc.right-rc.left;
			m_OriginalClientSize.cy=rc.bottom-rc.top;
			m_hwndSizeGrip=::CreateWindowEx(0,TEXT("SCROLLBAR"),NULL,
				WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SBS_SIZEGRIP |
												SBS_SIZEBOXBOTTOMRIGHTALIGN,
				0,0,rc.right,rc.bottom,m_hDlg,(HMENU)0,
				reinterpret_cast<HINSTANCE>(::GetWindowLongPtr(m_hDlg,GWLP_HINSTANCE)),NULL);
			::SetWindowPos(m_hwndSizeGrip,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
		}
		return TRUE;

	case WM_GETMINMAXINFO:
		{
			MINMAXINFO *pmmi=reinterpret_cast<MINMAXINFO*>(lParam);

			pmmi->ptMinTrackSize.x=m_MinSize.cx;
			pmmi->ptMinTrackSize.y=m_MinSize.cy;
		}
		return TRUE;

	case WM_SIZE:
		DoLayout();
		return TRUE;
	}
	return FALSE;
}


void CResizableDialog::DoLayout()
{
	RECT rc;
	int Width,Height;

	::GetClientRect(m_hDlg,&rc);
	Width=rc.right-rc.left;
	Height=rc.bottom-rc.top;
	for (size_t i=0;i<m_ControlList.size();i++) {
		rc=m_ControlList[i].rcOriginal;
		if ((m_ControlList[i].Align&ALIGN_RIGHT)!=0) {
			rc.right+=Width-m_OriginalClientSize.cx;
			if ((m_ControlList[i].Align&ALIGN_LEFT)==0)
				rc.left+=Width-m_OriginalClientSize.cx;
		}
		if ((m_ControlList[i].Align&ALIGN_BOTTOM)!=0) {
			rc.bottom+=Height-m_OriginalClientSize.cy;
			if ((m_ControlList[i].Align&ALIGN_TOP)==0)
				rc.top+=Height-m_OriginalClientSize.cy;
		}
		::MoveWindow(::GetDlgItem(m_hDlg,m_ControlList[i].ID),
					 rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,TRUE);
	}
	if (m_hwndSizeGrip!=NULL) {
		::GetWindowRect(m_hwndSizeGrip,&rc);
		::OffsetRect(&rc,-rc.left,-rc.top);
		::MoveWindow(m_hwndSizeGrip,Width-rc.right,Height-rc.bottom,
					 rc.right,rc.bottom,TRUE);
	}
}


bool CResizableDialog::AddControl(int ID,unsigned int Align)
{
	HWND hwnd=::GetDlgItem(m_hDlg,ID);
	if (hwnd==NULL)
		return false;

	LayoutItem Item;

	Item.ID=ID;
	::GetWindowRect(hwnd,&Item.rcOriginal);
	::MapWindowPoints(NULL,m_hDlg,reinterpret_cast<LPPOINT>(&Item.rcOriginal),2);
	Item.Align=Align;
	m_ControlList.push_back(Item);
	return true;
}


bool CResizableDialog::AddControls(int FirstID,int LastID,unsigned int Align)
{
	if (FirstID>LastID)
		return false;
	for (int i=FirstID;i<=LastID;i++) {
		if (!AddControl(i,Align))
			return false;
	}
	return true;
}
