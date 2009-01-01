#ifndef ACCELERATOR_H
#define ACCELERATOR_H


#include <vector>
#include "Options.h"
#include "Plugin.h"
#include "PointerArray.h"


class CMainMenu;

class CAccelerator : public COptions {
	HACCEL m_hAccel;
	struct KeyInfo {
		WORD Command;
		WORD KeyCode;
		BYTE Modifiers;
		bool fGlobal;
		bool operator==(const KeyInfo &Info) const {
			return Command==Info.Command && KeyCode==Info.KeyCode
				&& Modifiers==Info.Modifiers && fGlobal==Info.fGlobal;
		}
	};
	std::vector<KeyInfo> m_KeyList;
	CPointerVector<TCHAR> m_PluginList;
	HWND m_hwndHotKey;
	CMainMenu *m_pMainMenu;
	bool m_fRegisterHotKey;
	bool m_fFunctionKeyChangeChannel;
	bool m_fDigitKeyChangeChannel;
	bool m_fNumPadChangeChannel;
	static const KeyInfo m_DefaultAccelList[];
	static void FormatAccelText(LPTSTR pszText,int Key,int Modifiers);
	void SetMenuAccelText(HMENU hmenu,int Command);
	HACCEL CreateAccel();
	bool RegisterHotKey();
	bool UnregisterHotKey();
	static int CheckAccelKey(HWND hwndList,BYTE Mod,WORD Key);
	static void SetAccelItem(HWND hwndList,int Index,BYTE Mod,WORD Key,bool fGlobal);
	static void SetDlgItemStatus(HWND hDlg);
	static CAccelerator *GetThis(HWND hDlg);
public:
	CAccelerator();
	~CAccelerator();
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
	bool Load(LPCTSTR pszFileName);
	bool Save(LPCTSTR pszFileName) const;
	bool Initialize(HWND hwndHotKey,CMainMenu *pMainMenu,const CPluginList *pPluginList);
	void Finalize();
	bool TranslateMessage(HWND hwnd,LPMSG pmsg);
	int TranslateHotKey(WPARAM wParam,LPARAM lParam);
	void SetMenuAccel(HMENU hmenu);
	bool IsFunctionKeyChannelChange() const { return m_fFunctionKeyChangeChannel; }
	bool IsDigitKeyChannelChange() const { return m_fDigitKeyChangeChannel; }
	bool IsNumPadChannelChange() const { return m_fNumPadChangeChannel; }
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
