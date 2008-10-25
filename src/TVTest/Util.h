#ifndef TVTEST_UTIL_H
#define TVTEST_UTIL_H


template<typename t> t LimitRange(t value,t min,t max)
{
	return value<min?min:value>max?max:value;
}

LPSTR DuplicateString(LPCSTR pszString);
LPWSTR DuplicateString(LPCWSTR pszString);
bool ReplaceString(LPSTR *ppszString,LPCSTR pszNewString);
bool ReplaceString(LPWSTR *ppszString,LPCWSTR pszNewString);

float LevelToDeciBel(int Level);

COLORREF MixColor(COLORREF Color1,COLORREF Color2,BYTE Ratio);

DWORD DiffTime(DWORD Start,DWORD End);
#define FILETIME_SECOND ((LONGLONG)10000000)
FILETIME &operator+=(FILETIME &ft,LONGLONG Offset);
LONGLONG operator-(const FILETIME &ft1,const FILETIME &ft2);
int CompareSystemTime(const SYSTEMTIME *pTime1,const SYSTEMTIME *pTime2);
int CalcDayOfWeek(int Year,int Month,int Day);

void ClearMenu(HMENU hmenu);

void EnableDlgItem(HWND hDlg,int ID,bool fEnable);
void EnableDlgItems(HWND hDlg,int FirstID,int LastID,bool fEnable);
void InvalidateDlgItem(HWND hDlg,int ID,bool fErase=true);
int GetDlgItemTextLength(HWND hDlg,int ID);
int GetCheckedRadioButton(HWND hDlg,int FirstID,int LastID);

void InitOpenFileName(OPENFILENAME *pofn);

void ForegroundWindow(HWND hwnd);

bool ChooseColorDialog(HWND hwndOwner,COLORREF *pcrResult);
bool ChooseFontDialog(HWND hwndOwner,LOGFONT *plf);
bool BrowseFolderDialog(HWND hwndOwner,LPTSTR pszDirectory,LPCTSTR pszTitle);

class CFilePath {
	TCHAR m_szPath[MAX_PATH];
public:
	CFilePath();
	CFilePath(const CFilePath &Path);
	CFilePath(LPCTSTR pszPath);
	~CFilePath();
	bool SetPath(LPCTSTR pszPath);
	LPCTSTR GetPath() const { return m_szPath; }
	void GetPath(LPTSTR pszPath) const;
	LPCTSTR GetFileName() const;
	bool SetFileName(LPCTSTR pszFileName);
	bool RemoveFileName();
	LPCTSTR GetExtension() const;
	bool SetExtension(LPCTSTR pszExtension);
	bool RemoveExtension();
	bool AppendExtension(LPCTSTR pszExtension);
	bool Make(LPCTSTR pszDirectory,LPCTSTR pszFileName);
	bool Append(LPCTSTR pszMore);
	bool GetDirectory(LPTSTR pszDirectory) const;
	bool SetDirectory(LPCTSTR pszDirectory);
	bool RemoveDirectory();
	bool HasDirectory() const;
	bool IsExists() const;
};


#endif
