#ifndef CHANNEL_PANEL_H
#define CHANNEL_PANEL_H


#include "BasicWindow.h"
#include "EpgProgramList.h"
#include "ChannelList.h"
#include "PointerArray.h"


class CChannelPanel : public CBasicWindow {
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
		virtual void OnChannelClick(WORD ServiceID) {}
	};
	void SetEventHandler(CEventHandler *pEventHandler);
	bool SetColors(COLORREF ChannelBackColor,COLORREF ChannelTextColor,
				   COLORREF EventBackColor,COLORREF EventTextColor);
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
		TCHAR m_szChannelName[MAX_CHANNEL_NAME];
		WORD m_TransportStreamID;
		WORD m_ServiceID;
		CEventInfoData m_EventInfo[2];
	public:
		CChannelEventInfo(const CChannelInfo *pChannelInfo);
		~CChannelEventInfo();
		bool SetEventInfo(int Index,const CEventInfoData *pInfo);
		WORD GetTransportStreamID() const { return m_TransportStreamID; }
		WORD GetServiceID() const { return m_ServiceID; }
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
	void SetToolTips();
};


#endif
