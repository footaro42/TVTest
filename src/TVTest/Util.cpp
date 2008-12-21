#include "stdafx.h"
#include <math.h>
#include <shlwapi.h>
#include <shlobj.h>
#include "Util.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#pragma comment(lib,"shlwapi.lib")




LONGLONG StringToInt64(LPCTSTR pszString)
{
	return _ttoi64(pszString);
}


ULONGLONG StringToUInt64(LPCTSTR pszString)
{
	ULONGLONG Value=0;
	LPCTSTR p;

	p=pszString;
	while (*p>='0' && *p<='9') {
		Value=Value*10+(*p-'0');
		p++;
	}
	return Value;
}


bool Int64ToString(LONGLONG Value,LPTSTR pszString,int MaxLength,int Radix)
{
	return _i64tot_s(Value,pszString,MaxLength,Radix)==0;
}


bool UInt64ToString(ULONGLONG Value,LPTSTR pszString,int MaxLength,int Radix)
{
	return _ui64tot_s(Value,pszString,MaxLength,Radix)==0;
}


LPSTR DuplicateString(LPCSTR pszString)
{
	LPSTR p;

	if (pszString==NULL)
		return NULL;
	p=new char[lstrlenA(pszString)+1];
	lstrcpyA(p,pszString);
	return p;
}


LPWSTR DuplicateString(LPCWSTR pszString)
{
	LPWSTR p;

	if (pszString==NULL)
		return NULL;
	p=new WCHAR[lstrlenW(pszString)+1];
	lstrcpyW(p,pszString);
	return p;
}


bool ReplaceString(LPSTR *ppszString,LPCSTR pszNewString)
{
	ASSERT(ppszString);
	delete [] *ppszString;
	*ppszString=DuplicateString(pszNewString);
	return true;
}


bool ReplaceString(LPWSTR *ppszString,LPCWSTR pszNewString)
{
	ASSERT(ppszString);
	delete [] *ppszString;
	*ppszString=DuplicateString(pszNewString);
	return true;
}


bool IsRectIntersect(const RECT *pRect1,const RECT *pRect2)
{
	return pRect1->left<pRect2->right && pRect1->right>pRect2->left
		&& pRect1->top<pRect2->bottom && pRect1->bottom>pRect2->top;
}


float LevelToDeciBel(int Level)
{
	float Volume;

	if (Level==0)
		Volume=-100.0f;
	else
		Volume=(float)(20.0*log10((double)Level/100.0));
	return Volume;
}


COLORREF MixColor(COLORREF Color1,COLORREF Color2,BYTE Ratio)
{
	return RGB((GetRValue(Color1)*Ratio+GetRValue(Color2)*(255-Ratio))/255,
			   (GetGValue(Color1)*Ratio+GetGValue(Color2)*(255-Ratio))/255,
			   (GetBValue(Color1)*Ratio+GetBValue(Color2)*(255-Ratio))/255);
}


DWORD DiffTime(DWORD Start,DWORD End)
{
	if (Start<End)
		return End-Start;
	return (0xFFFFFFFFUL-Start+1)+End;
}


FILETIME &operator+=(FILETIME &ft,LONGLONG Offset)
{
	ULARGE_INTEGER Result;

	Result.LowPart=ft.dwLowDateTime;
	Result.HighPart=ft.dwHighDateTime;
	Result.QuadPart+=Offset;
	ft.dwLowDateTime=Result.LowPart;
	ft.dwHighDateTime=Result.HighPart;
	return ft;
}


LONGLONG operator-(const FILETIME &ft1,const FILETIME &ft2)
{
	LARGE_INTEGER Time1,Time2;

	Time1.LowPart=ft1.dwLowDateTime;
	Time1.HighPart=ft1.dwHighDateTime;
	Time2.LowPart=ft2.dwLowDateTime;
	Time2.HighPart=ft2.dwHighDateTime;
	return Time1.QuadPart-Time2.QuadPart;
}


