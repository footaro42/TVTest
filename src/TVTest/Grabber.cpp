#include "stdafx.h"
#include <dshow.h>
#include <Dvdmedia.h>
#include <Streams.h>
#include "Grabber.h"
#include "Image.h"


// {CDD88552-C42F-4b05-8A79-DE816BC47F29}
DEFINE_GUID(CLSID_GrabberFilter, 
0xcdd88552, 0xc42f, 0x4b05, 0x8a, 0x79, 0xde, 0x81, 0x6b, 0xc4, 0x7f, 0x29);




class CGrabberFilter : public CTransInPlaceFilter
{
	HANDLE m_hCaptureEvent;
	HANDLE m_hComplete;
	void *m_pBuffer;
	LONG m_DataSize;
	AM_MEDIA_TYPE *m_pMediaType;
	~CGrabberFilter();
	CGrabberFilter(IUnknown *pUnk,HRESULT *phr);
public:
	static CUnknown *CreateInstance(IUnknown *pUnk,HRESULT *phr);

	bool SetCapture(bool fCapture);
	bool WaitCapture(DWORD WaitTime);

	// CTransInPlaceFilter
	HRESULT CheckInputType(const CMediaType *mtIn);
	HRESULT Transform(IMediaSample *pSample);

#if 0
	// CTransformFilter
	HRESULT CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut);
	HRESULT Transform(IMediaSample *pIn,IMediaSample *pOut);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc,
											ALLOCATOR_PROPERTIES *pProperties);
	HRESULT GetMediaType(int iPosition,CMediaType *pMediaType);
	HRESULT CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin);
#endif

	AM_MEDIA_TYPE *GetMediaType() const { return m_pMediaType; }
	void *GetBuffer() const { return m_pBuffer; }
	SIZE_T GetBufferSize() const { return m_DataSize; }
};


CGrabberFilter::CGrabberFilter(IUnknown *pUnk,HRESULT *phr) :
	CTransInPlaceFilter(TEXT("GrabberFilter"),pUnk,CLSID_GrabberFilter,false)
	//CTransformFilter(TEXT("GrabberFilter"),pUnk,CLSID_GrabberFilter)
{
	m_hCaptureEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
	m_hComplete=CreateEvent(NULL,FALSE,FALSE,NULL);
	m_pBuffer=NULL;
	m_DataSize=0;
	m_pMediaType=NULL;
	*phr=S_OK;
}


CGrabberFilter::~CGrabberFilter()
{
	if (m_pBuffer!=NULL)
		delete [] m_pBuffer;
	if (m_pMediaType!=NULL)
		DeleteMediaType(m_pMediaType);
	CloseHandle(m_hCaptureEvent);
	CloseHandle(m_hComplete);
}


CUnknown *CGrabberFilter::CreateInstance(IUnknown *pUnk,HRESULT *phr)
{
	CGrabberFilter *pNewFilter=new CGrabberFilter(pUnk,phr);
	/*
	if (pNewFilter==NULL)
		*phr=E_OUTOFMEMORY;
	*/
	return pNewFilter;
}


bool CGrabberFilter::SetCapture(bool fCapture)
{
	if (fCapture) {
		if (m_pBuffer!=NULL) {
			delete [] m_pBuffer;
			m_pBuffer=NULL;
			m_DataSize=0;
		}
		SetEvent(m_hCaptureEvent);
	} else {
		ResetEvent(m_hCaptureEvent);
	}
	ResetEvent(m_hComplete);
	return true;
}


bool CGrabberFilter::WaitCapture(DWORD WaitTime)
{
	if (WaitForSingleObject(m_hComplete,WaitTime)!=WAIT_OBJECT_0)
		return false;
	return true;
}


HRESULT CGrabberFilter::CheckInputType(const CMediaType *mtIn)
{
	if (*mtIn->FormatType()!=FORMAT_VideoInfo2
			&& *mtIn->FormatType()!=FORMAT_MPEG2Video)
		return VFW_E_TYPE_NOT_ACCEPTED;
	if (*mtIn->Subtype()!=MEDIASUBTYPE_RGB24
			&& *mtIn->Subtype()!=MEDIASUBTYPE_RGB32
			&& *mtIn->Subtype()!=MEDIASUBTYPE_RGB555
			&& *mtIn->Subtype()!=MEDIASUBTYPE_RGB565)
		return VFW_E_TYPE_NOT_ACCEPTED;
	return S_OK;
}


