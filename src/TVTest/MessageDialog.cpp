#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "MessageDialog.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CMessageDialog::CMessageDialog()
	: m_hLib(NULL)
	, m_pszText(NULL)
	, m_pszTitle(NULL)
	, m_pszSystemMessage(NULL)
	, m_pszCaption(NULL)
{
}


CMessageDialog::~CMessageDialog()
{
	if (m_hLib)
		::FreeLibrary(m_hLib);
	delete [] m_pszText;
	delete [] m_pszTitle;
	delete [] m_pszSystemMessage;
	delete [] m_pszCaption;
}


void CMessageDialog::LogFontToCharFormat(const LOGFONT *plf,CHARFORMAT *pcf)
{
	pcf->cbSize=sizeof(CHARFORMAT);
	pcf->dwMask=CFM_BOLD | CFM_CHARSET | CFM_COLOR | CFM_FACE | CFM_ITALIC | CFM_SIZE | CFM_STRIKEOUT | CFM_UNDERLINE;
	HDC hdc=::GetDC(m_hDlg);
	pcf->yHeight=abs(plf->lfHeight)*72*20/::GetDeviceCaps(hdc,LOGPIXELSY);
	::ReleaseDC(m_hDlg,hdc);
	pcf->dwEffects=0;
	if (plf->lfWeight>=FW_BOLD)
		pcf->dwEffects|=CFE_BOLD;
	if (plf->lfItalic)
		pcf->dwEffects|=CFE_ITALIC;
	if (plf->lfUnderline)
		pcf->dwEffects|=CFE_UNDERLINE;
	if (plf->lfStrikeOut)
		pcf->dwEffects|=CFE_STRIKEOUT;
	pcf->crTextColor=::GetSysColor(COLOR_WINDOWTEXT);
	pcf->bPitchAndFamily=plf->lfPitchAndFamily;
	pcf->bCharSet=plf->lfCharSet;
	::lstrcpy(pcf->szFaceName,plf->lfFaceName);
}


void CMessageDialog::AppendText(HWND hwndEdit,LPCTSTR pszText,const CHARFORMAT *pcf)
{
	CHARRANGE cr;

	cr.cpMin=0;
	cr.cpMax=-1;
	::SendMessage(hwndEdit,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&cr));
	::SendMessage(hwndEdit,EM_EXGETSEL,0,reinterpret_cast<LPARAM>(&cr));
	cr.cpMin=cr.cpMax;
	::SendMessage(hwndEdit,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&cr));
	::SendMessage(hwndEdit,EM_REPLACESEL,0,reinterpret_cast<LPARAM>(pszText));
	cr.cpMin-=2;
	cr.cpMax=-1;
	::SendMessage(hwndEdit,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&cr));
	::SendMessage(hwndEdit,EM_SETCHARFORMAT,SCF_SELECTION,reinterpret_cast<LPARAM>(pcf));
	cr.cpMin=cr.cpMax=0;
	::SendMessage(hwndEdit,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&cr));
}


CMessageDialog *CMessageDialog::GetThis(HWND hDlg)
{
	return static_cast<CMessageDialog*>(::GetProp(hDlg,TEXT("This")));
}


BOOL CALLBACK CMessageDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CMessageDialog *pThis=reinterpret_cast<CMessageDialog*>(lParam);
			HWND hwndEdit=::GetDlgItem(hDlg,IDC_ERROR_MESSAGE);
			CHARFORMAT cf;
			CHARFORMAT cfBold;

			::SetProp(hDlg,TEXT("This"),pThis);
			pThis->m_hDlg=hDlg;

			if (pThis->m_pszCaption!=NULL)
				::SetWindowText(hDlg,pThis->m_pszCaption);

			::SendDlgItemMessage(hDlg,IDC_ERROR_ICON,STM_SETICON,
				reinterpret_cast<WPARAM>(::LoadIcon(NULL,
					pThis->m_MessageType==TYPE_INFO?IDI_INFORMATION:
					pThis->m_MessageType==TYPE_WARNING?IDI_WARNING:IDI_ERROR)),0);
			::SendDlgItemMessage(hDlg,IDC_ERROR_MESSAGE,EM_SETBKGNDCOLOR,0,::GetSysColor(COLOR_WINDOW));

			NONCLIENTMETRICS ncm;
#if WINVER<0x0600
			ncm.cbSize=sizeof(ncm);
#else
			ncm.cbSize=offsetof(NONCLIENTMETRICS,iPaddedBorderWidth);