int CompareSystemTime(const SYSTEMTIME *pTime1,const SYSTEMTIME *pTime2)
{
	/*
	FILETIME ft1,ft2;

	SystemTimeToFileTime(pTime1,&ft1);
	SystemTimeToFileTime(pTime2,&ft2);
	return CompareFileTime(&ft1,&ft2);
	*/
	DWORD Date1,Date2;

	Date1=((DWORD)pTime1->wYear<<16) | ((DWORD)pTime1->wMonth<<8) | pTime1->wDay;
	Date2=((DWORD)pTime2->wYear<<16) | ((DWORD)pTime2->wMonth<<8) | pTime2->wDay;
	if (Date1==Date2) {
		Date1=((DWORD)pTime1->wHour<<24) | ((DWORD)pTime1->wMinute<<16) |
			  ((DWORD)pTime1->wSecond<<10) | pTime1->wMilliseconds;
		Date2=((DWORD)pTime2->wHour<<24) | ((DWORD)pTime2->wMinute<<16) |
			  ((DWORD)pTime2->wSecond<<10) | pTime2->wMilliseconds;
	}
	if (Date1<Date2)
		return -1;
	if (Date1>Date2)
		return 1;
	return 0;
}


int CalcDayOfWeek(int Year,int Month,int Day)
{
	return (Year*365+Year/4-Year/100+Year/400+306*(Month+1)/10+Day-428)%7;
}


void ClearMenu(HMENU hmenu)
{
	int Count,i;

	Count=GetMenuItemCount(hmenu);
	for (i=Count-1;i>=0;i--)
		DeleteMenu(hmenu,i,MF_BYPOSITION);
}


int CopyToMenuText(LPCTSTR pszSrcText,LPTSTR pszDstText,int MaxLength)
{
	int i,j;

	i=j=0;
	while (pszSrcText[i]!='\0' && j+1<MaxLength) {
		pszDstText[j++]=pszSrcText[i];
#ifndef UNICODE
		if (::IsDBCSLeadByteEx(CP_ACP,pszSrcText[i])) {
			if (j+1>=MaxLength || pszSrcText[i+1]=='\0')
				break;
			pszDstText[j++]=pszSrcText[++i];
			i++;
			continue;
		}
#endif
		if (pszSrcText[i]=='&') {
			if (j+1>=MaxLength)
				break;
			pszDstText[j++]='&';
		}
		i++;
	}
	pszDstText[j]='\0';
	return j;
}


void InitOpenFileName(OPENFILENAME *pofn)
{
#if _WIN32_WINNT>=0x0500
	OSVERSIONINFO osvi;

	osvi.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	if (GetVersionEx(&osvi) && (osvi.dwMajorVersion>=5
												|| osvi.dwMinorVersion==90)) {
		/* Windows 2000/XP/Vista or Me */
		pofn->lStructSize=sizeof(OPENFILENAME);
		pofn->pvReserved=NULL;
		pofn->dwReserved=0;
		pofn->FlagsEx=0;
	} else
		pofn->lStructSize=OPENFILENAME_SIZE_VERSION_400;
#else
	pofn->lStructSize=sizeof(OPENFILENAME);
#endif
	pofn->hwndOwner=NULL;
	pofn->hInstance=NULL;
	pofn->lpstrCustomFilter=NULL;
	pofn->nMaxCustFilter=0;
	pofn->nFilterIndex=1;
	pofn->lpstrFileTitle=NULL;
	pofn->lpstrInitialDir=NULL;
	pofn->lpstrTitle=NULL;
	pofn->Flags=0;
	pofn->lpstrDefExt=NULL;
}


void ForegroundWindow(HWND hwnd)
{
	int TargetID,ForegroundID;

	ForegroundID=GetWindowThreadProcessId(GetForegroundWindow(),NULL);
	TargetID=GetWindowThreadProcessId(hwnd,NULL);
	AttachThreadInput(TargetID,ForegroundID,TRUE);
	SetForegroundWindow(hwnd);
	AttachThreadInput(TargetID,ForegroundID,FALSE);
}


bool ChooseColorDialog(HWND hwndOwner,COLORREF *pcrResult)
{
	CHOOSECOLOR cc;
	static COLORREF crCustomColors[16] = {
		RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),
		RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),
		RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),
		RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),
		RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),RGB(0xFF,0xFF,0xFF),
		RGB(0xFF,0xFF,0xFF)
	};

	cc.lStructSize=sizeof(CHOOSECOLOR);
	cc.hwndOwner=hwndOwner;
	cc.hInstance=NULL;
	cc.rgbResult=*pcrResult;
	cc.lpCustColors=crCustomColors;
	cc.Flags=CC_RGBINIT | CC_FULLOPEN;
	if (!ChooseColor(&cc))
		return false;
	*pcrResult=cc.rgbResult;
	return true;
}


bool ChooseFontDialog(HWND hwndOwner,LOGFONT *plf)
{
	CHOOSEFONT cf;

	cf.lStructSize=sizeof(CHOOSEFONT);
	cf.hwndOwner=hwndOwner;
	cf.lpLogFont=plf;
	cf.Flags=CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
	return ChooseFont(&cf)!=FALSE;
}


