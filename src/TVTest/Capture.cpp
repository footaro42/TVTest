#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "MainWindow.h"
#include "Capture.h"
#include "Image.h"
#include "DrawUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define CAPTURE_WINDOW_CLASS			APP_NAME TEXT(" Capture Window")
#define CAPTURE_PREVIEW_WINDOW_CLASS	APP_NAME TEXT(" Capture Preview")
#define TITLE_TEXT TEXT("キャプチャ")




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




CCapturePreview::CEventHandler::CEventHandler()
{
	m_pCapturePreview=NULL;
}


CCapturePreview::CEventHandler::~CEventHandler()
{
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
	m_pImage=NULL;
	m_crBackColor=RGB(0,0,0);
	m_pEventHandler=NULL;
}


CCapturePreview::~CCapturePreview()
{
}


bool CCapturePreview::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 CAPTURE_PREVIEW_WINDOW_CLASS,NULL,m_hinst);
}


bool CCapturePreview::SetImage(CCaptureImage *pImage)
{
	ClearImage();
	m_pImage=pImage;
	if (m_hwnd!=NULL) {
		Invalidate();
		Update();
	}
	return true;
}


bool CCapturePreview::ClearImage()
{
	if (m_pImage!=NULL) {
		m_pImage=NULL;
		if (m_hwnd!=NULL) {
			Invalidate();
		}
	}
	return true;
}


bool CCapturePreview::HasImage() const
{
	return m_pImage!=NULL;
}


bool CCapturePreview::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pCapturePreview=NULL;
	if (pEventHandler!=NULL)
		pEventHandler->m_pCapturePreview=this;
	m_pEventHandler=pEventHandler;
	return true;
}


CCapturePreview *CCapturePreview::GetThis(HWND hwnd)
{
	return static_cast<CCapturePreview*>(GetBasicWindow(hwnd));
}


LRESULT CALLBACK CCapturePreview::WndProc(HWND hwnd,UINT uMsg,
												WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CCapturePreview *pThis=static_cast<CCapturePreview*>(OnCreate(hwnd,lParam));
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

	case WM_LBUTTONDOWN:
		{
			CCapturePreview *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL)
				pThis->m_pEventHandler->OnLButtonDown(
									GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		}
		return TRUE;

	case WM_RBUTTONDOWN:
		{
			CCapturePreview *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL)
				pThis->m_pEventHandler->OnRButtonDown(
									GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		}
		return TRUE;

	case WM_KEYDOWN:
		{
			CCapturePreview *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL
					&& pThis->m_pEventHandler->OnKeyDown((UINT)wParam,(UINT)lParam))
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




CCaptureWindow::CEventHandler::CEventHandler()
{
	m_pCaptureWindow=NULL;
}


CCaptureWindow::CEventHandler::~CEventHandler()
{
}




HINSTANCE CCaptureWindow::m_hinst=NULL;


bool CCaptureWindow::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=0;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=(HBRUSH)(COLOR_3DFACE+1);
		wc.lpszMenuName=NULL;
		wc.lpszClassName=CAPTURE_WINDOW_CLASS;
		if (::RegisterClass(&wc)==0)
			return false;
		if (!CCapturePreview::Initialize(hinst))
			return false;
		m_hinst=hinst;
	}
	return true;
}


CCaptureWindow::CCaptureWindow()
	: m_PreviewEventHandler(this)
{
	m_WindowPosition.Left=0;
	m_WindowPosition.Top=0;
	m_WindowPosition.Width=320;
	m_WindowPosition.Height=240;
	m_fShowStatusBar=true;
	m_pImage=NULL;
	m_pEventHandler=NULL;
	m_hbmStatusIcons=NULL;
}


CCaptureWindow::~CCaptureWindow()
{
	if (m_pImage!=NULL)
		delete m_pImage;
	if (m_hbmStatusIcons!=NULL)
		::DeleteObject(m_hbmStatusIcons);
}


bool CCaptureWindow::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 CAPTURE_WINDOW_CLASS,TITLE_TEXT,m_hinst);
}


bool CCaptureWindow::SetImage(const BITMAPINFO *pbmi,const void *pBits)
{
	ClearImage();
	m_pImage=new CCaptureImage(pbmi,pBits);
	if (m_hwnd!=NULL) {
		m_Preview.SetImage(m_pImage);
		SetTitle();
	}
	return true;
}


