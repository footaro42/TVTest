#include "stdafx.h"
#include <d3d9.h>
#include <evr.h>
#include <evr9.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include "EVRenderer.h"
#include "DirectShowUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#pragma comment(lib,"mfuuid.lib")


typedef HRESULT (WINAPI *MFStartupFunc)(ULONG Version,DWORD dwFlags);
typedef HRESULT (WINAPI *MFShutdownFunc)();




static IMFVideoDisplayControl *GetVideoDisplayControl(IBaseFilter *pRenderer)
{
	HRESULT hr;
	IMFGetService *pGetService;
	IMFVideoDisplayControl *pDisplayControl;

	hr=pRenderer->QueryInterface(IID_IMFGetService,reinterpret_cast<LPVOID*>(&pGetService));
	if (FAILED(hr))
		return NULL;
	hr=pGetService->GetService(MR_VIDEO_RENDER_SERVICE,IID_IMFVideoDisplayControl,reinterpret_cast<LPVOID*>(&pDisplayControl));
	pGetService->Release();
	if (FAILED(hr))
		return NULL;
	return pDisplayControl;
}




CVideoRenderer_EVR::CVideoRenderer_EVR()
{
	m_hMFPlatLib=NULL;
}


CVideoRenderer_EVR::~CVideoRenderer_EVR()
{
	/*
	if (m_hMFPlatLib) {
		MFShutdownFunc pShutdown=reinterpret_cast<MFShutdownFunc>(::GetProcAddress(m_hMFPlatLib,"MFShutdown"));

		if (pShutdown)
			pShutdown();
		::FreeLibrary(m_hMFPlatLib);
	}
	*/
}


