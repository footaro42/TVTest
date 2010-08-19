#include "stdafx.h"
#include "DrawUtil.h"
#include "Util.h"

// このマクロを使うとGDI+のヘッダでエラーが出る
/*
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
*/


namespace DrawUtil {


// 単色で塗りつぶす
bool Fill(HDC hdc,const RECT *pRect,COLORREF Color)
{
	HBRUSH hbr=::CreateSolidBrush(Color);

	if (hbr==NULL)
		return false;
	::FillRect(hdc,pRect,hbr);
	::DeleteObject(hbr);
	return true;
}


// グラデーションで塗りつぶす
bool FillGradient(HDC hdc,const RECT *pRect,COLORREF Color1,COLORREF Color2,
				  FillDirection Direction)
{
	if (hdc==NULL || pRect==NULL
			|| pRect->left>pRect->right || pRect->top>pRect->bottom)
		return false;

	if (Direction==DIRECTION_HORZMIRROR || Direction==DIRECTION_VERTMIRROR) {
		RECT rc;

		rc=*pRect;
		if (Direction==DIRECTION_HORZMIRROR) {
			rc.right=(pRect->left+pRect->right)/2;
			if (rc.right>rc.left) {
				FillGradient(hdc,&rc,Color1,Color2,DIRECTION_HORZ);
				rc.left=rc.right;
			}
			rc.right=pRect->right;
			FillGradient(hdc,&rc,Color2,Color1,DIRECTION_HORZ);
		} else {
			rc.bottom=(pRect->top+pRect->bottom)/2;
			if (rc.bottom>rc.top) {
				FillGradient(hdc,&rc,Color1,Color2,DIRECTION_VERT);
				rc.top=rc.bottom;
			}
			rc.bottom=pRect->bottom;
			FillGradient(hdc,&rc,Color2,Color1,DIRECTION_VERT);
		}
		return true;
	}

	TRIVERTEX vert[2];
	GRADIENT_RECT rect={0,1};

	vert[0].x=pRect->left;
	vert[0].y=pRect->top;
	vert[0].Red=GetRValue(Color1)<<8;
	vert[0].Green=GetGValue(Color1)<<8;
	vert[0].Blue=GetBValue(Color1)<<8;
	vert[0].Alpha=0x0000;
	vert[1].x=pRect->right;
	vert[1].y=pRect->bottom;
	vert[1].Red=GetRValue(Color2)<<8;
	vert[1].Green=GetGValue(Color2)<<8;
	vert[1].Blue=GetBValue(Color2)<<8;
	vert[1].Alpha=0x0000;
	return ::GradientFill(hdc,vert,2,&rect,1,
		Direction==DIRECTION_HORZ?GRADIENT_FILL_RECT_H:GRADIENT_FILL_RECT_V)!=FALSE;
}


// 光沢のあるグラデーションで塗りつぶす
bool FillGlossyGradient(HDC hdc,const RECT *pRect,
						COLORREF Color1,COLORREF Color2,
						FillDirection Direction,int GlossRatio1,int GlossRatio2)
{
	RECT rc;
	COLORREF crCenter,crEnd;
	FillDirection Dir;

	rc.left=pRect->left;
	rc.top=pRect->top;
	if (Direction==DIRECTION_HORZ || Direction==DIRECTION_HORZMIRROR) {
		rc.right=(rc.left+pRect->right)/2;
		rc.bottom=pRect->bottom;
		Dir=DIRECTION_HORZ;
	} else {
		rc.right=pRect->right;
		rc.bottom=(rc.top+pRect->bottom)/2;
		Dir=DIRECTION_VERT;
	}
	if (Direction==DIRECTION_HORZ || Direction==DIRECTION_VERT) {
		crCenter=MixColor(Color1,Color2,128);
		crEnd=Color2;
	} else {
		crCenter=Color2;
		crEnd=Color1;
	}
	DrawUtil::FillGradient(hdc,&rc,
						   MixColor(RGB(255,255,255),Color1,GlossRatio1),
						   MixColor(RGB(255,255,255),crCenter,GlossRatio2),
						   Dir);
	if (Direction==DIRECTION_HORZ || Direction==DIRECTION_HORZMIRROR) {
		rc.left=rc.right;
		rc.right=pRect->right;
	} else {
		rc.top=rc.bottom;
		rc.bottom=pRect->bottom;
	}
	DrawUtil::FillGradient(hdc,&rc,crCenter,crEnd,Dir);
	return true;
}


// 
bool FillInterlacedGradient(HDC hdc,const RECT *pRect,
							COLORREF Color1,COLORREF Color2,FillDirection Direction,
							COLORREF LineColor,int LineOpacity)
{
	if (hdc==NULL || pRect==NULL)
		return false;

	int Width=pRect->right-pRect->left;
	int Height=pRect->bottom-pRect->top;
	if (Width<=0 || Height<=0)
		return false;
	if (Width==1 || Height==1)
		return Fill(hdc,pRect,MixColor(Color1,Color2));

	HPEN hpenOld=static_cast<HPEN>(::SelectObject(hdc,::GetStockObject(DC_PEN)));
	COLORREF OldPenColor=::GetDCPenColor(hdc);

	if (Direction==DIRECTION_HORZ || Direction==DIRECTION_HORZMIRROR) {
		int Center=pRect->left*2+Width-1;

		for (int x=pRect->left;x<pRect->right;x++) {
			COLORREF Color;

			Color=MixColor(Color1,Color2,
						   (BYTE)(Direction==DIRECTION_HORZ?
								  (pRect->right-1-x)*255/(Width-1):
								  abs(Center-x*2)*255/(Width-1)));
			if ((x-pRect->left)%2==1)
				Color=MixColor(LineColor,Color,LineOpacity);
			::SetDCPenColor(hdc,Color);
			::MoveToEx(hdc,x,pRect->top,NULL);
			::LineTo(hdc,x,pRect->bottom);
		}
	} else {
		int Center=pRect->top*2+Height-1;

		for (int y=pRect->top;y<pRect->bottom;y++) {
			COLORREF Color;

			Color=MixColor(Color1,Color2,
						   (BYTE)(Direction==DIRECTION_VERT?
								  (pRect->bottom-1-y)*255/(Height-1):
								  abs(Center-y*2)*255/(Height-1)));
			if ((y-pRect->top)%2==1)
				Color=MixColor(LineColor,Color,LineOpacity);
			::SetDCPenColor(hdc,Color);
			::MoveToEx(hdc,pRect->left,y,NULL);
			::LineTo(hdc,pRect->right,y);
		}
	}

	::SelectObject(hdc,hpenOld);
	::SetDCPenColor(hdc,OldPenColor);

	return true;
}


// 光沢を描画する
bool GlossOverlay(HDC hdc,const RECT *pRect,
				  int Highlight1,int Highlight2,int Shadow1,int Shadow2)
{
	const int Width=pRect->right-pRect->left;
	const int Height=pRect->bottom-pRect->top;
	if (Width<=0 || Height<=0)
		return false;

	BITMAPINFO bmi;
	::ZeroMemory(&bmi,sizeof(bmi));
	bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth=Width;
	bmi.bmiHeader.biHeight=-Height;
	bmi.bmiHeader.biPlanes=1;
	bmi.bmiHeader.biBitCount=32;
	void *pBits;
	HBITMAP hbm=::CreateDIBSection(NULL,&bmi,DIB_RGB_COLORS,&pBits,NULL,0);
	if (hbm==NULL)
		return false;

	const SIZE_T RowBytes=Width*4;
	const int Center=Height/2;
	int x,y;
	BYTE *p=static_cast<BYTE*>(pBits);
	for (y=0;y<Center;y++) {
		::FillMemory(p,RowBytes,
					 (BYTE)(((y*Highlight2)+(Center-1-y)*Highlight1)/(Center-1)));
		p+=RowBytes;
	}
	for (;y<Height;y++) {
		BYTE Alpha=(BYTE)(((y-Center)*Shadow2+(Height-1-y)*Shadow1)/(Height-Center-1));
		::ZeroMemory(p,RowBytes);
		for (x=0;x<Width;x++) {
			p[x*4+3]=Alpha;
		}
		p+=RowBytes;
	}

	HDC hdcMemory=::CreateCompatibleDC(hdc);
	if (hdcMemory==NULL) {
		::DeleteObject(hbm);
		return false;
	}
	HBITMAP hbmOld=SelectBitmap(hdcMemory,hbm);
	BLENDFUNCTION bf={AC_SRC_OVER,0,255,AC_SRC_ALPHA};
	::AlphaBlend(hdc,pRect->left,pRect->top,Width,Height,
				 hdcMemory,0,0,Width,Height,bf);
	::SelectObject(hdcMemory,hbmOld);
	::DeleteDC(hdcMemory);
	::DeleteObject(hbm);
	return true;
}


// 単色を合成する
bool ColorOverlay(HDC hdc,const RECT *pRect,COLORREF Color,BYTE Opacity)
{
	const int Width=pRect->right-pRect->left;
	const int Height=pRect->bottom-pRect->top;
	if (Width<=0 || Height<=0)
		return false;

	BITMAPINFO bmi;
	::ZeroMemory(&bmi,sizeof(bmi));
	bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth=Width;
	bmi.bmiHeader.biHeight=-Height;
	bmi.bmiHeader.biPlanes=1;
	bmi.bmiHeader.biBitCount=32;
	void *pBits;
	HBITMAP hbm=::CreateDIBSection(NULL,&bmi,DIB_RGB_COLORS,&pBits,NULL,0);
	if (hbm==NULL)
		return false;

	const DWORD Pixel=0xFF000000|((DWORD)GetRValue(Color)<<16)|((DWORD)GetGValue(Color)<<8)|(DWORD)GetBValue(Color);
	DWORD *p=static_cast<DWORD*>(pBits);
	DWORD *pEnd=p+Width*Height;
	do {
		*p++=Pixel;
	} while (p<pEnd);

	HDC hdcMemory=::CreateCompatibleDC(hdc);
	if (hdcMemory==NULL) {
		::DeleteObject(hbm);
		return false;
	}
	HBITMAP hbmOld=SelectBitmap(hdcMemory,hbm);
	BLENDFUNCTION bf={AC_SRC_OVER,0,Opacity,0};
	::AlphaBlend(hdc,pRect->left,pRect->top,Width,Height,
				 hdcMemory,0,0,Width,Height,bf);
	::SelectObject(hdcMemory,hbmOld);
	::DeleteDC(hdcMemory);
	::DeleteObject(hbm);
	return true;
}


// 指定された矩形の周囲を塗りつぶす
bool FillBorder(HDC hdc,const RECT *pBorderRect,const RECT *pEmptyRect,const RECT *pPaintRect,HBRUSH hbr)
{
	RECT rc;

	if (pPaintRect->left<pBorderRect->right && pPaintRect->right>pBorderRect->left) {
		rc.left=max(pPaintRect->left,pBorderRect->left);
		rc.right=min(pPaintRect->right,pBorderRect->right);
		rc.top=max(pPaintRect->top,pBorderRect->top);
		rc.bottom=min(pPaintRect->bottom,pEmptyRect->top);
		if (rc.top<rc.bottom)
			::FillRect(hdc,&rc,hbr);
		rc.top=max(pEmptyRect->bottom,pPaintRect->top);
		rc.bottom=min(pPaintRect->bottom,pBorderRect->bottom);
		if (rc.top<rc.bottom)
			::FillRect(hdc,&rc,hbr);
	}
	if (pPaintRect->top<pEmptyRect->bottom && pPaintRect->bottom>pEmptyRect->top) {
		rc.top=max(pEmptyRect->top,pPaintRect->top);
		rc.bottom=min(pEmptyRect->bottom,pPaintRect->bottom);
		rc.left=max(pPaintRect->left,pBorderRect->left);
		rc.right=min(pEmptyRect->left,pPaintRect->right);
		if (rc.left<rc.right)
			::FillRect(hdc,&rc,hbr);
		rc.left=max(pPaintRect->left,pEmptyRect->right);
		rc.right=min(pPaintRect->right,pBorderRect->right);
		if (rc.left<rc.right)
			::FillRect(hdc,&rc,hbr);
	}
	return true;
}


// ビットマップを描画する
bool DrawBitmap(HDC hdc,int DstX,int DstY,int DstWidth,int DstHeight,
				HBITMAP hbm,const RECT *pSrcRect,BYTE Opacity)
{
	if (hdc==NULL || hbm==NULL)
		return false;

	int SrcX,SrcY,SrcWidth,SrcHeight;
	if (pSrcRect!=NULL) {
		SrcX=pSrcRect->left;
		SrcY=pSrcRect->top;
		SrcWidth=pSrcRect->right-pSrcRect->left;
		SrcHeight=pSrcRect->bottom-pSrcRect->top;
	} else {
		BITMAP bm;
		if (::GetObject(hbm,sizeof(BITMAP),&bm)!=sizeof(BITMAP))
			return false;
		SrcX=SrcY=0;
		SrcWidth=bm.bmWidth;
		SrcHeight=bm.bmHeight;
	}

	HDC hdcMemory=::CreateCompatibleDC(hdc);
	if (hdcMemory==NULL)
		return false;
	HBITMAP hbmOld=static_cast<HBITMAP>(::SelectObject(hdcMemory,hbm));

	if (Opacity==255) {
		if (SrcWidth==DstWidth && SrcHeight==DstHeight) {
			::BitBlt(hdc,DstX,DstY,DstWidth,DstHeight,
					 hdcMemory,SrcX,SrcY,SRCCOPY);
		} else {
			int OldStretchMode=::SetStretchBltMode(hdc,STRETCH_HALFTONE);
			::StretchBlt(hdc,DstX,DstY,DstWidth,DstHeight,
						 hdcMemory,SrcX,SrcY,SrcWidth,SrcHeight,SRCCOPY);
			::SetStretchBltMode(hdc,OldStretchMode);
		}
	} else {
		BLENDFUNCTION bf={AC_SRC_OVER,0,Opacity,0};
		::AlphaBlend(hdc,DstX,DstY,DstWidth,DstHeight,
					 hdcMemory,SrcX,SrcY,SrcWidth,SrcHeight,bf);
	}

	::SelectObject(hdcMemory,hbmOld);
	::DeleteDC(hdcMemory);
	return true;
}


// 単色で画像を描画する
bool DrawMonoColorDIB(HDC hdcDst,int DstX,int DstY,
					  HDC hdcSrc,int SrcX,int SrcY,int Width,int Height,COLORREF Color)
{
	COLORREF TransColor=Color^0x00FFFFFF;
	RGBQUAD Palette[2];

	if (hdcDst==NULL || hdcSrc==NULL)
		return false;
	Palette[0].rgbBlue=GetBValue(Color);
	Palette[0].rgbGreen=GetGValue(Color);
	Palette[0].rgbRed=GetRValue(Color);
	Palette[0].rgbReserved=0;
	Palette[1].rgbBlue=GetBValue(TransColor);
	Palette[1].rgbGreen=GetGValue(TransColor);
	Palette[1].rgbRed=GetRValue(TransColor);
	Palette[1].rgbReserved=0;
	::SetDIBColorTable(hdcSrc,0,2,Palette);
	::TransparentBlt(hdcDst,DstX,DstY,Width,Height,
					 hdcSrc,SrcX,SrcY,Width,Height,TransColor);
	return true;
}


// テキストを描画する
bool DrawText(HDC hdc,LPCTSTR pszText,const RECT &Rect,UINT Format,
			  const CFont *pFont,COLORREF Color)
{
	if (hdc==NULL || pszText==NULL)
		return false;

	int OldBkMode;
	COLORREF OldTextColor;
	HFONT hfontOld;

	OldBkMode=::SetBkMode(hdc,TRANSPARENT);
	if (Color!=CLR_INVALID)
		OldTextColor=::SetTextColor(hdc,Color);
	if (pFont!=NULL)
		hfontOld=DrawUtil::SelectObject(hdc,*pFont);
	RECT rc=Rect;
	::DrawText(hdc,pszText,-1,&rc,Format);
	if (pFont!=NULL)
		::SelectObject(hdc,hfontOld);
	if (Color!=CLR_INVALID)
		::SetTextColor(hdc,OldTextColor);
	::SetBkMode(hdc,OldBkMode);
	return true;
}


// テキストを指定幅で折り返して何行になるか計算する
int CalcWrapTextLines(HDC hdc,LPCTSTR pszText,int Width)
{
	if (hdc==NULL || pszText==NULL)
		return 0;

	LPCTSTR p;
	int Length;
	int Fit;
	SIZE sz;
	int Lines=0;

	p=pszText;
	while (*p!='\0') {
		if (*p=='\r' || *p=='\n') {
			p++;
			if (*p=='\n')
				p++;
			if (*p=='\0')
				break;
			Lines++;
			continue;
		}
		for (Length=0;p[Length]!='\0' && p[Length]!='\r' && p[Length]!='\n';Length++);
		::GetTextExtentExPoint(hdc,p,Length,Width,&Fit,NULL,&sz);
		if (Fit<1)
			Fit=1;
		p+=Fit;
		Lines++;
		if (*p=='\r')
			p++;
		if (*p=='\n')
			p++;
	}
	return Lines;
}


// テキストを指定幅で折り返して描画する
bool DrawWrapText(HDC hdc,LPCTSTR pszText,const RECT *pRect,int LineHeight)
{
	if (hdc==NULL || pszText==NULL || pRect==NULL)
		return false;

	LPCTSTR p;
	int y;
	int Length;
	int Fit;
	SIZE sz;

	p=pszText;
	y=pRect->top;
	while (*p!='\0' && y<pRect->bottom) {
		if (*p=='\r' || *p=='\n') {
			p++;
			if (*p=='\n')
				p++;
			y+=LineHeight;
			continue;
		}
		for (Length=0;p[Length]!='\0' && p[Length]!='\r' && p[Length]!='\n';Length++);
		::GetTextExtentExPoint(hdc,p,Length,pRect->right-pRect->left,&Fit,NULL,&sz);
		if (Fit<1)
			Fit=1;
		::TextOut(hdc,pRect->left,y,p,Fit);
		p+=Fit;
		y+=LineHeight;
		if (*p=='\r')
			p++;
		if (*p=='\n')
			p++;
	}
	return true;
}


// システムフォントを取得する
bool GetSystemFont(FontType Type,LOGFONT *pLogFont)
{
	if (pLogFont==NULL)
		return false;
	if (Type==FONT_DEFAULT) {
		return ::GetObject(::GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),pLogFont)==sizeof(LOGFONT);
	} else {
		NONCLIENTMETRICS ncm;
		LOGFONT *plf;
#if WINVER<0x0600
		ncm.cbSize=sizeof(ncm);
#else
		ncm.cbSize=offsetof(NONCLIENTMETRICS,iPaddedBorderWidth);
#endif
		::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,ncm.cbSize,&ncm,0);
		switch (Type) {
		case FONT_MESSAGE:		plf=&ncm.lfMessageFont;		break;
		case FONT_MENU:			plf=&ncm.lfMenuFont;		break;
		case FONT_CAPTION:		plf=&ncm.lfCaptionFont;		break;
		case FONT_SMALLCAPTION:	plf=&ncm.lfSmCaptionFont;	break;
		case FONT_STATUS:		plf=&ncm.lfStatusFont;		break;
		default:
			return false;
		}
		*pLogFont=*plf;
	}
	return true;
}


