#ifndef TVTEST_UTIL_H
#define TVTEST_UTIL_H


template<typename t> t LimitRange(t value,t min,t max)
{
	return value<min?min:value>max?max:value;
}

LONGLONG StringToInt64(LPCTSTR pszString);
ULONGLONG StringToUInt64(LPCTSTR pszString);
bool Int64ToString(LONGLONG Value,LPTSTR pszString,int MaxLength,int Radix=10);
bool UInt64ToString(ULONGLONG Value,LPTSTR pszString,int MaxLength,int Radix=10);

LPSTR DuplicateString(LPCSTR pszString);
LPWSTR DuplicateString(LPCWSTR pszString);
bool ReplaceString(LPSTR *ppszString,LPCSTR pszNewString);
bool ReplaceString(LPWSTR *ppszString,LPCWSTR pszNewString);

bool IsRectIntersect(const RECT *pRect1,const RECT *pRect2);

float LevelToDeciBel(int Level);

COLORREF MixColor(COLORREF Color1,COLORREF Color2,BYTE Ratio);

DWORD DiffTime(DWORD Start,DWORD End);
#define FILETIME_SECOND			((LONGLONG)10000000)
#define FILETIME_MILLISECOND	((LONGLONG)10000)
FILETIME &operator+=(FILETIME &ft,LONGLONG Offset);
LONGLONG operator-(const FILETIME &ft1,const FILETIME &ft2);
int CompareSystemTime(const SYSTEMTIME *pTime1,const SYSTEMTIME *pTime2);
int CalcDayOfWeek(int Year,int Month,int Day);

void ClearMenu(HMENU hmenu);
int CopyToMenuText(LPCTSTR pszSrcText,LPTSTR pszDstText,int MaxLength);

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
	bool IsValid(bool fWildcard=false) const;
};

class CLocalTime {
protected:
	FILETIME m_Time;
public:
	CLocalTime();
	CLocalTime(const FILETIME &Time);
	CLocalTime(const SYSTEMTIME &Time);
	virtual ~CLocalTime();
	bool operator==(const CLocalTime &Time) const;
	bool operator!=(const CLocalTime &Time) const { return !(*this==Time); }
	bool operator<(const CLocalTime &Time) const;
	bool operator>(const CLocalTime &Time) const;
	bool operator<=(const CLocalTime &Time) const;
	bool operator>=(const CLocalTime &Time) const;
	CLocalTime &operator+=(LONGLONG Offset);
	CLocalTime &operator-=(LONGLONG Offset) { return *this+=-Offset; }
	LONGLONG operator-(const CLocalTime &Time) const;
	void SetCurrentTime();
	bool GetTime(FILETIME *pTime) const;
	bool GetTime(SYSTEMTIME *pTime) const;
};


#endif
