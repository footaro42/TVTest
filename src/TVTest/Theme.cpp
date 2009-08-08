#include "stdafx.h"
#include "Theme.h"
#include "DrawUtil.h"




bool Theme::FillGradient(HDC hdc,const RECT *pRect,const GradientInfo *pInfo)
{
	if (hdc==NULL || pRect==NULL || pInfo==NULL)
		return false;

	bool fResult;

	switch (pInfo->Type) {
	case GRADIENT_NORMAL:
		fResult=DrawUtil::FillGradient(hdc,pRect,pInfo->Color1,pInfo->Color2,
									   (DrawUtil::FillDirection)pInfo->Direction);
		break;
	case GRADIENT_GLOSSY:
		fResult=DrawUtil::FillGlossyGradient(hdc,pRect,pInfo->Color1,pInfo->Color2,
											 (DrawUtil::FillDirection)pInfo->Direction);
		break;
	default:
		fResult=false;
	}
	return fResult;
}


bool Theme::DrawBorder(HDC hdc,const RECT *pRect,BorderType Type)
{
	if (hdc==NULL || pRect==NULL)
		return false;

	RECT rc=*pRect;

	switch (Type) {
	case BORDER_FLAT:
		{
			HPEN hpen,hpenOld;
			HBRUSH hbrOld;

			hpen=::CreatePen(PS_SOLID,1,::GetSysColor(COLOR_3DFACE));
			hpenOld=static_cast<HPEN>(::SelectObject(hdc,hpen));
			hbrOld=static_cast<HBRUSH>(::SelectObject(hdc,::GetStockObject(NULL_BRUSH)));
			::Rectangle(hdc,rc.left,rc.top,rc.right,rc.bottom);
			::SelectObject(hdc,hpenOld);
			::SelectObject(hdc,hbrOld);
		}
		break;
	case BORDER_SUNKEN:
		::DrawEdge(hdc,&rc,BDR_SUNKENINNER,BF_RECT);
		break;
	case BORDER_RAISED:
		::DrawEdge(hdc,&rc,BDR_RAISEDOUTER,BF_RECT);
		break;
	default:
		return false;
	}
	return true;
}