int CALLBACK BrowseFolderCallback(HWND hwnd,UINT uMsg,LPARAM lpData,
																LPARAM lParam)
{
	switch (uMsg) {
	case BFFM_INITIALIZED:
		if (((LPTSTR)lParam)[0]!='\0') {
			TCHAR szDirectory[MAX_PATH];

			lstrcpy(szDirectory,(LPTSTR)lParam);
			PathRemoveBackslash(szDirectory);
			SendMessage(hwnd,BFFM_SETSELECTION,TRUE,(LPARAM)szDirectory);
		}
		break;
	}
	return 0;
}


bool BrowseFolderDialog(HWND hwndOwner,LPTSTR pszDirectory,LPCTSTR pszTitle)
{
	BROWSEINFO bi;
	ITEMIDLIST *pidl;
	BOOL fRet;

	bi.hwndOwner=hwndOwner;
	bi.pidlRoot=NULL;
	bi.pszDisplayName=pszDirectory;
	bi.lpszTitle=pszTitle;
	bi.ulFlags=BIF_RETURNONLYFSDIRS;
	bi.lpfn=BrowseFolderCallback;
	bi.lParam=(LPARAM)pszDirectory;
	pidl=SHBrowseForFolder(&bi);
	if (pidl==NULL)
		return false;
	fRet=SHGetPathFromIDList(pidl,pszDirectory);
	CoTaskMemFree(pidl);
	return fRet==TRUE;
}




CFilePath::CFilePath()
{
	m_szPath[0]='\0';
}


CFilePath::CFilePath(const CFilePath &Path)
{
	::lstrcpy(m_szPath,Path.m_szPath);
}


CFilePath::CFilePath(LPCTSTR pszPath)
{
	if (pszPath)
		::lstrcpy(m_szPath,pszPath);
	else
		m_szPath[0]='\0';
}


CFilePath::~CFilePath()
{
}


bool CFilePath::SetPath(LPCTSTR pszPath)
{
	if (::lstrlen(pszPath)>=MAX_PATH)
		return false;
	::lstrcpy(m_szPath,pszPath);
	return true;
}


void CFilePath::GetPath(LPTSTR pszPath) const
{
	::lstrcpy(pszPath,m_szPath);
}


LPCTSTR CFilePath::GetFileName() const
{
	if (m_szPath[0]=='\0')
		return m_szPath;
	return ::PathFindFileName(m_szPath);
}


bool CFilePath::SetFileName(LPCTSTR pszFileName)
{
	::lstrcpy(::PathFindFileName(m_szPath),pszFileName);
	return true;
}


bool CFilePath::RemoveFileName()
{
	LPTSTR pszFileName=::PathFindFileName(m_szPath);

	if (pszFileName>m_szPath) {
		*pszFileName='\0';
		::PathRemoveBackslash(m_szPath);
	}
	return true;
}


LPCTSTR CFilePath::GetExtension() const
{
	return ::PathFindExtension(m_szPath);
}


bool CFilePath::SetExtension(LPCTSTR pszExtension)
{
	return ::PathRenameExtension(m_szPath,pszExtension)!=FALSE;
}


bool CFilePath::RemoveExtension()
{
	if (m_szPath[0]!='\0')
		::PathRemoveExtension(m_szPath);
	return true;
}


bool CFilePath::AppendExtension(LPCTSTR pszExtension)
{
	if (::lstrlen(m_szPath)+::lstrlen(pszExtension)>=MAX_PATH)
		return false;
	::lstrcat(m_szPath,pszExtension);
	return true;
}


bool CFilePath::Make(LPCTSTR pszDirectory,LPCTSTR pszFileName)
{
	::PathCombine(m_szPath,pszDirectory,pszFileName);
	return true;
}


bool CFilePath::Append(LPCTSTR pszMore)
{
	return ::PathAppend(m_szPath,pszMore)!=FALSE;
}


bool CFilePath::GetDirectory(LPTSTR pszDirectory) const
{
	LPCTSTR pszFileName=GetFileName();

	if (pszFileName==m_szPath) {
		pszDirectory[0]='\0';
		return false;
	}
	::lstrcpyn(pszDirectory,m_szPath,pszFileName-m_szPath);
	::PathRemoveBackslash(pszDirectory);
	return true;
}


