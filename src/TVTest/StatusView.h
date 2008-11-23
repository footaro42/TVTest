#ifndef STATUS_VIEW_H
#define STATUS_VIEW_H


#include "BasicWindow.h"
#include "Tracer.h"
#include "PointerArray.h"


class CStatusView;

class CStatusItem {
protected:
	CStatusView *m_pStatus;
	int m_ID;
	int m_DefaultWidth;
	int m_Width;
	int m_MinWidth;
	bool m_fVisible;
	bool Update();
	bool GetMenuPos(POINT *pPos);
	void DrawText(HDC hdc,const RECT *pRect,LPCTSTR pszText) const;
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
	friend CStatusView;
};

class CStatusViewEventHandler {
protected:
	CStatusView *m_pStatusView;
public:
	CStatusViewEventHandler();
	virtual ~CStatusViewEventHandler();
	virtual void OnMouseLeave() {}
	friend CStatusView;
};

class CStatusView : public CBasicWindow, public CTracer {
	static HINSTANCE m_hinst;
	HFONT m_hfontStatus;
	int m_FontHeight;
	COLORREF m_crBackColor1;
	COLORREF m_crBackColor2;
	COLORREF m_crTextColor;
	COLORREF m_crHighlightBackColor1;
	COLORREF m_crHighlightBackColor2;
	COLORREF m_crHighlightTextColor;
	enum { MAX_STATUS_ITEMS=16 };
	CPointerVector<CStatusItem> m_ItemList;
	int m_NumItems;
	bool m_fSingleMode;
	LPTSTR m_pszSingleText;
	int m_HotItem;
	bool m_fTrackMouseEvent;
	CStatusViewEventHandler *m_pEventHandler;
	static CStatusView *GetStatusView(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,
												WPARAM wParam,LPARAM lParam);
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
	void SetSingleText(LPCTSTR pszText);
	void SetColor(COLORREF crBack1,COLORREF crBack2,COLORREF crText,
		COLORREF crHighlightBack1,COLORREF crHighlightBack2,COLORREF crHighlightText);
	bool SetFont(HFONT hfont);
	int GetCurItem() const;
	bool SetEventHandler(CStatusViewEventHandler *pEventHandler);
	bool SetItemOrder(const int *pOrderList);
	bool DrawItemPreview(CStatusItem *pItem,HDC hdc,const RECT *pRect,bool fHighlight=false) const;
	// CTracer
	void OnTrace(LPCTSTR pszOutput);
};


#endif