#endif
			::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,ncm.cbSize,&ncm,0);
			pThis->LogFontToCharFormat(&ncm.lfMessageFont,&cf);
			cfBold=cf;
			cfBold.dwEffects|=CFE_BOLD;
			if (pThis->m_pszTitle!=NULL) {
				pThis->AppendText(hwndEdit,pThis->m_pszTitle,&cfBold);
				pThis->AppendText(hwndEdit,TEXT("\n"),&cf);
			}
			if (pThis->m_pszText!=NULL) {
				pThis->AppendText(hwndEdit,pThis->m_pszText,&cf);
			}
			if (pThis->m_pszSystemMessage!=NULL) {
				pThis->AppendText(hwndEdit,TEXT("\n\nWindowsのエラーメッセージ :\n"),&cfBold);
				pThis->AppendText(hwndEdit,pThis->m_pszSystemMessage,&cf);
			}
			int NumLines=::SendMessage(hwndEdit,EM_GETLINECOUNT,0,0);
			int MaxWidth=0;
			for (int i=0;i<NumLines;i++) {
				int Index=::SendMessage(hwndEdit,EM_LINEINDEX,i,0);
				POINTL pt;
				::SendMessage(hwndEdit,EM_POSFROMCHAR,
							  reinterpret_cast<WPARAM>(&pt),
							  Index+::SendMessage(hwndEdit,EM_LINELENGTH,Index,0));
				if (pt.x>MaxWidth)
					MaxWidth=pt.x;
			}
			MaxWidth+=8;
			RECT rcEdit,rcDlg,rcClient,rcOK;
			::GetWindowRect(hwndEdit,&rcEdit);
			::OffsetRect(&rcEdit,-rcEdit.left,-rcEdit.top);
			::SetWindowPos(hwndEdit,NULL,0,0,MaxWidth,rcEdit.bottom,
						   SWP_NOMOVE | SWP_NOZORDER);
			::GetWindowRect(hDlg,&rcDlg);
			::GetClientRect(hDlg,&rcClient);
			::GetWindowRect(::GetDlgItem(hDlg,IDOK),&rcOK);
			MapWindowRect(NULL,hDlg,&rcOK);
			int Offset=MaxWidth-rcEdit.right;
			::SetWindowPos(::GetDlgItem(hDlg,IDOK),NULL,
						   rcOK.left+Offset,rcOK.top,0,0,
						   SWP_NOSIZE | SWP_NOZORDER);
			::SetWindowPos(hDlg,NULL,0,0,(rcDlg.right-rcDlg.left)+Offset,rcDlg.bottom-rcDlg.top,SWP_NOMOVE | SWP_NOZORDER);
			::SendMessage(hwndEdit,EM_SETEVENTMASK,0,ENM_REQUESTRESIZE | ENM_MOUSEEVENTS);
			::SendDlgItemMessage(hDlg,IDC_ERROR_MESSAGE,EM_REQUESTRESIZE,0,0);

			AdjustDialogPos(::GetParent(hDlg),hDlg);
		}
		return TRUE;

	/*
	case WM_SIZE:
		::SendDlgItemMessage(hDlg,IDC_ERROR_MESSAGE,EM_REQUESTRESIZE,0,0);
		return TRUE;
	*/

	case WM_PAINT:
		{
			CMessageDialog *pThis=GetThis(hDlg);
			PAINTSTRUCT ps;
			RECT rcClient,rcEdit,rc;

			::BeginPaint(hDlg,&ps);
			::GetWindowRect(::GetDlgItem(hDlg,IDC_ERROR_MESSAGE),&rcEdit);
			MapWindowRect(NULL,hDlg,&rcEdit);
			::GetClientRect(hDlg,&rcClient);
			::SetRect(&rc,0,0,rcClient.right,rcEdit.bottom);
			::FillRect(ps.hdc,&rc,::GetSysColorBrush(COLOR_WINDOW));
			::EndPaint(hDlg,&ps);
		}
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if (reinterpret_cast<HWND>(lParam)==::GetDlgItem(hDlg,IDC_ERROR_ICON))
			return reinterpret_cast<BOOL>(::GetSysColorBrush(COLOR_WINDOW));

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case EN_REQUESTRESIZE:
			{
				REQRESIZE *prr=reinterpret_cast<REQRESIZE*>(lParam);
				RECT rcEdit,rcDialog,rcClient,rcOK;
				int Width,Height,MinWidth;
				int XOffset,YOffset;
				HWND hwndOK;

				::GetWindowRect(hDlg,&rcDialog);
				::GetClientRect(hDlg,&rcClient);
				::GetWindowRect(prr->nmhdr.hwndFrom,&rcEdit);
				hwndOK=::GetDlgItem(hDlg,IDOK);
				::GetWindowRect(hwndOK,&rcOK);
				MapWindowRect(NULL,hDlg,&rcOK);
				MinWidth=(rcOK.right-rcOK.left)+(rcClient.right-rcOK.right)*2;
				Width=prr->rc.right-prr->rc.left;
				if (Width<MinWidth)
					Width=MinWidth;
				Height=prr->rc.bottom-prr->rc.top;
				if (Width==rcEdit.right-rcEdit.left
						&& Height==rcEdit.bottom-rcEdit.top)
					break;
				XOffset=Width-(rcEdit.right-rcEdit.left);
				YOffset=Height-(rcEdit.bottom-rcEdit.top);
				::SetWindowPos(prr->nmhdr.hwndFrom,NULL,0,0,Width,Height,
							   SWP_NOMOVE | SWP_NOZORDER);
				::SetRect(&rcEdit,0,0,Width,Height);
				::SendDlgItemMessage(hDlg,IDC_ERROR_MESSAGE,EM_SETRECT,0,reinterpret_cast<LPARAM>(&rcEdit));
				rcDialog.right+=XOffset;
				rcDialog.bottom+=YOffset;
				::MoveWindow(hDlg,rcDialog.left,rcDialog.top,
							 rcDialog.right-rcDialog.left,
							 rcDialog.bottom-rcDialog.top,TRUE);
				::MoveWindow(hwndOK,rcOK.left+XOffset,rcOK.top+YOffset,
							 rcOK.right-rcOK.left,rcOK.bottom-rcOK.top,TRUE);
			}
			return TRUE;

		case EN_MSGFILTER:
			if (reinterpret_cast<MSGFILTER*>(lParam)->msg==WM_RBUTTONDOWN) {
				HMENU hmenu;
				POINT pt;

				hmenu=::CreatePopupMenu();
				::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,IDC_ERROR_COPY,TEXT("コピー(&C)"));
				::GetCursorPos(&pt);
				::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON,pt.x,pt.y,0,hDlg,NULL);
				::DestroyMenu(hmenu);
			}
			return TRUE;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ERROR_COPY:
			{
				HWND hwndEdit=::GetDlgItem(hDlg,IDC_ERROR_MESSAGE);

				if (::SendMessage(hwndEdit,EM_SELECTIONTYPE,0,0)==SEL_EMPTY) {
					CHARRANGE cr,crOld;

					::SendMessage(hwndEdit,EM_HIDESELECTION,TRUE,0);
					::SendMessage(hwndEdit,EM_EXGETSEL,0,reinterpret_cast<LPARAM>(&crOld));
					cr.cpMin=0;
					cr.cpMax=-1;
					::SendMessage(hwndEdit,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&cr));
					::SendMessage(hwndEdit,WM_COPY,0,0);
					::SendMessage(hwndEdit,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&crOld));
					::SendMessage(hwndEdit,EM_HIDESELECTION,FALSE,0);
				} else {
					::SendMessage(hwndEdit,WM_COPY,0,0);
				}
			}
			return TRUE;

		case IDOK:
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CMessageDialog *pThis=GetThis(hDlg);

			SAFE_DELETE_ARRAY(pThis->m_pszText);
			SAFE_DELETE_ARRAY(pThis->m_pszTitle);
			SAFE_DELETE_ARRAY(pThis->m_pszSystemMessage);
			SAFE_DELETE_ARRAY(pThis->m_pszCaption);
			::RemoveProp(hDlg,TEXT("This"));
		}
		return TRUE;
	}
	return FALSE;
}


