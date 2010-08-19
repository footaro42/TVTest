#ifndef TVTEST_LAYOUT_H
#define TVTEST_LAYOUT_H


#include <vector>
#include "BasicWindow.h"
#include "DrawUtil.h"


namespace Layout
{

class CLayoutBase;

class ABSTRACT_CLASS(CContainer)
{
public:
	CContainer(int ID);
	virtual ~CContainer()=0;
	virtual void SetPosition(const RECT &Pos);
	virtual void GetMinSize(SIZE *pSize) const;
	virtual void SetVisible(bool fVisible);
	virtual int NumChildContainers() const;
	virtual CContainer *GetChildContainer(int Index) const;
	virtual void OnLButtonDown(int x,int y) {}
	virtual void OnLButtonUp(int x,int y) {}
	virtual void OnMouseMove(int x,int y) {}
	CLayoutBase *GetLayoutBase() const { return m_pBase; }
	void SetID(int ID) { m_ID=ID; }
	int GetID() const { return m_ID; }
	bool GetVisible() const { return m_fVisible; }
	const RECT &GetPosition() const { return m_Position; }

protected:
	CLayoutBase *m_pBase;
	int m_ID;
	bool m_fVisible;
	RECT m_Position;

	friend CLayoutBase;
};

class CWindowContainer : public CContainer
{
	CBasicWindow *m_pWindow;
	int m_MinWidth;
	int m_MinHeight;

public:
	CWindowContainer(int ID);
	~CWindowContainer();
// CContainer
	void SetPosition(const RECT &Pos);
	void GetMinSize(SIZE *pSize) const;
	void SetVisible(bool fVisible);
// CWindowContainer
	void SetWindow(CBasicWindow *pWindow);
	CBasicWindow *GetWindow() const { return m_pWindow; }
	bool SetMinSize(int Width,int Height);
};

class CSplitter : public CContainer
{
	struct PaneInfo {
		CContainer *pContainer;
		int FixedSize;
		PaneInfo() : pContainer(NULL), FixedSize(-1) {}
	};
	PaneInfo m_PaneList[2];
	unsigned int m_Style;
	int m_AdjustPane;
	int m_BarPos;
	int m_BarWidth;

	void Adjust();
	bool GetBarRect(RECT *pRect) const;

public:
	enum {
		STYLE_HORZ	=0x0000,
		STYLE_VERT	=0x0001,
		STYLE_FIXED	=0x0002
	};

	CSplitter(int ID);
	~CSplitter();
// CContainer
	void SetPosition(const RECT &Pos);
	void GetMinSize(SIZE *pSize) const;
	int NumChildContainers() const;
	CContainer *GetChildContainer(int Index) const;
	void OnLButtonDown(int x,int y);
	void OnLButtonUp(int x,int y);
	void OnMouseMove(int x,int y);
// CSplitter
	bool SetPane(int Index,CContainer *pContainer);
	CContainer *GetPane(int Index) const;
	CContainer *GetPaneByID(int ID) const;
	void SwapPane();
	bool SetPaneSize(int ID,int Size);
	int GetPaneSize(int ID);
	int IDToIndex(int ID) const;
	bool SetStyle(unsigned int Style,bool fAdjust=true);
	unsigned int GetStyle() const { return m_Style; }
	bool SetAdjustPane(int ID);
	bool SetBarPos(int Pos);
	int GetBarPos() const { return m_BarPos; }
	int GetBarWidth() const { return m_BarWidth; }
};


class CLayoutBase : public CBasicWindow
{
public:
	class ABSTRACT_CLASS(CEventHandler) {
	protected:
		CLayoutBase *m_pBase;
	public:
		CEventHandler();
		virtual ~CEventHandler()=0;
		friend CLayoutBase;
	};

	CLayoutBase();
	~CLayoutBase();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool SetTopContainer(CContainer *pContainer);
	CContainer *GetTopContainer() const { return m_pContainer; }
	CContainer *GetContainerByID(int ID) const;
	CContainer *GetContainerFromPoint(int x,int y) const;
	bool SetContainerVisible(int ID,bool fVisible);
	bool GetContainerVisible(int ID) const;
	void GetMinSize(SIZE *pSize) const;
	void Adjust();
	void SetEventHandler(CEventHandler *pHandler);
	bool SetBackColor(COLORREF Color);
	COLORREF GetBackColor() const { return m_BackColor; }
	HBRUSH GetBackBrush();

	static bool Initialize(HINSTANCE hinst);

protected:
	CEventHandler *m_pEventHandler;
	CContainer *m_pContainer;
	CContainer *m_pFocusContainer;
	COLORREF m_BackColor;
	DrawUtil::CBrush m_BackBrush;

	void SetBasePointer(CContainer *pContainer,CLayoutBase *pBase);
	CContainer *GetChildContainerByID(const CContainer *pContainer,int ID) const;
	CContainer *GetChildContainerFromPoint(const CContainer *pContainer,int x,int y) const;

	static const LPCTSTR m_pszWindowClass;
	static HINSTANCE m_hinst;

	static CLayoutBase *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
};

}	// namespace Layout


#endif
