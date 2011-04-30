#ifndef TVTEST_COMMAND_H
#define TVTEST_COMMAND_H


#include <vector>


class CDriverManager;
class CPluginManager;
class CZoomOptions;

class CCommandList
{
	struct PluginCommandInfo {
		CDynamicString Text;
		CDynamicString Name;
		PluginCommandInfo(LPCTSTR pszText,LPCTSTR pszName) : Text(pszText), Name(pszName) {}
	};

	std::vector<CDynamicString> m_DriverList;
	std::vector<CDynamicString> m_PluginList;
	std::vector<PluginCommandInfo> m_PluginCommandList;
	const CZoomOptions *m_pZoomOptions;

public:
	enum {
		MAX_COMMAND_TEXT=MAX_PATH,
		MAX_COMMAND_NAME=MAX_PATH+16
	};
	CCommandList();
	~CCommandList();
	bool Initialize(const CDriverManager *pDriverManager,
					const CPluginManager *pPluginManager,
					const CZoomOptions *pZoomOptions);
	int NumCommands() const;
	int GetCommandID(int Index) const;
	LPCTSTR GetCommandText(int Index) const;
	LPCTSTR GetCommandTextByID(int ID) const;
	int GetCommandName(int Index,LPTSTR pszName,int MaxLength) const;
	int GetCommandNameByID(int ID,LPTSTR pszName,int MaxLength) const;
	int IDToIndex(int ID) const;
	int ParseText(LPCTSTR pszText) const;
};


#endif