CFont::CFont()
	: m_hfont(NULL)
{
}

CFont::CFont(const CFont &Font)
	: m_hfont(NULL)
{
	*this=Font;
}

CFont::CFont(const LOGFONT &Font)
	: m_hfont(NULL)
{
	Create(&Font);
}

CFont::CFont(FontType Type)
	: m_hfont(NULL)
{
	Create(Type);
}

CFont::~CFont()
{
	Destroy();
}

CFont &CFont::operator=(const CFont &Font)
{
	if (Font.m_hfont) {
		LOGFONT lf;
		Font.GetLogFont(&lf);
		Create(&lf);
	} else {
		if (m_hfont)
			::DeleteObject(m_hfont);
		m_hfont=NULL;
	}
	return *this;
}

bool CFont::operator==(const CFont &Font) const
{
	if (m_hfont==NULL)
		return Font.m_hfont==NULL;
	if (Font.m_hfont==NULL)
		return m_hfont==NULL;
	LOGFONT lf1,lf2;
	GetLogFont(&lf1);
	Font.GetLogFont(&lf2);
	return CompareLogFont(&lf1,&lf2);
}

bool CFont::operator!=(const CFont &Font) const
{
	return !(*this==Font);
}

bool CFont::Create(const LOGFONT *pLogFont)
{
	if (pLogFont==NULL)
		return false;
	HFONT hfont=::CreateFontIndirect(pLogFont);
	if (hfont==NULL)
		return false;
	if (m_hfont)
		::DeleteObject(m_hfont);
	m_hfont=hfont;
	return true;
}

