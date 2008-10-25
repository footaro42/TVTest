#include "stdafx.h"
#include "CardReader.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CCardReader::CCardReader()
{
}


CCardReader::~CCardReader()
{
}


LPCTSTR CCardReader::EnumReader(int Index) const
{
	if (Index!=0)
		return NULL;
	return GetReaderName();
}


CCardReader *CCardReader::CreateCardReader(ReaderType Type)
{
	CCardReader *pReader;

	switch (Type) {
	case READER_SCARD:
		pReader=new CSCardReader;
		break;
	case READER_HDUS:
		pReader=new CHdusCardReader;
		break;
	default:
		return NULL;
	}
	pReader->m_ReaderType=Type;
	return pReader;
}




///////////////////////////////////////////////////////////////////////////////
//
// 標準スマートカードリーダー
//
///////////////////////////////////////////////////////////////////////////////


#pragma comment(lib, "WinScard.lib")


CSCardReader::CSCardReader()
{
	m_hBcasCard=NULL;
	m_bIsEstablish=false;
	m_pReaderList=NULL;
	m_NumReaders=0;
	m_pszReaderName=NULL;
	if (::SCardEstablishContext(SCARD_SCOPE_USER,NULL,NULL,&m_ScardContext)==SCARD_S_SUCCESS) {
		m_bIsEstablish=true;

		// カードリーダを列挙する
		DWORD dwBuffSize = 0UL;

		if (::SCardListReaders(m_ScardContext,NULL,NULL,&dwBuffSize)==SCARD_S_SUCCESS) {
			m_pReaderList=new TCHAR[dwBuffSize];
			if (::SCardListReaders(m_ScardContext,NULL,m_pReaderList,&dwBuffSize)==SCARD_S_SUCCESS) {
				LPCTSTR p=m_pReaderList;

				while (*p) {
					p+=lstrlen(p)+1;
					m_NumReaders++;
				}
			} else {
				delete [] m_pReaderList;
				m_pReaderList=NULL;
			}
		}
	}
}


CSCardReader::~CSCardReader()
{
	Close();
	if (m_bIsEstablish)
		::SCardReleaseContext(m_ScardContext);
	delete [] m_pReaderList;
	delete [] m_pszReaderName;
}


bool CSCardReader::Open(LPCTSTR pszReader)
{
	bool bOK=false;

	if (!m_bIsEstablish)
		return false;

	// 一旦クローズする
	Close();

	if (pszReader) {
		// 指定されたカードリーダに対してオープンを試みる
		DWORD dwActiveProtocol = SCARD_PROTOCOL_UNDEFINED;

		if (::SCardConnect(m_ScardContext,pszReader,SCARD_SHARE_SHARED,SCARD_PROTOCOL_T1,&m_hBcasCard,&dwActiveProtocol)!=SCARD_S_SUCCESS)
			return false;

		if (dwActiveProtocol!=SCARD_PROTOCOL_T1) {
			Close();
			return false;
		}
		bOK=true;
	} else {
		// 全てのカードリーダに対してオープンを試みる
		if (m_pReaderList==NULL)
			return false;

		LPCTSTR p=m_pReaderList;

		while (*p) {
			if (Open(p))
				return true;
			p+=lstrlen(p)+1;
		}
	}
	if (bOK) {
		m_pszReaderName=new TCHAR[::lstrlen(pszReader)+1];
		::lstrcpy(m_pszReaderName,pszReader);
	}
	return bOK;
}


void CSCardReader::Close()
{
	if (m_hBcasCard) {
		::SCardDisconnect(m_hBcasCard,SCARD_LEAVE_CARD);
		m_hBcasCard=NULL;
		delete [] m_pszReaderName;
		m_pszReaderName=NULL;
	}
}


LPCTSTR CSCardReader::GetReaderName() const
{
	return m_pszReaderName;
}


int CSCardReader::NumReaders() const
{
	return m_NumReaders;
}


