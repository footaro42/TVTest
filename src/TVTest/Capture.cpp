#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "Capture.h"
#include "Image.h"
#include "DrawUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define CAPTURE_PREVIEW_WINDOW_CLASS APP_NAME TEXT(" Capture Preview")
#define TITLE_TEXT TEXT("キャプチャプレビュー")




CCaptureImage::CCaptureImage(HGLOBAL hData)
{
	m_hData=hData;
	m_fLocked=false;
	m_pszComment=NULL;
	::GetLocalTime(&m_stCaptureTime);
}


CCaptureImage::CCaptureImage(const BITMAPINFO *pbmi,const void *pBits)
{
	SIZE_T InfoSize,BitsSize;

	InfoSize=CalcDIBInfoSize(&pbmi->bmiHeader);
	BitsSize=CalcDIBBitsSize(&pbmi->bmiHeader);
	m_hData=::GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE,InfoSize+BitsSize);
	if (m_hData!=NULL) {
		BYTE *pData=static_cast<BYTE*>(::GlobalLock(m_hData));

		if (pData!=NULL) {
			::CopyMemory(pData,pbmi,InfoSize);
			::CopyMemory(pData+InfoSize,pBits,BitsSize);
			::GlobalUnlock(m_hData);
		} else {
			::GlobalFree(m_hData);
			m_hData=NULL;
		}
	}
	m_fLocked=false;
	m_pszComment=NULL;
	::GetLocalTime(&m_stCaptureTime);
}


CCaptureImage::~CCaptureImage()
{
	if (m_hData!=NULL) {
		if (m_fLocked)
			::GlobalUnlock(m_hData);
		::GlobalFree(m_hData);
	}
	delete [] m_pszComment;
}


bool CCaptureImage::SetClipboard(HWND hwnd)
{
	if (m_hData==NULL || m_fLocked)
		return false;

	HGLOBAL hCopy;
	SIZE_T Size;

	Size=::GlobalSize(m_hData);
	hCopy=::GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE,Size);
	if (hCopy==NULL)
		return false;
	::CopyMemory(::GlobalLock(hCopy),::GlobalLock(m_hData),Size);
	::GlobalUnlock(hCopy);
	::GlobalUnlock(m_hData);
	if (!::OpenClipboard(hwnd)) {
		::GlobalFree(hCopy);
		return false;
	}
	::EmptyClipboard();
	bool fOK=::SetClipboardData(CF_DIB,hCopy)!=NULL;
	::CloseClipboard();
	return fOK;
}


bool CCaptureImage::GetBitmapInfoHeader(BITMAPINFOHEADER *pbmih) const
{
	if (m_hData==NULL || m_fLocked)
		return false;

	BITMAPINFOHEADER *pbmihSrc=static_cast<BITMAPINFOHEADER*>(::GlobalLock(m_hData));

	if (pbmihSrc==NULL)
		return false;
	*pbmih=*pbmihSrc;
	::GlobalUnlock(m_hData);
	return true;
}


bool CCaptureImage::LockData(BITMAPINFO **ppbmi,BYTE **ppBits)
{
	if (m_hData==NULL || m_fLocked)
		return false;

	void *pDib=::GlobalLock(m_hData);
	BITMAPINFO *pbmi;

	if (pDib==NULL)
		return false;
	pbmi=static_cast<BITMAPINFO*>(pDib);
	if (ppbmi!=NULL)
		*ppbmi=pbmi;
	if (ppBits!=NULL)
		*ppBits=static_cast<BYTE*>(pDib)+CalcDIBInfoSize(&pbmi->bmiHeader);
	m_fLocked=true;
	return true;
}


bool CCaptureImage::UnlockData()
{
	if (!m_fLocked)
		return false;
	::GlobalUnlock(m_hData);
	m_fLocked=false;
	return true;
}


bool CCaptureImage::SetComment(LPCTSTR pszComment)
{
	return ReplaceString(&m_pszComment,pszComment);
}




CCapturePreviewEvent::CCapturePreviewEvent()
{
	m_pCapturePreview=NULL;
}




HINSTANCE CCapturePreview::m_hinst=NULL;


