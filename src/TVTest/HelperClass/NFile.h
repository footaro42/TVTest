// NFile.h: CNFile クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


class CNFile
{
public:
	enum {
		CNF_READ		= 0x01U,
		CNF_WRITE		= 0x02U,
		CNF_NEW			= 0x04U,
		CNF_SHAREREAD	= 0x08U,
		CNF_SHAREWRITE	= 0x10U
	};

	CNFile();
	virtual ~CNFile();

	virtual const bool Open(LPCTSTR lpszName, const BYTE bFlags);
	virtual const bool Close(void);
	virtual const bool IsOpen() const;

	virtual const DWORD Read(void *pBuff, const DWORD dwLen);
	virtual const DWORD Read(void *pBuff, const DWORD dwLen, const ULONGLONG llPos);

	virtual const bool Write(const void *pBuff, const DWORD dwLen);
	virtual const bool Write(const void *pBuff, const DWORD dwLen, const ULONGLONG llPos);
	virtual const bool Flush(void);

	virtual const ULONGLONG GetSize(void);
	virtual const ULONGLONG GetPos(void);
	virtual const bool SetPos(const ULONGLONG llPos);

	bool GetTime(FILETIME *pCreationTime, FILETIME *pLastAccessTime = NULL, FILETIME *pLastWriteTime = NULL) const;
	LPCTSTR GetFileName(void) const;

	DWORD GetLastError(void) const;
	DWORD GetLastErrorMessage(LPTSTR pszMessage, const DWORD MaxLength) const;

protected:
	HANDLE m_hFile;
	LPTSTR m_pszFileName;
	DWORD m_LastError;
};
