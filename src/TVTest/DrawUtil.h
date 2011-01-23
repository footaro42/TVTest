#ifndef DRAW_UTIL_H
#define DRAW_UTIL_H


namespace DrawUtil {

// ìhÇËÇ¬Ç‘ÇµÇÃï˚å¸
enum FillDirection {
	DIRECTION_HORZ,			// êÖïΩï˚å¸
	DIRECTION_VERT,			// êÇíºï˚å¸
	DIRECTION_HORZMIRROR,	// ç∂âEëŒèÃ
	DIRECTION_VERTMIRROR	// è„â∫ëŒèÃ
};

bool Fill(HDC hdc,const RECT *pRect,COLORREF Color);
bool FillGradient(HDC hdc,const RECT *pRect,COLORREF Color1,COLORREF Color2,
				  FillDirection Direction=DIRECTION_HORZ);
bool FillGlossyGradient(HDC hdc,const RECT *pRect,
						COLORREF Color1,COLORREF Color2,
						FillDirection Direction=DIRECTION_HORZ,
						int GlossRatio1=96,int GlossRatio2=48);
bool FillInterlacedGradient(HDC hdc,const RECT *pRect,
							COLORREF Color1,COLORREF Color2,
							FillDirection Direction=DIRECTION_HORZ,
							COLORREF LineColor=RGB(0,0,0),int LineOpacity=48);
bool GlossOverlay(HDC hdc,const RECT *pRect,
				  int Highlight1=192,int Highlight2=32,
				  int Shadow1=32,int Shadow2=0);
bool ColorOverlay(HDC hdc,const RECT *pRect,COLORREF Color,BYTE Opacity=128);
bool FillBorder(HDC hdc,const RECT *pBorderRect,const RECT *pEmptyRect,const RECT *pPaintRect,HBRUSH hbr);

bool DrawBitmap(HDC hdc,int DstX,int DstY,int DstWidth,int DstHeight,
				HBITMAP hbm,const RECT *pSrcRect=NULL,BYTE Opacity=255);
bool DrawMonoColorDIB(HDC hdcDst,int DstX,int DstY,
					  HDC hdcSrc,int SrcX,int SrcY,int Width,int Height,COLORREF Color);

int CalcWrapTextLines(HDC hdc,LPCTSTR pszText,int Width);
bool DrawWrapText(HDC hdc,LPCTSTR pszText,const RECT *pRect,int LineHeight);

enum FontType {
	FONT_DEFAULT,
	FONT_MESSAGE,
	FONT_MENU,
	FONT_CAPTION,
	FONT_SMALLCAPTION,
	FONT_STATUS
};

bool GetSystemFont(FontType Type,LOGFONT *pLogFont);

class CFont {
	HFONT m_hfont;
public:
	CFont();
	CFont(const CFont &Font);
	CFont(const LOGFONT &Font);
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
	int GetHeight(bool fCell=true) const;
	int GetHeight(HDC hdc,bool fCell=true) const;
};

bool DrawText(HDC hdc,LPCTSTR pszText,const RECT &Rect,UINT Format,
			  const CFont *pFont=NULL,COLORREF Color=CLR_INVALID);

class CBrush {
	HBRUSH m_hbr;
public:
	CBrush();
	CBrush(const CBrush &Brush);
	CBrush(COLORREF Color);
	~CBrush();
	CBrush &operator=(const CBrush &Brush);
	bool Create(COLORREF Color);
	bool IsCreated() const { return m_hbr!=NULL; }
	void Destroy();
	HBRUSH GetHandle() const { return m_hbr; }
};

class CBitmap {
	HBITMAP m_hbm;
public:
	CBitmap();
	CBitmap(const CBitmap &Src);
	~CBitmap();
	CBitmap &operator=(const CBitmap &Src);
	bool Load(HINSTANCE hinst,LPCTSTR pszName,UINT Flags=LR_CREATEDIBSECTION);
	bool Load(HINSTANCE hinst,int ID,UINT Flags=LR_CREATEDIBSECTION) {
		return Load(hinst,MAKEINTRESOURCE(ID),Flags);
	}
	bool IsCreated() const { return m_hbm!=NULL; }
	void Destroy();
	HBITMAP GetHandle() const { return m_hbm; }
	bool IsDIB() const;
};

inline HFONT SelectObject(HDC hdc,const CFont &Font) {
	return static_cast<HFONT>(::SelectObject(hdc,Font.GetHandle()));
}
inline HBRUSH SelectObject(HDC hdc,const CBrush &Brush) {
	return static_cast<HBRUSH>(::SelectObject(hdc,Brush.GetHandle()));
}
inline HBITMAP SelectObject(HDC hdc,const CBitmap &Bitmap) {
	return static_cast<HBITMAP>(::SelectObject(hdc,Bitmap.GetHandle()));
}

class COffscreen {
	HDC m_hdc;
	HBITMAP m_hbm;
	HBITMAP m_hbmOld;
	int m_Width;
	int m_Height;

public:
	COffscreen();
	~COffscreen();
	bool Create(int Width,int Height,HDC hdc=NULL);
	void Destroy();
	bool IsCreated() const { return m_hdc!=NULL; }
	HDC GetDC() const { return m_hdc; }
	int GetWidth() const { return m_Width; }
	int GetHeight() const { return m_Height; }
	bool CopyTo(HDC hdc,const RECT *pDstRect=NULL);
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
		CImage(const CImage &Src);
		~CImage();
		CImage &operator=(const CImage &Src);
		void Free();
		bool LoadFromFile(LPCWSTR pszFileName);
		bool LoadFromResource(HINSTANCE hinst,LPCWSTR pszName);
		bool LoadFromResource(HINSTANCE hinst,LPCTSTR pszName,LPCTSTR pszType);
		bool Create(int Width,int Height,int BitsPerPixel);
		bool CreateFromBitmap(HBITMAP hbm,HPALETTE hpal=NULL);
		bool CreateFromDIB(const BITMAPINFO *pbmi,const void *pBits);
		bool IsCreated() const;
		int GetWidth() const;
		int GetHeight() const;
		void Clear();
		friend CGdiPlus;
		friend class CCanvas;
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
		CCanvas(CImage *pImage);
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
	bool DrawImage(CCanvas *pCanvas,int DstX,int DstY,int DstWidth,int DstHeight,
				   CImage *pImage,int SrcX,int SrcY,int SrcWidth,int SrcHeight,float Opacity=1.0f);
	bool FillRect(CCanvas *pCanvas,CBrush *pBrush,const RECT *pRect);
};


#include <uxtheme.h>
#include <vssym32.h>

class CUxTheme
{
	HTHEME m_hTheme;
#ifdef WINDOWS2000_SUPPORT
	HMODULE m_hLib;
#endif

public:
	CUxTheme();
	~CUxTheme();
	bool Initialize();
	bool Open(HWND hwnd,LPCWSTR pszClassList);
	void Close();
	bool IsOpen() const;
	bool IsActive();
	bool DrawBackground(HDC hdc,int PartID,int StateID,const RECT *pRect);
	bool DrawBackground(HDC hdc,int PartID,int StateID,
						int BackgroundPartID,int BackgroundStateID,
						const RECT *pRect);
	bool DrawText(HDC hdc,int PartID,int StateID,LPCWSTR pszText,
				  DWORD TextFlags,const RECT *pRect);
	bool GetTextExtent(HDC hdc,int PartID,int StateID,LPCWSTR pszText,
					   DWORD TextFlags,RECT *pExtentRect);
	bool GetMargins(int PartID,int StateID,int PropID,MARGINS *pMargins);
	bool GetColor(int PartID,int StateID,int PropID,COLORREF *pColor);
	bool GetFont(int PartID,int StateID,int PropID,LOGFONT *pFont);
	bool GetInt(int PartID,int StateID,int PropID,int *pValue);
	bool GetPartSize(HDC hdc,int PartID,int StateID,SIZE *pSize);
};


#endif
