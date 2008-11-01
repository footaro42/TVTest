#ifndef DRIVER_MANAGER_H
#define DRIVER_MANAGER_H


#include "PointerArray.h"


class CDriverInfo {
	LPTSTR m_pszFileName;
public:
	CDriverInfo(LPCTSTR pszFileName);
	~CDriverInfo();
	LPCTSTR GetFileName() const { return m_pszFileName; }
};

class CDriverManager {
	CPointerVector<CDriverInfo> m_DriverList;
	LPTSTR m_pszBaseDirectory;
	static int CompareDriverFileName(const CDriverInfo *pDriver1,const CDriverInfo *pDriver2);
public:
	CDriverManager();
	~CDriverManager();
	void Clear();
	int NumDrivers() const { return m_DriverList.Length(); }
	bool Find(LPCTSTR pszDirectory);
	CDriverInfo *GetDriverInfo(int Index);
	const CDriverInfo *GetDriverInfo(int Index) const;
	LPCTSTR GetBaseDirectory() const { return m_pszBaseDirectory; }
};


#endif
