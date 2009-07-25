#ifndef STREAM_INFO_H
#define STREAM_INFO_H


#include <commctrl.h>
#include "Dialog.h"


class CStreamInfo : public CResizableDialog {
public:
	class CEventHandler {
	public:
		virtual ~CEventHandler() {}
		virtual bool OnClose() { return true; }
	};

	CStreamInfo();
	~CStreamInfo();
	bool Create(HWND hwndOwner);
	bool GetPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const;
	bool SetPosition(int Left,int Top,int Width,int Height);
	bool SetEventHandler(CEventHandler *pHandler);

private:
	struct {
		int x,y;
		int Width,Height;
		void Set(const RECT *pRect) {
			x=pRect->left;
			y=pRect->top;
			Width=pRect->right-x;
			Height=pRect->bottom-y;
		}
		void Get(RECT *pRect) const {
			pRect->left=x;
			pRect->top=y;
			pRect->right=x+Width;
			pRect->bottom=y+Height;
		}
	} m_WindowPosition;
	CEventHandler *m_pEventHandler;
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	void SetService();
	static int CopyTreeViewText(HWND hwndTree,HTREEITEM hItem,LPTSTR pszText,int MaxText,int Level=0);
};


#endif