bool CCaptureWindow::SetImage(CCaptureImage *pImage)
{
	ClearImage();
	m_pImage=pImage;
	if (m_hwnd!=NULL) {
		m_Preview.SetImage(m_pImage);
		SetTitle();
	}
	return true;
}


bool CCaptureWindow::ClearImage()
{
	if (m_pImage!=NULL) {
		delete m_pImage;
		m_pImage=NULL;
		if (m_hwnd!=NULL) {
			m_Preview.ClearImage();
			Invalidate();
			SetTitle();
		}
	}
	return true;
}


bool CCaptureWindow::HasImage() const
{
	return m_pImage!=NULL;
}


bool CCaptureWindow::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pCaptureWindow=NULL;
	if (pEventHandler!=NULL)
		pEventHandler->m_pCaptureWindow=this;
	m_pEventHandler=pEventHandler;
	return true;
}


void CCaptureWindow::ShowStatusBar(bool fShow)
{
	if (m_fShowStatusBar!=fShow) {
		m_fShowStatusBar=fShow;
		if (m_hwnd!=NULL) {
			RECT rc;

			GetClientRect(&rc);
			SendMessage(WM_SIZE,0,MAKELPARAM(rc.right,rc.bottom));
			m_Status.SetVisible(fShow);
		}
	}
}


void CCaptureWindow::SetStatusColor(const Theme::GradientInfo *pBackGradient,COLORREF crText,
	const Theme::GradientInfo *pHighlightBackGradient,COLORREF crHighlightText)
{
	m_Status.SetColor(pBackGradient,crText,pHighlightBackGradient,crHighlightText);
}


void CCaptureWindow::SetStatusBorderType(Theme::BorderType Type)
{
	m_Status.SetBorderType(Type);
}


void CCaptureWindow::SetTitle()
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


CCaptureWindow *CCaptureWindow::GetThis(HWND hwnd)
{
	return static_cast<CCaptureWindow*>(GetBasicWindow(hwnd));
}