bool CFont::Create(FontType Type)
{
	LOGFONT lf;

	if (!GetSystemFont(Type,&lf))
		return false;
	return Create(&lf);
}

void CFont::Destroy()
{
	if (m_hfont) {
		::DeleteObject(m_hfont);
		m_hfont=NULL;
	}
}

bool CFont::GetLogFont(LOGFONT *pLogFont) const
{
	if (m_hfont==NULL || pLogFont==NULL)
		return false;
	return ::GetObject(m_hfont,sizeof(LOGFONT),pLogFont)==sizeof(LOGFONT);
}

int CFont::GetHeight(bool fCell) const
{
	if (m_hfont==NULL)
		return 0;

	HDC hdc=::CreateDC(TEXT("DISPLAY"),NULL,NULL,NULL);
	if (hdc==NULL) {
		LOGFONT lf;
		if (!GetLogFont(&lf))
			return 0;
		return abs(lf.lfHeight);
	}
	int Height=GetHeight(hdc,fCell);
	::DeleteDC(hdc);
	return Height;
}

int CFont::GetHeight(HDC hdc,bool fCell) const
{
	if (m_hfont==NULL || hdc==NULL)
		return 0;
	HGDIOBJ hOldFont=::SelectObject(hdc,m_hfont);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc,&tm);
	::SelectObject(hdc,hOldFont);
	if (!fCell)
		tm.tmHeight-=tm.tmInternalLeading;
	return tm.tmHeight;
}