bool CVideoRenderer_EVR::Initialize(IGraphBuilder *pFilterGraph,IPin *pInputPin,HWND hwndRender,HWND hwndMessageDrain)
{
	HRESULT hr;

	// MFStartupは呼ばなくていいらしい
	/*
	m_hMFPlatLib=::LoadLibrary(TEXT("mfplat.dll"));
	if (m_hMFPlatLib==NULL) {
		SetError(TEXT("mfplat.dllをロードできません。"));
		return false;
	}
	MFStartupFunc pStartup=reinterpret_cast<MFStartupFunc>(::GetProcAddress(m_hMFPlatLib,"MFStartup"));
	if (pStartup==NULL) {
		SetError(TEXT("MFStartup関数のアドレスを取得できません。"));
		goto OnError;
	}
	hr=pStartup(MF_VERSION,MFSTARTUP_LITE);
	if (FAILED(hr)) {
		SetError(TEXT("Media Foundationの初期化ができません。"));
		goto OnError;
	}
	*/

	hr=::CoCreateInstance(CLSID_EnhancedVideoRenderer,NULL,CLSCTX_INPROC_SERVER,
						IID_IBaseFilter,reinterpret_cast<LPVOID*>(&m_pRenderer));
	if (FAILED(hr)) {
		SetError(TEXT("EVRのインスタンスを作成できません。"),
				 TEXT("システムがEVRに対応していない可能性があります。"));
		goto OnError;
	}

	IEVRFilterConfig *pFilterConfig;
	hr=m_pRenderer->QueryInterface(IID_IEVRFilterConfig,reinterpret_cast<LPVOID*>(&pFilterConfig));
	if (FAILED(hr)) {
		SetError(TEXT("IEVRFilterConfigを取得できません。"));
		goto OnError;
	}
	pFilterConfig->SetNumberOfStreams(1);
	pFilterConfig->Release();

	hr=pFilterGraph->AddFilter(m_pRenderer,L"VMR7");
	if (FAILED(hr)) {
		SetError(TEXT("EVRをフィルタグラフに追加できません。"));
		goto OnError;
	}

	IFilterGraph2 *pFilterGraph2;
	hr=pFilterGraph->QueryInterface(IID_IFilterGraph2,
									reinterpret_cast<LPVOID*>(&pFilterGraph2));
	if (FAILED(hr)) {
		SetError(TEXT("IFilterGraph2を取得できません。"));
		goto OnError;
	}
	hr=pFilterGraph2->RenderEx(pInputPin,
								AM_RENDEREX_RENDERTOEXISTINGRENDERERS,NULL);
	pFilterGraph2->Release();
	if (FAILED(hr)) {
		SetError(TEXT("映像レンダラを構築できません。"));
		goto OnError;
	}

	IMFGetService *pGetService;
	hr=m_pRenderer->QueryInterface(IID_IMFGetService,reinterpret_cast<LPVOID*>(&pGetService));
	if (FAILED(hr)) {
		SetError(TEXT("IMFGetServiceを取得できません。"));
		goto OnError;
	}

	IMFVideoDisplayControl *pDisplayControl;
	hr=pGetService->GetService(MR_VIDEO_RENDER_SERVICE,IID_IMFVideoDisplayControl,reinterpret_cast<LPVOID*>(&pDisplayControl));
	if (FAILED(hr)) {
		pGetService->Release();
		SetError(TEXT("IMFVideoDisplayControlを取得できません。"));
		goto OnError;
	}
	pDisplayControl->SetVideoWindow(hwndRender);
	pDisplayControl->SetAspectRatioMode(MFVideoARMode_None);
	/*
	RECT rc;
	::GetClientRect(hwndRender,&rc);
	pDisplayControl->SetVideoPosition(NULL,&rc);
	*/
	pDisplayControl->SetBorderColor(RGB(0,0,0));
	pDisplayControl->Release();

	IMFVideoProcessor *pVideoProcessor;
	hr=pGetService->GetService(MR_VIDEO_MIXER_SERVICE,IID_IMFVideoProcessor,reinterpret_cast<LPVOID*>(&pVideoProcessor));
	if (FAILED(hr)) {
		pGetService->Release();
		SetError(TEXT("IMFVideoProcessorを取得できません。"));
		goto OnError;
	}
	pVideoProcessor->SetBackgroundColor(RGB(0,0,0));
/*
	UINT NumModes;
	GUID *pProcessingModes;
	if (SUCCEEDED(pVideoProcessor->GetAvailableVideoProcessorModes(&NumModes,&pProcessingModes))) {
#ifdef _DEBUG
		for (UINT i=0;i<NumModes;i++) {
			DXVA2_VideoProcessorCaps Caps;

			if (SUCCEEDED(pVideoProcessor->GetVideoProcessorCaps(&pProcessingModes[i],&Caps))) {
				TRACE(TEXT("EVR Video Processor %u\n"),i);
				TRACE(TEXT("DeviceCaps : %s\n"),
					  Caps.DeviceCaps==DXVA2_VPDev_EmulatedDXVA1?
						TEXT("DXVA2_VPDev_EmulatedDXVA1"):
					  Caps.DeviceCaps==DXVA2_VPDev_HardwareDevice?
						TEXT("DXVA2_VPDev_HardwareDevice"):
					  Caps.DeviceCaps==DXVA2_VPDev_SoftwareDevice?
						TEXT("DXVA2_VPDev_SoftwareDevice"):TEXT("Unknown"));
			}
		}
#endif
		for (UINT i=0;i<NumModes;i++) {
			DXVA2_VideoProcessorCaps Caps;

			if (SUCCEEDED(pVideoProcessor->GetVideoProcessorCaps(&pProcessingModes[i],&Caps))) {
				if (Caps.DeviceCaps==DXVA2_VPDev_HardwareDevice) {
					pVideoProcessor->SetVideoProcessorMode(&pProcessingModes[i]);
					break;
				}
			}
		}
		::CoTaskMemFree(pProcessingModes);
	}
*/
	pVideoProcessor->Release();

	pGetService->Release();

	m_pFilterGraph=pFilterGraph;
	m_hwndRender=hwndRender;

	ClearError();

	return true;

OnError:
	SAFE_RELEASE(m_pRenderer);
	/*
	if (m_hMFPlatLib) {
		::FreeLibrary(m_hMFPlatLib);
		m_hMFPlatLib=NULL;
	}
	*/
	return false;
}