bool CCapturePreview::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=CAPTURE_PREVIEW_WINDOW_CLASS;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CCapturePreview::CCapturePreview()
{
	m_WindowPosition.Left=0;
	m_WindowPosition.Top=0;
	m_WindowPosition.Width=320;
	m_WindowPosition.Height=240;
	m_pImage=NULL;
	m_crBackColor=RGB(0,0,0);
	m_pEvent=NULL;
}


CCapturePreview::~CCapturePreview()
{
	if (m_pImage!=NULL)
		delete m_pImage;
}


bool CCapturePreview::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 CAPTURE_PREVIEW_WINDOW_CLASS,TITLE_TEXT,m_hinst);
}


bool CCapturePreview::SetImage(const BITMAPINFO *pbmi,const void *pBits)
{
	SIZE_T InfoSize,BitsSize;
	BYTE *pData;

	ClearImage();
	m_pImage=new CCaptureImage(pbmi,pBits);
	if (m_hwnd!=NULL) {
		Invalidate();
		Update();
		SetTitle();
	}
	return true;
}


bool CCapturePreview::SetImage(CCaptureImage *pImage)
{
	ClearImage();
	m_pImage=pImage;
	if (m_hwnd!=NULL) {
		Invalidate();
		Update();
		SetTitle();
	}
	return true;
}


bool CCapturePreview::ClearImage()
{
	if (m_pImage!=NULL) {
		delete m_pImage;
		m_pImage=NULL;
		if (m_hwnd!=NULL) {
			Invalidate();
			SetTitle();
		}
	}
	return true;
}


bool CCapturePreview::HasImage() const
{
	return m_pImage!=NULL;
}


bool CCapturePreview::SetEventHandler(CCapturePreviewEvent *pEvent)
{
	if (m_pEvent!=NULL)
		m_pEvent->m_pCapturePreview=NULL;
	if (pEvent!=NULL)
		pEvent->m_pCapturePreview=this;
	m_pEvent=pEvent;
	return true;
}


void CCapturePreview::SetTitle()
{
	if (m_hwnd!=NULL) {
		TCHAR szTitle[64];

		if (m_pImage!=NULL) {
			BITMAPINFOHEADER bmih;

			if (m_pImage->GetBitmapInfoHeader(&bmih)) {
				::wsprintf(szTitle,TITLE_TEXT TEXT(" - %d x %d (%d bpp)"),
							bmih.biWidth,abs(bmih.biHeight),bmih.biBitCount);
			}
		} else
			::lstrcpy(szTitle,TITLE_TEXT);
		::SetWindowText(m_hwnd,szTitle);
	}
}