LPCTSTR CSCardReader::EnumReader(int Index) const
{
	if (Index<0 || Index>=m_NumReaders)
		return NULL;
	LPCTSTR p=m_pReaderList;
	for (int i=0;i<Index;i++)
		p+=lstrlen(p)+1;
	return p;
}


bool CSCardReader::Transmit(const void *pSendData,DWORD SendSize,void *pRecvData,DWORD *pRecvSize)
{
	if (m_hBcasCard==NULL)
		return false;
	return ::SCardTransmit(m_hBcasCard,SCARD_PCI_T1,(LPCBYTE)pSendData,SendSize,NULL,(LPBYTE)pRecvData,pRecvSize)==SCARD_S_SUCCESS;
}




///////////////////////////////////////////////////////////////////////////////
//
// HDUS内蔵カードリーダー
//
///////////////////////////////////////////////////////////////////////////////


static const GUID CLSID_KSCATEGORY_BDA_NETWORK_TUNER
 = {0x71985F48,0x1CA1,0x11D3,{0x9C,0xC8,0x00,0xC0,0x4F,0x79,0x71,0xE0}};
static const GUID CLSID_PropSet
 = {0x9E1781E1,0x9CB1,0x4407,{0xBB,0xCE,0x54,0x26,0xC8,0xD0,0x0A,0x4B}};


CHdusCardReader::CHdusCardReader()
{
	m_pTuner=NULL;
	m_bSent=false;
	::CoInitialize(NULL);
}


CHdusCardReader::~CHdusCardReader()
{
	Close();
	::CoUninitialize();
}


bool CHdusCardReader::Open(LPCTSTR pszReader)
{
	Close();
	m_pTuner=FindDevice(CLSID_KSCATEGORY_BDA_NETWORK_TUNER,L"SKNET HDTV BDA Digital Tuner_0");
	if (m_pTuner==NULL)
		return false;
	return true;
}


void CHdusCardReader::Close()
{
	if (m_pTuner) {
		m_pTuner->Release();
		m_pTuner=NULL;
		m_bSent=false;
	}
}


LPCTSTR CHdusCardReader::GetReaderName() const
{
	return TEXT("HDUS Card Reader");
}


bool CHdusCardReader::Transmit(const void *pSendData,DWORD SendSize,void *pRecvData,DWORD *pRecvSize)
{
	if (m_pTuner==NULL)
		return false;

#ifdef USE_MUTEX
	HANDLE hMutex=::CreateMutex(NULL,TRUE,L"hduscard");

	if (hMutex==NULL) {
		return SCARD_F_INTERNAL_ERROR;
	} else if (::GetLastError()==ERROR_ALREADY_EXISTS) {
		::WaitForSingleObject(hMutex,INFINITE);
	}
#endif

	bool bOK=false;

	if (SUCCEEDED(Send(pSendData,SendSize))) {
		::Sleep(50);
		if (SUCCEEDED(Receive(pRecvData,pRecvSize)))
			bOK=true;
	}

#ifdef USE_MUTEX
	::ReleaseMutex(hMutex);
	::CloseHandle(hMutex);
#endif
	return bOK;
}


