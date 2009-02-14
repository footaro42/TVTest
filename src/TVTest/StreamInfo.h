#ifndef STREAM_INFO_H
#define STREAM_INFO_H


class CStreamInfo {
	HWND m_hDlg;
	POINT m_WindowPos;
	static CStreamInfo *GetThis(HWND hDlg);
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	CStreamInfo();
	~CStreamInfo();
	bool Show(HWND hwndOwner);
};


#endif