CBrush::CBrush()
	: m_hbr(NULL)
{
}

CBrush::CBrush(const CBrush &Brush)
	: m_hbr(NULL)
{
	*this=Brush;
}

CBrush::CBrush(COLORREF Color)
	: m_hbr(NULL)
{
	Create(Color);
}

CBrush::~CBrush()
{
	Destroy();
}

CBrush &CBrush::operator=(const CBrush &Brush)
{
	if (&Brush!=this) {
		Destroy();
		if (Brush.m_hbr!=NULL) {
			LOGBRUSH lb;

			if (::GetObject(Brush.m_hbr,sizeof(LOGBRUSH),&lb)==sizeof(LOGBRUSH))
				m_hbr=::CreateBrushIndirect(&lb);
		}
	}
	return *this;
}

bool CBrush::Create(COLORREF Color)
{
	HBRUSH hbr=::CreateSolidBrush(Color);

	if (hbr==NULL)
		return false;
	Destroy();
	m_hbr=hbr;
	return true;
}

void CBrush::Destroy()
{
	if (m_hbr!=NULL) {
		::DeleteObject(m_hbr);
		m_hbr=NULL;
	}
}


COffscreen::COffscreen()
	: m_hdc(NULL)
	, m_hbm(NULL)
	, m_hbmOld(NULL)
	, m_Width(0)
	, m_Height(0)
{
}

