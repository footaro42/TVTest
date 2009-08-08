#ifndef INFORMATION_H
#define INFORMATION_H


#include "InfoPanel.h"
#include "Util.h"
#include "Settings.h"


class CInformationPanel : public CInfoPanelPage {
public:
	class CEventHandler {
	public:
		virtual ~CEventHandler() {}
		virtual bool OnProgramInfoUpdate(bool fNext) { return false; }
	};

	enum {
		ITEM_VIDEO,
		ITEM_DECODER,
		ITEM_VIDEORENDERER,
		ITEM_AUDIODEVICE,
		ITEM_BITRATE,
		ITEM_ERROR,
		ITEM_RECORD,
		ITEM_PROGRAMINFO,
		NUM_ITEMS,
		ITEM_SIGNALLEVEL=ITEM_BITRATE,
	};

private:
	static HINSTANCE m_hinst;
	HWND m_hwndProgramInfo;
	WNDPROC m_pOldProgramInfoProc;
	HWND m_hwndProgramInfoPrev;
	HWND m_hwndProgramInfoNext;
	COLORREF m_crBackColor;
	COLORREF m_crTextColor;
	COLORREF m_crProgramInfoBackColor;
	COLORREF m_crProgramInfoTextColor;
	HBRUSH m_hbrBack;
	HBRUSH m_hbrProgramInfoBack;
	HFONT m_hFont;
	int m_FontHeight;
	int m_LineMargin;
	unsigned int m_ItemVisibility;
	CEventHandler *m_pEventHandler;

	int m_OriginalVideoWidth;
	int m_OriginalVideoHeight;
	int m_DisplayVideoWidth;
	int m_DisplayVideoHeight;
	int m_AspectX;
	int m_AspectY;
	CDynamicString m_VideoDecoderName;
	CDynamicString m_VideoRendererName;
	CDynamicString m_AudioDeviceName;
	bool m_fSignalLevel;
	float m_SignalLevel;
	bool m_fBitRate;
	float m_BitRate;
	bool m_fRecording;
	ULONGLONG m_RecordWroteSize;
	unsigned int m_RecordTime;
	ULARGE_INTEGER m_DiskFreeSpace;
	CDynamicString m_ProgramInfo;
	bool m_fNextProgramInfo;

	static const LPCTSTR m_pszItemNameList[];

	static CInformationPanel *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static LRESULT CALLBACK ProgramInfoHookProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	void GetItemRect(int Item,RECT *pRect) const;
	void UpdateItem(int Item);
	void CalcFontHeight();

public:
	static bool Initialize(HINSTANCE hinst);
	CInformationPanel();
	~CInformationPanel();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	void ResetStatistics();
	bool IsVisible() const;
	void SetColor(COLORREF crBackColor,COLORREF crTextColor);
	void SetProgramInfoColor(COLORREF crBackColor,COLORREF crTextColor);
	bool SetFont(const LOGFONT *pFont);
	bool SetItemVisible(int Item,bool fVisible);
	bool IsItemVisible(int Item) const;
	void SetVideoSize(int OriginalWidth,int OriginalHeight,int DisplayWidth,int DisplayHeight);
	void SetAspectRatio(int AspectX,int AspectY);
	void SetVideoDecoderName(LPCTSTR pszName);
	void SetVideoRendererName(LPCTSTR pszName);
	void SetAudioDeviceName(LPCTSTR pszName);
	void SetSignalLevel(float Level);
	void ShowSignalLevel(bool fShow);
	bool IsSignalLevelEnabled() const { return m_fSignalLevel; }
	void SetBitRate(float BitRate);
	void UpdateErrorCount();
	void SetRecordStatus(bool fRecording,LPCTSTR pszFileName=NULL,
							ULONGLONG WroteSize=0,unsigned int RecordTime=0);
	void SetProgramInfo(LPCTSTR pszInfo);
	bool GetProgramInfoNext() { return m_fNextProgramInfo; }
	bool SetEventHandler(CEventHandler *pHandler);
	bool Load(LPCTSTR pszFileName);
	bool Save(LPCTSTR pszFileName) const;
};


#endif
