#ifndef CHANNEL_PANEL_H
#define CHANNEL_PANEL_H


#include "InfoPanel.h"
#include "EpgProgramList.h"
#include "ChannelList.h"
#include "PointerArray.h"


class CChannelPanel : public CInfoPanelPage {
public:
	CChannelPanel();
	~CChannelPanel();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool SetEpgProgramList(CEpgProgramList *pList);
	bool SetChannelList(const CChannelList *pChannelList);
	bool UpdateChannelList();
	class CEventHandler {
	public:
		virtual ~CEventHandler() {}
		virtual void OnChannelClick(const CChannelInfo *pChannelInfo) {}
		virtual void OnRButtonDown() {}
	};
	void SetEventHandler(CEventHandler *pEventHandler);
	bool SetColors(COLORREF ChannelBackColor,COLORREF ChannelTextColor,
				   COLORREF EventBackColor,COLORREF EventTextColor);
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
	COLORREF m_ChannelBackColor;
	COLORREF m_ChannelTextColor;
	COLORREF m_EventBackColor;
	COLORREF m_EventTextColor;
	int m_ScrollPos;
	class CChannelEventInfo {
		CChannelInfo m_ChannelInfo;
		CEventInfoData m_EventInfo[2];
	public:
		CChannelEventInfo(const CChannelInfo *pChannelInfo);
		~CChannelEventInfo();
		bool SetEventInfo(int Index,const CEventInfoData *pInfo);
		const CChannelInfo *GetChannelInfo() const { return &m_ChannelInfo; }
		WORD GetTransportStreamID() const { return m_ChannelInfo.GetTransportStreamID(); }
		WORD GetServiceID() const { return m_ChannelInfo.GetServiceID(); }
		int FormatEventText(LPTSTR pszText,int MaxLength,int Index) const;
		void DrawChannelName(HDC hdc,const RECT *pRect);
		void DrawEventName(HDC hdc,const RECT *pRect,int Index);
	};
	CPointerVector<CChannelEventInfo> m_ChannelList;
	CEventHandler *m_pEventHandler;
	HWND m_hwndToolTip;
	static HINSTANCE m_hinst;
	static CChannelPanel *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	void Draw(HDC hdc,const RECT *prcPaint);
	void SetScrollBar();
	void CalcItemHeight();
	void SetToolTips();
};


#endif