COffscreen::~COffscreen()
{
	Destroy();
}

bool COffscreen::Create(int Width,int Height,HDC hdc)
{
	if (Width<=0 || Height<=0)
		return false;
	Destroy();
	HDC hdcScreen;
	if (hdc==NULL) {
		hdcScreen=::GetDC(NULL);
		if (hdcScreen==NULL)
			return false;
		hdc=hdcScreen;
	} else {
		hdcScreen=NULL;
	}
	m_hdc=::CreateCompatibleDC(hdc);
	if (m_hdc==NULL) {
		if (hdcScreen!=NULL)
			::ReleaseDC(NULL,hdcScreen);
		return false;
	}
	m_hbm=::CreateCompatibleBitmap(hdc,Width,Height);
	if (hdcScreen!=NULL)
		::ReleaseDC(NULL,hdcScreen);
	if (m_hbm==NULL) {
		Destroy();
		return false;
	}
	m_hbmOld=static_cast<HBITMAP>(::SelectObject(m_hdc,m_hbm));
	m_Width=Width;
	m_Height=Height;
	return true;
}

void COffscreen::Destroy()
{
	if (m_hbmOld!=NULL) {
		::SelectObject(m_hdc,m_hbmOld);
		m_hbmOld=NULL;
	}
	if (m_hdc!=NULL) {
		::DeleteDC(m_hdc);
		m_hdc=NULL;
	}
	if (m_hbm!=NULL) {
		::DeleteObject(m_hbm);
		m_hbm=NULL;
		m_Width=0;
		m_Height=0;
	}
}

bool COffscreen::CopyTo(HDC hdc,const RECT *pDstRect)
{
	int DstX,DstY,Width,Height;

	if (m_hdc==NULL || hdc==NULL)
		return false;
	if (pDstRect!=NULL) {
		DstX=pDstRect->left;
		DstY=pDstRect->top;
		Width=pDstRect->right-pDstRect->left;
		Height=pDstRect->bottom-pDstRect->top;
		if (Width<=0 || Height<=0)
			return false;
		if (Width>m_Width)
			Width=m_Width;
		if (Height>m_Height)
			Height=m_Height;
	} else {
		DstX=DstY=0;
		Width=m_Width;
		Height=m_Height;
	}
	::BitBlt(hdc,DstX,DstY,Width,Height,m_hdc,0,0,SRCCOPY);
	return true;
}


CDeviceContext::CDeviceContext(HDC hdc)
	: m_Flags(0)
	, m_hdc(hdc)
	, m_hwnd(NULL)
	, m_pPaint(NULL)
{
}

CDeviceContext::CDeviceContext(HWND hwnd)
	: m_Flags(FLAG_DCFROMHWND)
	, m_hdc(::GetDC(hwnd))
	, m_hwnd(hwnd)
	, m_pPaint(NULL)
{
}

CDeviceContext::CDeviceContext(HWND hwnd,PAINTSTRUCT *pPaint)
	: m_Flags(FLAG_WMPAINT)
	, m_hdc(::BeginPaint(hwnd,pPaint))
	, m_hwnd(hwnd)
	, m_pPaint(pPaint)
{
}

CDeviceContext::~CDeviceContext()
{
	Release();
}

void CDeviceContext::Restore()
{
	if (m_hdc) {
		if ((m_Flags&FLAG_BRUSHSELECTED)!=0)
			::SelectObject(m_hdc,m_hbrOld);
		if ((m_Flags&FLAG_PENSELECTED)!=0)
			::SelectObject(m_hdc,m_hpenOld);
		if ((m_Flags&FLAG_FONTSELECTED)!=0)
			::SelectObject(m_hdc,m_hfontOld);
		if ((m_Flags&FLAG_TEXTCOLOR)!=0)
			::SetTextColor(m_hdc,m_OldTextColor);
		if ((m_Flags&FLAG_BKCOLOR)!=0)
			::SetBkColor(m_hdc,m_OldBkColor);
		if ((m_Flags&FLAG_BKMODE)!=0)
			::SetBkMode(m_hdc,m_OldBkMode);
		m_Flags&=~FLAG_RESTOREMASK;
	}
}

void CDeviceContext::Release()
{
	if (m_hdc) {
		Restore();
		if ((m_Flags&FLAG_DCFROMHWND)!=0)
			::ReleaseDC(m_hwnd/*::WindowFromDC(m_hdc)*/,m_hdc);
		else if ((m_Flags&FLAG_WMPAINT)!=0)
			::EndPaint(m_hwnd,m_pPaint);
		m_hdc=NULL;
	}
	m_Flags=0;
	m_hwnd=NULL;
	m_pPaint=NULL;
}

void CDeviceContext::SetBrush(HBRUSH hbr)
{
	if (m_hdc && hbr) {
		HBRUSH hbrOld=static_cast<HBRUSH>(::SelectObject(m_hdc,hbr));
		if ((m_Flags&FLAG_BRUSHSELECTED)==0) {
			m_hbrOld=hbrOld;
			m_Flags|=FLAG_BRUSHSELECTED;
		}
	}
}

void CDeviceContext::SetPen(HPEN hpen)
{
	if (m_hdc && hpen) {
		HPEN hpenOld=static_cast<HPEN>(::SelectObject(m_hdc,hpen));
		if ((m_Flags&FLAG_PENSELECTED)==0) {
			m_hpenOld=hpenOld;
			m_Flags|=FLAG_PENSELECTED;
		}
	}
}

void CDeviceContext::SetFont(HFONT hfont)
{
	if (m_hdc && hfont) {
		HFONT hfontOld=static_cast<HFONT>(::SelectObject(m_hdc,hfont));
		if ((m_Flags&FLAG_FONTSELECTED)==0) {
			m_hfontOld=hfontOld;
			m_Flags|=FLAG_FONTSELECTED;
		}
	}
}