#if 1

HRESULT CGrabberFilter::Transform(IMediaSample *pSample)
{
	AM_MEDIA_TYPE *pMediaType;

	if (SUCCEEDED(pSample->GetMediaType(&pMediaType))) {
		if (pMediaType!=NULL) {
			if (m_pMediaType!=NULL)
				DeleteMediaType(pMediaType);
			m_pMediaType=pMediaType;
		}
	}
	if (WaitForSingleObject(m_hCaptureEvent,0)==WAIT_OBJECT_0) {
		if (m_pMediaType!=NULL) {
			if ((m_pMediaType->formattype==FORMAT_VideoInfo2
						|| m_pMediaType->formattype==FORMAT_MPEG2Video)
					&& m_pMediaType->cbFormat>=sizeof(VIDEOINFOHEADER2)
					&& m_pMediaType->pbFormat!=NULL) {
				LONG Size=pSample->GetActualDataLength();
				BYTE *pBuff;

				if (Size>0 && SUCCEEDED(pSample->GetPointer(&pBuff))) {
					m_pBuffer=new BYTE[Size];
					CopyMemory(m_pBuffer,pBuff,Size);
					m_DataSize=Size;
					SetEvent(m_hComplete);
				}
			}
		}
	}
	return S_OK;
}

#else

HRESULT CGrabberFilter::CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut)
{
	if (*mtIn!=*mtOut)
		return VFW_E_TYPE_NOT_ACCEPTED;
	return S_OK;
}


HRESULT CGrabberFilter::Transform(IMediaSample *pIn,IMediaSample *pOut)
{
	HRESULT hr;
	LONG SrcSize=pIn->GetActualDataLength();
	BYTE *pSrcBuff,*pDstBuff;
	AM_MEDIA_TYPE *pMediaType;

	pIn->GetPointer(&pSrcBuff);
	pOut->GetPointer(&pDstBuff);
	CopyMemory(pDstBuff,pSrcBuff,SrcSize);
	pOut->SetActualDataLength(SrcSize);

	REFERENCE_TIME ts,te;
	if (SUCCEEDED(pIn->GetTime(&ts,&te)))
		pOut->SetTime(&ts,&te);
	LONGLONG ms,me;
	if (SUCCEEDED(pIn->GetMediaTime(&ms,&me)))
		pOut->SetMediaTime(&ms,&me);

	hr=pIn->IsSyncPoint();
	if (hr==S_OK)
		pOut->SetSyncPoint(TRUE);
	else if (hr==S_FALSE)
		pOut->SetSyncPoint(FALSE);
	else
		return E_UNEXPECTED;

	pIn->GetMediaType(&pMediaType);
	pOut->SetMediaType(pMediaType);

	hr=pIn->IsDiscontinuity();
	if (hr==S_OK)
		pOut->SetDiscontinuity(TRUE);
	else if (hr==S_FALSE)
		pOut->SetDiscontinuity(FALSE);

	if (pMediaType!=NULL) {
		if (m_pMediaType!=NULL)
			DeleteMediaType(pMediaType);
		m_pMediaType=pMediaType;
	}
	if (WaitForSingleObject(m_hCaptureEvent,0)==WAIT_OBJECT_0) {
		if (m_pMediaType!=NULL) {
			if ((m_pMediaType->formattype==FORMAT_VideoInfo2
						|| m_pMediaType->formattype==FORMAT_MPEG2Video)
					&& m_pMediaType->cbFormat>=sizeof(VIDEOINFOHEADER2)
					&& m_pMediaType->pbFormat!=NULL) {
				m_pBuffer=new BYTE[SrcSize];
				CopyMemory(m_pBuffer,pSrcBuff,SrcSize);
				m_DataSize=SrcSize;
				SetEvent(m_hComplete);
			}
		}
	}
	return S_OK;
}


