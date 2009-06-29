#ifndef INFORMATION_H
#define INFORMATION_H


#include "InfoPanel.h"


class CInformation : public CInfoPanelPage {
public:
	class CEventHandler {
	public:
		virtual ~CEventHandler() {}
		virtual bool OnProgramInfoUpdate(bool fNext) { return false; }
	};

private:
	static HINSTANCE m_hinst;
	HWND m_hwndProgramInfo;
	WNDPROC m_pOldProgramInfoProc;
	HWND m_hwndProgramInfoPrev;
	HWND m_hwndProgramInfoNext;
	COLORREF m_crBackColor;
	COLORREF m_crTextColor;
	COLORREF m_crProgramInfoBackColor;
	COLORREF m_crProgramInfoTextColor;
	HBRUSH m_hbrBack;
	HBRUSH m_hbrProgramInfoBack;
	HFONT m_hFont;
	int m_FontHeight;
	int m_LineMargin;
	int m_OriginalVideoWidth;
	int m_OriginalVideoHeight;
	int m_DisplayVideoWidth;
	int m_DisplayVideoHeight;
	int m_AspectX;
	int m_AspectY;
	LPTSTR m_pszDecoderName;
	bool m_fSignalLevel;
	float m_SignalLevel;
	bool m_fBitRate;
	float m_BitRate;
	bool m_fRecording;
	ULONGLONG m_RecordWroteSize;
	unsigned int m_RecordTime;
	ULARGE_INTEGER m_DiskFreeSpace;
	LPTSTR m_pszProgramInfo;
	bool m_fNextProgramInfo;
	CEventHandler *m_pEventHandler;
	static CInformation *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static LRESULT CALLBACK ProgramInfoHookProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	void GetItemRect(int Item,RECT *pRect);
	void UpdateItem(int Item);
	void CalcFontHeight();

public:
	static bool Initialize(HINSTANCE hinst);
	CInformation();
	~CInformation();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	void Reset();
	bool IsVisible() const;
	void SetColor(COLORREF crBackColor,COLORREF crTextColor);
	void SetProgramInfoColor(COLORREF crBackColor,COLORREF crTextColor);
	bool SetFont(const LOGFONT *pFont);
	void SetVideoSize(int OriginalWidth,int OriginalHeight,int DisplayWidth,int DisplayHeight);
	void SetAspectRatio(int AspectX,int AspectY);
	void SetDecoderName(LPCTSTR pszName);
	void SetSignalLevel(float Level);
	void ShowSignalLevel(bool fShow);
	void SetBitRate(float BitRate);
	void UpdateErrorCount();
	void SetRecordStatus(bool fRecording,LPCTSTR pszFileName=NULL,
							ULONGLONG WroteSize=0,unsigned int RecordTime=0);
	void SetProgramInfo(LPCTSTR pszInfo);
	bool GetProgramInfoNext() { return m_fNextProgramInfo; }
	bool SetEventHandler(CEventHandler *pHandler);
};


#endif