void CDeviceContext::SetFont(const CFont &Font)
{
	SetFont(Font.GetHandle());
}

void CDeviceContext::SetTextColor(COLORREF Color)
{
	if (m_hdc) {
		COLORREF OldTextColor=::SetTextColor(m_hdc,Color);
		if ((m_Flags&FLAG_TEXTCOLOR)==0) {
			m_OldTextColor=OldTextColor;
			m_Flags|=FLAG_TEXTCOLOR;
		}
	}
}

void CDeviceContext::SetBkColor(COLORREF Color)
{
	if (m_hdc) {
		COLORREF OldBkColor=::SetBkColor(m_hdc,Color);
		if ((m_Flags&FLAG_BKCOLOR)==0) {
			m_OldBkColor=OldBkColor;
			m_Flags|=FLAG_BKCOLOR;
		}
	}
}

void CDeviceContext::SetBkMode(int BkMode)
{
	if (m_hdc) {
		int OldBkMode=::SetBkMode(m_hdc,BkMode);
		if ((m_Flags&FLAG_BKMODE)==0) {
			m_OldBkMode=OldBkMode;
			m_Flags|=FLAG_BKMODE;
		}
	}
}

void CDeviceContext::DrawRectangle(int Left,int Top,int Right,int Bottom,RectangleStyle Style)
{
	if (m_hdc==NULL)
		return;
	switch (Style) {
	case RECTANGLE_NORMAL:
		::Rectangle(m_hdc,Left,Top,Right,Bottom);
		break;
	case RECTANGLE_FILL:
		{
			HBRUSH hbr=static_cast<HBRUSH>(::GetCurrentObject(m_hdc,OBJ_BRUSH));
			if (hbr) {
				RECT rc;
				::SetRect(&rc,Left,Top,Right,Bottom);
				::FillRect(m_hdc,&rc,hbr);
			}
		}
		break;
	case RECTANGLE_BORDER:
		{
			HBRUSH hbrOld=static_cast<HBRUSH>(::SelectObject(m_hdc,::GetStockObject(NULL_BRUSH)));
			::Rectangle(m_hdc,Left,Top,Right,Bottom);
			::SelectObject(m_hdc,hbrOld);
		}
		break;
	}
}

void CDeviceContext::DrawRectangle(const RECT *pRect,RectangleStyle Style)
{
	if (m_hdc && pRect)
		DrawRectangle(pRect->left,pRect->top,pRect->right,pRect->bottom,Style);
}

void CDeviceContext::DrawLine(int x1,int y1,int x2,int y2)
{
	if (m_hdc) {
		::MoveToEx(m_hdc,x1,y1,NULL);
		::LineTo(m_hdc,x2,y2);
	}
}

void CDeviceContext::DrawText(LPCTSTR pszText,int Length,RECT *pRect,UINT Format)
{
	if (m_hdc)
		::DrawText(m_hdc,pszText,Length,pRect,Format);
}


}	// namespace DrawUtil




#include <gdiplus.h>


#pragma comment(lib, "gdiplus.lib")
//#pragma comment(linker, "/DELAYLOAD:gdiplus.dll")


CGdiPlus::CGdiPlus()
	: m_fInitialized(false)
{
}

CGdiPlus::~CGdiPlus()
{
	Finalize();
}

bool CGdiPlus::Initialize()
{
	if (!m_fInitialized) {
		// GDI+ の DLL がロードできるか調べる
		// (gdiplus.dllが無くても起動するように遅延ロードの指定をしている)
		HMODULE hLib=::LoadLibrary(TEXT("gdiplus.dll"));
		if (hLib==NULL)
			return false;
		::FreeLibrary(hLib);

		Gdiplus::GdiplusStartupInput si;
		si.GdiplusVersion=1;
		si.DebugEventCallback=NULL;
		si.SuppressBackgroundThread=FALSE;
		si.SuppressExternalCodecs=FALSE;
		if (Gdiplus::GdiplusStartup(&m_Token,&si,NULL)!=Gdiplus::Ok)
			return false;
		m_fInitialized=true;
	}
	return true;
}

void CGdiPlus::Finalize()
{
	if (m_fInitialized) {
		Gdiplus::GdiplusShutdown(m_Token);
		m_fInitialized=false;
	}
}

bool CGdiPlus::DrawImage(CCanvas *pCanvas,CImage *pImage,int x,int y)
{
	if (pCanvas!=NULL && pCanvas->m_pGraphics!=NULL
			 && pImage!=NULL && pImage->m_pBitmap!=NULL) {
		return pCanvas->m_pGraphics->DrawImage(pImage->m_pBitmap,x,y,
											   pImage->GetWidth(),
											   pImage->GetHeight())==Gdiplus::Ok;
	}
	return false;
}

bool CGdiPlus::DrawImage(CCanvas *pCanvas,int DstX,int DstY,int DstWidth,int DstHeight,
	CImage *pImage,int SrcX,int SrcY,int SrcWidth,int SrcHeight,float Opacity)
{
	if (pCanvas!=NULL && pCanvas->m_pGraphics!=NULL
			 && pImage!=NULL && pImage->m_pBitmap!=NULL) {
		Gdiplus::ImageAttributes Attributes;
		Gdiplus::ColorMatrix Matrix = {
			1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
 		};
		Matrix.m[3][3]=Opacity;
		Attributes.SetColorMatrix(&Matrix);
		return pCanvas->m_pGraphics->DrawImage(pImage->m_pBitmap,
			Gdiplus::Rect(DstX,DstY,DstWidth,DstHeight),
			SrcX,SrcY,SrcWidth,SrcHeight,
			Gdiplus::UnitPixel,&Attributes)==Gdiplus::Ok;
	}
	return false;
}

