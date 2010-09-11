#ifndef TVTEST_CONTROLLER_H
#define TVTEST_CONTROLLER_H


#include <vector>
#include "Options.h"
#include "Tooltip.h"


class ABSTRACT_CLASS(CController)
{
public:
	class ABSTRACT_CLASS(CEventHandler) {
	public:
		virtual ~CEventHandler() {}
		virtual bool OnButtonDown(CController *pController,int Index) = 0;
	};

	struct ButtonInfo {
		LPCTSTR pszName;
		int DefaultCommand;
		struct {
			WORD Left,Top,Width,Height;
		} ImageButtonRect;
		struct {
			WORD Left,Top;
		} ImageSelButtonPos;
	};

	enum ImageType {
		IMAGE_CONTROLLER,
		IMAGE_SELBUTTONS
	};

	CController();
	virtual ~CController();
	virtual LPCTSTR GetName() const = 0;
	virtual LPCTSTR GetText() const = 0;
	virtual int NumButtons() const = 0;
	virtual bool GetButtonInfo(int Index,ButtonInfo *pInfo) const = 0;
	virtual bool Enable(bool fEnable) = 0;
	virtual bool IsEnabled() const = 0;
	virtual bool IsActiveOnly() const { return false; }
	virtual bool SetTargetWindow(HWND hwnd) = 0;
	virtual bool TranslateMessage(HWND hwnd,MSG *pMessage);
	virtual HBITMAP GetImage(ImageType Type) const;
	virtual bool GetIniFileName(LPTSTR pszFileName,int MaxLength) const;
	virtual LPCTSTR GetIniFileSection() const;
	void SetEventHandler(CEventHandler *pEventHandler);

protected:
	CEventHandler *m_pEventHandler;

	bool OnButtonDown(int Index);
};

class CControllerManager : public COptions, public CController::CEventHandler
{
public:
	struct ControllerSettings {
		std::vector<WORD> AssignList;
		bool fActiveOnly;
	};

	CControllerManager();
	~CControllerManager();
// COptions
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
// CControllerManager
	bool AddController(CController *pController);
	bool DeleteController(LPCTSTR pszName);
	void DeleteAllControllers();
	bool IsControllerEnabled(LPCTSTR pszName) const;
	bool LoadControllerSettings(LPCTSTR pszName);
	bool SaveControllerSettings(LPCTSTR pszName) const;
	bool TranslateMessage(HWND hwnd,MSG *pMessage);
	bool IsFocus() const { return m_fFocus; }
	bool IsActive() const { return m_fActive; }
	bool OnActiveChange(HWND hwnd,bool fActive);
	bool OnFocusChange(HWND hwnd,bool fFocus);
	bool OnButtonDown(LPCTSTR pszName,int Button) const;
	const ControllerSettings *GetControllerSettings(LPCTSTR pszName) const;
	static INT_PTR CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

private:
	struct ControllerInfo {
		CController *pController;
		bool fSettingsLoaded;
		ControllerSettings Settings;
		ControllerInfo(CController *p) : pController(p), fSettingsLoaded(false) {}
	};

	std::vector<ControllerInfo> m_ControllerList;
	std::vector<ControllerSettings> m_CurSettingsList;
	bool m_fFocus;
	bool m_fActive;
	CDynamicString m_CurController;
	HBITMAP m_hbmController;
	HBITMAP m_hbmSelButtons;
	RECT m_ImageRect;
	CTooltip m_Tooltip;

// CController::CEventHandler
	bool OnButtonDown(CController *pController,int Index);
// CControllerManager
	int FindController(LPCTSTR pszName) const;
	void InitDlgItems();
	void SetButtonCommand(HWND hwndList,int Index,int Command);
	void SetDlgItemStatus();
	CController *GetCurController() const;
	static CControllerManager *GetThis(HWND hDlg);
};


#endif
