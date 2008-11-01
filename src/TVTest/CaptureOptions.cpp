#include "stdafx.h"
#include "TVTest.h"
#include <commctrl.h>
#include <shlwapi.h>
#include "CaptureOptions.h"
#include "resource.h"




const SIZE CCaptureOptions::m_SizeList[SIZE_LAST+1] = {
	{0,		0},
	{0,		0},
	{0,		0},
	// 16:9
	{1920,	1080},
	{1440,	810},
	{1280,	720},
	{1024,	576},
	{960,	540},
	{800,	450},
	{640,	360},
	// 4:3
	{1440,	1080},
	{1280,	960},
	{1024,	768},
	{800,	600},
	{720,	540},
	{640,	480},
};


CCaptureOptions::CCaptureOptions()
{
	m_szSaveFolder[0]='\0';
	::lstrcpy(m_szFileName,TEXT("Capture"));
	m_SaveFormat=0;
	m_JPEGQuality=90;
	m_PNGCompressionLevel=6;
	m_fCaptureSaveToFile=false;
	m_fSetComment=false;
	m_CaptureSize=SIZE_ORIGINAL;
}


CCaptureOptions::~CCaptureOptions()
{
}


bool CCaptureOptions::Read(CSettings *pSettings)
{
	pSettings->Read(TEXT("CaptureFolder"),m_szSaveFolder,lengthof(m_szSaveFolder));
	pSettings->Read(TEXT("CaptureFileName"),m_szFileName,lengthof(m_szFileName));
	TCHAR szFormat[32];
	if (pSettings->Read(TEXT("CaptureSaveFormat"),szFormat,lengthof(szFormat))) {
		int Format=m_ImageCodec.FormatNameToIndex(szFormat);
		if (Format>=0)
			m_SaveFormat=Format;
	}
	pSettings->Read(TEXT("CaptureIconSaveFile"),&m_fCaptureSaveToFile);
	pSettings->Read(TEXT("CaptureSetComment"),&m_fSetComment);
	pSettings->Read(TEXT("JpegQuality"),&m_JPEGQuality);
	pSettings->Read(TEXT("PngCompressionLevel"),&m_PNGCompressionLevel);
	int Size;
	if (pSettings->Read(TEXT("CaptureSizeType"),&Size)
			&& Size>=0 && Size<=SIZE_CUSTOM_FIRST)
		m_CaptureSize=Size;
	if (m_CaptureSize==SIZE_CUSTOM_FIRST) {
		int Width,Height;

		m_CaptureSize=SIZE_ORIGINAL;
		if (pSettings->Read(TEXT("CaptureWidth"),&Width)
				&& pSettings->Read(TEXT("CaptureHeight"),&Height)) {
			for (int i=SIZE_CUSTOM_FIRST;i<=SIZE_CUSTOM_LAST;i++) {
				if (m_SizeList[i].cx==Width && m_SizeList[i].cy==Height) {
					m_CaptureSize=i;
					break;
				}
			}
		}
	}
	return true;
}


bool CCaptureOptions::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("CaptureFolder"),m_szSaveFolder);
	pSettings->Write(TEXT("CaptureFileName"),m_szFileName);
	pSettings->Write(TEXT("CaptureSaveFormat"),
									m_ImageCodec.EnumSaveFormat(m_SaveFormat));
	pSettings->Write(TEXT("CaptureIconSaveFile"),m_fCaptureSaveToFile);
	pSettings->Write(TEXT("CaptureSetComment"),m_fSetComment);
	pSettings->Write(TEXT("JpegQuality"),m_JPEGQuality);
	pSettings->Write(TEXT("PngCompressionLevel"),m_PNGCompressionLevel);
	pSettings->Write(TEXT("CaptureSizeType"),min(m_CaptureSize,SIZE_CUSTOM_FIRST));
	if (m_CaptureSize>=SIZE_CUSTOM_FIRST) {
		pSettings->Write(TEXT("CaptureWidth"),m_SizeList[m_CaptureSize].cx);
		pSettings->Write(TEXT("CaptureHeight"),m_SizeList[m_CaptureSize].cy);
	}
	return true;
}


