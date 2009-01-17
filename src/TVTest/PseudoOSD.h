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
	struct {
		int Left,Top,Width,Height;
	} m_Position;
	unsigned int m_TimerID;
	int m_AnimationCount;
	static CPseudoOSD *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,
												WPARAM wParam,LPARAM lParam);
public:
	static bool Initialize(HINSTANCE hinst);
	CPseudoOSD();
	~CPseudoOSD();
	bool Create(HWND hwndParent);
	bool Destroy();
	bool Show(DWORD Time=0,bool fAnimation=false);
	bool Hide();
	bool IsVisible() const;
	bool SetText(LPCTSTR pszText);
	bool SetPosition(int Left,int Top,int Width,int Height);
	void GetPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const;
	void SetTextColor(COLORREF crText);
	bool SetTextHeight(int Height);
	bool CalcTextSize(SIZE *pSize);
	bool SetImage(HBITMAP hbm,int Left,int Top);
};


#endif
