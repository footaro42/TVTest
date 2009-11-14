#ifndef DISPLAY_MENU_H
#define DISPLAY_MENU_H


#include <vector>
#include "BasicWindow.h"
#include "DriverManager.h"
#include "EpgProgramList.h"
#include "DrawUtil.h"
#include "Theme.h"
#include "PointerArray.h"


class CChannelDisplayMenu : public CBasicWindow
{
public:
	class CEventHandler {
	public:
		enum {
			SPACE_NOTSPECIFIED	=-2,
			SPACE_ALL			=-1
		};
		CEventHandler();
		virtual ~CEventHandler();
		virtual void OnTunerSelect(LPCTSTR pszDriverFileName,int TuningSpace)=0;
		virtual void OnChannelSelect(LPCTSTR pszDriverFileName,const CChannelInfo *pChannelInfo)=0;
		virtual void OnClose()=0;
		virtual void OnRButtonDown(int x,int y) {}
	};

	CChannelDisplayMenu(CEpgProgramList *pEpgProgramList);
	~CChannelDisplayMenu();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	void Clear();
	bool SetDriverManager(CDriverManager *pDriverManager);
	void SetEventHandler(CEventHandler *pEventHandler);
	bool SetSelect(LPCTSTR pszDriverFileName,const CChannelInfo *pChannelInfo);
	bool SetFont(const LOGFONT *pFont,bool fAutoSize);
	bool IsMessageNeed(const MSG *pmsg) const;

	static bool Initialize(HINSTANCE hinst);

private:
	class CTuner {
		CPointerVector<CTuningSpaceInfo> m_TuningSpaceList;
		LPTSTR m_pszDriverFileName;
		LPTSTR m_pszTunerName;
		LPTSTR m_pszDisplayName;
		HICON m_hIcon;
	public:
		CTuner(const CDriverInfo *pDriverInfo);
		~CTuner();
		void Clear();
		LPCTSTR GetDriverFileName() const { return m_pszDriverFileName; }
		LPCTSTR GetTunerName() const { return m_pszTunerName; }
		LPCTSTR GetDisplayName() const;
		void SetDisplayName(LPCTSTR pszName);
		int NumSpaces() const;
		const CTuningSpaceInfo *GetTuningSpaceInfo(int Index) const;
		void SetIcon(HICON hico);
		HICON GetIcon() const { return m_hIcon; }
	};

	Theme::GradientInfo m_TunerAreaBackGradient;
	Theme::GradientInfo m_ChannelAreaBackGradient;
	Theme::GradientInfo m_TunerBackGradient;
	COLORREF m_TunerTextColor;
	Theme::GradientInfo m_CurTunerBackGradient;
	COLORREF m_CurTunerTextColor;
	Theme::GradientInfo m_ChannelBackGradient[2];
	COLORREF m_ChannelTextColor;
	Theme::GradientInfo m_CurChannelBackGradient;
	COLORREF m_CurChannelTextColor;
	DrawUtil::CFont m_Font;
	bool m_fAutoFontSize;
	int m_FontHeight;
	int m_TunerItemWidth;
	int m_TunerItemHeight;
	int m_TunerAreaWidth;
	int m_TunerItemLeft;
	int m_TunerItemTop;
	int m_ChannelItemWidth;
	int m_ChannelItemHeight;
	int m_ChannelItemLeft;
	int m_ChannelItemTop;
	int m_ChannelNameWidth;
	int m_VisibleTunerItems;
	int m_TunerScrollPos;
	int m_VisibleChannelItems;
	int m_ChannelScrollPos;
	HWND m_hwndTunerScroll;
	HWND m_hwndChannelScroll;
	SYSTEMTIME m_EpgBaseTime;

	CPointerVector<CTuner> m_TunerList;
	int m_TotalTuningSpaces;
	int m_CurTuner;
	int m_CurChannel;
	CEpgProgramList *m_pEpgProgramList;
	CEventHandler *m_pEventHandler;
	POINT m_LastCursorPos;

	struct DriverInfo {
		TCHAR DriverMasks[MAX_PATH];
		TCHAR szDisplayName[64];
		TCHAR szIconFile[MAX_PATH];
		int Index;
	};
	std::vector<DriverInfo> m_DriverInfoList;

	void LoadSettings();
	void Layout();
	const CTuningSpaceInfo *GetTuningSpaceInfo(int Index) const;
	const CTuner *GetTuner(int Index,int *pSpace=NULL) const;
	void GetTunerItemRect(int Index,RECT *pRect) const;
	void GetChannelItemRect(int Index,RECT *pRect) const;
	void UpdateTunerItem(int Index) const;
	void UpdateChannelItem(int Index) const;
	int TunerItemHitTest(int x,int y) const;
	int ChannelItemHitTest(int x,int y) const;
	bool SetCurTuner(int Index);
	bool SetCurChannel(int Index);
	void SetTunerScrollPos(int Pos,bool fScroll);
	void SetChannelScrollPos(int Pos,bool fScroll);
	void GetCloseButtonRect(RECT *pRect) const;
	bool CloseButtonHitTest(int x,int y) const;
	void NotifyTunerSelect() const;
	void NotifyChannelSelect() const;

	static const LPCTSTR m_pszWindowClass;
	static HINSTANCE m_hinst;
	static CChannelDisplayMenu *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
