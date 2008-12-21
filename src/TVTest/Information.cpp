#include "stdafx.h"
#include <shlwapi.h>
#include "TVTest.h"
#include "AppMain.h"
#include "Information.h"
#include "resource.h"


#define INFORMATION_WINDOW_CLASS APP_NAME TEXT(" Information")
#define IDC_PROGRAMINFO		1000
#define IDC_PROGRAMINFOPREV	1001
#define IDC_PROGRAMINFONEXT	1002

#define PROGRAMINFO_BUTTON_SIZE	16

enum {
	INFO_ITEM_VIDEO,
	INFO_ITEM_DECODER,
	INFO_ITEM_BITRATE,
	INFO_ITEM_ERROR,
	INFO_ITEM_RECORD,
	INFO_ITEM_PROGRAMINFO
};

#define INFO_ITEM_SIGNALLEVEL INFO_ITEM_BITRATE




HINSTANCE CInformation::m_hinst=NULL;


bool CInformation::Initialize(HINSTANCE hinst)
{
	WNDCLASS wc;

	m_hinst=hinst;
	wc.style=CS_HREDRAW;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=hinst;
	wc.hIcon=NULL;
	wc.hCursor=LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground=NULL;
	wc.lpszMenuName=NULL;
	wc.lpszClassName=INFORMATION_WINDOW_CLASS;
	return RegisterClass(&wc)!=0;
}


CInformation::CInformation()
{
	m_crBackColor=RGB(0,0,0);
	m_crTextColor=RGB(255,255,255);
	m_crProgramInfoBackColor=RGB(0,0,0);
	m_crProgramInfoTextColor=RGB(255,255,255);
	m_hbrBack=NULL;
	m_hbrProgramInfoBack=NULL;
	LOGFONT lf;
	GetObject(GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),&lf);
	m_hFont=CreateFontIndirect(&lf);
	m_LineMargin=1;
	m_VideoWidth=0;
	m_VideoHeight=0;
	m_AspectX=0;
	m_AspectY=0;
	m_pszDecoderName=NULL;
	m_fSignalLevel=false;
	m_SignalLevel=0.0f;
	m_fBitRate=false;
	m_BitRate=0.0f;
	m_fRecording=false;
	m_pszProgramInfo=NULL;
	m_fNextProgramInfo=false;
}


CInformation::~CInformation()
{
	if (m_hwnd!=NULL)
		DestroyWindow(m_hwnd);
	DeleteObject(m_hFont);
	delete [] m_pszDecoderName;
	delete [] m_pszProgramInfo;
}


bool CInformation::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 INFORMATION_WINDOW_CLASS,TEXT("èÓïÒ"),m_hinst);
}


void CInformation::Reset()
{
	m_VideoWidth=0;
	m_VideoHeight=0;
	m_AspectX=0;
	m_AspectY=0;
	/*
	if (m_pszDecoderName!=NULL) {
		delete [] m_pszDecoderName;
		m_pszDecoderName=NULL;
	}
	*/
	m_BitRate=0.0;
	m_fRecording=false;
	if (m_pszProgramInfo!=NULL) {
		delete [] m_pszProgramInfo;
		m_pszProgramInfo=NULL;
	}
	m_fNextProgramInfo=false;
	if (m_hwnd!=NULL) {
		InvalidateRect(m_hwnd,NULL,TRUE);
		SetWindowText(m_hwndProgramInfo,TEXT(""));
	}
}


bool CInformation::IsVisible() const
{
	return m_hwnd!=NULL;
}


void CInformation::SetColor(COLORREF crBackColor,COLORREF crTextColor)
{
	m_crBackColor=crBackColor;
	m_crTextColor=crTextColor;
	if (m_hwnd!=NULL) {
		DeleteObject(m_hbrBack);
		m_hbrBack=CreateSolidBrush(crBackColor);
		InvalidateRect(m_hwnd,NULL,TRUE);
		InvalidateRect(m_hwndProgramInfoPrev,NULL,TRUE);
		InvalidateRect(m_hwndProgramInfoNext,NULL,TRUE);
	}
}