bool CGdiPlus::FillRect(CCanvas *pCanvas,CBrush *pBrush,const RECT *pRect)
{
	if (pCanvas!=NULL && pCanvas->m_pGraphics!=NULL
			&& pBrush!=NULL && pBrush->m_pBrush!=NULL && pRect!=NULL) {
		return pCanvas->m_pGraphics->FillRectangle(pBrush->m_pBrush,
												   pRect->left,pRect->top,
												   pRect->right-pRect->left,
												   pRect->bottom-pRect->top)==Gdiplus::Ok;
	}
	return false;
}


CGdiPlus::CImage::CImage()
	: m_pBitmap(NULL)
{
}

CGdiPlus::CImage::CImage(const CImage &Src)
	: m_pBitmap(NULL)
{
	*this=Src;
}

CGdiPlus::CImage::~CImage()
{
	Free();
}

CGdiPlus::CImage &CGdiPlus::CImage::operator=(const CImage &Src)
{
	if (&Src!=this) {
		Free();
		if (Src.m_pBitmap!=NULL)
			m_pBitmap=Src.m_pBitmap->Clone(0,0,Src.m_pBitmap->GetWidth(),Src.m_pBitmap->GetHeight(),Src.m_pBitmap->GetPixelFormat());
	}
	return *this;
}

void CGdiPlus::CImage::Free()
{
	if (m_pBitmap!=NULL) {
		delete m_pBitmap;
		m_pBitmap=NULL;
	}
}

bool CGdiPlus::CImage::LoadFromFile(LPCWSTR pszFileName)
{
	Free();
	m_pBitmap=Gdiplus::Bitmap::FromFile(pszFileName);
	return m_pBitmap!=NULL;
}

bool CGdiPlus::CImage::LoadFromResource(HINSTANCE hinst,LPCWSTR pszName)
{
	Free();
	m_pBitmap=Gdiplus::Bitmap::FromResource(hinst,pszName);
	return m_pBitmap!=NULL;
}

bool CGdiPlus::CImage::LoadFromResource(HINSTANCE hinst,LPCTSTR pszName,LPCTSTR pszType)
{
	Free();

	HRSRC hRes=::FindResource(hinst,pszName,pszType);
	if (hRes==NULL)
		return false;
	DWORD Size=::SizeofResource(hinst,hRes);
	if (Size==0)
		return false;
	HGLOBAL hData=::LoadResource(hinst,hRes);
	const void *pData=::LockResource(hData);
	if (pData==NULL)
		return false;
	HGLOBAL hBuffer=::GlobalAlloc(GMEM_MOVEABLE,Size);
	if (hBuffer==NULL)
		return false;
	void *pBuffer=::GlobalLock(hBuffer);
	if (pBuffer==NULL) {
		::GlobalFree(hBuffer);
		return false;
	}
	::CopyMemory(pBuffer,pData,Size);
	::GlobalUnlock(hBuffer);
	IStream *pStream;
	if (::CreateStreamOnHGlobal(hBuffer,TRUE,&pStream)!=S_OK) {
		::GlobalFree(hBuffer);
		return false;
	}
	m_pBitmap=Gdiplus::Bitmap::FromStream(pStream);
	pStream->Release();
	return m_pBitmap!=NULL;
}

bool CGdiPlus::CImage::Create(int Width,int Height,int BitsPerPixel)
{
	Free();
	if (Width<=0 || Height<=0)
		return false;
	Gdiplus::PixelFormat Format;
	switch (BitsPerPixel) {
	case 1:		Format=PixelFormat1bppIndexed;	break;
	case 4:		Format=PixelFormat4bppIndexed;	break;
	case 8:		Format=PixelFormat8bppIndexed;	break;
	case 24:	Format=PixelFormat24bppRGB;	break;
	case 32:	Format=PixelFormat32bppARGB;	break;
	default:	return false;
	}
	m_pBitmap=new Gdiplus::Bitmap(Width,Height,Format);
	if (m_pBitmap==NULL)
		return false;
	Clear();
	return true;
}

bool CGdiPlus::CImage::CreateFromBitmap(HBITMAP hbm,HPALETTE hpal)
{
	Free();
	m_pBitmap=Gdiplus::Bitmap::FromHBITMAP(hbm,hpal);
	return m_pBitmap!=NULL;
}

bool CGdiPlus::CImage::CreateFromDIB(const BITMAPINFO *pbmi,const void *pBits)
{
	Free();
	m_pBitmap=new Gdiplus::Bitmap(pbmi,const_cast<void*>(pBits));
	return m_pBitmap!=NULL;
}

bool CGdiPlus::CImage::IsCreated() const
{
	return m_pBitmap!=NULL;
}

int CGdiPlus::CImage::GetWidth() const
{
	if (m_pBitmap==NULL)
		return 0;
	return m_pBitmap->GetWidth();
}

int CGdiPlus::CImage::GetHeight() const
{
	if (m_pBitmap==NULL)
		return 0;
	return m_pBitmap->GetHeight();
}

void CGdiPlus::CImage::Clear()
{
	if (m_pBitmap!=NULL) {
		Gdiplus::Rect rc(0,0,m_pBitmap->GetWidth(),m_pBitmap->GetHeight());
		Gdiplus::BitmapData Data;

		if (m_pBitmap->LockBits(&rc,Gdiplus::ImageLockModeWrite,
								m_pBitmap->GetPixelFormat(),&Data)==Gdiplus::Ok) {
			BYTE *pBits=static_cast<BYTE*>(Data.Scan0);
			for (UINT y=0;y<Data.Height;y++) {
				::ZeroMemory(pBits,abs(Data.Stride));
				pBits+=Data.Stride;
			}
			m_pBitmap->UnlockBits(&Data);
		}
	}
}


CGdiPlus::CBrush::CBrush()
	: m_pBrush(NULL)
{
}

CGdiPlus::CBrush::CBrush(BYTE r,BYTE g,BYTE b,BYTE a)
{
	m_pBrush=new Gdiplus::SolidBrush(Gdiplus::Color(a,r,g,b));
}

