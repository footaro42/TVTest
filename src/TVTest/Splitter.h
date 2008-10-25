#ifndef SPLITTER_H
#define SPLITTER_H


#include "BasicWindow.h"


class CSplitter : public CBasicWindow {
	DWORD m_Style;
	int m_BarWidth;
	int m_BarPos;
	struct PaneInfo {
		CBasicWindow *pWindow;
		int ID;
		bool fVisible;
		int MinSize;
	};
	PaneInfo m_PaneList[2];
	int m_FixedPane;
	void SetSplitter();
	static HINSTANCE m_hinst;
	static CSplitter *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	enum {
		STYLE_VERT =0x0000,
		STYLE_HORZ =0x0001,
		STYLE_FIXED=0x0002
	};
	CSplitter();
	~CSplitter();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool SetBarPos(int Pos);
	int GetBarPos() const;
	int GetBarWidth() const { return m_BarWidth; }
	bool SetPane(int Index,CBasicWindow *pWindow,int ID);
	CBasicWindow *GetPane(int Index) const;
	int IDToIndex(int ID) const;
	bool SetPaneVisible(int ID,bool fVisible);
	bool SetMinSize(int ID,int Size);
	int GetPaneSize(int ID) const;
	bool SetPaneSize(int ID,int Size);
	bool SetFixedPane(int ID);
	bool SetStyle(DWORD Style);
	bool SwapPane();
	static bool Initialize(HINSTANCE hinst);
};


#endif