bool CMessageDialog::Show(HWND hwndOwner,MessageType Type,LPCTSTR pszText,LPCTSTR pszTitle,LPCTSTR pszSystemMessage,LPCTSTR pszCaption)
{
	if (pszText==NULL && pszTitle==NULL && pszSystemMessage==NULL)
		return false;
	if (m_hLib==NULL) {
		m_hLib=::LoadLibrary(TEXT("Riched32.dll"));
		if (m_hLib==NULL) {
			TCHAR szMessage[1024];

			szMessage[0]='\0';
			if (pszTitle!=NULL)
				::lstrcat(szMessage,pszTitle);
			if (pszText!=NULL) {
				if (szMessage[0]!='\0')
					::lstrcat(szMessage,TEXT("\n"));
				::lstrcat(szMessage,pszText);
			}
			if (pszSystemMessage!=NULL) {
				if (szMessage[0]!='\0')
					::lstrcat(szMessage,TEXT("\n\n"));
				::lstrcat(szMessage,TEXT("Windowsのエラーメッセージ:\n"));
				::lstrcat(szMessage,pszSystemMessage);
			}
			return ::MessageBox(hwndOwner,szMessage,pszCaption,MB_OK |
								(Type==TYPE_INFO?MB_ICONINFORMATION:
								 Type==TYPE_WARNING?MB_ICONEXCLAMATION:
								 Type==TYPE_ERROR?MB_ICONSTOP:0))==IDOK;
		}
	}
	ReplaceString(&m_pszText,pszText);
	ReplaceString(&m_pszTitle,pszTitle);
	ReplaceString(&m_pszSystemMessage,pszSystemMessage);
	ReplaceString(&m_pszCaption,pszCaption);
	m_MessageType=Type;
	return ::DialogBoxParam(GetAppClass().GetResourceInstance(),
							MAKEINTRESOURCE(IDD_ERROR),hwndOwner,DlgProc,
							reinterpret_cast<LPARAM>(this))==IDOK;
}
