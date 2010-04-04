#ifndef PSEUDO_OSD_H
#define PSEUDO_OSD_H


class CPseudoOSD
{
	HWND m_hwnd;
	COLORREF m_crBackColor;
	COLORREF m_crTextColor;
	HFONT m_hFont;
	LPTSTR m_pszText;
	HBITMAP m_hbmIcon;
	int m_IconWidth;
	int m_IconHeight;
	HBITMAP m_hbm;
	struct {
		int Left,Top,Width,Height;
	} m_Position;
	UINT_PTR m_TimerID;
	int m_AnimationCount;

	static const LPCTSTR m_pszWindowClass;
	static HINSTANCE m_hinst;
	static CPseudoOSD *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	static bool Initialize(HINSTANCE hinst);
	static bool IsPseudoOSD(HWND hwnd);

	CPseudoOSD();
	~CPseudoOSD();
	bool Create(HWND hwndParent);
	bool Destroy();
	bool Show(DWORD Time=0,bool fAnimation=false);
	bool Hide();
	bool IsVisible() const;
	bool SetText(LPCTSTR pszText,HBITMAP hbmIcon=NULL,int IconWidth=0,int IconHeight=0);
	bool SetPosition(int Left,int Top,int Width,int Height);
	void GetPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const;
	void SetTextColor(COLORREF crText);
	bool SetTextHeight(int Height);
	bool CalcTextSize(SIZE *pSize);
	bool SetImage(HBITMAP hbm);
};


#endif
