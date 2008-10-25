#pragma once

// EVRを利用しない場合はコメントアウトする
//#define USE_MEDIA_FOUNDATION

#include <vector>
#include <streams.h>
#include <d3d9.h>
#include <vmr9.h>
#ifdef USE_MEDIA_FOUNDATION
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <evr9.h>
#endif


// guidに入っていない追加分GUID
#ifndef WAVE_FORMAT_AAC
#define WAVE_FORMAT_AAC 0x00FF
#endif

// AAC Mediatype {000000FF-0000-0010-8000-00AA00389B71}
#ifndef MEDIASUBTYPE_AAC
DEFINE_GUID(MEDIASUBTYPE_AAC,WAVE_FORMAT_AAC, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
#endif
                         
// ColorSpace Converter {1643E180-90F5-11CE-97D5-00AA0055595A}
#ifndef CLSID_ColorSpaceConverter
DEFINE_GUID(CLSID_ColorSpaceConverter, 0x1643E180, 0x90F5, 0x11CE, 0x97, 0xD5, 0x00, 0xAA, 0x00, 0x55, 0x59, 0x5A);
#endif

#ifdef USE_MEDIA_FOUNDATION
// IMFGetService FA993888-4383-415A-A930-DD472A8CF6F7}
#ifndef IID_IMFGetService
DEFINE_GUID(IID_IMFGetService , 0xFA993888, 0x4383, 0x415A, 0xA9, 0x30, 0xDD, 0x47, 0x2A, 0x8C, 0xF6, 0xF7); 
#endif
#endif

// Release
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(pOblect)			if((pOblect)) {(pOblect)->Release(); (pOblect) = NULL;}
#endif
#ifndef SAFE_RELEASE_EX
#define SAFE_RELEASE_EX(pOblect,nRef)	{nRef=0;if((pOblect)) {nRef=(pOblect)->Release(); (pOblect) = NULL;}}
#endif

class CDirectShowFilterFinder
{
public:
	CDirectShowFilterFinder();
	~CDirectShowFilterFinder();

	void Clear();
	bool FindFilter(const CLSID *pidInType,const CLSID *pidInSubType,const CLSID *pidOutType=NULL,const CLSID *pidOutubType=NULL);
	bool PriorityFilterGoToHead(const CLSID idPriorityClass);
	bool IgnoreFilterGoToTail(const CLSID idIgnoreClass,bool bRemoveIt=false);
	int GetFilterCount();
	bool GetFilterInfo(const int iIndex,CLSID *pidClass=NULL,LPWSTR pwszFriendryName=NULL,int iBufLen=0);
protected:
	class CFilterInfo {
	public:
		LPWSTR m_pwszFriendryName;
		CLSID m_clsid;
		CFilterInfo();
		CFilterInfo(const CFilterInfo &Info);
		~CFilterInfo();
		CFilterInfo &operator=(const CFilterInfo &Info);
		void SetFriendryName(LPCWSTR pwszFriendryName);
	};
	std::vector<CFilterInfo> m_FilterList;
};

namespace DirectShowUtil {

// 以下、便利関数
// デバッグ用(実行テーブルに追加・削除)
// ・RemoveFromRot()時にはGrapheditを閉じておく必要があります。
// ・WindowsVistaでこの機能を使うには、Windows SDK for Vista の C:\Program Files\Microsoft SDKs\Windows\v6.0\Bin にある
//   proppage.dll を regsvr32 で登録しておく必要があります。
HRESULT AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister);
void RemoveFromRot(const DWORD dwRegister);

// 構築用ユーティリティ
IPin* GetFilterPin(IBaseFilter *pFilter, const PIN_DIRECTION dir, const AM_MEDIA_TYPE *pMediaType=NULL);
bool ShowPropertyPage(IBaseFilter *pFilter, HWND hWndParent);
//bool AppendMpeg2Decoder_and_Connect(IGraphBuilder *pFilterGraph, CDirectShowUtil *pUtil, IBaseFilter **ppMpeg2DecoderFilter,wchar_t *lpszDecoderName,int iDecNameBufLen, IPin **ppCurrentOutputPin, IPin **ppNewOutputPin=NULL);
bool AppendFilterAndConnect(IGraphBuilder *pFilterGraph,
	IBaseFilter *pFilter,LPCWSTR lpwszFilterName,
	IPin **ppCurrentOutputPin,IPin **ppNewOutputPin=NULL,bool fDirect=false);
bool AppendFilterAndConnect(IGraphBuilder *pFilterGraph,
	const CLSID guidFilter,LPCWSTR lpwszFilterName,IBaseFilter **ppAppendedFilter,
	IPin **ppCurrentOutputPin, IPin **ppNewOutputPin=NULL, bool fDirect=false);
bool AppendColorSpaceConverterFilter_and_Connect(IGraphBuilder *pFilterGraph, IBaseFilter **ppColorSpaceConverterFilter, IPin **ppCurrentOutputPin, IPin **ppNewOutputPin=NULL);

// 汎用ユーティリティ
IVideoWindow* GetVideoWindow(IGraphBuilder *pGraph);
IBasicVideo2* GetBasicVideo2(IGraphBuilder *pGraph);
IMediaControl* GetMediaControl(IGraphBuilder *pGraph);
bool FilterGrapph_Play(IGraphBuilder *pFilterGraph);
bool FilterGrapph_Stop(IGraphBuilder *pFilterGraph);
bool FilterGrapph_Pause(IGraphBuilder *pFilterGraph);

// フィルタ参照カウンタ取得
inline LONG GetRefCount(IUnknown *pUkn)
{
	if(!pUkn) {
		return 0;
	} else {
		pUkn->AddRef();
		LONG ret = pUkn->Release();
		return ret;
	}
};

//////////////////////////////////////////////////////////////////////
// 以下 EVR専用ユーティリティ
#ifdef USE_MEDIA_FOUNDATION
void						MF_Init();
void						MF_Term();
IEVRFilterConfig*		MF_GetEVRFilterConfig(IBaseFilter *pEvr);
IMFGetService*			MF_GetService(IBaseFilter *pEvr);
bool						MF_SetNumberOfStreams(IBaseFilter *pEvr,int iStreamNumber=1);
IMFVideoDisplayControl*	MF_GetVideoDisplayControl(IBaseFilter *pEvr);
IMFVideoMixerControl*	MF_GetVideoMixerControl(IBaseFilter *pEvr);
IMFVideoProcessor*		MF_GetVideoProcessor(IBaseFilter *pEvr);
#endif

}
