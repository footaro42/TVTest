// MediaViewer.h: CMediaViewer クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include <Bdaiface.h>
#include "MediaDecoder.h"
#include "TsUtilClass.h"
#include "../DirectShowFilter/DirectShowUtil.h"
#include "../DirectShowFilter/BonSrcFilter.h"
#include "../DirectShowFilter/AacDecFilter.h"
#include "../DirectShowFilter/VideoRenderer.h"
#include "../DirectShowFilter/ImageMixer.h"
#ifndef TVH264
#include "../DirectShowFilter/Mpeg2SequenceFilter.h"
#else
#include "../DirectShowFilter/H264ParserFilter.h"
#define USE_TBS_FILTER	// TBSフィルタ有効
#endif


/////////////////////////////////////////////////////////////////////////////
// メディアビューア(映像及び音声を再生する)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CTsPacket		入力データ
/////////////////////////////////////////////////////////////////////////////

class CMediaViewer : public CMediaDecoder
{
public:
	enum EVENTID {
		EID_VIDEO_SIZE_CHANGED,
		EID_FILTER_GRAPH_FLUSH
	};
	enum {
		PID_INVALID=0xFFFF
	};

	CMediaViewer(IEventHandler *pEventHandler = NULL);
	virtual ~CMediaViewer();

// IMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CMediaViewer
	const bool OpenViewer(HWND hOwnerHwnd = NULL,HWND hMessageDrainHwnd = NULL,
		CVideoRenderer::RendererType RendererType = CVideoRenderer::RENDERER_DEFAULT,
		LPCWSTR pszVideoDecoder = NULL, LPCWSTR pszAudioDevice = NULL);
	void CloseViewer(void);
	const bool IsOpen() const;

	const bool Play(void);
	const bool Stop(void);

	const bool SetVideoPID(const WORD wPID);
	const bool SetAudioPID(const WORD wPID);
	const WORD GetVideoPID(void) const;
	const WORD GetAudioPID(void) const;

	// Append by Meru
	const bool SetViewSize(const int x,const int y);
	const bool SetVolume(const float fVolume);
	const bool GetVideoSize(WORD *pwWidth,WORD *pwHeight);
	const bool GetVideoAspectRatio(BYTE *pbyAspectRatioX,BYTE *pbyAspectRatioY);
	enum {
		AUDIO_CHANNEL_DUALMONO	= 0x00,
		AUDIO_CHANNEL_INVALID	= 0xFF
	};
	const BYTE GetAudioChannelNum();
	const bool SetStereoMode(const int iMode);
	const int GetStereoMode() const;
	const bool GetVideoDecoderName(LPWSTR lpName,int iBufLen);

	enum PropertyFilter {
		PROPERTY_FILTER_VIDEODECODER,
		PROPERTY_FILTER_VIDEORENDERER,
		PROPERTY_FILTER_MPEG2DEMULTIPLEXER,
		PROPERTY_FILTER_AUDIOFILTER,
		PROPERTY_FILTER_AUDIORENDERER
	};
	const bool DisplayFilterProperty(PropertyFilter Filter, HWND hwndOwner);
	const bool FilterHasProperty(PropertyFilter Filter);

