#ifndef CHANNEL_PANEL_H
#define CHANNEL_PANEL_H


#include "PanelForm.h"
#include "EpgProgramList.h"
#include "ChannelList.h"
#include "PointerArray.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "EventInfoPopup.h"
#include "LogoManager.h"
#include "Tooltip.h"


class CChannelPanel : public CPanelForm::CPage
{
public:
	struct ThemeInfo {
		Theme::Style ChannelNameStyle;
		Theme::Style CurChannelNameStyle;
		Theme::Style EventStyle[2];
		Theme::Style CurChannelEventStyle[2];
		COLORREF MarginColor;
	};

	CChannelPanel();
	~CChannelPanel();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool SetEpgProgramList(CEpgProgramList *pList);
	bool SetChannelList(const CChannelList *pChannelList,bool fSetEvent=true);
	bool UpdateChannelList();
	bool UpdateChannel(int ChannelIndex);
	void ClearChannelList() { SetChannelList(NULL); }
	bool IsChannelListEmpty() const;
	bool SetCurrentChannel(int CurChannel);
	class CEventHandler {
	public:
		virtual ~CEventHandler() {}
		virtual void OnChannelClick(const CChannelInfo *pChannelInfo) {}
		virtual void OnRButtonDown() {}
	};
	void SetEventHandler(CEventHandler *pEventHandler);
	bool SetTheme(const ThemeInfo *pTheme);
	bool GetTheme(ThemeInfo *pTheme) const;
	/*
	bool SetColors(const Theme::GradientInfo *pChannelBackGradient,COLORREF ChannelTextColor,
				   const Theme::GradientInfo *pCurChannelBackGradient,COLORREF CurChannelTextColor,
				   const Theme::GradientInfo *pEventBackGradient,COLORREF EventTextColor,
				   const Theme::GradientInfo *pEventBackGradient,COLORREF EventTextColor,
				   const Theme::GradientInfo *pCurChannelEventBackGradient,COLORREF CurChannelEventTextColor,
				   const Theme::GradientInfo *pCurChannelEventBackGradient,COLORREF CurChannelEventTextColor,
				   COLORREF MarginColor);
	*/
	bool SetFont(const LOGFONT *pFont);
	bool SetEventInfoFont(const LOGFONT *pFont);
	void SetDetailToolTip(bool fDetail);
	bool GetDetailToolTip() const { return m_fDetailToolTip; }
	bool SetEventsPerChannel(int Events);
	int GetEventsPerChannel() const { return m_EventsPerChannel; }
	bool ExpandChannel(int Channel,bool fExpand);
	void SetLogoManager(CLogoManager *pLogoManager);
	bool QueryUpdate() const;
	static bool Initialize(HINSTANCE hinst);

private:
	CEpgProgramList *m_pProgramList;
	DrawUtil::CFont m_Font;
	DrawUtil::CFont m_ChannelFont;
	int m_FontHeight;
	int m_ChannelNameMargin;
	int m_EventNameMargin;
	int m_EventNameLines;
	int m_ItemHeight;
	int m_ExpandedItemHeight;
	/*
	Theme::GradientInfo m_ChannelBackGradient;
	COLORREF m_ChannelTextColor;
	Theme::GradientInfo m_CurChannelBackGradient;
	COLORREF m_CurChannelTextColor;
	Theme::GradientInfo m_EventBackGradient[2];
	COLORREF m_EventTextColor[2];
	Theme::GradientInfo m_CurEventBackGradient[2];
	COLORREF m_CurEventTextColor[2];
	COLORREF m_MarginColor;
	*/
	ThemeInfo m_Theme;
	HBITMAP m_hbmChevron;
	int m_EventsPerChannel;
	int m_ExpandEvents;
	int m_ScrollPos;
	class CChannelEventInfo {
		CChannelInfo m_ChannelInfo;
		int m_OriginalChannelIndex;
		std::vector<CEventInfoData> m_EventList;
		HBITMAP m_hbmLogo;
		bool m_fExpanded;
	public:
		CChannelEventInfo(const CChannelInfo *pChannelInfo,int OriginalIndex);
		~CChannelEventInfo();
		bool SetEventInfo(int Index,const CEventInfoData *pInfo);
		const CChannelInfo *GetChannelInfo() const { return &m_ChannelInfo; }
		const CEventInfoData &GetEventInfo(int Index) const { return m_EventList[Index]; }
		int NumEvents() const { return (int)m_EventList.size(); }
		void SetMaxEvents(int Events);
		bool IsEventEnabled(int Index) const;
		WORD GetTransportStreamID() const { return m_ChannelInfo.GetTransportStreamID(); }
		WORD GetNetworkID() const { return m_ChannelInfo.GetNetworkID(); }
		WORD GetServiceID() const { return m_ChannelInfo.GetServiceID(); }
		int FormatEventText(LPTSTR pszText,int MaxLength,int Index) const;
		void DrawChannelName(HDC hdc,const RECT *pRect);
		void DrawEventName(HDC hdc,const RECT *pRect,int Index);
		int GetOriginalChannelIndex() const { return m_OriginalChannelIndex; }
		HBITMAP GetLogo() const { return m_hbmLogo; }
		void SetLogo(HBITMAP hbm) { m_hbmLogo=hbm; }
		bool IsExpanded() const { return m_fExpanded; }
		void Expand(bool fExpand) { m_fExpanded=fExpand; }
	};
	CPointerVector<CChannelEventInfo> m_ChannelList;
	int m_CurChannel;
	CEventHandler *m_pEventHandler;
	CTooltip m_Tooltip;
	bool m_fDetailToolTip;
	CEventInfoPopup m_EventInfoPopup;
	CEventInfoPopupManager m_EventInfoPopupManager;
	class CEventInfoPopupHandler : public CEventInfoPopupManager::CEventHandler
	{
		CChannelPanel *m_pChannelPanel;
	public:
		CEventInfoPopupHandler(CChannelPanel *pChannelPanel);
		bool HitTest(int x,int y,LPARAM *pParam);
		bool GetEventInfo(LPARAM Param,const CEventInfoData **ppInfo);
	};
	friend CEventInfoPopupHandler;
	CEventInfoPopupHandler m_EventInfoPopupHandler;
	CLogoManager *m_pLogoManager;
	SYSTEMTIME m_UpdatedTime;

	static const LPCTSTR m_pszClassName;
	static HINSTANCE m_hinst;
	static CChannelPanel *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	bool UpdateEvents(CChannelEventInfo *pInfo,const SYSTEMTIME *pTime=NULL);
	void Draw(HDC hdc,const RECT *prcPaint);
	void SetScrollPos(int Pos);
	void SetScrollBar();
	void CalcItemHeight();
	int CalcHeight() const;
	void GetItemRect(int Index,RECT *pRect);
	enum HitType {
		HIT_CHANNELNAME,
		HIT_CHEVRON,
		HIT_EVENT1
	};
	int HitTest(int x,int y,HitType *pType=NULL) const;
	bool CreateTooltip();
	void SetTooltips();
	bool EventInfoPopupHitTest(int x,int y,LPARAM *pParam);
	bool GetEventInfoPopupEventInfo(LPARAM Param,const CEventInfoData **ppInfo);
};


#endif
