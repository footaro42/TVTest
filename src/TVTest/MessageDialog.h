#ifndef MESSAGE_DIALOG_H
#define MESSAGE_DIALOG_H


#include <richedit.h>


class CMessageDialog {
public:
	CMessageDialog();
	~CMessageDialog();
	enum MessageType {
		TYPE_INFO,
		TYPE_WARNING,
		TYPE_ERROR
	};
	bool Show(HWND hwndOwner,MessageType Type,LPCTSTR pszText,LPCTSTR pszTitle=NULL,LPCTSTR pszSystemMessage=NULL,LPCTSTR pszCaption=NULL);
private:
	HMODULE m_hLib;
	LPTSTR m_pszText;
	LPTSTR m_pszTitle;
	LPTSTR m_pszSystemMessage;
	LPTSTR m_pszCaption;
	MessageType m_MessageType;
	HWND m_hDlg;
	void LogFontToCharFormat(const LOGFONT *plf,CHARFORMAT *pcf);
	void AppendText(HWND hwndEdit,LPCTSTR pszText,const CHARFORMAT *pcf);
	static CMessageDialog *GetThis(HWND hDlg);
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
