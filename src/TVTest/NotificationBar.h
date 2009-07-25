#ifndef NOTIFICATION_BAR_H
#define NOTIFICATION_BAR_H


#include "BasicWindow.h"


class CNotificationBar : public CBasicWindow
{
	enum MessageType {
		MESSAGE_INFO,
		MESSAGE_ERROR
	};
private:
	COLORREF m_BackColor1;
	COLORREF m_BackColor2;
	COLORREF m_TextColor;
	COLORREF m_ErrorTextColor;
	HFONT m_hfont;
	int m_BarHeight;
	bool m_fAnimate;
	LPTSTR m_pszText;
	MessageType m_MessageType;
	static HINSTANCE m_hinst;
	static CNotificationBar *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	CNotificationBar();
	~CNotificationBar();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool Show(DWORD Timeout=0);
	bool Hide();
	bool SetText(LPCTSTR pszText,MessageType Type=MESSAGE_INFO);
	bool SetColors(COLORREF crBackColor1,COLORREF crBackColor2,COLORREF crTextColor);
	bool SetFont(const LOGFONT *pFont);
	void SetAnimate(bool fAnimate) { m_fAnimate=fAnimate; }
	static bool Initialize(HINSTANCE hinst);
};


#endif
