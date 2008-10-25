#pragma once


class CBonException
{
	LPTSTR m_pszText;
	LPTSTR m_pszAdvise;
	int m_ErrorCode;
	void SetText(LPCTSTR pszText);
	void SetAdvise(LPCTSTR pszAdvise);
	void Clear();
public:
	CBonException();
	CBonException(LPCTSTR pszText,LPCTSTR pszAdvise=NULL);
	CBonException(int ErrorCode,LPCTSTR pszText,LPCTSTR pszAdvise=NULL);
	CBonException(const CBonException &Exception);
	~CBonException();
	CBonException &operator=(const CBonException &Exception);
	LPCTSTR GetText() const { return m_pszText; }
	LPCTSTR GetAdvise() const { return m_pszAdvise; }
	int GetErrorCode() const { return m_ErrorCode; }
	friend class CBonErrorHandler;
};

class CBonErrorHandler
{
	CBonException m_Exception;
protected:
	void SetErrorText(LPCTSTR pszText);
	void SetAdvise(LPCTSTR pszAdvise);
	void SetErrorCode(int ErrorCode);
	void SetError(int ErrorCode,LPCTSTR pszText,LPCTSTR pszAdvise=NULL);
	void SetError(const CBonException &Exception);
	void ClearError();
public:
	CBonErrorHandler();
	CBonErrorHandler(const CBonErrorHandler &ErrorHandler);
	virtual ~CBonErrorHandler();
	CBonErrorHandler &operator=(const CBonErrorHandler &ErrorHandler);
	LPCTSTR GetLastErrorText() const;
	LPCTSTR GetLastErrorAdvise() const;
	int GetLastErrorCode() const;
	const CBonException &GetLastErrorException() const { return m_Exception; }
	void FormatLastErrorText(LPTSTR pszText,int MaxLength) const;
};
