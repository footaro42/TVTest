#ifndef COLOR_PALETTE_H
#define COLOR_PALETTE_H


#include "BasicWindow.h"


class CColorPalette : public CBasicWindow {
	int m_NumColors;
	RGBQUAD *m_pPalette;
	int m_SelColor;
	int m_HotColor;
	int m_Left;
	int m_Top;
	int m_ItemWidth;
	int m_ItemHeight;
	HWND m_hwndToolTip;
	void GetItemRect(int Index,RECT *pRect) const;
	void DrawSelRect(HDC hdc,int Sel,bool fSel);
	void DrawNewSelHighlight(int OldSel,int NewSel);
	void SetToolTip();
	void SendNotify(int Code);
	static HINSTANCE m_hinst;
	static CColorPalette *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	CColorPalette();
	~CColorPalette();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool GetPalette(RGBQUAD *pPalette);
	bool SetPalette(const RGBQUAD *pPalette,int NumColors);
	COLORREF GetColor(int Index) const;
	bool SetColor(int Index,COLORREF Color);
	int GetSel() const;
	bool SetSel(int Sel);
	int GetHot() const;
	int FindColor(COLORREF Color) const;
	static bool Initialize(HINSTANCE hinst);
	enum {
		NOTIFY_SELCHANGE=1,
		NOTIFY_HOTCHANGE,
		NOTIFY_RBUTTONDOWN,
		NOTIFY_DOUBLECLICK
	};
};


#endif
