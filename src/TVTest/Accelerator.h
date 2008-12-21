#ifndef ACCELERATOR_H
#define ACCELERATOR_H


#include "Options.h"
#include <vector>


class CMainMenu;

class CAccelerator : public COptions {
	HACCEL m_hAccel;
	struct KeyInfo {
		WORD Command;
		BYTE KeyCode;
		BYTE Modifiers;
		bool operator==(const KeyInfo &Key) const {
			return Command==Key.Command && KeyCode==Key.KeyCode && Modifiers==Key.Modifiers;
		}
	};
	std::vector<KeyInfo> m_KeyList;
	CMainMenu *m_pMainMenu;
	bool m_fFunctionKeyChangeChannel;
	bool m_fDigitKeyChangeChannel;
	bool m_fNumPadChangeChannel;
	static const KeyInfo m_DefaultAccelList[];
	static void FormatAccelText(LPTSTR pszText,int Key,int Modifiers);
	void SetMenuAccelText(HMENU hmenu,int Command);
	HACCEL CreateAccel();
	static int CheckAccelKey(HWND hwndList,BYTE Mod,BYTE Key);
	static void SetAccelItem(HWND hwndList,int Index,BYTE Mod,BYTE Key);
	static void SetDlgItemStatus(HWND hDlg);
	static CAccelerator *GetThis(HWND hDlg);
public:
	CAccelerator();
	~CAccelerator();
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
	bool Load(LPCTSTR pszFileName);
	bool Save(LPCTSTR pszFileName) const;
	bool Initialize(CMainMenu *pMainMenu);
	bool TranslateMessage(HWND hwnd,LPMSG pmsg);
	void SetMenuAccel(HMENU hmenu);
	bool IsFunctionKeyChannelChange() const { return m_fFunctionKeyChangeChannel; }
	bool IsDigitKeyChannelChange() const { return m_fDigitKeyChangeChannel; }
	bool IsNumPadChannelChange() const { return m_fNumPadChangeChannel; }
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
