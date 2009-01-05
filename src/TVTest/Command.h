#ifndef COMMAND_H
#define COMMAND_H


#include "PointerArray.h"
#include "DriverManager.h"
#include "Plugin.h"


class CCommandList {
	CPointerVector<TCHAR> m_PluginList;
	CPointerVector<TCHAR> m_DriverList;
public:
	enum {
		MAX_COMMAND_TEXT=MAX_PATH,
		MAX_COMMAND_NAME=MAX_PATH+16
	};
	CCommandList();
	~CCommandList();
	bool Initialize(const CDriverManager *pDriverManager,const CPluginList *pPluginList);
	int NumCommands() const;
	int GetCommandID(int Index) const;
	LPCTSTR GetCommandText(int Index) const;
	int GetCommandName(int Index,LPTSTR pszName,int MaxLength) const;
	int IDToIndex(int ID) const;
	int ParseText(LPCTSTR pszText) const;
};


#endif
