#ifndef REMOTE_CONTROLLER_H
#define REMOTE_CONTROLLER_H


#include "Options.h"
#include "Command.h"


class CHDUSController : public COptions {
	enum { NUM_BUTTONS=36 };
	bool m_fUseHDUSController;
	bool m_fActiveOnly;
	WORD m_AssignList[NUM_BUTTONS];
	class CRemoteController *m_pRemoteController;
	HACCEL m_hAccel;
	const CCommandList *m_pCommandList;
	HBITMAP m_hbmController;
	HBITMAP m_hbmSelButtons;
	RECT m_ImageRect;
	HWND m_hwndToolTip;
	void SetButtonCommand(HWND hwndList,int Index,int Command);
	void SetDlgItemStatus();
	static CHDUSController *GetThis(HWND hDlg);
	// COptions
	bool Load(LPCTSTR pszFileName);

public:
	CHDUSController();
	~CHDUSController();
	// COptions
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
	bool Save(LPCTSTR pszFileName) const;
	// CHDUSController
	bool Initialize(HWND hwnd,LPCTSTR pszSettingFileName,const CCommandList *pCommandList);
	void Finalize();
	bool TranslateMessage(HWND hwnd,LPMSG pmsg);
	bool HandleMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	bool OnActivateApp(HWND hwnd,WPARAM wParam,LPARAM lParam);
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
