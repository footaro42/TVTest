#include "stdafx.h"
#include <commctrl.h>
#include "TVTest.h"
#include "ColorPalette.h"


#define PALETTE_WINDOW_CLASS APP_NAME TEXT(" Color Palette")




HINSTANCE CColorPalette::m_hinst=NULL;


bool CColorPalette::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=(HBRUSH)(COLOR_3DFACE+1);
		wc.lpszMenuName=NULL;
		wc.lpszClassName=PALETTE_WINDOW_CLASS;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CColorPalette::CColorPalette()
{
	m_NumColors=0;
	m_pPalette=NULL;
}


CColorPalette::~CColorPalette()
{
	delete [] m_pPalette;
}


bool CColorPalette::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 PALETTE_WINDOW_CLASS,NULL,m_hinst);
}


bool CColorPalette::GetPalette(RGBQUAD *pPalette)
{
	if (m_pPalette==NULL)
		return false;
	CopyMemory(pPalette,m_pPalette,m_NumColors*sizeof(RGBQUAD));
	return true;
}


bool CColorPalette::SetPalette(const RGBQUAD *pPalette,int NumColors)
{
	if (NumColors<1 || NumColors>256)
		return false;
	if (NumColors!=m_NumColors) {
		delete [] m_pPalette;
		m_pPalette=new RGBQUAD[NumColors];
		m_NumColors=NumColors;
	}
	CopyMemory(m_pPalette,pPalette,NumColors*sizeof(RGBQUAD));
	m_SelColor=-1;
	m_HotColor=-1;
	InvalidateRect(m_hwnd,NULL,TRUE);
	SetToolTip();
	return true;
}


COLORREF CColorPalette::GetColor(int Index) const
{
	if (Index<0 || Index>=m_NumColors)
		return CLR_INVALID;
	return RGB(m_pPalette[Index].rgbRed,m_pPalette[Index].rgbGreen,
			   m_pPalette[Index].rgbBlue);
}


bool CColorPalette::SetColor(int Index,COLORREF Color)
{
	RECT rc;

	if (Index<0 || Index>=m_NumColors)
		return false;
	m_pPalette[Index].rgbBlue=GetBValue(Color);
	m_pPalette[Index].rgbGreen=GetGValue(Color);
	m_pPalette[Index].rgbRed=GetRValue(Color);
	GetItemRect(Index,&rc);
	InvalidateRect(m_hwnd,&rc,TRUE);
	return true;
}


int CColorPalette::GetSel() const
{
	return m_SelColor;
}


bool CColorPalette::SetSel(int Sel)
{
	if (m_pPalette==NULL)
		return false;
	if (Sel<0 || Sel>=m_NumColors)
		Sel=-1;
	if (Sel!=m_SelColor) {
		DrawNewSelHighlight(m_SelColor,Sel);
		m_SelColor=Sel;
	}
	return true;
}


int CColorPalette::GetHot() const
{
	return m_HotColor;
}


int CColorPalette::FindColor(COLORREF Color) const
{
	int i;

	for (i=0;i<m_NumColors;i++) {
		if (RGB(m_pPalette[i].rgbRed,m_pPalette[i].rgbGreen,m_pPalette[i].rgbBlue)==Color)
			return i;
	}
	return -1;
}


void CColorPalette::GetItemRect(int Index,RECT *pRect) const
{
	int x,y;

	x=m_Left+Index%16*m_ItemWidth;
	y=m_Top+Index/16*m_ItemHeight;
	pRect->left=x;
	pRect->top=y;
	pRect->right=x+m_ItemWidth;
	pRect->bottom=y+m_ItemHeight;
}


void CColorPalette::DrawSelRect(HDC hdc,int Sel,bool fSel)
{
	HPEN hpen,hpenOld;
	HBRUSH hbrOld;
	RECT rc;

	hpen=CreatePen(PS_SOLID,1,GetSysColor(fSel?COLOR_HIGHLIGHT:COLOR_3DFACE));
	hpenOld=SelectPen(hdc,hpen);
	hbrOld=SelectBrush(hdc,GetStockObject(NULL_BRUSH));
	GetItemRect(Sel,&rc);
	Rectangle(hdc,rc.left,rc.top,rc.right,rc.bottom);
	SelectPen(hdc,hpenOld);
	SelectBrush(hdc,hbrOld);
	DeleteObject(hpen);
}


