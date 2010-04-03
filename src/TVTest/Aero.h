#ifndef AERO_H
#define AERO_H


class CAeroGlass
{
	HMODULE m_hDwmLib;
	bool LoadDwmLib();

public:
	CAeroGlass();
	~CAeroGlass();
	bool IsEnabled();
	bool ApplyAeroGlass(HWND hwnd,const RECT *pRect);
	bool EnableNcRendering(HWND hwnd,bool fEnable);
};

class CBufferedPaint
{
	HMODULE m_hThemeLib;
	HANDLE m_hPaintBuffer;

public:
	CBufferedPaint();
	~CBufferedPaint();
	HDC Begin(HDC hdc,const RECT *pRect,bool fErase=false);
	void End();
	bool Clear(const RECT *pRect=NULL);
	bool SetAlpha(BYTE Alpha);
	bool SetOpaque() { return SetAlpha(255); }
};


#endif