void CInformation::SetProgramInfoColor(COLORREF crBackColor,COLORREF crTextColor)
{
	m_crProgramInfoBackColor=crBackColor;
	m_crProgramInfoTextColor=crTextColor;
	if (m_hwnd!=NULL) {
		DeleteObject(m_hbrProgramInfoBack);
		m_hbrProgramInfoBack=CreateSolidBrush(crBackColor);
		InvalidateRect(m_hwndProgramInfo,NULL,TRUE);
	}
}


void CInformation::SetVideoSize(int Width,int Height)
{
	if (Width!=m_VideoWidth || Height!=m_VideoHeight) {
		m_VideoWidth=Width;
		m_VideoHeight=Height;
		UpdateItem(INFO_ITEM_VIDEO);
	}
}


void CInformation::SetAspectRatio(int AspectX,int AspectY)
{
	if (AspectX!=m_AspectX || AspectY!=m_AspectY) {
		m_AspectX=AspectX;
		m_AspectY=AspectY;
		UpdateItem(INFO_ITEM_VIDEO);
	}
}


void CInformation::SetDecoderName(LPCTSTR pszName)
{
	if (m_pszDecoderName==NULL || lstrcmp(pszName,m_pszDecoderName)!=0) {
		delete [] m_pszDecoderName;
		m_pszDecoderName=new TCHAR[lstrlen(pszName)+1];
		lstrcpy(m_pszDecoderName,pszName);
		UpdateItem(INFO_ITEM_DECODER);
	}
}


void CInformation::SetSignalLevel(float Level)
{
	if (!m_fSignalLevel || Level!=m_SignalLevel) {
		m_fSignalLevel=true;
		m_SignalLevel=Level;
		UpdateItem(INFO_ITEM_SIGNALLEVEL);
	}
}


void CInformation::ShowSignalLevel(bool fShow)
{
	if (fShow!=m_fSignalLevel) {
		m_fSignalLevel=fShow;
		UpdateItem(INFO_ITEM_SIGNALLEVEL);
	}
}


void CInformation::SetBitRate(float BitRate)
{
	if (!m_fBitRate || BitRate!=m_BitRate) {
		m_fBitRate=true;
		m_BitRate=BitRate;
		UpdateItem(INFO_ITEM_BITRATE);
	}
}


void CInformation::UpdateErrorCount()
{
	UpdateItem(INFO_ITEM_ERROR);
}


void CInformation::SetRecordStatus(bool fRecording,LPCTSTR pszFileName,
								ULONGLONG WroteSize,unsigned int RecordTime)
{
	m_fRecording=fRecording;
	if (fRecording) {
		m_RecordWroteSize=WroteSize;
		m_RecordTime=RecordTime;
		TCHAR szPath[MAX_PATH];
		lstrcpy(szPath,pszFileName);
		*PathFindFileName(szPath)='\0';
		if (!GetDiskFreeSpaceEx(szPath,&m_DiskFreeSpace,NULL,NULL))
			m_DiskFreeSpace.QuadPart=0;
	}
	UpdateItem(INFO_ITEM_RECORD);
}


void CInformation::SetProgramInfo(LPCTSTR pszInfo)
{
	if (m_pszProgramInfo==NULL || lstrcmp(m_pszProgramInfo,pszInfo)!=0) {
		delete [] m_pszProgramInfo;
		m_pszProgramInfo=new TCHAR[lstrlen(pszInfo)+1];
		lstrcpy(m_pszProgramInfo,pszInfo);
		SetWindowText(m_hwndProgramInfo,m_pszProgramInfo);
	}
}