void CColorPalette::DrawNewSelHighlight(int OldSel,int NewSel)
{
	HDC hdc;

	hdc=GetDC(m_hwnd);
	if (OldSel>=0)
		DrawSelRect(hdc,OldSel,false);
	if (NewSel>=0)
		DrawSelRect(hdc,NewSel,true);
	ReleaseDC(m_hwnd,hdc);
}


void CColorPalette::SetToolTip()
{
	int NumTools,i;
	TOOLINFO ti;

	if (m_hwndToolTip==NULL)
		return;
	NumTools=(int)::SendMessage(m_hwndToolTip,TTM_GETTOOLCOUNT,0,0);
	ti.cbSize=TTTOOLINFO_V1_SIZE;
	ti.uFlags=0;
	ti.hwnd=m_hwnd;
	if (NumTools>m_NumColors) {
		do {
			ti.uId=--NumTools;
			::SendMessage(m_hwndToolTip,TTM_DELTOOL,0,(LPARAM)&ti);
		} while (NumTools>m_NumColors);
	}
	if (NumTools>0) {
		for (i=0;i<NumTools;i++) {
			ti.uId=i;
			GetItemRect(i,&ti.rect);
			::SendMessage(m_hwndToolTip,TTM_NEWTOOLRECT,0,(LPARAM)&ti);
		}
	}
	if (NumTools<m_NumColors) {
		ti.uFlags=TTF_SUBCLASS;
		ti.hinst=NULL;
		ti.lpszText=LPSTR_TEXTCALLBACK;
		for (i=NumTools;i<m_NumColors;i++) {
			ti.uId=i;
			GetItemRect(i,&ti.rect);
			::SendMessage(m_hwndToolTip,TTM_ADDTOOL,0,(LPARAM)&ti);
		}
	}
}


void CColorPalette::SendNotify(int Code)
{
	::SendMessage(GetParent(),WM_COMMAND,
				MAKEWPARAM(GetWindowLong(m_hwnd,GWL_ID),Code),(LPARAM)m_hwnd);
}