	const bool Pause();
	const bool Flush();
	const bool GetVideoRendererName(LPTSTR pszName,int Length) const;
	const bool GetAudioRendererName(LPWSTR pszName,int Length) const;
	const bool ForceAspectRatio(int AspectX,int AspectY);
	const bool GetForceAspectRatio(int *pAspectX,int *pAspectY) const;
	const bool GetEffectiveAspectRatio(BYTE *pAspectX,BYTE *pAspectY);
	enum {
		PANANDSCAN_HORZ_DEFAULT	= 0x00,
		PANANDSCAN_HORZ_NONE	= 0x01,
		PANANDSCAN_HORZ_CUT		= 0x02,
		PANANDSCAN_VERT_DEFAULT	= 0x00,
		PANANDSCAN_VERT_NONE	= 0x04,
		PANANDSCAN_VERT_CUT		= 0x08
	};
	const bool SetPanAndScan(int AspectX,int AspectY,BYTE PanScanFlags = 0);
	BYTE GetPanAndScan() const { return m_PanAndScan; }
	enum ViewStretchMode {
		STRETCH_KEEPASPECTRATIO,	// アスペクト比保持
		STRETCH_CUTFRAME,			// 全体表示(収まらない分はカット)
		STRETCH_FIT					// ウィンドウサイズに合わせる
	};
	const bool SetViewStretchMode(ViewStretchMode Mode);
	const ViewStretchMode GetViewStretchMode() const { return m_ViewStretchMode; }
	const bool SetNoMaskSideCut(bool bNoMask, bool bAdjust = true);
	const bool GetNoMaskSideCut() const { return m_bNoMaskSideCut; }
	const bool SetIgnoreDisplayExtension(bool bIgnore);
	const bool GetIgnoreDisplayExtension() const { return m_bIgnoreDisplayExtension; }
	const bool GetOriginalVideoSize(WORD *pWidth,WORD *pHeight);
	const bool GetCroppedVideoSize(WORD *pWidth,WORD *pHeight);
	const bool GetSourceRect(RECT *pRect);
	const bool GetDestRect(RECT *pRect);
	const bool GetDestSize(WORD *pWidth,WORD *pHeight);
	bool SetVisible(bool fVisible);
	const void HideCursor(bool bHide);
	const bool GetCurrentImage(BYTE **ppDib);
#ifdef USE_GABBER_FILTER
	bool SetGrabber(bool bGrabber);
	bool GetGrabber() const { return m_pGrabber!=NULL; }
	void *DoCapture(DWORD WaitTime);
#endif
	bool SetDownMixSurround(bool bDownMix);
	bool GetDownMixSurround() const;
	bool SetAudioNormalize(bool bNormalize,float Level=1.0f);
	CVideoRenderer::RendererType GetVideoRendererType() const;
	bool SetUseAudioRendererClock(bool bUse);
	bool GetUseAudioRendererClock() const { return m_bUseAudioRendererClock; }
	bool SetAdjustAudioStreamTime(bool bAdjust);
	bool SetAudioStreamCallback(CAacDecFilter::StreamCallback pCallback, void *pParam = NULL);
	bool SetAudioFilter(LPCWSTR pszFilterName);
	const bool RepaintVideo(HWND hwnd,HDC hdc);
	const bool DisplayModeChanged();
	const bool DrawText(LPCTSTR pszText,int x,int y,HFONT hfont,COLORREF crColor,int Opacity);
	const bool IsDrawTextSupported() const;
	const bool ClearOSD();
	//bool SetAudioOnly(bool bOnly);
	bool CheckHangUp(DWORD TimeOut);
#ifdef TVH264
	bool SetAdjustSampleTime(bool bAdjust);
#endif
#ifdef USE_TBS_FILTER
	bool EnableTBSFilter(bool bEnable);
	bool IsTBSFilterEnabled() const;
#endif

protected:
	const bool AdjustVideoPosition();
	const bool CalcSourceRect(RECT *pRect);

	bool m_bInit;

	// DirectShowインタフェース
	IGraphBuilder *m_pFilterGraph;
	IMediaControl *m_pMediaControl;

	// DirectShowフィルタ
	IBaseFilter *m_pVideoDecoderFilter;
	// Source
	IBaseFilter *m_pSrcFilter;
	CBonSrcFilter *m_pBonSrcFilterClass;
	// AACデコーダ
	IBaseFilter *m_pAacDecFilter;
	CAacDecFilter *m_pAacDecClass;
	// 音声フィルタ
	LPWSTR m_pszAudioFilterName;
	IBaseFilter *m_pAudioFilter;
	// 映像レンダラ
	CVideoRenderer *m_pVideoRenderer;
	// 音声レンダラ
	IBaseFilter *m_pAudioRenderer;

#ifndef TVH264
	// Mpeg2-Sequence
	IBaseFilter *m_pMpeg2SeqFilter;
	CMpeg2SequenceFilter *m_pMpeg2SeqClass;
#else
	// H.264 parser
	IBaseFilter *m_pH264ParserFilter;
	CH264ParserFilter *m_pH264ParserClass;
#endif

	LPWSTR m_pszVideoDecoderName;

	// MPEG2Demultiplexerインタフェース
	IBaseFilter *m_pMp2DemuxFilter;

	// PIDマップ
	IMPEG2PIDMap *m_pMp2DemuxVideoMap;
	IMPEG2PIDMap *m_pMp2DemuxAudioMap;

	// Elementary StreamのPID
	WORD m_wVideoEsPID;
	WORD m_wAudioEsPID;

	static void CALLBACK OnMpeg2VideoInfo(const CMpeg2VideoInfo *pVideoInfo,const LPVOID pParam);
	WORD m_wVideoWindowX;
	WORD m_wVideoWindowY;
	CMpeg2VideoInfo m_VideoInfo;
	HWND m_hOwnerWnd;

	CCriticalLock m_ResizeLock;
	CVideoRenderer::RendererType m_VideoRendererType;
	LPWSTR m_pszAudioRendererName;
	int m_ForceAspectX,m_ForceAspectY;
	BYTE m_PanAndScan;
	ViewStretchMode m_ViewStretchMode;
	bool m_bNoMaskSideCut;
	bool m_bIgnoreDisplayExtension;
	bool m_bUseAudioRendererClock;
	bool m_bAdjustAudioStreamTime;
	CAacDecFilter::StreamCallback m_pAudioStreamCallback;
	void *m_pAudioStreamCallbackParam;
	CImageMixer *m_pImageMixer;
#ifdef USE_GABBER_FILTER
	bool m_bGrabber;
	class CGrabber *m_pGrabber;
#endif
	CTracer *m_pTracer;

#ifdef _DEBUG
private:
	HRESULT AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister) const;
	void RemoveFromRot(const DWORD pdwRegister) const;
	DWORD m_dwRegister;
#endif
};
