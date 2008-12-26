#ifndef PSEUDO_OSD_H
#define PSEUDO_OSD_H


class CPseudoOSD {
	static HINSTANCE m_hinst;
	HWND m_hwnd;
	COLORREF m_crBackColor;
	COLORREF m_crTextColor;
	HFONT m_hFont;
	LPTSTR m_pszText;
	HBITMAP m_hbm;
	unsigned int m_HideTimerID;
	static CPseudoOSD *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,
												WPARAM wParam,LPARAM lParam);
public:
	static bool Initialize(HINSTANCE hinst);
	CPseudoOSD();
	~CPseudoOSD();
	bool Create(HWND hwndParent);
	bool Destroy();
	bool Show(DWORD Time=0);
	bool Hide();
	bool SetText(LPCTSTR pszText);
	bool SetPosition(int Left,int Top,int Width,int Height);
	void SetTextColor(COLORREF crText);
	bool SetTextHeight(int Height);
	bool CalcTextSize(SIZE *pSize);
	bool SetImage(HBITMAP hbm,int Left,int Top);
};


#endif