CColorPalette *CColorPalette::GetThis(HWND hwnd)
{
	return reinterpret_cast<CColorPalette*>(GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CColorPalette::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CColorPalette *pThis=dynamic_cast<CColorPalette*>(OnCreate(hwnd,lParam));

			pThis->m_SelColor=-1;
			pThis->m_HotColor=-1;
			pThis->m_hwndToolTip=CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS,NULL,
						WS_POPUP | TTS_ALWAYSTIP,0,0,0,0,hwnd,NULL,m_hinst,NULL);
		}
		return 0;

	case WM_SIZE:
		{
			CColorPalette *pThis=GetThis(hwnd);
			int sx=LOWORD(lParam),sy=HIWORD(lParam);

			pThis->m_ItemWidth=max(sx/16,6);
			pThis->m_ItemHeight=max(sy/16,6);
			pThis->m_Left=(sx-pThis->m_ItemWidth*16)/2;
			pThis->m_Top=(sy-pThis->m_ItemHeight*16)/2;
			if (pThis->m_hwndToolTip!=NULL && pThis->m_pPalette!=NULL)
				pThis->SetToolTip();
		}
		return 0;

	case WM_PAINT:
		{
			CColorPalette *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;
			int x,y;
			int i;
			HBRUSH hbr;
			RECT rc;

			if (pThis->m_pPalette==NULL)
				break;
			BeginPaint(hwnd,&ps);
			for (i=0;i<pThis->m_NumColors;i++) {
				x=i%16;
				y=i/16;
				rc.left=pThis->m_Left+x*pThis->m_ItemWidth+2;
				rc.top=pThis->m_Top+y*pThis->m_ItemHeight+2;
				rc.right=rc.left+pThis->m_ItemWidth-4;
				rc.bottom=rc.top+pThis->m_ItemHeight-4;
				if (rc.left<ps.rcPaint.right && rc.top<ps.rcPaint.bottom
						&& rc.right>ps.rcPaint.left && rc.bottom>ps.rcPaint.top) {
					hbr=CreateSolidBrush(RGB(pThis->m_pPalette[i].rgbRed,
											 pThis->m_pPalette[i].rgbGreen,
											 pThis->m_pPalette[i].rgbBlue));
					FillRect(ps.hdc,&rc,hbr);
					DeleteObject(hbr);
				}
			}
			if (pThis->m_SelColor>=0)
				pThis->DrawSelRect(ps.hdc,pThis->m_SelColor,true);
			EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			CColorPalette *pThis=GetThis(hwnd);
			POINT ptCursor;
			int Hot;

			if (pThis->m_pPalette==NULL)
				return 0;
			ptCursor.x=GET_X_LPARAM(lParam);
			ptCursor.y=GET_Y_LPARAM(lParam);
			Hot=(ptCursor.y-pThis->m_Top)/pThis->m_ItemHeight*16+
				(ptCursor.x-pThis->m_Left)/pThis->m_ItemWidth;
			if (ptCursor.x<pThis->m_Left
					|| ptCursor.x>=pThis->m_Left+pThis->m_ItemWidth*16
					|| ptCursor.y<pThis->m_Top
					|| ptCursor.y>=pThis->m_Top+pThis->m_ItemHeight*16
					|| Hot>=pThis->m_NumColors)
				Hot=-1;
			if (Hot==pThis->m_HotColor)
				return 0;
			pThis->m_HotColor=Hot;
			pThis->SendNotify(NOTIFY_HOTCHANGE);
		}
		return 0;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		{
			CColorPalette *pThis=GetThis(hwnd);
			POINT ptCursor;
			int Sel;

			if (pThis->m_pPalette==NULL)
				return 0;
			ptCursor.x=GET_X_LPARAM(lParam);
			ptCursor.y=GET_Y_LPARAM(lParam);
			Sel=(ptCursor.y-pThis->m_Top)/pThis->m_ItemHeight*16+
				(ptCursor.x-pThis->m_Left)/pThis->m_ItemWidth;
			if (ptCursor.x<pThis->m_Left
					|| ptCursor.x>=pThis->m_Left+pThis->m_ItemWidth*16
					|| ptCursor.y<pThis->m_Top
					|| ptCursor.y>=pThis->m_Top+pThis->m_ItemHeight*16
					|| Sel>=pThis->m_NumColors || Sel==pThis->m_SelColor)
				return 0;
			pThis->DrawNewSelHighlight(pThis->m_SelColor,Sel);
			pThis->m_SelColor=Sel;
			pThis->SendNotify(NOTIFY_SELCHANGE);
			if (uMsg==WM_RBUTTONDOWN)
				pThis->SendNotify(NOTIFY_RBUTTONDOWN);
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		{
			CColorPalette *pThis=GetThis(hwnd);

			if (pThis->m_SelColor>=0)
				pThis->SendNotify(NOTIFY_DOUBLECLICK);
		}
		return 0;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case TTN_NEEDTEXT:
			{
				CColorPalette *pThis=GetThis(hwnd);
				LPNMTTDISPINFO pttdi=reinterpret_cast<LPNMTTDISPINFO>(lParam);
				int Index=(int)pttdi->hdr.idFrom;
				int r,g,b;

				pttdi->lpszText=pttdi->szText;
				pttdi->hinst=NULL;
				r=pThis->m_pPalette[Index].rgbRed;
				g=pThis->m_pPalette[Index].rgbGreen;
				b=pThis->m_pPalette[Index].rgbBlue;
				wsprintf(pttdi->szText,TEXT("%d,%d,%d #%02X%02X%02X"),r,g,b,r,g,b);
			}
			return 0;
		}
		break;

	case WM_DESTROY:
		{
			CColorPalette *pThis=GetThis(hwnd);

			pThis->OnDestroy();
		}
		return 0;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}
