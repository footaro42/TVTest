#ifndef CARD_READER_DIALOG_H
#define CARD_READER_DIALOG_H


#include "CoreEngine.h"


class CCardReaderErrorDialog
{
	CDynamicString m_Message;
	CCoreEngine::CardReaderType m_ReaderType;
	CDynamicString m_ReaderName;

	static CCardReaderErrorDialog *GetThis(HWND hDlg);
	static INT_PTR CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	CCardReaderErrorDialog();
	~CCardReaderErrorDialog();
	bool Show(HWND hwndOwner);
	bool SetMessage(LPCTSTR pszMessage);
	CCoreEngine::CardReaderType GetReaderType() const { return m_ReaderType; }
	LPCTSTR GetReaderName() const { return m_ReaderName.Get(); }
};


#endif
