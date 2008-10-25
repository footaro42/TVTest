#ifndef INFORMATION_H
#define INFORMATION_H


#include "BasicWindow.h"


class CInformation : public CBasicWindow {
	static HINSTANCE m_hinst;
	HWND m_hwndProgramInfo;
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
	int m_VideoWidth;
	int m_VideoHeight;
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
	static CInformation *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,
																LPARAM lParam);
	void GetItemRect(int Item,RECT *pRect);
	void UpdateItem(int Item);
public:
	static bool Initialize(HINSTANCE hinst);
	CInformation();
	~CInformation();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	void Reset();
	bool IsVisible() const;
	void SetColor(COLORREF crBackColor,COLORREF crTextColor);
	void SetProgramInfoColor(COLORREF crBackColor,COLORREF crTextColor);
	void SetVideoSize(int Width,int Height);
	void SetAspectRatio(int AspectX,int AspectY);
	void SetDecoderName(LPCTSTR pszName);
	void SetSignalLevel(float Level);
	void ShowSignalLevel(bool fShow);
	void SetBitRate(float BitRate);
	void SetRecordStatus(bool fRecording,LPCTSTR pszFileName=NULL,
							ULONGLONG WroteSize=0,unsigned int RecordTime=0);
	void SetProgramInfo(LPCTSTR pszInfo);
	bool GetProgramInfoNext() { return m_fNextProgramInfo; }
};


#endif