bool CCaptureOptions::SetCaptureSize(int Size)
{
	if (Size<0 || Size>SIZE_LAST)
		return false;
	m_CaptureSize=Size;
	return true;
}


bool CCaptureOptions::GetCustomSize(int Size,int *pWidth,int *pHeight) const
{
	if (Size<SIZE_CUSTOM_FIRST || Size>SIZE_CUSTOM_LAST)
		return false;
	if (pWidth)
		*pWidth=m_SizeList[Size].cx;
	if (pHeight)
		*pHeight=m_SizeList[Size].cy;
	return true;
}


bool CCaptureOptions::GenerateFileName(LPTSTR pszFileName,int MaxLength,const SYSTEMTIME *pst) const
{
	SYSTEMTIME st;
	int ExtOffset;

	if (m_szSaveFolder[0]!='\0') {
		if (::lstrlen(m_szSaveFolder)+1+::lstrlen(m_szFileName)>=MaxLength)
			return false;
		::PathCombine(pszFileName,m_szSaveFolder,m_szFileName);
	} else {
		::GetModuleFileName(NULL,pszFileName,MaxLength);
		::lstrcpy(::PathFindFileName(pszFileName),m_szFileName);
	}
	if (pst==NULL) {
		::GetLocalTime(&st);
	} else {
		st=*pst;
	}
	::wsprintf(pszFileName+::lstrlen(pszFileName),
		TEXT("%04d%02d%02d-%02d%02d%02d"),
		st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
	ExtOffset=::lstrlen(pszFileName);
	::wsprintf(pszFileName+ExtOffset,TEXT(".%s"),
				m_ImageCodec.GetExtension(m_SaveFormat));
	if (::PathFileExists(pszFileName)) {
		for (int i=0;;i++) {
			::wsprintf(pszFileName+ExtOffset,TEXT("-%d.%s"),
				i+1,m_ImageCodec.GetExtension(m_SaveFormat));
			if (!::PathFileExists(pszFileName))
				break;
		}
	}
	return true;
}


bool CCaptureOptions::GetOptionText(LPTSTR pszOption,int MaxLength) const
{
	LPCTSTR pszFormatName=m_ImageCodec.EnumSaveFormat(m_SaveFormat);

	if (::lstrcmpi(pszFormatName,TEXT("JPEG"))==0) {
		if (MaxLength<4)
			return false;
		::wsprintf(pszOption,TEXT("%d"),m_JPEGQuality);
	} else if (::lstrcmpi(pszFormatName,TEXT("PNG"))==0) {
		if (MaxLength<2)
			return false;
		::wsprintf(pszOption,TEXT("%d"),m_PNGCompressionLevel);
	} else {
		if (MaxLength<1)
			return false;
		pszOption[0]='\0';
	}
	return true;
}


bool CCaptureOptions::GetCommentText(LPTSTR pszComment,int MaxComment,
									LPCTSTR pszChannelName,LPCTSTR pszEventName)
{
	SYSTEMTIME st;
	TCHAR szDate[64],szTime[64];

	::GetLocalTime(&st);
	::GetDateFormat(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&st,NULL,
													szDate,lengthof(szDate));
	::GetTimeFormat(LOCALE_USER_DEFAULT,TIME_FORCE24HOURFORMAT,&st,NULL,
													szTime,lengthof(szTime));
	::wsprintf(pszComment,TEXT("%s %s"),szDate,szTime);
	if (pszChannelName!=NULL)
		::wsprintf(pszComment+::lstrlen(pszComment),TEXT(" %s"),pszChannelName);
	if (pszEventName!=NULL)
		::wsprintf(pszComment+::lstrlen(pszComment),TEXT("\r\n%s"),pszEventName);
	return true;
}


bool CCaptureOptions::SaveImage(CCaptureImage *pImage)
{
	TCHAR szFileName[MAX_PATH],szOption[16];
	BITMAPINFO *pbmi;
	BYTE *pBits;
	bool fOK;

	GenerateFileName(szFileName,lengthof(szFileName),&pImage->GetCaptureTime());
	GetOptionText(szOption,lengthof(szOption));
	if (!pImage->LockData(&pbmi,&pBits))
		return false;
	fOK=m_ImageCodec.SaveImage(szFileName,m_SaveFormat,szOption,
						pbmi,pBits,m_fSetComment?pImage->GetComment():NULL);
	pImage->UnlockData();
	return fOK;
}


int CCaptureOptions::TranslateCommand(int Command)
{
	if (Command==CM_CAPTURE)
		return m_fCaptureSaveToFile?CM_SAVEIMAGE:CM_COPY;
	return -1;
}


bool CCaptureOptions::OpenSaveFolder() const
{
	TCHAR szFolder[MAX_PATH];

	if (m_szSaveFolder[0]!='\0') {
		::lstrcpy(szFolder,m_szSaveFolder);
	} else {
		::GetModuleFileName(NULL,szFolder,lengthof(szFolder));
		*(::PathFindFileName(szFolder)-1)='\0';
	}
	return (ULONG_PTR)::ShellExecute(NULL,TEXT("open"),szFolder,NULL,NULL,SW_SHOWNORMAL)>32;
}


CCaptureOptions *CCaptureOptions::GetThis(HWND hDlg)
{
	return static_cast<CCaptureOptions*>(::GetProp(hDlg,TEXT("This")));
}


static void SyncTrackBar(HWND hDlg,int nEditID,int nTrackbarID)
{
	int nMin,nMax,nVal;

	nMin=(int)SendDlgItemMessage(hDlg,nTrackbarID,TBM_GETRANGEMIN,0,0);
	nMax=(int)SendDlgItemMessage(hDlg,nTrackbarID,TBM_GETRANGEMAX,0,0);
	nVal=GetDlgItemInt(hDlg,nEditID,NULL,TRUE);
	SendDlgItemMessage(hDlg,nTrackbarID,TBM_SETPOS,TRUE,
										nVal<nMin?nMin:nVal>nMax?nMax:nVal);
}


static void SyncEditControl(HWND hDlg,int nTrackbarID,int nEditID)
{
	int nVal;

	nVal=(int)SendDlgItemMessage(hDlg,nTrackbarID,TBM_GETPOS,0,0);
	SetDlgItemInt(hDlg,nEditID,nVal,TRUE);
}


BOOL CALLBACK CCaptureOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CCaptureOptions *pThis=dynamic_cast<CCaptureOptions*>(OnInitDialog(hDlg,lParam));
			int i;
			LPCTSTR pszFormat;

			SetProp(hDlg,TEXT("This"),pThis);
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_SAVEFOLDER,EM_LIMITTEXT,MAX_PATH-1,0);
			SetDlgItemText(hDlg,IDC_CAPTUREOPTIONS_SAVEFOLDER,pThis->m_szSaveFolder);
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_FILENAME,EM_LIMITTEXT,MAX_PATH-1,0);
			SetDlgItemText(hDlg,IDC_CAPTUREOPTIONS_FILENAME,pThis->m_szFileName);
			for (i=0;(pszFormat=pThis->m_ImageCodec.EnumSaveFormat(i))!=NULL;i++)
				SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_FORMAT,CB_ADDSTRING,
										0,reinterpret_cast<LPARAM>(pszFormat));
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_FORMAT,CB_SETCURSEL,
														pThis->m_SaveFormat,0);
			// JPEG quality
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_TB,
										TBM_SETRANGE,TRUE,MAKELPARAM(0,100));
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_TB,
										TBM_SETPOS,TRUE,pThis->m_JPEGQuality);
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_TB,
														TBM_SETPAGESIZE,0,10);
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_TB,
														TBM_SETTICFREQ,10,0);
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_EDIT,
															EM_LIMITTEXT,3,0);
			SetDlgItemInt(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_EDIT,
													pThis->m_JPEGQuality,TRUE);
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_UD,
											UDM_SETRANGE,0,MAKELPARAM(100,0));
			// PNG compression level
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_TB,
											TBM_SETRANGE,TRUE,MAKELPARAM(0,9));
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_TB,
								TBM_SETPOS,TRUE,pThis->m_PNGCompressionLevel);
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_TB,
														TBM_SETPAGESIZE,0,1);
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_TB,
														TBM_SETTICFREQ,1,0);
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_EDIT,
															EM_LIMITTEXT,0,1);
			SetDlgItemInt(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_EDIT,
										pThis->m_PNGCompressionLevel,FALSE);
			SendDlgItemMessage(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_UD,
											UDM_SETRANGE,0,MAKELPARAM(9,0));
			CheckDlgButton(hDlg,IDC_CAPTUREOPTIONS_ICONSAVEFILE,
						pThis->m_fCaptureSaveToFile?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hDlg,IDC_CAPTUREOPTIONS_SETCOMMENT,
						pThis->m_fSetComment?BST_CHECKED:BST_UNCHECKED);
		}
		return TRUE;

	case WM_HSCROLL:
		if (reinterpret_cast<HWND>(lParam)==
						GetDlgItem(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_TB)) {
			SyncEditControl(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_TB,
										IDC_CAPTUREOPTIONS_JPEGQUALITY_EDIT);
		} else if (reinterpret_cast<HWND>(lParam)==
						GetDlgItem(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_TB)) {
			SyncEditControl(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_TB,
											IDC_CAPTUREOPTIONS_PNGLEVEL_EDIT);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CAPTUREOPTIONS_SAVEFOLDER_BROWSE:
			{
				TCHAR szFolder[MAX_PATH];

				GetDlgItemText(hDlg,IDC_CAPTUREOPTIONS_SAVEFOLDER,szFolder,
																	MAX_PATH);
				if (BrowseFolderDialog(hDlg,szFolder,
												TEXT("‰æ‘œ‚Ì•Û‘¶æƒtƒHƒ‹ƒ_:")))
					SetDlgItemText(hDlg,IDC_CAPTUREOPTIONS_SAVEFOLDER,szFolder);
			}
			return TRUE;

		case IDC_CAPTUREOPTIONS_JPEGQUALITY_EDIT:
			if (HIWORD(wParam)==EN_CHANGE)
				SyncTrackBar(hDlg,IDC_CAPTUREOPTIONS_JPEGQUALITY_EDIT,
											IDC_CAPTUREOPTIONS_JPEGQUALITY_TB);
			return TRUE;

		case IDC_CAPTUREOPTIONS_PNGLEVEL_EDIT:
			if (HIWORD(wParam)==EN_CHANGE)
				SyncTrackBar(hDlg,IDC_CAPTUREOPTIONS_PNGLEVEL_EDIT,
											IDC_CAPTUREOPTIONS_PNGLEVEL_TB);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CCaptureOptions *pThis=GetThis(hDlg);

				GetDlgItemText(hDlg,IDC_CAPTUREOPTIONS_SAVEFOLDER,
					pThis->m_szSaveFolder,lengthof(pThis->m_szSaveFolder));
				GetDlgItemText(hDlg,IDC_CAPTUREOPTIONS_FILENAME,
					pThis->m_szFileName,lengthof(pThis->m_szFileName));
				pThis->m_SaveFormat=SendDlgItemMessage(hDlg,
							IDC_CAPTUREOPTIONS_FORMAT,CB_GETCURSEL,0,0);
				pThis->m_JPEGQuality=SendDlgItemMessage(hDlg,
							IDC_CAPTUREOPTIONS_JPEGQUALITY_TB,TBM_GETPOS,0,0);
				pThis->m_PNGCompressionLevel=SendDlgItemMessage(hDlg,
							IDC_CAPTUREOPTIONS_PNGLEVEL_TB,TBM_GETPOS,0,0);
				pThis->m_fCaptureSaveToFile=IsDlgButtonChecked(hDlg,
								IDC_CAPTUREOPTIONS_ICONSAVEFILE)==BST_CHECKED;
				pThis->m_fSetComment=IsDlgButtonChecked(hDlg,
								IDC_CAPTUREOPTIONS_SETCOMMENT)==BST_CHECKED;
			}
			break;
		}
		break;

	case WM_DESTROY:
		{
			CCaptureOptions *pThis=GetThis(hDlg);

			pThis->OnDestroy();
		}
		return TRUE;
	}
	return FALSE;
}
