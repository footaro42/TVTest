#ifndef CAPTION_PANEL_H
#define CAPTION_PANEL_H


#include <deque>
#include "PanelForm.h"
#include "BonTsEngine/CaptionDecoder.h"


class CCaptionPanel : public CPanelForm::CPage, protected CCaptionDecoder::IHandler
{
	COLORREF m_BackColor;
	COLORREF m_TextColor;
	HBRUSH m_hbrBack;
	HFONT m_hfont;
	HWND m_hwndEdit;
	WNDPROC m_pOldEditProc;
	bool m_fEnable;
	bool m_fAutoScroll;
#ifndef TVH264
	bool m_fIgnoreSmall;
#endif
	BYTE m_Language;
	bool m_fClearLast;
	bool m_fContinue;
	std::deque<LPTSTR> m_CaptionList;
	enum { MAX_QUEUE_TEXT=10000 };
	CCriticalLock m_Lock;

// CCaptionDecoder::IHandler
	virtual void OnLanguageUpdate(CCaptionDecoder *pDecoder);
	virtual void OnCaption(CCaptionDecoder *pDecoder,BYTE Language, LPCTSTR pszText,const CAribString::FormatList *pFormatList);

	void ClearCaptionList();
	void AppendText(LPCTSTR pszText);

	static const LPCTSTR m_pszClassName;
	static const LPCTSTR m_pszPropName;
	static HINSTANCE m_hinst;
	static CCaptionPanel *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static LRESULT CALLBACK EditWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	CCaptionPanel();
	~CCaptionPanel();
// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	void SetVisible(bool fVisible);
// CCaptionPanel
	void SetColor(COLORREF BackColor,COLORREF TextColor);
	bool SetFont(const LOGFONT *pFont);
	void Clear();

	static bool Initialize(HINSTANCE hinst);
};


#endif