HRESULT CGrabberFilter::DecideBufferSize(IMemAllocator *pAlloc,
											ALLOCATOR_PROPERTIES *pProperties)
{
	HRESULT hr;
	ALLOCATOR_PROPERTIES Actual;

	if (!m_pInput->IsConnected())
		return E_UNEXPECTED;
	pProperties->cBuffers=1;
	pProperties->cbBuffer=m_pInput->CurrentMediaType().GetSampleSize();
	hr=pAlloc->SetProperties(pProperties,&Actual);
	if (FAILED(hr))
		return hr;
	if (pProperties->cBuffers>Actual.cBuffers
			|| pProperties->cbBuffer>Actual.cbBuffer)
		return E_FAIL;
	return S_OK;
}


HRESULT CGrabberFilter::GetMediaType(int iPosition,CMediaType *pMediaType)
{
	if (m_pInput->IsConnected()==FALSE )
		return E_UNEXPECTED;
	if (iPosition<0)
		return E_INVALIDARG;
	if (0<iPosition )
		return VFW_S_NO_MORE_ITEMS;
	*pMediaType=m_pInput->CurrentMediaType();
	return S_OK;
}


HRESULT CGrabberFilter::CompleteConnect(PIN_DIRECTION direction,
															IPin *pReceivePin)
{
	return S_OK;
}

#endif


// Code from MSDN Library
void GetVideoInfoParameters(
	const VIDEOINFOHEADER2 *pvih, // フォーマット ヘッダーへのポインタ。
	const BYTE *pbData,   // バッファ内の最初のアドレスへのポインタ。
	bool bYuv,      // これは YUV フォーマットか? (true = YUV, false = RGB)
	DWORD *pdwWidth,        // 幅 (ピクセル単位) を返す。
	DWORD *pdwHeight,       // 高さ (ピクセル単位) を返す。
	LONG  *plStrideInBytes, // 新しい行を下げるために行にこれを追加する。
	const BYTE **ppbTop          // ピクセルの最上位行の最初のバイトへのポインタを
                            // 返す。
	)
{
	LONG lStride;

	//  '標準' のフォーマットの場合、biWidth はピクセル単位である。 
	//  バイトに拡張し、4 の倍数に切り上げる。
	if ((pvih->bmiHeader.biBitCount != 0) &&
		(0 == (7 & pvih->bmiHeader.biBitCount))) 
	{
		lStride = (pvih->bmiHeader.biWidth * (pvih->bmiHeader.biBitCount / 8) + 3) & ~3;
	} 
	else   // それ以外の場合は、biWidth はバイト単位である。
	{
		lStride = pvih->bmiHeader.biWidth;
	}

	//  rcTarget が空である場合は、イメージ全体を使用する。
	if (IsRectEmpty(&pvih->rcTarget)) 
	{
		*pdwWidth = (DWORD)pvih->bmiHeader.biWidth;
		*pdwHeight = (DWORD)(abs(pvih->bmiHeader.biHeight));

		if (pvih->bmiHeader.biHeight < 0 || bYuv)   // トップダウン ビットマップ。 
		{
			*plStrideInBytes = lStride; // ストライドは "下" へ向かう。
			*ppbTop           = pbData; // 最上位行が最初である。
		} 
		else        // ボトムアップ ビットマップ。
		{
			*plStrideInBytes = -lStride;    // ストライドは "上" へ向かう。
			// 最下位行が最初である。
			*ppbTop = pbData + lStride * (*pdwHeight - 1);  
		}
	} 
	else   // rcTarget は空ではない。 イメージ内のサブ矩形を使用する。
	{
		*pdwWidth = (DWORD)(pvih->rcTarget.right - pvih->rcTarget.left);
		*pdwHeight = (DWORD)(pvih->rcTarget.bottom - pvih->rcTarget.top);

		if (pvih->bmiHeader.biHeight < 0 || bYuv)   // トップダウン ビットマップ。
		{
			// 上と同じストライドだが、最初のピクセルがターゲット矩形によって、
			// 次のように変更されている。
			*plStrideInBytes = lStride;     
			*ppbTop = pbData +
				lStride * pvih->rcTarget.top +
				(pvih->bmiHeader.biBitCount * pvih->rcTarget.left) / 8;
		} 
		else  // ボトムアップ ビットマップ。
		{
			*plStrideInBytes = -lStride;
			*ppbTop = pbData +
				lStride * (pvih->bmiHeader.biHeight - pvih->rcTarget.top - 1) +
				(pvih->bmiHeader.biBitCount * pvih->rcTarget.left) / 8;
		}
	}
}