CGdiPlus::CBrush::CBrush(COLORREF Color)
{
	m_pBrush=new Gdiplus::SolidBrush(Gdiplus::Color(255,GetRValue(Color),GetGValue(Color),GetBValue(Color)));
}

CGdiPlus::CBrush::~CBrush()
{
	Free();
}

void CGdiPlus::CBrush::Free()
{
	if (m_pBrush!=NULL) {
		delete m_pBrush;
		m_pBrush=NULL;
	}
}

bool CGdiPlus::CBrush::CreateSolidBrush(BYTE r,BYTE g,BYTE b,BYTE a)
{
	Gdiplus::Color Color(a,r,g,b);

	if (m_pBrush!=NULL) {
		m_pBrush->SetColor(Color);
	} else {
		m_pBrush=new Gdiplus::SolidBrush(Color);
		if (m_pBrush==NULL)
			return false;
	}
	return true;
}


CGdiPlus::CCanvas::CCanvas(HDC hdc)
{
	m_pGraphics=new Gdiplus::Graphics(hdc);
}

CGdiPlus::CCanvas::CCanvas(CImage *pImage)
	: m_pGraphics(NULL)
{
	if (pImage!=NULL)
		m_pGraphics=new Gdiplus::Graphics(pImage->m_pBitmap);
}

CGdiPlus::CCanvas::~CCanvas()
{
	if (m_pGraphics!=NULL)
		delete m_pGraphics;
}

bool CGdiPlus::CCanvas::Clear(BYTE r,BYTE g,BYTE b,BYTE a)
{
	if (m_pGraphics==NULL)
		return false;
	return m_pGraphics->Clear(Gdiplus::Color(a,r,g,b))==Gdiplus::Ok;
}




#pragma comment(lib, "uxtheme.lib")
//#pragma comment(linker, "/DELAYLOAD:uxtheme.dll")


CUxTheme::CUxTheme()
	: m_hLib(NULL)
	, m_hTheme(NULL)
{
}

CUxTheme::~CUxTheme()
{
	Close();
	if (m_hLib!=NULL)
		::FreeLibrary(m_hLib);
}

bool CUxTheme::Initialize()
{
	if (m_hLib==NULL) {
		// uxtheme.dll がロードできるか調べる
		// (uxtheme.dllが無くても起動するように遅延ロードの指定をしている)
		m_hLib=::LoadLibrary(TEXT("uxtheme.dll"));
		if (m_hLib==NULL)
			return false;
	}
	return true;
}

bool CUxTheme::Open(HWND hwnd,LPCWSTR pszClassList)
{
	Close();
	if (!Initialize())
		return false;
	m_hTheme=::OpenThemeData(hwnd,pszClassList);
	if (m_hTheme==NULL)
		return false;
	return true;
}

void CUxTheme::Close()
{
	if (m_hTheme!=NULL) {
		::CloseThemeData(m_hTheme);
		m_hTheme=NULL;
	}
}

bool CUxTheme::IsOpen() const
{
	return m_hTheme!=NULL;
}

bool CUxTheme::IsActive()
{
	if (m_hLib==NULL)
		return false;
	return ::IsThemeActive()!=FALSE;
}

bool CUxTheme::DrawBackground(HDC hdc,int PartID,int StateID,const RECT *pRect)
{
	if (m_hTheme==NULL)
		return false;
	return ::DrawThemeBackground(m_hTheme,hdc,PartID,StateID,pRect,NULL)==S_OK;
}

bool CUxTheme::DrawBackground(HDC hdc,int PartID,int StateID,
							  int BackgroundPartID,int BackgroundStateID,
							  const RECT *pRect)
{
	if (m_hTheme==NULL)
		return false;
	if (::IsThemeBackgroundPartiallyTransparent(m_hTheme,PartID,StateID)) {
		if (::DrawThemeBackground(m_hTheme,hdc,
								  BackgroundPartID,BackgroundStateID,
								  pRect,NULL)!=S_OK)
			return false;
	}
	return ::DrawThemeBackground(m_hTheme,hdc,PartID,StateID,pRect,NULL)==S_OK;
}

bool CUxTheme::DrawText(HDC hdc,int PartID,int StateID,LPCWSTR pszText,
						DWORD TextFlags,const RECT *pRect)
{
	if (m_hTheme==NULL)
		return false;
	return ::DrawThemeText(m_hTheme,hdc,PartID,StateID,pszText,lstrlenW(pszText),
						   TextFlags,0,pRect)==S_OK;
}

bool CUxTheme::GetTextExtent(HDC hdc,int PartID,int StateID,LPCWSTR pszText,
							 DWORD TextFlags,RECT *pExtentRect)
{
	if (m_hTheme==NULL)
		return false;
	return ::GetThemeTextExtent(m_hTheme,hdc,PartID,StateID,
								pszText,lstrlenW(pszText),TextFlags,
								NULL,pExtentRect)==S_OK;
}

bool CUxTheme::GetMargins(int PartID,int StateID,int PropID,MARGINS *pMargins)
{
	if (m_hTheme==NULL)
		return false;
	return ::GetThemeMargins(m_hTheme,NULL,PartID,StateID,PropID,NULL,pMargins)==S_OK;
}

bool CUxTheme::GetColor(int PartID,int StateID,int PropID,COLORREF *pColor)
{
	if (m_hTheme==NULL)
		return false;
	return ::GetThemeColor(m_hTheme,PartID,StateID,PropID,pColor)==S_OK;
}

bool CUxTheme::GetFont(int PartID,int StateID,int PropID,LOGFONT *pFont)
{
	if (m_hTheme==NULL)
		return false;
	return ::GetThemeFont(m_hTheme,NULL,PartID,StateID,PropID,pFont)==S_OK;
}

bool CUxTheme::GetInt(int PartID,int StateID,int PropID,int *pValue)
{
	if (m_hTheme==NULL)
		return false;
	return ::GetThemeInt(m_hTheme,PartID,StateID,PropID,pValue)==S_OK;
}
