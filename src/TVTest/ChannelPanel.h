#ifndef CHANNEL_PANEL_H
#define CHANNEL_PANEL_H


#include "InfoPanel.h"
#include "EpgProgramList.h"
#include "ChannelList.h"
#include "PointerArray.h"
#include "Theme.h"


class CChannelPanel : public CInfoPanelPage {
public:
	CChannelPanel();
	~CChannelPanel();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool SetEpgProgramList(CEpgProgramList *pList);
	bool SetChannelList(const CChannelList *pChannelList);
	bool UpdateChannelList(bool fUpdateProgramList);
	bool UpdateChannel(int ChannelIndex,bool fUpdateProgramList);
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
	bool SetColors(const Theme::GradientInfo *pChannelBackGradient,COLORREF ChannelTextColor,
				   const Theme::GradientInfo *pCurChannelBackGradient,COLORREF CurChannelTextColor,
				   const Theme::GradientInfo *pEventBackGradient,COLORREF EventTextColor,
				   const Theme::GradientInfo *pCurEventBackGradient,COLORREF CurEventTextColor,
				   COLORREF MarginColor);
	bool SetFont(const LOGFONT *pFont);
	static bool Initialize(HINSTANCE hinst);

private:
	CEpgProgramList *m_pProgramList;
	HFONT m_hfont;
	HFONT m_hfontChannel;
	int m_FontHeight;
	int m_ChannelNameMargin;
	int m_EventNameLines;
	int m_ItemHeight;
	Theme::GradientInfo m_ChannelBackGradient;
	COLORREF m_ChannelTextColor;
	Theme::GradientInfo m_CurChannelBackGradient;
	COLORREF m_CurChannelTextColor;
	Theme::GradientInfo m_EventBackGradient;
	COLORREF m_EventTextColor;
	Theme::GradientInfo m_CurEventBackGradient;
	COLORREF m_CurEventTextColor;
	COLORREF m_MarginColor;
	int m_ScrollPos;
	class CChannelEventInfo {
		CChannelInfo m_ChannelInfo;
		int m_OriginalChannelIndex;
		CEventInfoData m_EventInfo[2];
	public:
		CChannelEventInfo(const CChannelInfo *pChannelInfo,int OriginalIndex);
		~CChannelEventInfo();
		bool SetEventInfo(int Index,const CEventInfoData *pInfo);
		const CChannelInfo *GetChannelInfo() const { return &m_ChannelInfo; }
		WORD GetTransportStreamID() const { return m_ChannelInfo.GetTransportStreamID(); }
		WORD GetServiceID() const { return m_ChannelInfo.GetServiceID(); }
		int FormatEventText(LPTSTR pszText,int MaxLength,int Index) const;
		void DrawChannelName(HDC hdc,const RECT *pRect);
		void DrawEventName(HDC hdc,const RECT *pRect,int Index);
		int GetOriginalChannelIndex() const { return m_OriginalChannelIndex; }
	};
	CPointerVector<CChannelEventInfo> m_ChannelList;
	int m_CurChannel;
	CEventHandler *m_pEventHandler;
	HWND m_hwndToolTip;
	static HINSTANCE m_hinst;
	static CChannelPanel *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	void Draw(HDC hdc,const RECT *prcPaint);
	void SetScrollBar();
	void CalcItemHeight();
	void GetItemRect(int Index,RECT *pRect);
	void SetToolTips();
};


#endif