CGrabber::CGrabber()
{
	m_pGrabberFilter=NULL;
}


CGrabber::~CGrabber()
{
	if (m_pGrabberFilter!=NULL)
		m_pGrabberFilter->Release();
}


bool CGrabber::Init()
{
	HRESULT hr;

	if (m_pGrabberFilter==NULL)
		m_pGrabberFilter=dynamic_cast<CGrabberFilter*>(
									CGrabberFilter::CreateInstance(NULL,&hr));
	m_pGrabberFilter->AddRef();
	return true;
}


IBaseFilter *CGrabber::GetGrabberFilter()
{
	return m_pGrabberFilter;
}


bool CGrabber::SetCapture(bool fCapture)
{
	return m_pGrabberFilter->SetCapture(fCapture);
}


bool CGrabber::WaitCapture(DWORD WaitTime)
{
	if (!m_pGrabberFilter->WaitCapture(WaitTime))
		return false;
	return true;
}


void *CGrabber::GetCaptureBitmap() const
{
	const AM_MEDIA_TYPE *pMediaType;
	const void *pBuffer;
	const VIDEOINFOHEADER2 *pvih2;
	DWORD Width,Height;
	LONG RowStride;
	const BYTE *pTop;
	BITMAPINFOHEADER bmih;
	SIZE_T Size;
	void *pDIB;
	SIZE_T DstRowBytes;
	BYTE *p;

	pMediaType=m_pGrabberFilter->GetMediaType();
	pBuffer=m_pGrabberFilter->GetBuffer();
	if (pMediaType==NULL || pBuffer==NULL)
		return NULL;
	pvih2=(VIDEOINFOHEADER2*)pMediaType->pbFormat;
	GetVideoInfoParameters(pvih2,static_cast<const BYTE*>(pBuffer),false,
											&Width,&Height,&RowStride,&pTop);
	bmih.biSize=sizeof(BITMAPINFOHEADER);
	bmih.biWidth=Width;
	bmih.biHeight=Height;
	bmih.biPlanes=1;
	bmih.biCompression=BI_RGB;
	if (pMediaType->subtype==MEDIASUBTYPE_RGB24) {
		bmih.biBitCount=24;
	} else if (pMediaType->subtype==MEDIASUBTYPE_RGB32) {
		bmih.biBitCount=32;
	} else if (pMediaType->subtype==MEDIASUBTYPE_RGB565) {
		bmih.biBitCount=16;
	} else if (pMediaType->subtype==MEDIASUBTYPE_RGB555) {
		bmih.biBitCount=16;
		bmih.biCompression=BI_BITFIELDS;
	} else {
		return NULL;
	}
	bmih.biSizeImage=0;
	bmih.biXPelsPerMeter=0;
	bmih.biYPelsPerMeter=0;
	bmih.biClrUsed=0;
	bmih.biClrImportant=0;
	Size=CalcDIBSize(&bmih);
	pDIB=CoTaskMemAlloc(Size);
	if (pDIB==NULL)
		return NULL;
	CopyMemory(pDIB,&bmih,sizeof(BITMAPINFOHEADER));
	p=(BYTE*)pDIB+sizeof(BITMAPINFOHEADER);
	if (pMediaType->subtype==MEDIASUBTYPE_RGB555) {
		DWORD BitFields[3];

		BitFields[0]=0x00007C00;
		BitFields[1]=0x000003E0;
		BitFields[2]=0x0000001F;
		CopyMemory(p,BitFields,sizeof(BitFields));
		p+=sizeof(BitFields);
	}
	DstRowBytes=DIB_ROW_BYTES(Width,bmih.biBitCount);
	p=p+(Height-1)*DstRowBytes;
	for (DWORD y=0;y<Height;y++) {
		CopyMemory(p,pTop,DstRowBytes);
		pTop+=RowStride;
		p-=DstRowBytes;
	}
	return pDIB;
}