bool CVideoRenderer_EVR::Finalize()
{
	SAFE_RELEASE(m_pRenderer);
	return true;
}


bool CVideoRenderer_EVR::SetVideoPosition(int SourceWidth,int SourceHeight,const RECT *pSourceRect,
								const RECT *pDestRect,const RECT *pWindowRect)
{
	bool fOK=false;

	if (m_pRenderer) {
		IMFVideoDisplayControl *pDisplayControl=GetVideoDisplayControl(m_pRenderer);

		if (pDisplayControl) {
			MFVideoNormalizedRect rcSrc;
			RECT rcDest;

			rcSrc.left=(float)pSourceRect->left/(float)SourceWidth;
			rcSrc.top=(float)pSourceRect->top/(float)SourceHeight;
			rcSrc.right=(float)pSourceRect->right/(float)SourceWidth;
			rcSrc.bottom=(float)pSourceRect->bottom/(float)SourceHeight;
			rcDest=*pDestRect;
			::OffsetRect(&rcDest,pWindowRect->left,pWindowRect->top);
			fOK=SUCCEEDED(pDisplayControl->SetVideoPosition(&rcSrc,&rcDest));

			// EVRのバグでバックバッファがクリアされない時があるので、強制的にクリアする
			COLORREF crBorder;
			pDisplayControl->GetBorderColor(&crBorder);
			pDisplayControl->SetBorderColor(crBorder==0?RGB(1,1,1):RGB(0,0,0));

			::InvalidateRect(m_hwndRender,NULL,TRUE);
			pDisplayControl->Release();
		}
	}
	return fOK;
}


bool CVideoRenderer_EVR::GetDestPosition(RECT *pRect)
{
	bool fOK=false;

	if (m_pRenderer) {
		IMFVideoDisplayControl *pDisplayControl=GetVideoDisplayControl(m_pRenderer);

		if (pDisplayControl) {
			MFVideoNormalizedRect rcSrc;

			fOK=SUCCEEDED(pDisplayControl->GetVideoPosition(&rcSrc,pRect));
			pDisplayControl->Release();
		}
	}
	return fOK;
}


bool CVideoRenderer_EVR::GetCurrentImage(void **ppBuffer)
{
	bool fOK=false;

	if (m_pRenderer) {
		IMFVideoDisplayControl *pDisplayControl=GetVideoDisplayControl(m_pRenderer);

		if (pDisplayControl) {
			BITMAPINFOHEADER bmih;
			BYTE *pBits;
			DWORD BitsSize;
			LONGLONG TimeStamp=0;

			bmih.biSize=sizeof(BITMAPINFOHEADER);
			if (SUCCEEDED(pDisplayControl->GetCurrentImage(&bmih,&pBits,&BitsSize,&TimeStamp))) {
				BYTE *pDib;

				pDib=static_cast<BYTE*>(::CoTaskMemAlloc(sizeof(BITMAPINFOHEADER)+BitsSize));
				if (pDib) {
					::CopyMemory(pDib,&bmih,sizeof(BITMAPINFOHEADER));
					::CopyMemory(pDib+sizeof(BITMAPINFOHEADER),pBits,BitsSize);
					*ppBuffer=pDib;
					fOK=true;
				}
				::CoTaskMemFree(pBits);
			}
			pDisplayControl->Release();
		}
	}
	return fOK;
}


bool CVideoRenderer_EVR::RepaintVideo(HWND hwnd,HDC hdc)
{
	bool fOK=false;

	if (m_pRenderer) {
		IMFVideoDisplayControl *pDisplayControl=GetVideoDisplayControl(m_pRenderer);

		if (pDisplayControl) {
			fOK=SUCCEEDED(pDisplayControl->RepaintVideo());
			pDisplayControl->Release();
		}
	}
	return fOK;
}


bool CVideoRenderer_EVR::DisplayModeChanged()
{
	return true;
}


bool CVideoRenderer_EVR::SetVisible(bool fVisible)
{
	return true;
}