CCapturePreview *CCapturePreview::GetThis(HWND hwnd)
{
	return reinterpret_cast<CCapturePreview*>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CCapturePreview::WndProc(HWND hwnd,UINT uMsg,
												WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CCapturePreview *pThis=dynamic_cast<CCapturePreview*>(OnCreate(hwnd,lParam));

			if (pThis->m_pImage!=NULL)
				pThis->SetTitle();
		}
		return 0;

	case WM_PAINT:
		{
			CCapturePreview *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;
			HBRUSH hbr;
			RECT rc;
			BITMAPINFO *pbmi;
			BYTE *pBits;

			::BeginPaint(hwnd,&ps);
			hbr=::CreateSolidBrush(pThis->m_crBackColor);
			pThis->GetClientRect(&rc);
			if (pThis->m_pImage!=NULL
					&& pThis->m_pImage->LockData(&pbmi,&pBits)) {
				int DstX,DstY,DstWidth,DstHeight;
				RECT rcDest;

				DstWidth=pbmi->bmiHeader.biWidth*rc.bottom/
												abs(pbmi->bmiHeader.biHeight);
				if (DstWidth>rc.right)
					DstWidth=rc.right;
				DstHeight=pbmi->bmiHeader.biHeight*rc.right/
												pbmi->bmiHeader.biWidth;
				if (DstHeight>rc.bottom)
					DstHeight=rc.bottom;
				DstX=(rc.right-DstWidth)/2;
				DstY=(rc.bottom-DstHeight)/2;
				if (DstWidth>0 && DstHeight>0) {
					int OldStretchBltMode;

					OldStretchBltMode=::SetStretchBltMode(ps.hdc,STRETCH_HALFTONE);
					::StretchDIBits(ps.hdc,DstX,DstY,DstWidth,DstHeight,
						0,0,pbmi->bmiHeader.biWidth,pbmi->bmiHeader.biHeight,
						pBits,pbmi,DIB_RGB_COLORS,SRCCOPY);
					::SetStretchBltMode(ps.hdc,OldStretchBltMode);
				}
				pThis->m_pImage->UnlockData();
				rcDest.left=DstX;
				rcDest.top=DstY;
				rcDest.right=DstX+DstWidth;
				rcDest.bottom=DstY+DstHeight;
				DrawUtil::FillBorder(ps.hdc,&rc,&rcDest,&ps.rcPaint,hbr);
			} else
				::FillRect(ps.hdc,&rc,hbr);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_RBUTTONDOWN:
		{
			CCapturePreview *pThis=GetThis(hwnd);
			HMENU hmenu;
			POINT pt;

			hmenu=::LoadMenu(GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDM_CAPTUREPREVIEW));
			::EnableMenuItem(hmenu,CM_COPY,MF_BYCOMMAND |
							 (pThis->m_pImage!=NULL?MFS_ENABLED:MFS_GRAYED));
			::EnableMenuItem(hmenu,CM_SAVEIMAGE,MF_BYCOMMAND |
							 (pThis->m_pImage!=NULL?MFS_ENABLED:MFS_GRAYED));
			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			::ClientToScreen(hwnd,&pt);
			::TrackPopupMenu(::GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,pt.x,pt.y,0,
							 hwnd,NULL);
			::DestroyMenu(hmenu);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case CM_COPY:
			{
				CCapturePreview *pThis=GetThis(hwnd);

				if (pThis->m_pImage!=NULL) {
					if (!pThis->m_pImage->SetClipboard(hwnd)) {
						::MessageBox(hwnd,TEXT("クリップボードにデータを設定できません。"),NULL,
									 MB_OK | MB_ICONEXCLAMATION);
					}
				}
			}
			return TRUE;

		case CM_SAVEIMAGE:
			{
				CCapturePreview *pThis=GetThis(hwnd);

				if (pThis->m_pImage!=NULL && pThis->m_pEvent!=NULL) {
					if (!pThis->m_pEvent->OnSave(pThis->m_pImage)) {
						::MessageBox(hwnd,TEXT("画像の保存ができません。"),NULL,
									 MB_OK | MB_ICONEXCLAMATION);
					}
				}
			}
			return TRUE;
		}
		return TRUE;

	/*
	case WM_NCHITTEST:
		if (::DefWindowProc(hwnd,uMsg,wParam,lParam)==HTCLIENT)
			return HTCAPTION;
		break;
	*/

	case WM_CLOSE:
		{
			CCapturePreview *pThis=GetThis(hwnd);

			if (pThis->m_pEvent!=NULL
					&& !pThis->m_pEvent->OnClose())
				return 0;
		}
		break;

	case WM_DESTROY:
		{
			CCapturePreview *pThis=GetThis(hwnd);

			pThis->OnDestroy();
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}




/*
CImageSaveThread::CImageSaveThread(CCaptureImage *pImage,LPCTSTR pszFileName,int Format,
										LPCTSTR pszOption,LPCTSTR pszComment)
{
	m_pImage=new CCaptureImage(*pImage);
	m_pszFileName=DuplicateString(pszFileName);
	m_Format=Format;
	m_pszOptions=DuplicateString(pszOption);
	m_pszComment=DuplicateString(pszComment);
}


~CImageSaveThread::CImageSaveThread()
{
	delete m_pImage;
	delete [] m_pszFileName;
	delete [] m_pszOptions;
	delete [] m_pszComment;
}


bool CImageSaveThread::BeginSave()
{
	_beginthread(SaveProc,0,this);
	return true;
}


void CImageSaveThread::SaveProc(void *pParam)
{
	CImageSaveThread *pThis=static_cast<CImageSaveThread*>(pParam);
	BITMAPINFO *pbmi;
	BYTE *pBits;
	bool fResult=false;

	if (pThis->m_pImage->LockData(&pbmi,&pBits)) {
		fResult=m_ImageCodec.SaveImage(pThis->m_pszFileName,pThis->m_Format,
							pThis->m_pszOption,pbmi,pBits,pThis->m_pszComment);
		pThis->m_pImage->UnlockData();
	}
	delete pThis;
	_endthread();
}
*/