bool CFilePath::SetDirectory(LPCTSTR pszDirectory)
{
	if (pszDirectory[0]=='\0') {
		RemoveDirectory();
	} else {
		TCHAR szPath[MAX_PATH];

		::PathCombine(szPath,pszDirectory,GetFileName());
		::lstrcpy(m_szPath,szPath);
	}
	return true;
}


bool CFilePath::RemoveDirectory()
{
	LPTSTR pszFileName=::PathFindFileName(m_szPath);

	if (pszFileName>m_szPath)
		::MoveMemory(m_szPath,pszFileName,(::lstrlen(pszFileName)+1)*sizeof(TCHAR));
	return true;
}


bool CFilePath::HasDirectory() const
{
	if (m_szPath[0]=='\0')
		return false;
	return !::PathIsFileSpec(m_szPath);
}


bool CFilePath::IsExists() const
{
	if (m_szPath[0]=='\0')
		return false;
	return ::PathFileExists(m_szPath)!=FALSE;
}


bool CFilePath::IsValid(bool fWildcard) const
{
	UINT Mask;
	LPCTSTR p;

	if (m_szPath[0]=='\0')
		return false;
	Mask=GCT_INVALID;
	if (!fWildcard)
		Mask|=GCT_WILD;
	p=m_szPath;
	while (*p!='\0') {
#ifndef UNICODE
		if (::IsDBCSLeadByteEx(CP_ACP,*p)) {
			p+=2;
			continue;
		}
#endif
		if ((::PathGetCharType(*p)&Mask)!=0)
			return false;
		p++;
	}
	return true;
}




CLocalTime::CLocalTime()
{
	SetCurrentTime();
}


CLocalTime::CLocalTime(const FILETIME &Time)
{
	m_Time=Time;
}


CLocalTime::CLocalTime(const SYSTEMTIME &Time)
{
	::SystemTimeToFileTime(&Time,&m_Time);
}


CLocalTime::~CLocalTime()
{
}


bool CLocalTime::operator==(const CLocalTime &Time) const
{
	return m_Time.dwLowDateTime==Time.m_Time.dwLowDateTime
		&& m_Time.dwHighDateTime==Time.m_Time.dwHighDateTime;
}


bool CLocalTime::operator<(const CLocalTime &Time) const
{
	return m_Time.dwHighDateTime<Time.m_Time.dwHighDateTime
		|| (m_Time.dwHighDateTime==Time.m_Time.dwHighDateTime
			&& m_Time.dwLowDateTime<Time.m_Time.dwLowDateTime);
}


bool CLocalTime::operator>(const CLocalTime &Time) const
{
	return m_Time.dwHighDateTime>Time.m_Time.dwHighDateTime
		|| (m_Time.dwHighDateTime==Time.m_Time.dwHighDateTime
			&& m_Time.dwLowDateTime>Time.m_Time.dwLowDateTime);
}


bool CLocalTime::operator<=(const CLocalTime &Time) const
{
	return m_Time.dwHighDateTime<Time.m_Time.dwHighDateTime
		|| (m_Time.dwHighDateTime==Time.m_Time.dwHighDateTime
			&& m_Time.dwLowDateTime<=Time.m_Time.dwLowDateTime);
}


bool CLocalTime::operator>=(const CLocalTime &Time) const
{
	return m_Time.dwHighDateTime>Time.m_Time.dwHighDateTime
		|| (m_Time.dwHighDateTime==Time.m_Time.dwHighDateTime
			&& m_Time.dwLowDateTime>=Time.m_Time.dwLowDateTime);
}


CLocalTime &CLocalTime::operator+=(LONGLONG Offset)
{
	m_Time+=Offset*FILETIME_MILLISECOND;
	return *this;
}


LONGLONG CLocalTime::operator-(const CLocalTime &Time) const
{
	return (((LONGLONG)m_Time.dwHighDateTime<<32)|(LONGLONG)m_Time.dwLowDateTime)-
		(((LONGLONG)Time.m_Time.dwHighDateTime<<32)|(LONGLONG)Time.m_Time.dwLowDateTime);
}


void CLocalTime::SetCurrentTime()
{
	SYSTEMTIME st;

	::GetLocalTime(&st);
	::SystemTimeToFileTime(&st,&m_Time);
}


bool CLocalTime::GetTime(FILETIME *pTime) const
{
	*pTime=m_Time;
	return true;
}


bool CLocalTime::GetTime(SYSTEMTIME *pTime) const
{
	return ::FileTimeToSystemTime(&m_Time,pTime)!=0;
}