CInformation *CInformation::GetThis(HWND hwnd)
{
	return reinterpret_cast<CInformation*>(GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CInformation::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,
																LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CInformation *pThis=static_cast<CInformation*>(
					reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);

			SetWindowLongPtr(hwnd,GWLP_USERDATA,
											reinterpret_cast<LONG_PTR>(pThis));
			pThis->m_hwnd=hwnd;

			HDC hdc;
			HFONT hfontOld;
			TEXTMETRIC tm;

			hdc=GetDC(hwnd);
			hfontOld=static_cast<HFONT>(SelectObject(hdc,pThis->m_hFont));
			GetTextMetrics(hdc,&tm);
			pThis->m_FontHeight=tm.tmHeight;
			SelectObject(hdc,hfontOld);
			ReleaseDC(hwnd,hdc);
			pThis->m_hwndProgramInfo=CreateWindowEx(0,TEXT("EDIT"),
				pThis->m_pszProgramInfo!=NULL?pThis->m_pszProgramInfo:TEXT(""),
				WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_VSCROLL | ES_MULTILINE | ES_READONLY,
				0,0,0,0,hwnd,(HMENU)IDC_PROGRAMINFO,m_hinst,NULL);
			SetWindowFont(pThis->m_hwndProgramInfo,pThis->m_hFont,FALSE);
			pThis->m_hwndProgramInfoPrev=CreateWindowEx(0,TEXT("BUTTON"),TEXT(""),
				WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | (pThis->m_fNextProgramInfo?0:WS_DISABLED)
												| BS_PUSHBUTTON | BS_OWNERDRAW,
				0,0,0,0,hwnd,(HMENU)IDC_PROGRAMINFOPREV,m_hinst,NULL);
			pThis->m_hwndProgramInfoNext=CreateWindowEx(0,TEXT("BUTTON"),TEXT(""),
				WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | (pThis->m_fNextProgramInfo?WS_DISABLED:0)
												| BS_PUSHBUTTON | BS_OWNERDRAW,
				0,0,0,0,hwnd,(HMENU)IDC_PROGRAMINFONEXT,m_hinst,NULL);
			pThis->m_hbrBack=CreateSolidBrush(pThis->m_crBackColor);
			pThis->m_hbrProgramInfoBack=CreateSolidBrush(pThis->m_crProgramInfoBackColor);
		}
		return 0;

	case WM_SIZE:
		{
			CInformation *pThis=GetThis(hwnd);
			RECT rc;

			pThis->GetItemRect(INFO_ITEM_PROGRAMINFO,&rc);
			MoveWindow(pThis->m_hwndProgramInfo,rc.left,rc.top,
						rc.right-rc.left,rc.bottom-rc.top,TRUE);
			MoveWindow(pThis->m_hwndProgramInfoPrev,
						rc.right-PROGRAMINFO_BUTTON_SIZE*2,rc.bottom,
						PROGRAMINFO_BUTTON_SIZE,PROGRAMINFO_BUTTON_SIZE,TRUE);
			MoveWindow(pThis->m_hwndProgramInfoNext,
						rc.right-PROGRAMINFO_BUTTON_SIZE,rc.bottom,
						PROGRAMINFO_BUTTON_SIZE,PROGRAMINFO_BUTTON_SIZE,TRUE);
		}
		return 0;

	case WM_PAINT:
		{
			CInformation *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;
			HFONT hfontOld;
			COLORREF crOldTextColor;
			int OldBkMode;
			RECT rc;
			TCHAR szText[256];

			BeginPaint(hwnd,&ps);
			hfontOld=static_cast<HFONT>(SelectObject(ps.hdc,pThis->m_hFont));
			FillRect(ps.hdc,&ps.rcPaint,pThis->m_hbrBack);
			crOldTextColor=SetTextColor(ps.hdc,pThis->m_crTextColor);
			OldBkMode=SetBkMode(ps.hdc,TRANSPARENT);
			pThis->GetItemRect(INFO_ITEM_VIDEO,&rc);
			if (IsRectIntersect(&ps.rcPaint,&rc)) {
				wsprintf(szText,TEXT("%d x %d [%d:%d]"),
					pThis->m_VideoWidth,pThis->m_VideoHeight,
					pThis->m_AspectX,pThis->m_AspectY);
				DrawText(ps.hdc,szText,-1,&rc,DT_LEFT | DT_SINGLELINE);
			}
			if (pThis->m_pszDecoderName!=NULL) {
				pThis->GetItemRect(INFO_ITEM_DECODER,&rc);
				if (IsRectIntersect(&ps.rcPaint,&rc))
					DrawText(ps.hdc,pThis->m_pszDecoderName,-1,&rc,
						DT_LEFT | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
			}
			pThis->GetItemRect(INFO_ITEM_BITRATE,&rc);
			if (IsRectIntersect(&ps.rcPaint,&rc)) {
				if (pThis->m_fSignalLevel) {
					int SignalLevel=(int)(pThis->m_SignalLevel*100);
					wsprintf(szText,TEXT("%d.%02d dB"),
											SignalLevel/100,SignalLevel%100);
				} else
					szText[0]='\0';
				if (pThis->m_fBitRate) {
					if (pThis->m_fSignalLevel)
						lstrcat(szText,TEXT(" / "));
					int BitRate=(int)(pThis->m_BitRate*100);
					wsprintf(szText+lstrlen(szText),TEXT("%d.%02d Mbps"),
													BitRate/100,BitRate%100);
				}
				DrawText(ps.hdc,szText,-1,&rc,DT_LEFT | DT_SINGLELINE);
			}
			pThis->GetItemRect(INFO_ITEM_ERROR,&rc);
			if (IsRectIntersect(&ps.rcPaint,&rc)) {
				const CCoreEngine *pCoreEngine=GetAppClass().GetCoreEngine();
				int Length;

				Length=wsprintf(szText,TEXT("D %u / E %u"),
								pCoreEngine->GetContinuityErrorPacketCount(),
								pCoreEngine->GetErrorPacketCount());
				if (pCoreEngine->GetDescramble()
						&& pCoreEngine->GetCardReaderType()!=CCardReader::READER_NONE)
				wsprintf(szText+Length,TEXT(" / S %u"),pCoreEngine->GetScramblePacketCount());
				DrawText(ps.hdc,szText,-1,&rc,DT_LEFT | DT_SINGLELINE);
			}
			pThis->GetItemRect(INFO_ITEM_RECORD,&rc);
			if (IsRectIntersect(&ps.rcPaint,&rc)) {
				if (pThis->m_fRecording) {
					unsigned int RecordSec=pThis->m_RecordTime/1000;
					unsigned int Size=(unsigned int)(
						pThis->m_RecordWroteSize/(ULONGLONG)(1024*1024/100));
					unsigned int FreeSpace=(unsigned int)(
						pThis->m_DiskFreeSpace.QuadPart/(ULONGLONG)(1024*1024*1024/100));

					wsprintf(szText,
						TEXT("Åú %d:%02d:%02d / %d.%02d MB / %d.%02d GBãÛÇ´"),
						RecordSec/(60*60),(RecordSec/60)%60,RecordSec%60,
						Size/100,Size%100,FreeSpace/100,FreeSpace%100);
				} else
					lstrcpy(szText,TEXT("Å° <ò^âÊÇµÇƒÇ¢Ç‹ÇπÇÒ>"));
				DrawText(ps.hdc,szText,-1,&rc,DT_LEFT | DT_SINGLELINE);
			}
			SetBkMode(ps.hdc,OldBkMode);
			SetTextColor(ps.hdc,crOldTextColor);
			SelectObject(ps.hdc,hfontOld);
			EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_CTLCOLORSTATIC:
		{
			CInformation *pThis=GetThis(hwnd);
			HDC hdc=reinterpret_cast<HDC>(wParam);

			SetTextColor(hdc,pThis->m_crProgramInfoTextColor);
			SetBkColor(hdc,pThis->m_crProgramInfoBackColor);
			return (LRESULT)pThis->m_hbrProgramInfoBack;
		}

	case WM_DRAWITEM:
		{
			CInformation *pThis=GetThis(hwnd);
			LPDRAWITEMSTRUCT pdis=reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
			HBRUSH hbrOld,hbr;
			HPEN hpen,hpenOld;
			POINT Points[3];

			hpen=CreatePen(PS_SOLID,1,pThis->m_crTextColor);
			hbrOld=SelectBrush(pdis->hDC,pThis->m_hbrBack);
			hpenOld=SelectPen(pdis->hDC,hpen);
			Rectangle(pdis->hDC,pdis->rcItem.left,pdis->rcItem.top,
								pdis->rcItem.right,pdis->rcItem.bottom);
			if (pdis->CtlID==IDC_PROGRAMINFOPREV) {
				Points[0].x=pdis->rcItem.right-5;
				Points[0].y=pdis->rcItem.top+3;
				Points[1].x=Points[0].x;
				Points[1].y=pdis->rcItem.bottom-4;
				Points[2].x=pdis->rcItem.left+4;
				Points[2].y=pdis->rcItem.top+(pdis->rcItem.bottom-pdis->rcItem.top)/2;
			} else {
				Points[0].x=pdis->rcItem.left+4;
				Points[0].y=pdis->rcItem.top+3;
				Points[1].x=Points[0].x;
				Points[1].y=pdis->rcItem.bottom-4;
				Points[2].x=pdis->rcItem.right-5;
				Points[2].y=pdis->rcItem.top+(pdis->rcItem.bottom-pdis->rcItem.top)/2;
			}
			if ((pdis->itemState&ODS_DISABLED)!=0) {
				hbr=CreateSolidBrush(RGB(
					(GetRValue(pThis->m_crBackColor)+GetRValue(pThis->m_crTextColor))/2,
					(GetGValue(pThis->m_crBackColor)+GetGValue(pThis->m_crTextColor))/2,
					(GetBValue(pThis->m_crBackColor)+GetBValue(pThis->m_crTextColor))/2));
			} else
				hbr=CreateSolidBrush(pThis->m_crTextColor);
			SelectBrush(pdis->hDC,hbr);
			SelectObject(pdis->hDC,GetStockObject(NULL_PEN));
			Polygon(pdis->hDC,Points,3);
			SelectPen(pdis->hDC,hpenOld);
			SelectBrush(pdis->hDC,hbrOld);
			DeleteObject(hbr);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PROGRAMINFOPREV:
		case IDC_PROGRAMINFONEXT:
			{
				CInformation *pThis=GetThis(hwnd);
				bool fNext=LOWORD(wParam)==IDC_PROGRAMINFONEXT;

				if (fNext!=pThis->m_fNextProgramInfo) {
					pThis->m_fNextProgramInfo=fNext;
					if (pThis->m_pszProgramInfo!=NULL) {
						delete [] pThis->m_pszProgramInfo;
						pThis->m_pszProgramInfo=NULL;
						SetWindowText(pThis->m_hwndProgramInfo,TEXT(""));
					}
					EnableWindow(pThis->m_hwndProgramInfoPrev,fNext);
					EnableWindow(pThis->m_hwndProgramInfoNext,!fNext);
				}
			}
			return 0;
		}
		return 0;

	case WM_DESTROY:
		{
			CInformation *pThis=GetThis(hwnd);

			DeleteObject(pThis->m_hbrBack);
			DeleteObject(pThis->m_hbrProgramInfoBack);
			pThis->OnDestroy();
		}
		return 0;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CInformation::GetItemRect(int Item,RECT *pRect)
{
	GetClientRect(pRect);
	pRect->top=Item*(m_FontHeight+m_LineMargin);
	if (Item==INFO_ITEM_PROGRAMINFO)
		pRect->bottom-=PROGRAMINFO_BUTTON_SIZE;
	if (Item!=INFO_ITEM_PROGRAMINFO || pRect->top>=pRect->bottom)
		pRect->bottom=pRect->top+m_FontHeight;
}


void CInformation::UpdateItem(int Item)
{
	if (m_hwnd!=NULL) {
		RECT rc;

		GetItemRect(Item,&rc);
		::InvalidateRect(m_hwnd,&rc,TRUE);
	}
}
