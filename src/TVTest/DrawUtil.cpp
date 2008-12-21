#include "stdafx.h"
#include "DrawUtil.h"




bool DrawUtil::Fill(HDC hdc,const RECT *pRect,COLORREF Color)
{
	HBRUSH hbr=::CreateSolidBrush(Color);

	if (hbr==NULL)
		return false;
	::FillRect(hdc,pRect,hbr);
	::DeleteObject(hbr);
	return true;
}


bool DrawUtil::FillGradient(HDC hdc,const RECT *pRect,COLORREF Color1,COLORREF Color2,FillDirection Direction)
{
	HPEN hpenOld,hpenCur,hpen;
	int i,Max;
	COLORREF cr,crPrev;

	if (pRect->left>pRect->right || pRect->top>pRect->bottom)
		return false;
	hpenOld=static_cast<HPEN>(::GetCurrentObject(hdc,OBJ_PEN));
	hpenCur=NULL;
	crPrev=CLR_INVALID;
	if (Direction==DIRECTION_HORZ)
		Max=pRect->right-pRect->left-1;
	else
		Max=pRect->bottom-pRect->top-1;
	for (i=0;i<=Max;i++) {
		cr=RGB((GetRValue(Color1)*(Max-i)+GetRValue(Color2)*i)/Max,
			   (GetGValue(Color1)*(Max-i)+GetGValue(Color2)*i)/Max,
			   (GetBValue(Color1)*(Max-i)+GetBValue(Color2)*i)/Max);
		if (cr!=crPrev) {
			hpen=::CreatePen(PS_SOLID,1,cr);
			if (hpen) {
				::SelectObject(hdc,hpen);
				if (hpenCur)
					::DeleteObject(hpenCur);
				hpenCur=hpen;
			}
			crPrev=cr;
		}
		if (Direction==DIRECTION_HORZ) {
			::MoveToEx(hdc,pRect->left+i,pRect->top,NULL);
			::LineTo(hdc,pRect->left+i,pRect->bottom);
		} else {
			::MoveToEx(hdc,pRect->left,pRect->top+i,NULL);
			::LineTo(hdc,pRect->right,pRect->top+i);
		}
	}
	if (hpenCur) {
		::SelectObject(hdc,hpenOld);
		::DeleteObject(hpenCur);
	}
	return true;
}


bool DrawUtil::FillBorder(HDC hdc,const RECT *pBorderRect,const RECT *pEmptyRect,const RECT *pPaintRect,HBRUSH hbr)
{
	RECT rc;

	if (pPaintRect->left<pEmptyRect->right && pPaintRect->right>pEmptyRect->left) {
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
