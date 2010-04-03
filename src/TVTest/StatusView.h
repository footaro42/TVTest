#ifndef STATUS_VIEW_H
#define STATUS_VIEW_H


#include "BasicWindow.h"
#include "TsUtilClass.h"
#include "PointerArray.h"
#include "VirtualScreen.h"
#include "Theme.h"
#include "Aero.h"


class CStatusView;

class __declspec(novtable) CStatusItem {
protected:
	CStatusView *m_pStatus;
	int m_ID;
	int m_DefaultWidth;
	int m_Width;
	int m_MinWidth;
	bool m_fVisible;
	bool Update();
	bool GetMenuPos(POINT *pPos,UINT *pFlags);
	enum {
		DRAWTEXT_HCENTER = 0x00000001UL
	};
	void DrawText(HDC hdc,const RECT *pRect,LPCTSTR pszText,DWORD Flags=0) const;
	void DrawIcon(HDC hdc,const RECT *pRect,HBITMAP hbm,int SrcX=0,int SrcY=0,
				  int IconWidth=16,int IconHeight=16,bool fEnabled=true) const;

public:
	CStatusItem(int ID,int DefaultWidth);
	virtual ~CStatusItem() {}
	int GetIndex() const;
	bool GetRect(RECT *pRect) const;
	bool GetClientRect(RECT *pRect) const;
	int GetID() const { return m_ID; }
	int GetDefaultWidth() const { return m_DefaultWidth; }
	int GetWidth() const { return m_Width; }
	bool SetWidth(int Width);
	int GetMinWidth() const { return m_MinWidth; }
	void SetVisible(bool fVisible);
	bool GetVisible() const { return m_fVisible; }
	virtual LPCTSTR GetName() const=0;
	virtual void Draw(HDC hdc,const RECT *pRect)=0;
	virtual void DrawPreview(HDC hdc,const RECT *pRect) { Draw(hdc,pRect); }
	virtual void OnLButtonDown(int x,int y) {}
	virtual void OnRButtonDown(int x,int y) { OnLButtonDown(x,y); }
	virtual void OnMouseMove(int x,int y) {}
	virtual void OnVisibleChange(bool fVisible) {}
	virtual void OnFocus(bool fFocus) {}
	virtual bool OnMouseHover(int x,int y) { return false; }
	virtual LRESULT OnNotifyMessage(LPNMHDR pnmh) { return 0; }
	friend CStatusView;
};

class CStatusView : public CBasicWindow, public CTracer {
public:
	class CEventHandler {
	protected:
		CStatusView *m_pStatusView;
	public:
		CEventHandler();
		virtual ~CEventHandler();
		virtual void OnMouseLeave() {}
		friend CStatusView;
	};

private:
	static HINSTANCE m_hinst;
	HFONT m_hfontStatus;
	int m_FontHeight;
	Theme::GradientInfo m_BackGradient;
	COLORREF m_crTextColor;
	Theme::GradientInfo m_HighlightBackGradient;
	COLORREF m_crHighlightTextColor;
	Theme::BorderType m_BorderType;
	CPointerVector<CStatusItem> m_ItemList;
	int m_NumItems;
	bool m_fSingleMode;
	LPTSTR m_pszSingleText;
	int m_HotItem;
	bool m_fTrackMouseEvent;
	bool m_fOnButtonDown;
	CEventHandler *m_pEventHandler;
	CVirtualScreen m_VirtualScreen;
	bool m_fBufferedPaint;
	CBufferedPaint m_BufferedPaint;

	void SetHotItem(int Item);
	void Draw(HDC hdc,const RECT *pPaintRect);

	static CStatusView *GetStatusView(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	static bool Initialize(HINSTANCE hinst);
	CStatusView();
	~CStatusView();
// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	void SetVisible(bool fVisible);
// CStatusView
	int NumItems() const { return m_NumItems; }
	const CStatusItem *GetItem(int Index) const;
	CStatusItem *GetItem(int Index);
	const CStatusItem *GetItemByID(int ID) const;
	CStatusItem *GetItemByID(int ID);
	bool AddItem(CStatusItem *pItem);
	int IDToIndex(int ID) const;
	int IndexToID(int Index) const;
	void UpdateItem(int ID);
	bool GetItemRect(int ID,RECT *pRect) const;
	bool GetItemClientRect(int ID,RECT *pRect) const;
	int GetItemHeight() const;
	int GetFontHeight() const { return m_FontHeight; }
	int GetIntegralWidth() const;
	void SetSingleText(LPCTSTR pszText);
	void SetColor(const Theme::GradientInfo *pBackGradient,COLORREF crText,
				  const Theme::GradientInfo *pHighlightBackGradient,COLORREF crHighlightText);
	void SetBorderType(Theme::BorderType Type);
	bool SetFont(HFONT hfont);
	int GetCurItem() const;
	bool SetEventHandler(CEventHandler *pEventHandler);
	bool SetItemOrder(const int *pOrderList);
	bool DrawItemPreview(CStatusItem *pItem,HDC hdc,const RECT *pRect,bool fHighlight=false) const;
	bool EnableBufferedPaint(bool fEnable);
// CTracer
	void OnTrace(LPCTSTR pszOutput);
};


#endif
