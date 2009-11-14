#ifndef STREAM_INFO_H
#define STREAM_INFO_H


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
	bool SetEventHandler(CEventHandler *pHandler);

private:
	CEventHandler *m_pEventHandler;
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	void SetService();
	static int GetTreeViewText(HWND hwndTree,HTREEITEM hItem,bool fSiblings,LPTSTR pszText,int MaxText,int Level=0);
};


#endif
