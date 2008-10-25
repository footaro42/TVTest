#ifndef CARD_READER_H
#define CARD_READER_H


#include <winscard.h>
#include <dshow.h>


class CCardReader
{
public:
	enum ReaderType {
		READER_NONE,
		READER_SCARD,
		READER_HDUS,
	};
	enum { READER_LAST=READER_HDUS };
private:
	ReaderType m_ReaderType;
public:
	CCardReader();
	virtual ~CCardReader();
	virtual bool Open(LPCTSTR pszReader)=0;
	virtual void Close()=0;
	virtual LPCTSTR GetReaderName() const=0;
	virtual int NumReaders() const { return 1; }
	virtual LPCTSTR EnumReader(int Index) const;
	ReaderType GetReaderType() const { return m_ReaderType; }
	virtual bool Transmit(const void *pSendData,DWORD SendSize,void *pRecvData,DWORD *pRecvSize)=0;
	static CCardReader *CreateCardReader(ReaderType Type);
};

class CSCardReader : public CCardReader
{
	SCARDCONTEXT m_ScardContext;
	SCARDHANDLE m_hBcasCard;
	bool m_bIsEstablish;
	LPTSTR m_pReaderList;
	int m_NumReaders;
	LPTSTR m_pszReaderName;
public:
	CSCardReader();
	~CSCardReader();
	bool Open(LPCTSTR pszReader);
	void Close();
	LPCTSTR GetReaderName() const;
	int NumReaders() const;
	LPCTSTR EnumReader(int Index) const;
	bool Transmit(const void *pSendData,DWORD SendSize,void *pRecvData,DWORD *pRecvSize);
};

class CHdusCardReader : public CCardReader
{
	IBaseFilter *m_pTuner;
	bool m_bSent;
	IBaseFilter *FindDevice(REFCLSID category,BSTR varFriendlyName);
	HRESULT Send(const void *pSendData,DWORD SendSize);
	HRESULT Receive(void *pRecvData,DWORD *pRecvSize);
public:
	CHdusCardReader();
	~CHdusCardReader();
	bool Open(LPCTSTR pszReader);
	void Close();
	LPCTSTR GetReaderName() const;
	bool Transmit(const void *pSendData,DWORD SendSize,void *pRecvData,DWORD *pRecvSize);
};


#endif