LRESULT CALLBACK CCaptureWindow::WndProc(HWND hwnd,UINT uMsg,
												WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CCaptureWindow *pThis=dynamic_cast<CCaptureWindow*>(OnCreate(hwnd,lParam));

			pThis->m_Preview.Create(hwnd,WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
									WS_EX_CLIENTEDGE);
			pThis->m_Preview.SetEventHandler(&pThis->m_PreviewEventHandler);
			pThis->m_Status.Create(hwnd,WS_CHILD | WS_CLIPSIBLINGS |
								   (pThis->m_fShowStatusBar?WS_VISIBLE:0),
								   /*WS_EX_STATICEDGE*/0);
			//pThis->m_Status.SetEventHandler(pThis);
			if (pThis->m_Status.NumItems()==0) {
				if (pThis->m_hbmStatusIcons==NULL)
					pThis->m_hbmStatusIcons=
						static_cast<HBITMAP>(::LoadImage(GetAppClass().GetResourceInstance(),
						MAKEINTRESOURCE(IDB_CAPTURE),IMAGE_BITMAP,0,0,
						LR_DEFAULTCOLOR | LR_CREATEDIBSECTION));
				pThis->m_Status.AddItem(new CCaptureStatusItem(pThis->m_hbmStatusIcons));
				//pThis->m_Status.AddItem(new CContinuousStatusItem(pThis->m_hbmStatusIcons));
				pThis->m_Status.AddItem(new CSaveStatusItem(pThis,pThis->m_hbmStatusIcons));
				pThis->m_Status.AddItem(new CCopyStatusItem(pThis,pThis->m_hbmStatusIcons));
			}
			if (pThis->m_pImage!=NULL) {
				pThis->m_Preview.SetImage(pThis->m_pImage);
				pThis->SetTitle();
			}
		}
		return 0;

	case WM_SIZE:
		{
			CCaptureWindow *pThis=GetThis(hwnd);
			int Width=LOWORD(lParam),Height=HIWORD(lParam);

			if (pThis->m_fShowStatusBar) {
				Height-=pThis->m_Status.GetHeight();
				pThis->m_Status.SetPosition(0,Height,Width,pThis->m_Status.GetHeight());
			}
			pThis->m_Preview.SetPosition(0,0,Width,Height);
		}
		return 0;

	case WM_KEYDOWN:
		{
			CCaptureWindow *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL
					&& pThis->m_pEventHandler->OnKeyDown((UINT)wParam,(UINT)lParam))
				return 0;
		}
		break;

	case WM_ACTIVATE:
		{
			CCaptureWindow *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL
					&& pThis->m_pEventHandler->OnActivate(LOWORD(wParam)!=WA_INACTIVE))
				return 0;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case CM_SAVEIMAGE:
			{
				CCaptureWindow *pThis=GetThis(hwnd);

				if (pThis->m_pImage!=NULL && pThis->m_pEventHandler!=NULL) {
					if (!pThis->m_pEventHandler->OnSave(pThis->m_pImage)) {
						::MessageBox(hwnd,TEXT("画像の保存ができません。"),NULL,
									 MB_OK | MB_ICONEXCLAMATION);
					}
				}
			}
			return 0;

		case CM_COPY:
			{
				CCaptureWindow *pThis=GetThis(hwnd);

				if (pThis->m_pImage!=NULL) {
					if (!pThis->m_pImage->SetClipboard(hwnd)) {
						::MessageBox(hwnd,TEXT("クリップボードにデータを設定できません。"),NULL,
									 MB_OK | MB_ICONEXCLAMATION);
					}
				}
			}
			return 0;

		case CM_CAPTURESTATUSBAR:
			{
				CCaptureWindow *pThis=GetThis(hwnd);

				pThis->ShowStatusBar(!pThis->m_fShowStatusBar);
			}
			return 0;
		}
		return 0;

	/*
	case WM_NCHITTEST:
		if (::DefWindowProc(hwnd,uMsg,wParam,lParam)==HTCLIENT)
			return HTCAPTION;
		break;
	*/

	case WM_CLOSE:
		{
			CCaptureWindow *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL
					&& !pThis->m_pEventHandler->OnClose())
				return 0;
		}
		break;

	case WM_DESTROY:
		{
			CCaptureWindow *pThis=GetThis(hwnd);

			pThis->OnDestroy();
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}




CCaptureWindow::CPreviewEventHandler::CPreviewEventHandler(CCaptureWindow *pCaptureWindow)
{
	m_pCaptureWindow=pCaptureWindow;
}


void CCaptureWindow::CPreviewEventHandler::OnRButtonDown(int x,int y)
{
	HMENU hmenu;
	POINT pt;

	hmenu=::LoadMenu(GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDM_CAPTUREPREVIEW));
	::EnableMenuItem(hmenu,CM_COPY,MF_BYCOMMAND |
					 (m_pCaptureWindow->HasImage()?MFS_ENABLED:MFS_GRAYED));
	::EnableMenuItem(hmenu,CM_SAVEIMAGE,MF_BYCOMMAND |
					 (m_pCaptureWindow->HasImage()?MFS_ENABLED:MFS_GRAYED));
	::CheckMenuItem(hmenu,CM_CAPTURESTATUSBAR,MF_BYCOMMAND |
					(m_pCaptureWindow->IsStatusBarVisible()?MFS_CHECKED:MFS_UNCHECKED));
	pt.x=x;
	pt.y=y;
	::ClientToScreen(m_pCapturePreview->GetHandle(),&pt);
	::TrackPopupMenu(::GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,pt.x,pt.y,0,m_pCaptureWindow->GetHandle(),NULL);
	::DestroyMenu(hmenu);
}


bool CCaptureWindow::CPreviewEventHandler::OnKeyDown(UINT KeyCode,UINT Flags)
{
	::SendMessage(m_pCaptureWindow->GetHandle(),WM_KEYDOWN,KeyCode,Flags);
	return true;
}




CCaptureWindow::CCaptureStatusItem::CCaptureStatusItem(HBITMAP hbmIcon) : CStatusItem(STATUS_ITEM_CAPTURE,16)
{
	m_MinWidth=16;
	m_hbmIcon=hbmIcon;
}

void CCaptureWindow::CCaptureStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	DrawIcon(hdc,pRect,m_hbmIcon,0,0,16,16);
}

