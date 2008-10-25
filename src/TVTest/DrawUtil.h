#ifndef DRAW_UTIL_H
#define DRAW_UTIL_H


namespace DrawUtil {

enum FillDirection {
	DIRECTION_HORZ,
	DIRECTION_VERT
};

bool Fill(HDC hdc,const RECT *pRect,COLORREF Color);
bool FillGradient(HDC hdc,const RECT *pRect,COLORREF Color1,COLORREF Color2,FillDirection Direction=DIRECTION_HORZ);
bool FillBorder(HDC hdc,const RECT *pBorderRect,const RECT *pEmptyRect,const RECT *pPaintRect,HBRUSH hbr);

}


#endif
