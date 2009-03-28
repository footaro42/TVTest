#ifndef CAPTURE_H
#define CAPTURE_H


#include "BasicWindow.h"
#include "StatusView.h"


class CCaptureImage {
	HGLOBAL m_hData;
	bool m_fLocked;
	SYSTEMTIME m_stCaptureTime;
	LPTSTR m_pszComment;

public:
	CCaptureImage(HGLOBAL hData);
	CCaptureImage(const BITMAPINFO *pbmi,const void *pBits);
	~CCaptureImage();
	bool SetClipboard(HWND hwnd);
	bool GetBitmapInfoHeader(BITMAPINFOHEADER *pbmih) const;
	bool LockData(BITMAPINFO **ppbmi,BYTE **ppBits);
	bool UnlockData();
	const SYSTEMTIME &GetCaptureTime() const { return m_stCaptureTime; }
	bool SetComment(LPCTSTR pszComment);
	LPCTSTR GetComment() const { return m_pszComment; }
};

class CCapturePreview : public CBasicWindow {
public:
	class CEventHandler {
	protected:
		CCapturePreview *m_pCapturePreview;
	public:
		CEventHandler();
		virtual ~CEventHandler()=0;
		virtual void OnLButtonDown(int x,int y) {}
		virtual void OnRButtonDown(int x,int y) {}
		virtual bool OnKeyDown(UINT KeyCode,UINT Flags) { return false; }
		friend class CCapturePreview;
	};
	static bool Initialize(HINSTANCE hinst);
	CCapturePreview();
	~CCapturePreview();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool SetImage(CCaptureImage *pImage);
	bool ClearImage();
	bool HasImage() const;
	bool SetEventHandler(CEventHandler *pEventHandler);

private:
	static HINSTANCE m_hinst;
	CCaptureImage *m_pImage;
	COLORREF m_crBackColor;
	CEventHandler *m_pEventHandler;
	static CCapturePreview *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
};

class CCaptureWindow : public CBasicWindow {
public:
	class CEventHandler {
	protected:
		CCaptureWindow *m_pCaptureWindow;
	public:
		CEventHandler();
		virtual ~CEventHandler()=0;
		virtual bool OnClose() { return true; }
		virtual bool OnSave(CCaptureImage *pImage) { return false; }
		virtual bool OnKeyDown(UINT KeyCode,UINT Flags) { return false; }
		friend class CCaptureWindow;
	};
	static bool Initialize(HINSTANCE hinst);
	CCaptureWindow();
	~CCaptureWindow();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool SetImage(const BITMAPINFO *pbmi,const void *pBits);
	bool SetImage(CCaptureImage *pImage);
	bool ClearImage();
	bool HasImage() const;
	bool SetEventHandler(CEventHandler *pEventHandler);
	void ShowStatusBar(bool fShow);
	bool IsStatusBarVisible() const { return m_fShowStatusBar; }
	void SetStatusColor(COLORREF crBack1,COLORREF crBack2,COLORREF crText,
		COLORREF crHighlightBack1,COLORREF crHighlightBack2,COLORREF crHighlightText);

private:
	static HINSTANCE m_hinst;
	CCapturePreview m_Preview;
	class CPreviewEventHandler : public CCapturePreview::CEventHandler {
		CCaptureWindow *m_pCaptureWindow;
	public:
		CPreviewEventHandler(CCaptureWindow *pCaptureWindow);
		void OnRButtonDown(int x,int y);
		bool OnKeyDown(UINT KeyCode,UINT Flags);
	};
	CPreviewEventHandler m_PreviewEventHandler;
	CStatusView m_Status;
	bool m_fShowStatusBar;
	HBITMAP m_hbmStatusIcons;
	enum {
		STATUS_ITEM_CAPTURE,
		//STATUS_ITEM_CONTINUOUS,
		STATUS_ITEM_SAVE,
		STATUS_ITEM_COPY
	};
	class CCaptureStatusItem : public CStatusItem {
		HBITMAP m_hbmIcon;
	public:
		CCaptureStatusItem(HBITMAP hbmIcon);
		LPCTSTR GetName() const { return TEXT("キャプチャ"); }
		void Draw(HDC hdc,const RECT *pRect);
		void OnLButtonDown(int x,int y);
		void OnRButtonDown(int x,int y);
	};
	/*
	class CContinuousStatusItem : public CStatusItem {
		HBITMAP m_hbmIcon;
	public:
		CContinuousStatusItem(HBITMAP hbmIcon);
		LPCTSTR GetName() const { return TEXT("連写"); }
		void Draw(HDC hdc,const RECT *pRect);
		void OnLButtonDown(int x,int y);
		void OnRButtonDown(int x,int y);
	};
	*/
	class CSaveStatusItem : public CStatusItem {
		CCaptureWindow *m_pCaptureWindow;
		HBITMAP m_hbmIcon;
	public:
		CSaveStatusItem(CCaptureWindow *pCaptureWindow,HBITMAP hbmIcon);
		LPCTSTR GetName() const { return TEXT("保存"); }
		void Draw(HDC hdc,const RECT *pRect);
		void OnLButtonDown(int x,int y);
	};
	class CCopyStatusItem : public CStatusItem {
		CCaptureWindow *m_pCaptureWindow;
		HBITMAP m_hbmIcon;
	public:
		CCopyStatusItem(CCaptureWindow *pCaptureWindow,HBITMAP hbmIcon);
		LPCTSTR GetName() const { return TEXT("コピー"); }
		void Draw(HDC hdc,const RECT *pRect);
		void OnLButtonDown(int x,int y);
	};
	CCaptureImage *m_pImage;
	CEventHandler *m_pEventHandler;
	static CCaptureWindow *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	void SetTitle();
};

/*
class CImageSaveThread {
	CCaptureImage *m_pImage;
	LPTSTR m_pszFileName;
	int m_Format;
	LPTSTR m_pszOption;
	LPTSTR m_pszComment;
	CImageCodec m_ImageCodec;
	static void SaveProc(void *pParam);
public:
	CImageSaveThread(CCaptureImage *pImage,LPCTSTR pszFileName,int Format,
										LPCTSTR pszOption,LPCTSTR pszComment);
	~CImageSaveThread();
	bool BeginSave();
};
*/


#endif