void CCaptureWindow::CCaptureStatusItem::OnLButtonDown(int x,int y)
{
	GetAppClass().GetMainWindow()->SendCommand(CM_CAPTURE);
}

void CCaptureWindow::CCaptureStatusItem::OnRButtonDown(int x,int y)
{
	/*
	HMENU hmenu;
	POINT pt;

	hmenu=LoadMenu(hInst,MAKEINTRESOURCE(IDM_CAPTURE));
	CheckMenuRadioItem(hmenu,CM_CAPTURESIZE_FIRST,CM_CAPTURESIZE_LAST,
		CM_CAPTURESIZE_FIRST+CaptureOptions.GetPresetCaptureSize(),MF_BYCOMMAND);
	if (fShowCapturePreview)
		CheckMenuItem(hmenu,CM_CAPTUREPREVIEW,MF_BYCOMMAND | MFS_CHECKED);
	Accelerator.SetMenuAccel(GetSubMenu(hmenu,0));
	GetMenuPos(&pt);
	TrackPopupMenu(GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,pt.x,pt.y,0,
												MainWindow.GetHandle(),NULL);
	DestroyMenu(hmenu);
	*/
}


#if 0

CCaptureWindow::CContinuousStatusItem::CContinuousStatusItem(HBITMAP hbmIcon) : CStatusItem(STATUS_ITEM_CONTINUOUS,16)
{
	m_MinWidth=16;
	m_hbmIcon=hbmIcon;
}

void CCaptureWindow::CContinuousStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	DrawIcon(hdc,pRect,m_hbmIcon,16,0,16,16);
}

void CCaptureWindow::CContinuousStatusItem::OnLButtonDown(int x,int y)
{
	GetAppClass().GetMainWindow()->SendCommand(CM_CONTINUOUSSHOOTING);
}

void CCaptureWindow::CContinuousStatusItem::OnRButtonDown(int x,int y)
{
	/*
	HMENU hmenu;
	POINT pt;

	hmenu=LoadMenu(hInst,MAKEINTRESOURCE(IDM_CAPTURE));
	CheckMenuRadioItem(hmenu,CM_CAPTURESIZE_FIRST,CM_CAPTURESIZE_LAST,
		CM_CAPTURESIZE_FIRST+CaptureOptions.GetPresetCaptureSize(),MF_BYCOMMAND);
	if (fShowCapturePreview)
		CheckMenuItem(hmenu,CM_CAPTUREPREVIEW,MF_BYCOMMAND | MFS_CHECKED);
	Accelerator.SetMenuAccel(GetSubMenu(hmenu,0));
	GetMenuPos(&pt);
	TrackPopupMenu(GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,pt.x,pt.y,0,
												MainWindow.GetHandle(),NULL);
	DestroyMenu(hmenu);
	*/
}

#endif


CCaptureWindow::CSaveStatusItem::CSaveStatusItem(CCaptureWindow *pCaptureWindow,HBITMAP hbmIcon) : CStatusItem(STATUS_ITEM_SAVE,16)
{
	m_MinWidth=16;
	m_pCaptureWindow=pCaptureWindow;
	m_hbmIcon=hbmIcon;
}

void CCaptureWindow::CSaveStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	DrawIcon(hdc,pRect,m_hbmIcon,32,0,16,16);
}

void CCaptureWindow::CSaveStatusItem::OnLButtonDown(int x,int y)
{
	m_pCaptureWindow->SendMessage(WM_COMMAND,CM_SAVEIMAGE,0);
}


CCaptureWindow::CCopyStatusItem::CCopyStatusItem(CCaptureWindow *pCaptureWindow,HBITMAP hbmIcon) : CStatusItem(STATUS_ITEM_COPY,16)
{
	m_MinWidth=16;
	m_pCaptureWindow=pCaptureWindow;
	m_hbmIcon=hbmIcon;
}

void CCaptureWindow::CCopyStatusItem::Draw(HDC hdc,const RECT *pRect)
{
	DrawIcon(hdc,pRect,m_hbmIcon,48,0,16,16);
}

void CCaptureWindow::CCopyStatusItem::OnLButtonDown(int x,int y)
{
	m_pCaptureWindow->SendMessage(WM_COMMAND,CM_COPY,0);
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
