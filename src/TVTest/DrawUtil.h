#ifndef DRAW_UTIL_H
#define DRAW_UTIL_H


namespace DrawUtil {

enum FillDirection {
	DIRECTION_HORZ,
	DIRECTION_VERT
};

bool Fill(HDC hdc,const RECT *pRect,COLORREF Color);
bool FillGradient(HDC hdc,const RECT *pRect,COLORREF Color1,COLORREF Color2,FillDirection Direction=DIRECTION_HORZ);
bool FillGlossyGradient(HDC hdc,const RECT *pRect,COLORREF Color1,COLORREF Color2,FillDirection Direction=DIRECTION_HORZ,int GlossRatio1=96,int GlossRatio2=48);
bool FillBorder(HDC hdc,const RECT *pBorderRect,const RECT *pEmptyRect,const RECT *pPaintRect,HBRUSH hbr);

int CalcWrapTextLines(HDC hdc,LPCTSTR pszText,int Width);
bool DrawWrapText(HDC hdc,LPCTSTR pszText,const RECT *pRect,int LineHeight);

enum FontType {
	FONT_DEFAULT,
	FONT_MESSAGE,
	FONT_MENU,
	FONT_CAPTION,
	FONT_SMALLCAPTION
};

bool GetSystemFont(FontType Type,LOGFONT *pLogFont);

class CFont {
	HFONT m_hfont;
public:
	CFont();
	CFont(const CFont &Font);
	CFont(const LOGFONT *pFont);
	CFont(FontType Type);
	~CFont();
	CFont &operator=(const CFont &Font);
	bool operator==(const CFont &Font) const;
	bool operator!=(const CFont &Font) const;
	bool Create(const LOGFONT *pLogFont);
	bool Create(FontType Type);
	bool IsCreated() const { return m_hfont!=NULL; }
	void Destroy();
	bool GetLogFont(LOGFONT *pLogFont) const;
	HFONT GetHandle() const { return m_hfont; }
};

class CDeviceContext {
	enum {
		FLAG_DCFROMHWND		=0x00000001UL,
		FLAG_WMPAINT		=0x00000002UL,
		FLAG_BRUSHSELECTED	=0x00000100UL,
		FLAG_PENSELECTED	=0x00000200UL,
		FLAG_FONTSELECTED	=0x00000400UL,
		FLAG_TEXTCOLOR		=0x00000800UL,
		FLAG_BKCOLOR		=0x00001000UL,
		FLAG_BKMODE			=0x00002000UL,
		FLAG_RESTOREMASK	=0xFFFFFF00UL,
	};
	DWORD m_Flags;
	HDC m_hdc;
	HWND m_hwnd;
	PAINTSTRUCT *m_pPaint;
	HBRUSH m_hbrOld;
	HPEN m_hpenOld;
	HFONT m_hfontOld;
	COLORREF m_OldTextColor;
	COLORREF m_OldBkColor;
	int m_OldBkMode;
public:
	enum RectangleStyle {
		RECTANGLE_NORMAL,
		RECTANGLE_FILL,
		RECTANGLE_BORDER
	};

	CDeviceContext(HDC hdc);
	CDeviceContext(HWND hwnd);
	CDeviceContext(HWND hwnd,PAINTSTRUCT *pPaint);
	~CDeviceContext();
	HDC GetHandle() const { return m_hdc; }
	void Restore();
	void Release();
	void SetBrush(HBRUSH hbr);
	void SetPen(HPEN hpen);
	void SetFont(HFONT hfont);
	void SetFont(const CFont &Font);
	void SetTextColor(COLORREF Color);
	void SetBkColor(COLORREF Color);
	void SetBkMode(int BkMode);
	void DrawRectangle(int Left,int Top,int Right,int Bottom,RectangleStyle Style=RECTANGLE_NORMAL);
	void DrawRectangle(const RECT *pRect,RectangleStyle Style=RECTANGLE_NORMAL);
	void DrawLine(int x1,int y1,int x2,int y2);
	void DrawText(LPCTSTR pszText,int Length,RECT *pRect,UINT Format);
};


}	// namespace DrawUtil


namespace Gdiplus {
class Graphics;
class Bitmap;
class SolidBrush;
}

class CGdiPlus {
	bool m_fInitialized;
	ULONG_PTR m_Token;
public:
	class CImage {
		Gdiplus::Bitmap *m_pBitmap;
	public:
		CImage();
		~CImage();
		void Free();
		bool LoadFromFile(LPCWSTR pszFileName);
		bool LoadFromResource(HINSTANCE hinst,LPCWSTR pszName);
		bool LoadFromResource(HINSTANCE hinst,LPCTSTR pszName,LPCTSTR pszType);
		int Width() const;
		int Height() const;
		friend CGdiPlus;
	};

	class CBrush {
		Gdiplus::SolidBrush *m_pBrush;
	public:
		CBrush();
		CBrush(BYTE r,BYTE g,BYTE b,BYTE a=255);
		CBrush(COLORREF Color);
		~CBrush();
		void Free();
		bool CreateSolidBrush(BYTE r,BYTE g,BYTE b,BYTE a=255);
		friend CGdiPlus;
	};

	class CCanvas {
		Gdiplus::Graphics *m_pGraphics;
	public:
		CCanvas(HDC hdc);
		~CCanvas();
		bool Clear(BYTE r,BYTE g,BYTE b,BYTE a=255);
		friend CGdiPlus;
	};

	CGdiPlus();
	~CGdiPlus();
	bool Initialize();
	void Finalize();
	bool IsInitialized() const { return m_fInitialized; }
	bool DrawImage(CCanvas *pCanvas,CImage *pImage,int x,int y);
	bool FillRect(CCanvas *pCanvas,CBrush *pBrush,const RECT *pRect);
};


#endif
