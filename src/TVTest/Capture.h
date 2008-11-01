#ifndef CAPTURE_H
#define CAPTURE_H


#include "BasicWindow.h"


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

class CCapturePreview;

class CCapturePreviewEvent {
protected:
	CCapturePreview *m_pCapturePreview;
public:
	CCapturePreviewEvent();
	virtual ~CCapturePreviewEvent() {}
	virtual bool OnClose() { return true; }
	virtual bool OnSave(CCaptureImage *pImage) { return false; }
	friend CCapturePreview;
};

class CCapturePreview : public CBasicWindow {
	static HINSTANCE m_hinst;
	CCaptureImage *m_pImage;
	COLORREF m_crBackColor;
	CCapturePreviewEvent *m_pEvent;
	static CCapturePreview *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	void SetTitle();
public:
	static bool Initialize(HINSTANCE hinst);
	CCapturePreview();
	~CCapturePreview();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool SetImage(const BITMAPINFO *pbmi,const void *pBits);
	bool SetImage(CCaptureImage *pImage);
	bool ClearImage();
	bool HasImage() const;
	bool SetEventHandler(CCapturePreviewEvent *pEvent);
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
