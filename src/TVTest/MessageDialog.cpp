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
	: m_pszText(NULL)
	, m_pszTitle(NULL)
	, m_pszSystemMessage(NULL)
	, m_pszCaption(NULL)
{
}


CMessageDialog::~CMessageDialog()
{
	delete [] m_pszText;
	delete [] m_pszTitle;
	delete [] m_pszSystemMessage;
	delete [] m_pszCaption;
}


void CMessageDialog::LogFontToCharFormat(const LOGFONT *plf,CHARFORMAT *pcf)
{
	HDC hdc=::GetDC(m_hDlg);
	CRichEditUtil::LogFontToCharFormat(hdc,plf,pcf);
	::ReleaseDC(m_hDlg,hdc);
}


CMessageDialog *CMessageDialog::GetThis(HWND hDlg)
{
	return static_cast<CMessageDialog*>(::GetProp(hDlg,TEXT("This")));
}


INT_PTR CALLBACK CMessageDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CMessageDialog *pThis=reinterpret_cast<CMessageDialog*>(lParam);

			::SetProp(hDlg,TEXT("This"),pThis);
			pThis->m_hDlg=hDlg;

			if (pThis->m_pszCaption!=NULL)
				::SetWindowText(hDlg,pThis->m_pszCaption);

			::SendDlgItemMessage(hDlg,IDC_ERROR_ICON,STM_SETICON,
				reinterpret_cast<WPARAM>(::LoadIcon(NULL,
					pThis->m_MessageType==TYPE_INFO?IDI_INFORMATION:
					pThis->m_MessageType==TYPE_WARNING?IDI_WARNING:IDI_ERROR)),0);
			::SendDlgItemMessage(hDlg,IDC_ERROR_MESSAGE,EM_SETBKGNDCOLOR,0,::GetSysColor(COLOR_WINDOW));

			HWND hwndEdit=::GetDlgItem(hDlg,IDC_ERROR_MESSAGE);
			CHARFORMAT cf,cfBold;

			NONCLIENTMETRICS ncm;
#if WINVER<0x0600
			ncm.cbSize=sizeof(ncm);
#else
			ncm.cbSize=offsetof(NONCLIENTMETRICS,iPaddedBorderWidth);
#endif
			::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,ncm.cbSize,&ncm,0);
			pThis->LogFontToCharFormat(&ncm.lfMessageFont,&cf);

			cfBold=cf;
			cfBold.dwMask|=CFM_BOLD;
			cfBold.dwEffects|=CFE_BOLD;
			if (pThis->m_pszTitle!=NULL) {
				CRichEditUtil::AppendText(hwndEdit,pThis->m_pszTitle,&cfBold);
				CRichEditUtil::AppendText(hwndEdit,TEXT("\n"),&cf);
			}
			if (pThis->m_pszText!=NULL) {
				CRichEditUtil::AppendText(hwndEdit,pThis->m_pszText,&cf);
			}
			if (pThis->m_pszSystemMessage!=NULL) {
				CRichEditUtil::AppendText(hwndEdit,TEXT("\n\nWindowsのエラーメッセージ :\n"),&cfBold);
				CRichEditUtil::AppendText(hwndEdit,pThis->m_pszSystemMessage,&cf);
			}
			int MaxWidth=CRichEditUtil::GetMaxLineWidth(hwndEdit)+8;
			RECT rcEdit,rcIcon,rcDlg,rcClient,rcOK;
			::GetWindowRect(hwndEdit,&rcEdit);
			::OffsetRect(&rcEdit,-rcEdit.left,-rcEdit.top);
			::GetWindowRect(::GetDlgItem(hDlg,IDC_ERROR_ICON),&rcIcon);
			rcIcon.bottom-=rcIcon.top;
			if (rcEdit.bottom<rcIcon.bottom)
				rcEdit.bottom=rcIcon.bottom;
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
					CRichEditUtil::CopyAllText(hwndEdit);
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
			pThis->m_hDlg=NULL;
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
	if (!m_RichEditUtil.LoadRichEditLib()) {
		TCHAR szMessage[2048];

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

	ReplaceString(&m_pszText,pszText);
	ReplaceString(&m_pszTitle,pszTitle);
	ReplaceString(&m_pszSystemMessage,pszSystemMessage);
	ReplaceString(&m_pszCaption,pszCaption);
	m_MessageType=Type;
	return ::DialogBoxParam(GetAppClass().GetResourceInstance(),
							MAKEINTRESOURCE(IDD_ERROR),hwndOwner,DlgProc,
							reinterpret_cast<LPARAM>(this))==IDOK;
}
