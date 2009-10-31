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

}


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
