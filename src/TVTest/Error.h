#ifndef TVTEST_ERROR_H
#define TVTEST_ERROR_H


#include <richedit.h>


class CErrorDialog {
public:
	CErrorDialog();
	~CErrorDialog();
	enum MessageType {
		TYPE_INFO,
		TYPE_WARNING,
		TYPE_ERROR
	};
	bool Show(HWND hwndOwner,MessageType Type,LPCTSTR pszText,LPCTSTR pszTitle=NULL,LPCTSTR pszSystemMessage=NULL);
private:
	HMODULE m_hLib;
	LPTSTR m_pszText;
	LPTSTR m_pszTitle;
	LPTSTR m_pszSystemMessage;
	MessageType m_MessageType;
	HWND m_hDlg;
	void LogFontToCharFormat(const LOGFONT *plf,CHARFORMAT *pcf);
	void AppendText(HWND hwndEdit,LPCTSTR pszText,const CHARFORMAT *pcf);
	static CErrorDialog *GetThis(HWND hDlg);
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