IBaseFilter *CHdusCardReader::FindDevice(REFCLSID category,BSTR varFriendlyName)
{
	HRESULT hr;
	ICreateDevEnum *pSysDevEnum;
	IEnumMoniker *pEnumCat;
	IMoniker *pMoniker;
	IPropertyBag *pPropBag;
	IBaseFilter *pFilter=NULL;
	ULONG cFetched;
	VARIANT varName;

	hr=::CoCreateInstance(CLSID_SystemDeviceEnum,NULL,CLSCTX_INPROC,
							IID_ICreateDevEnum,(void**)&pSysDevEnum);
	if (FAILED(hr))
		return NULL;
	hr=pSysDevEnum->CreateClassEnumerator(category,&pEnumCat,0);
	if (hr!=S_OK) {
		pSysDevEnum->Release();
		return NULL;
	}
	::VariantInit(&varName);
	while (pEnumCat->Next(1,&pMoniker,&cFetched)==S_OK) {
		hr=pMoniker->BindToStorage(0,0,IID_IPropertyBag,(void **)&pPropBag);
		if (FAILED(hr)) {
			pMoniker->Release();
			break;
		}
		hr=pPropBag->Read(L"FriendlyName",&varName,NULL);
		pPropBag->Release();
		if (FAILED(hr) || ::lstrcmpW(varName.bstrVal,varFriendlyName)!=0) {
			pMoniker->Release();
			continue;
		}
		hr=pMoniker->BindToObject(NULL,NULL,IID_IBaseFilter,(void**)&pFilter);
		pMoniker->Release();
		if (SUCCEEDED(hr))
			break;
	}
	::VariantClear(&varName);
	pEnumCat->Release();
	pSysDevEnum->Release();
	return pFilter;
}


static inline DWORD DiffTime(DWORD Start,DWORD End)
{
	if (Start<=End)
		return End-Start;
	return 0xFFFFFFFFUL-Start+1+End;
}


HRESULT CHdusCardReader::Send(const void *pSendData,DWORD SendSize)
{
	IKsPropertySet *pKsPropertySet;
	HRESULT hr;
	BYTE Buffer[0x42];
	DWORD Time;

	if (SendSize>0x3C)
		return E_FAIL;
	hr=m_pTuner->QueryInterface(IID_IKsPropertySet,(void**)&pKsPropertySet);
	if (FAILED(hr))
		return hr;
	::ZeroMemory(Buffer,sizeof(Buffer));
	Buffer[1]=(BYTE)SendSize;
	::CopyMemory(Buffer+2,pSendData,SendSize);
	Time=::GetTickCount();
	do {
		hr=pKsPropertySet->Set(CLSID_PropSet,0x10,NULL,0,Buffer,0x42);
		if (m_bSent)
			break;
		if (hr==0x8007001F)
			::Sleep(50);
	} while (hr==0x8007001F && DiffTime(Time,::GetTickCount())<2000);
	/*
	if (hr!=S_OK) {
		TCHAR szErr[128];
		::wsprintf(szErr,TEXT("CHdusCardReader::Send Error %08X\n"),hr);
		::OutputDebugString(szErr);
	}
	*/
	pKsPropertySet->Release();
	m_bSent=true;
	return hr;
}


HRESULT CHdusCardReader::Receive(void *pRecvData,DWORD *pRecvSize)
{
	IKsPropertySet *pKsPropertySet;
	HRESULT hr;
	BYTE Buffer[0x42];
	DWORD Time,Returned;

	if (*pRecvSize<0x3C)
		return E_FAIL;
	hr=m_pTuner->QueryInterface(IID_IKsPropertySet,(void**)&pKsPropertySet);
	if (FAILED(hr))
		return hr;
	Time=::GetTickCount();
	Buffer[0]=0;
	do {
		hr=pKsPropertySet->Get(CLSID_PropSet,0x11,NULL,0,Buffer,0x42,&Returned);
		if (hr!=S_OK) {
			/*
			TCHAR szErr[128];
			::wsprintf(szErr,TEXT("CHdusCardReader::Recieve Error %08X\n"),hr);
			::OutputDebugString(szErr);
			*/
			pKsPropertySet->Release();
			return hr;
		}
		if (Buffer[0]!=1)
			::Sleep(50);
	} while (Buffer[0]!=1 && DiffTime(Time,::GetTickCount())<2000);
	pKsPropertySet->Release();
	if (Buffer[0]!=1)
		return E_FAIL;
	if (*pRecvSize<Buffer[1])
		return E_FAIL;
	::CopyMemory(pRecvData,Buffer+2,Buffer[1]);
	*pRecvSize=Buffer[1];
	return S_OK;
}
